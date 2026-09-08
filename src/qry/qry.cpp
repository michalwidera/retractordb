#include "qry.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <expected>
#include <iostream>
#include <print>
#include <sstream>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>
#include <boost/system/system_error.hpp>

#include "constants.hpp"
#include "fatalError.hpp"
#include "formatters.hpp"
#include "ipcClient.hpp"
#include "uxSysTermTools.hpp"

using namespace boost;
using boost::property_tree::ptree;

// Rozmiar bufora dla formatowania wyjścia kolumnowego w dir() (sprintf).
// Musi pomieścić jedną sformatowaną linię z nazwami wszystkich kolumn strumienia.
constexpr int kDirLineBufferSize = 1024;

qry::qry(int serverNoDataTimeoutMs, int clientResponseMaxFails, int responseQueueOpenMaxFails, std::string_view serverName)
    : serverNoDataTimeoutMs_(std::max(1, serverNoDataTimeoutMs)),
      transport_(std::make_unique<IpcClient>(clientResponseMaxFails, responseQueueOpenMaxFails, serverName)),
      formatter_(std::make_unique<Formatter>()) {}
qry::~qry() = default;

ptree qry::netClient(const std::string &cmd, const std::string &arg) { return transport_->netClient(cmd, arg); }

bool qry::adhoc(const std::string &sAdhoc) {
  if (sAdhoc.empty()) {
    SPDLOG_ERROR("qry::adhoc: adhoc query string must not be empty");
    return true;
  }
  ptree pt = netClient("adhoc", sAdhoc);

  std::string rcv("fail.");
  for (auto &[first, second] : pt) {
    rcv = second.get<std::string>("");
  }

  if (rcv != "OK") {
    SPDLOG_ERROR("bad rcv: {}", rcv.c_str());
    return true;
  }
  return false;
}

namespace {

/// Werdykt serwera wyjety z odpowiedzi. Ten sam ksztalt co w qry::adhoc: serwer wpisuje
/// jedna wartosc pod kluczem `db` albo `error.response`.
std::string serverVerdict(const boost::property_tree::ptree &pt) {
  std::string rcv("fail.");
  for (const auto &[first, second] : pt) {
    rcv = second.get<std::string>("");
  }
  return rcv;
}

/// Rozstrzygniecie odpowiedzi serwera PRZED siegnieciem po jej zawartosci. Dotyczy kazdej
/// komendy wymagajacej modelu danych ('get', 'detail'), bo kazda z nich moze zastac te same
/// stany instancji.
enum class answerVerdict : std::uint8_t {
  streams,     ///< odpowiedz niesie liste strumieni
  idle,        ///< instancja odpowiedziala, ale nie ma wczytanego planu
  stopping,    ///< instancja odpowiedziala, ale wlasnie sie zamyka
  noResponse,  ///< serwer nie odpowiedzial w wyznaczonym czasie
  malformed    ///< serwer odpowiedzial, ale odpowiedz nie niesie ani listy, ani znanego werdyktu
};

/// Kolejnosc jest tu cala trescia: brak odpowiedzi i kazdy stan bez planu tak samo NIE niosa
/// `db.stream`, a to rozne awarie i rozne naprawy. Rozpoznanie po samym braku wezla kazalo
/// `dir()` i `dirYaml()` meldowac zdrowo wygladajacy stan bezczynny wtedy, gdy serwer w ogole
/// nie odpowiedzial — i konczyc sie zerem. Najpierw wiec `error.response`, potem DOKLADNE
/// odpowiedzi serwera, a dopiero na koncu worek na wszystko inne.
///
/// Odpowiedz na 'detail' przechodzi tym samym sitem: `db.stream` jest w niej wezlem z nazwa
/// strumienia, wiec udana odpowiedz daje `streams`, a przeladowanie planu miedzy 'get'
/// a 'detail' — `idle` albo `stopping`, zamiast wyjatku o brakujacym `db.field`.
answerVerdict classifyAnswer(const boost::property_tree::ptree &pt) {
  if (pt.get_optional<std::string>("error.response")) return answerVerdict::noResponse;
  if (pt.get_child_optional("db.stream")) return answerVerdict::streams;
  const auto reason = pt.get_optional<std::string>("db");
  if (!reason) return answerVerdict::malformed;
  if (*reason == constants::kNoActivePlanReply) return answerVerdict::idle;
  if (*reason == constants::kServerStoppingReply) return answerVerdict::stopping;
  return answerVerdict::malformed;
}

/// Komunikat dla operatora, jeden na werdykt. Trzyma sie tu, przy klasyfikacji, bo kazde
/// wywolanie 'get' opisuje ten sam stan serwera — komenda, ktora go zastala, niczego w nim
/// nie zmienia.
const char *describe(answerVerdict verdict) {
  switch (verdict) {
    case answerVerdict::streams:
      return "server returned the stream list";
    case answerVerdict::idle:
      return "server has no plan loaded";
    case answerVerdict::stopping:
      return "server is shutting down";
    case answerVerdict::noResponse:
      return "server did not answer within the timeout";
    case answerVerdict::malformed:
      return "server response carries no stream list";
  }
  return "unknown server verdict";
}

/// Werdykt widziany przez operatora i przez kod wyjscia. Odpowiedz zepsuta idzie tu razem
/// z brakiem odpowiedzi: dla wolajacego to ta sama porazka — listy nie ma — a rozroznia je
/// komunikat z describe().
selectResult toSelectResult(answerVerdict verdict) {
  switch (verdict) {
    case answerVerdict::streams:
      return selectResult::ok;
    case answerVerdict::idle:
      return selectResult::noActivePlan;
    case answerVerdict::stopping:
      return selectResult::serverStopping;
    case answerVerdict::noResponse:
    case answerVerdict::malformed:
      return selectResult::serverNoResponse;
  }
  return selectResult::serverNoResponse;
}

}  // namespace

bool qry::reset(const std::string &planText) {
  const std::size_t chunkCount = (planText.size() + kResetChunkBytes - 1) / kResetChunkBytes;

  const auto refuse = [](const std::string &stage, const std::string &verdict) {
    // Werdykt idzie na stderr raz. SPDLOG w xqry pisze wlasnie na stderr (i do pliku logu),
    // wiec dolozenie tam drugiej kopii dawalo operatorowi ten sam komunikat dwa razy.
    std::println(std::cerr, "xqry: plan reload refused at {}: {}", stage, verdict);
    return true;
  };

  if (const std::string verdict = serverVerdict(netClient("reset-begin", std::to_string(chunkCount))); verdict != "OK")
    return refuse("reset-begin", verdict);

  for (std::size_t offset = 0; offset < planText.size(); offset += kResetChunkBytes) {
    const std::string chunk = planText.substr(offset, kResetChunkBytes);
    if (const std::string verdict = serverVerdict(netClient("reset-chunk", chunk)); verdict != "OK")
      return refuse("reset-chunk", verdict);
  }

  // Dopiero commit uruchamia walidacje po stronie serwera: parsowanie, kompilacje i
  // rozlacznosc nazw. Do tej chwili dzialajacy plan nie jest niczym dotkniety.
  if (const std::string verdict = serverVerdict(netClient("reset-commit", "")); verdict != "OK")
    return refuse("reset-commit", verdict);

  return false;
}

selectResult qry::select(boost::program_options::variables_map &vm, const int iElemLimit, const std::string &input,
                         std::tuple<int, int, int> gnuplotDim, bool gnuplotRightToLeft) {
  elemLimitCnt = (iElemLimit > 0) ? iElemLimit + 1 : iElemLimit;

  // Zatrzymanie klawiszem nalezy wylacznie do przebiegu NIEOGRANICZONEGO. Przebieg
  // z zadeklarowanym budzetem elementow (-m N) konczy sie po tym budzecie i po niczym innym,
  // bo jego wynik ma byc powtarzalny. Bez tego warunku bajt czekajacy na terminalu konczyl
  // petle przed odczytaniem czegokolwiek: klient wychodzil z zerem elementow i -- co gorsza --
  // z werdyktem obciazajacym serwer, bo watek producenta byl wtedy zrywany, zanim raz sprobowal
  // otworzyc swoja kolejke. Tak padl it_fncall_runtime_case na CI (2026-09-04): stdin kroku jest
  // tam terminalem, a CTest przekazuje go testom. Ta sama regula i to samo uzasadnienie co dla
  // silnika w executorsm.cpp (`ignoreanykey`), gdzie ta pulapka wywrocila it_agse_array.
  // Ctrl+C (SIGINT) zatrzymuje klienta bez zmian, obiema drogami.
  const bool ignoreAnyKey = vm.contains("needctrlc") || iElemLimit > 0;

  // SUBSKRYPCJA IDZIE PIERWSZA, przed jakakolwiek inna komenda. Serwer wstrzymany bramka
  // --xqrywait rusza po PIERWSZEJ obsluzonej komendzie, a kolejke odpowiedzi tego klienta
  // tworzy dopiero handler 'show'. Gdy przed nim szedl 'get' (walidacja nazwy strumienia),
  // miedzy jedna komenda a druga serwer juz liczyl i emitowal do nikogo: przy takcie 1/8 s
  // okno wynosilo 125 ms na wiersz, a wiersze z niego przepadaly bezpowrotnie. Lokalnie
  // przerwa 'get'->'show' to pojedyncze milisekundy, na obciazonym kontenerze CI przekracza
  // takt -- tak padl it_null_divide_by_zero (2026-09-08): zamiast 25, null, 20 klient dostal
  // null, 20, null. Drugi czlon naprawy jest po stronie serwera (ipcServer.cpp: bramke
  // zdejmuje komenda OBSLUZONA, nie odebrana), i dopiero oba razem zamykaja to okno.
  //
  // Walidacja nazwy nie znika, tylko idzie za subskrypcja: 'get' nizej rozstrzyga, czy
  // strumien w ogole istnieje, i to jego werdykt pada pierwszy. Subskrypcja nieistniejacego
  // strumienia jest po stronie serwera odmowa bez skutkow ubocznych -- kolejka nie powstaje.
  streamTable[input] = netClient("show", input);

  ptree pt = netClient("get", "");

  // Brak odpowiedzi serwera jest ODPOWIEDZIĄ, a nie niespodzianką w strukturze
  // danych. `netClient` po wyczerpaniu prób zwraca ptree z samym
  // `error.response`; poprzednia wersja szła prosto do `get_child("db.stream")`
  // i wywracała się wyjątkiem „No such node (db.stream)". Operator dostawał
  // komunikat o brakującym węźle zamiast informacji, że serwer nie zdążył
  // odpowiedzieć — a to dwie różne awarie i dwie różne naprawy (issue_215).
  // Rozpoznanie należy do `classifyAnswer`, wspólnego z `dir()`, `dirYaml()`
  // i `detailNode()`: gdy każda z tych ścieżek miała własną kopię tej kolejności,
  // trzy z nich się rozjechały i uznawały milczenie serwera za stan bezczynny.
  if (const answerVerdict verdict = classifyAnswer(pt); verdict != answerVerdict::streams) {
    SPDLOG_ERROR("{} (stream: {})", describe(verdict), input);
    return toSelectResult(verdict);
  }

  const bool found = std::ranges::any_of(pt.get_child("db.stream"), [input](const auto &node) {
    const ptree &v = node.second;
    return input == v.get<std::string>("");
  });

  if (!found) {
    SPDLOG_ERROR("not found: {}", input);
    return selectResult::streamNotFound;
  }

  // Odpowiedź na 'show' sprawdzana tak samo jak odpowiedź na 'get' powyżej — i dopiero
  // TERAZ, czyli po rozstrzygnięciu, że strumień istnieje: subskrypcja idzie pierwsza,
  // więc dla nieznanej nazwy serwer odmawia jej, zanim ktokolwiek zdąży to nazwać, a
  // werdyktem tego przypadku pozostaje `streamNotFound` powyżej, nie awaria kolejki. Do
  // 2026-09-04 odpowiedź na 'show' nie była sprawdzana wcale, a handler 'show' nie wpisuje niczego do
  // odpowiedzi TAKŻE po udanej subskrypcji — połknięty po stronie serwera wyjątek dawał
  // więc odpowiedź nie do odróżnienia od powodzenia. Klient ruszał z wątkiem producenta
  // i meldował dopiero brak kolejki, sekundę później i bez nazwania przyczyny; zdanie
  // nazywające wyjątek zostawało w logu serwera. Werdykt jest ten sam
  // (clientQueueMissing: kolejki faktycznie nie ma), ale pada od razu i z powodem.
  if (const auto reason = streamTable[input].get_optional<std::string>("error.response")) {
    SPDLOG_ERROR("server rejected the 'show' command (stream: {}): {}", input, *reason);
    return selectResult::clientQueueMissing;
  }

  std::jthread producer_thread([this] { transport_->producer(); });

  if (outputFormatMode == formatMode::GNUPLOT) {
    Formatter::initGnuplot(gnuplotDim, gnuplotRightToLeft);
  }

  ptree schema;
  if (outputFormatMode != formatMode::RAW) schema = netClient("detail", input);

  int noDataCounter = 0;
  // Liczba faktycznie wyrenderowanych elementów. Bez niej „koniec pętli" i „nic
  // nie przyszło" były nieodróżnialne, a klient meldował sukces po zerze danych.
  long long rendered = 0;

  ptree e_value;
  try {
    while (!transport_->done) {
      if (_kbhit(ignoreAnyKey)) break;
      if (elemLimitCnt == 1) {
        if (vm.contains("kill")) {
          netClient("kill", "");
          transport_->done = true;
        }
        break;
      }
      while (transport_->popQueue(e_value)) {
        const std::string streamN = e_value.get("stream", "");
        const std::string nullmap = e_value.get("nullmap", "");
        if (streamN == constants::Reserved_id_oob) {
          transport_->done = true;
          break;
        }
        for (auto &[w, k] : streamTable)
          if (w == streamN) {
            const int count = std::stoi(e_value.get("count", ""));
            if (outputFormatMode == formatMode::RAW)
              Formatter::renderRaw(e_value, count, nullmap, vm.contains("null"));
            else if (outputFormatMode == formatMode::GNUPLOT)
              formatter_->renderGnuplot(e_value, count, nullmap, input, schema, gnuplotDim);
            else if (outputFormatMode == formatMode::GRAPHITE)
              Formatter::renderGraphite(e_value, nullmap, input, schema);
            else if (outputFormatMode == formatMode::INFLUXDB)
              Formatter::renderInfluxDB(e_value, nullmap, input, schema);

            if (elemLimitCnt > 1) --elemLimitCnt;
            ++rendered;
            noDataCounter = 0;
          }
        // Budzet elementow (-m N) musi zamykac TAKZE ta petle, nie tylko zewnetrzna.
        // Pojedynczy obrot oproznia kolejke do konca, wiec gdy producent zdazyl wlozyc
        // wiecej niz jeden wiersz, wszystkie szly na wyjscie i dopiero potem petla
        // zewnetrzna sprawdzala budzet. `-m 1` na strumieniu z tablica dawalo dwie klatki
        // gnuplota zamiast jednej -- wynik zalezny od wyscigu, a mial byc powtarzalny.
        if (elemLimitCnt == 1) break;
      }
      std::this_thread::sleep_for(ipc::kQueuePollInterval);
      if (++noDataCounter > serverNoDataTimeoutMs_) {
        SPDLOG_WARN("No data received for {} ms, assuming server is dead.", serverNoDataTimeoutMs_);
        transport_->done = true;
        break;
      }
    }
    while (transport_->popQueue(e_value) && !transport_->done)
      std::this_thread::sleep_for(ipc::kQueuePollInterval);

    if (elemLimitCnt != 1 && !transport_->done) _getch();

  } catch (...) {
    SPDLOG_ERROR("General exception catched.");
  }

  transport_->done = true;

  // Werdykt producenta trzeba ODEBRAĆ, a nie podejrzeć w locie. `responseQueueMissing`
  // ustawia wątek producenta dopiero wtedy, gdy wyczerpie próby otwarcia własnej kolejki
  // odpowiedzi. Pętla powyżej wychodzi zwykle na `done` OD producenta — ale nie zawsze:
  // `_kbhit()` (klawisz operatora, a na CI terminal z bajtem w buforze), limit elementów
  // albo wyjątek kończą ją WCZEŚNIEJ. Bez `join()` flaga była wtedy jeszcze fałszem
  // i klient melduje „brak danych" zamiast „serwer nie utworzył kolejki" — czyli mylną
  // diagnozę tej samej awarii. `done` jest już ustawione, a każda pętla producenta
  // sprawdza tę flagę, więc oczekiwanie jest krótkie.
  producer_thread.join();

  // Reguła: klient, który nie przeczytał ani jednego elementu, nie kończy się
  // sukcesem. Rozróżniamy przy tym DLACZEGO nic nie przyszło, bo „serwer nie
  // utworzył mojej kolejki" i „kolejka była, ale pusta" to dwie różne awarie.
  if (rendered == 0) {
    if (transport_->responseQueueMissing) {
      SPDLOG_ERROR("server did not create this client's response queue (stream: {})", input);
      return selectResult::clientQueueMissing;
    }
    SPDLOG_ERROR("stream '{}' delivered no elements", input);
    return selectResult::noData;
  }
  return selectResult::ok;
}

const char *toString(selectResult result) {
  switch (result) {
    case selectResult::ok:
      return "ok";
    case selectResult::streamNotFound:
      return "stream unknown to the server";
    case selectResult::serverNoResponse:
      return "server did not answer within the timeout";
    case selectResult::clientQueueMissing:
      return "server did not create the client response queue";
    case selectResult::noData:
      return "no data in stream";
    case selectResult::noActivePlan:
      return "server has no plan loaded (idle); load one with --reset";
    case selectResult::serverStopping:
      return "server is shutting down";
  }
  return "unknown";
}

int qry::hello() {
  ptree pt = netClient("hello", "");

  std::string rcv("fail.");
  for (auto &[first, second] : pt) {
    rcv = second.get<std::string>("");
  }
  if (rcv != "world") {
    SPDLOG_ERROR("bad rcv: {}", rcv.c_str());
    return system::errc::protocol_error;
  }
  return system::errc::success;
}

std::expected<std::string, selectResult> qry::dirYaml() {
  std::stringstream retval;
  ptree pt = netClient("get", "");

  retval << "---\napiVersion: xqry/v1\n";
  const answerVerdict verdict = classifyAnswer(pt);
  // Instancja bez planu daje dokument z pusta lista, tak samo jak `--bus -y` przy pustej
  // magistrali. Konsument YAML-a ma dostac dokument, a nie wyjatek o brakujacym wezle.
  if (verdict == answerVerdict::idle) {
    retval << "streams: []\n";
    return retval.str();
  }
  if (verdict != answerVerdict::streams) {
    SPDLOG_ERROR("qry::dirYaml: {}", describe(verdict));
    return std::unexpected(toSelectResult(verdict));
  }
  retval << "streams:\n";
  for (const auto &v : pt.get_child("db.stream")) {
    auto location = v.second.get<std::string>("location");
    auto size     = v.second.get<std::string>("size");

    retval << "  - name: " << v.second.get<std::string>("") << "\n";
    retval << "    delta: " << v.second.get<std::string>("duration") << "\n";
    if (size != "-1") retval << "    size: " << size << "\n";
    retval << "    count: " << v.second.get<std::string>("count") << "\n";
    if (!location.empty()) retval << "    location: " << location << "\n";
  }

  return retval.str();
}

std::expected<std::string, selectResult> qry::dir() {
  std::stringstream retval;
  ptree pt = netClient("get", "");
  // Instancja bez planu nie odsyla listy strumieni, ale ODPOWIADA — i dostaje wlasny,
  // niepusty wydruk. Do 2026-09-05 get_child ponizej rzucalo wtedy "No such node
  // (db.stream)", a wyjatek wychodzil do operatora jako "Std: ..." — komunikat o strukturze
  // ptree zamiast o stanie serwera.
  const answerVerdict verdict = classifyAnswer(pt);
  if (verdict == answerVerdict::idle) return std::string(constants::kNoActivePlanReply) + "\n";
  if (verdict != answerVerdict::streams) {
    SPDLOG_ERROR("qry::dir: {}", describe(verdict));
    return std::unexpected(toSelectResult(verdict));
  }
  // Klucz w ptree ("" to nazwa strumienia) i naglowek kolumny w wydruku.
  const std::array vcols{std::pair{std::string{""}, std::string{"name"}},
                         std::pair{std::string{"duration"}, std::string{"duration"}},
                         std::pair{std::string{"size"}, std::string{"size"}},
                         std::pair{std::string{"count"}, std::string{"count"}},
                         std::pair{std::string{"location"}, std::string{"location"}},
                         std::pair{std::string{"cap"}, std::string{"cap"}}};
  // Forma tabeli jest wspolna z `xqry --bus` (routing::describe): kolumny do lewej,
  // separator " | ", bez brzegowych kresek. Ostatnia kolumna nie jest dopelniana, wiec
  // wiersz nie konczy sie spacjami.
  std::stringstream ss;
  std::stringstream separator;
  for (std::size_t column = 0; column < vcols.size(); ++column) {
    const auto &[key, title] = vcols[column];
    std::size_t width        = title.length();
    for (const auto &v : pt.get_child("db.stream"))
      width = std::max(width, v.second.get<std::string>(key).length());
    const bool last = column + 1 == vcols.size();
    if (last)
      ss << "%s\n";
    else
      ss << "%-" << width << "s | ";
    separator << std::string(width, '-') << (last ? "" : "-+-");
  }
  separator << "\n";

  auto emitRow = [&](const std::string &name, const std::string &duration, const std::string &size, const std::string &count,
                     const std::string &location, const std::string &cap) {
    std::array<char, static_cast<std::size_t>(kDirLineBufferSize)> buffer{};
    int n = snprintf(buffer.data(), buffer.size(), ss.str().c_str(),  //
                     name.c_str(),                                    //
                     duration.c_str(),                                //
                     size.c_str(),                                    //
                     count.c_str(),                                   //
                     location.c_str(),                                //
                     cap.c_str());
    if (n < 0) {
      SPDLOG_ERROR("qry::dir: snprintf failed while formatting stream '{}'", name);
      return;
    }
    if (static_cast<std::size_t>(n) >= buffer.size()) {
      SPDLOG_ERROR("qry::dir: formatted output truncated for stream '{}' (required {}, buffer {})", name, n, buffer.size());
      buffer[buffer.size() - 1] = '\0';
    }
    retval << buffer.data();
  };

  std::apply([&](const auto &...column) { emitRow(column.second...); }, vcols);
  retval << separator.str();
  for (const auto &v : pt.get_child("db.stream"))
    emitRow(v.second.get<std::string>(""),          //
            v.second.get<std::string>("duration"),  //
            v.second.get<std::string>("size"),      //
            v.second.get<std::string>("count"),     //
            v.second.get<std::string>("location"),  //
            v.second.get<std::string>("cap"));

  return retval.str();
}

static const std::string indent = "  ";

// Tabela kolumnowa w formie wspolnej z `dir()` i `xqry --bus`: kolumny do lewej, laczone
// " | ", bez brzegowych kresek, ostatnia kolumna niedopelniana (wiersz nie konczy sie
// spacjami). Szerokosc kolumny wynika z najszerszej wartosci, naglowek wliczony.
static std::string columnTable(const std::vector<std::string> &header, const std::vector<std::vector<std::string>> &rows) {
  std::vector<std::size_t> widths;
  widths.reserve(header.size());
  for (std::size_t column = 0; column < header.size(); ++column) {
    std::size_t width = header[column].length();
    for (const auto &row : rows)
      width = std::max(width, row[column].length());
    widths.push_back(width);
  }

  auto emitRow = [&widths](const std::vector<std::string> &row) {
    std::string line;
    for (std::size_t column = 0; column < row.size(); ++column) {
      line += row[column];
      if (column + 1 != row.size()) line += std::string(widths[column] - row[column].length(), ' ') + " | ";
    }
    return line + "\n";
  };

  std::string retval = emitRow(header);
  for (std::size_t column = 0; column < widths.size(); ++column)
    retval += std::string(widths[column], '-') + (column + 1 == widths.size() ? "\n" : "-+-");
  for (const auto &row : rows)
    retval += emitRow(row);
  return retval;
}

std::expected<ptree, selectResult> qry::detailNode(const std::string &input) {
  ptree pt = netClient("get", "");

  // Brak listy strumieni nie jest niespodzianka w strukturze danych, tylko odpowiedzia.
  // Do 2026-09-06 `get_child` ponizej rzucalo tu "No such node (db.stream)" — zarowno dla
  // instancji bezczynnej, jak i dla milczacego serwera — a operator dostawal komunikat
  // o wezle ptree zamiast o stanie serwera.
  if (const answerVerdict verdict = classifyAnswer(pt); verdict != answerVerdict::streams) {
    SPDLOG_ERROR("{} (stream: {})", describe(verdict), input);
    return std::unexpected(toSelectResult(verdict));
  }

  const auto streams = pt.get_child("db.stream");
  const bool found   = std::ranges::any_of(streams, [&input](const auto &node) {
    const ptree &v = node.second;
    return input == v.get<std::string>("");
  });

  if (!found) {
    SPDLOG_ERROR("not found: {}", input);
    return std::unexpected(selectResult::streamNotFound);
  }

  // Drugi obrot IPC ma te same tryby porazki co pierwszy — i wlasny wyscig: plan moze zostac
  // przeladowany MIEDZY 'get' a 'detail', wiec strumien policzony przed chwila juz nie
  // istnieje. Bez tego sita `db.field` ponizej rzucalo w takim wyscigu wyjatkiem.
  ptree detail = netClient("detail", input);
  if (const answerVerdict verdict = classifyAnswer(detail); verdict != answerVerdict::streams) {
    SPDLOG_ERROR("{} (stream: {})", describe(verdict), input);
    return std::unexpected(toSelectResult(verdict));
  }
  return detail;
}

std::expected<std::string, selectResult> qry::detailShow(const std::string &input) {
  const auto ptsh = detailNode(input);
  if (!ptsh) return std::unexpected(ptsh.error());

  std::vector<std::vector<std::string>> fields;
  for (const auto &v : ptsh->get_child("db.field")) {
    const auto name           = v.second.get<std::string>("");
    std::string qualifiedName = input;
    qualifiedName += '.';
    qualifiedName += name;
    std::string typePath = "db.field_type.";
    typePath += name;
    fields.push_back({std::move(qualifiedName), ptsh->get<std::string>(typePath)});
  }

  // Naglowek strumienia i lista pol to dwie osobne tabele: ich kolumny nie maja ze soba
  // nic wspolnego, a wspolna szerokosc rozjezdzalaby obie.
  return columnTable({"name", "delta", "query"}, {{ptsh->get_child("db.stream").get_value<std::string>(),
                                                   ptsh->get_child("db.duration").get_value<std::string>(),
                                                   ptsh->get_child("db.processed_line").get_value<std::string>()}}) +
         "\n" + columnTable({"field", "type"}, fields);
}

std::expected<std::string, selectResult> qry::detailShowYaml(const std::string &input) {
  const auto ptsh = detailNode(input);
  if (!ptsh) return std::unexpected(ptsh.error());

  std::stringstream retval;
  retval << "---\napiVersion: xqry/v1\n";
  retval << "stream:\n";
  retval << indent << "name: " << ptsh->get_child("db.stream").get_value<std::string>() << "\n";
  retval << indent << "delta: " << ptsh->get_child("db.duration").get_value<std::string>() << "\n";
  retval << "query: " << ptsh->get_child("db.processed_line").get_value<std::string>() << "\n";
  retval << "fields:\n";
  for (const auto &v : ptsh->get_child("db.field")) {
    retval << indent << input << "." << v.second.get<std::string>("") << ":\n";
    retval << indent << indent << "type: " << ptsh->get<std::string>("db.field_type." + v.second.get<std::string>("")) << "\n";
  }

  return retval.str();
}
