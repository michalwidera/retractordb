#include "qry.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

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

selectResult qry::select(boost::program_options::variables_map &vm, const int iElemLimit, const std::string &input,
                         std::tuple<int, int, int> gnuplotDim, bool gnuplotRightToLeft) {
  elemLimitCnt = (iElemLimit > 0) ? iElemLimit + 1 : iElemLimit;
  ptree pt     = netClient("get", "");

  // Brak odpowiedzi serwera jest ODPOWIEDZIĄ, a nie niespodzianką w strukturze
  // danych. `netClient` po wyczerpaniu prób zwraca ptree z samym
  // `error.response`; poprzednia wersja szła prosto do `get_child("db.stream")`
  // i wywracała się wyjątkiem „No such node (db.stream)". Operator dostawał
  // komunikat o brakującym węźle zamiast informacji, że serwer nie zdążył
  // odpowiedzieć — a to dwie różne awarie i dwie różne naprawy (issue_215).
  if (pt.get_optional<std::string>("error.response")) {
    SPDLOG_ERROR("server did not answer the 'get' command within the timeout (stream: {})", input);
    return selectResult::serverNoResponse;
  }
  const auto streamNode = pt.get_child_optional("db.stream");
  if (!streamNode) {
    SPDLOG_ERROR("server response carries no stream list (stream: {})", input);
    return selectResult::serverNoResponse;
  }

  const bool found = std::ranges::any_of(*streamNode, [input, this](const auto &node) {
    const ptree &v = node.second;
    bool ret       = (input == v.get<std::string>(""));
    if (ret) streamTable[input] = netClient("show", input);
    return ret;
  });

  if (!found) {
    SPDLOG_ERROR("not found: {}", input);
    return selectResult::streamNotFound;
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
      if (_kbhit(vm.contains("needctrlc"))) break;
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

std::string qry::dirYaml() {
  std::stringstream retval;
  ptree pt = netClient("get", "");

  retval << "---\napiVersion: xqry/v1\n";
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

std::string qry::dir() {
  std::stringstream retval;
  ptree pt = netClient("get", "");
  // Klucz w ptree ("" to nazwa strumienia) i naglowek kolumny w wydruku.
  const std::vector<std::pair<std::string, std::string>> vcols = {{"", "name"},       {"duration", "duration"}, {"size", "size"},
                                                                  {"count", "count"}, {"location", "location"}, {"cap", "cap"}};
  std::stringstream ss;
  for (const auto &[key, title] : vcols) {
    std::size_t width = title.length();
    for (const auto &v : pt.get_child("db.stream"))
      width = std::max(width, v.second.get<std::string>(key).length());
    ss << "|%" << width << "s";
  }
  ss << "|\n";

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

  emitRow(vcols[0].second, vcols[1].second, vcols[2].second, vcols[3].second, vcols[4].second, vcols[5].second);
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

std::string qry::detailShow(const std::string &input) {
  std::stringstream retval;
  ptree pt = netClient("get", "");

  const auto streams = pt.get_child("db.stream");
  bool found         = std::ranges::any_of(streams, [&input](const auto &node) {
    const ptree &v = node.second;
    return input == v.get<std::string>("");
  });

  if (found) {
    ptree ptsh = netClient("detail", input);
    auto delta = ptsh.get_child("db.duration");
    auto query = ptsh.get_child("db.processed_line");
    auto id    = ptsh.get_child("db.stream");

    retval << "---\napiVersion: xqry/v1\n";
    retval << "stream:\n";
    retval << indent << "name: " << id.get_value<std::string>() << "\n";
    retval << indent << "delta: " << delta.get_value<std::string>() << "\n";
    retval << "query: " << query.get_value<std::string>() << "\n";
    retval << "fields:\n";
    for (const auto &v : ptsh.get_child("db.field")) {
      retval << indent << input << "." << v.second.get<std::string>("") << ":\n";
      retval << indent << indent << "type: " << ptsh.get<std::string>("db.field_type." + v.second.get<std::string>("")) << "\n";
    }
  } else
    SPDLOG_ERROR("not found");

  return retval.str();
}
