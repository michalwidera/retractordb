#include "rdb/embed/engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include <boost/rational.hpp>

#include "retractor/lib/compiler.hpp"
#include "retractor/lib/CRSMath.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/planSource.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/query.hpp"
#include "retractor/lib/rule.hpp"

namespace rdb::embed {

/// Wszystko, co zyje od compile() do close(). Kolejnosc pol jest kolejnoscia zaleznosci:
/// kompilator, model i os czasu trzymaja referencje do drzewa, wiec drzewo stoi pierwsze i
/// niszczy sie ostatnie. To ta sama zasada, ktorej pilnuje EpochPublication w executorsm.cpp -
/// tutaj egzekwowana ukladem struktury, a nie komentarzem przy `proc`.
struct Engine::Plan {
  qTree tree;
  compiler cm{tree};
  std::unique_ptr<dataModel> model;
  std::unique_ptr<CRationalStreamMath::TimeLine> timeline;
  bool untilEof{true};
  /// Krok zerowy wykonany - rekord startowy kazdej deklaracji wczytany.
  bool bootstrapped{false};
  bool endOfInput{false};
  std::uint64_t slotsDone{0};
  boost::rational<int> time{0};
};

Engine::Engine(std::string storageDir) : storageDir_(std::move(storageDir)) {}

Engine::~Engine() { close(); }

std::unique_ptr<storage> Engine::openStorage(const std::string_view qryID,         //
                                             const std::string_view fileName,      //
                                             const std::string_view storageParam,  //
                                             const std::string_view storageType,   //
                                             const bool oneShot,                   //
                                             const bool isHold,                    //
                                             const int percounter) {
  return std::make_unique<storage>(qryID, fileName, storageParam, storageType, oneShot, isHold, percounter, &memory_);
}

void Engine::compile(const std::string_view rql, const bool untilEof) {
  // Poprzedni plan schodzi PRZED budowa nowego: oba pisalyby do tych samych plikow magazynu.
  close();

  auto plan      = std::make_unique<Plan>();
  plan->untilEof = untilEof;

  // Ta sama droga, ktora idzie launcher (parsePlanText + compiler::compile) i przeladowanie
  // planu w locie (`xqry --reset`); status "OK" albo tresc bledu - obie sa tu wyjatkiem.
  const PlanSource loaded = parsePlanText(plan->tree, std::string(rql));
  if (loaded.status != "OK") throw SyntaxError(loaded.status);
  if (plan->tree.empty()) throw CompileError("plan holds no statements");

  std::string response;
  try {
    response = plan->cm.compile();
  } catch (const std::logic_error &error) {
    // Odwolanie do strumienia, ktorego plan nie deklaruje, kompilator zglasza rzutem
    // std::logic_error z qTree::getQuery (nie rdb::Error i nie statusem "Check result").
    // Dla planu podanego przez hosta to blad WEJSCIA, wiec tu zostaje CompileError; rdb::Error
    // (LogicError, ConfigError) nie dziedziczy po std::logic_error i przechodzi bez zmian.
    throw CompileError(error.what());
  }
  if (response != "OK") throw CompileError(response);

  // Trzy rzeczy, ktorych silnik osadzony NIE MA, odrzucane tutaj, a nie w polowie slotu:
  //  - :ROTATION czyta licznik rotacji z globalnego pCounterPtr demona (streamInstance.cpp);
  //    bez niego dyrektywa bylaby po cichu ignorowana, a plan pisalby bez rotacji;
  //  - regula DUMP siega po model przez globalny pProc (dumpManager.cpp), ktorego tu nie ma -
  //    zamiast LogicError o pustym wskazniku w srodku slotu uzytkownik dostaje odpowiedz
  //    przy compile();
  //  - regula SYSTEM wykonuje ::system() w procesie hosta; roadmapa (iOS faza 3, pkt 5)
  //    kaze ja bramkowac wywolaniem zwrotnym hosta, DOMYSLNIE WYLACZONYM. Bramki jeszcze nie
  //    ma, wiec obowiazuje "wylaczone".
  for (const auto &qry : plan->tree) {
    if (qry.id == ":ROTATION") {
      throw CompileError(":ROTATION is not available in the embedded engine - the rotation counter belongs to the daemon");
    }
    for (const auto &item : qry.lRules) {
      if (item.action == rule::DUMP) {
        throw CompileError(
            std::format("stream '{}', rule '{}': DUMP actions are not available in the embedded engine", qry.id, item.name));
      }
      if (item.action == rule::SYSTEM) {
        throw CompileError(
            std::format("stream '{}', rule '{}': SYSTEM actions are not available in the embedded engine", qry.id, item.name));
      }
    }
  }

  // Domyslny katalog magazynu, gdy plan nie niesie :STORAGE - dokladnie jak launcher z
  // `[storage] dir`: RQL ma pierwszenstwo, katalog silnika jest tylko domyslny.
  if (!storageDir_.empty() && std::ranges::none_of(plan->tree, [](const query &qry) { return qry.id == ":STORAGE"; })) {
    query directive;
    directive.id       = ":STORAGE";
    directive.filename = storageDir_;
    plan->tree.push_back(directive);
  }

  // Artefakty poprzedniego przebiegu, jak przed startem demona i przed `xqry --reset`:
  // magazyn SELECT-a zastany na dysku liczylby dalej od starych rekordow.
  dropStalePlanArtifacts(plan->tree, plan->cm, loaded.lines);
  // Deklaracje demon pomija, a tu ich .desc MUSI zejsc: storage::attachDescriptor bierze
  // istniejacy plik .desc przed deskryptorem z planu i sprawdza tylko pola danych, wiec
  // zastane `REF "stary.txt"` kazaloby czytac poprzedni plik zrodlowy mimo nowego FILE w
  // planie. W notatniku, ktory uruchamia komorki wielokrotnie w jednym katalogu, to jest
  // przypadek normalny, nie brzegowy. Plan jest jedynym zrodlem prawdy o deklaracji.
  const std::filesystem::path storageDir(planStorageDir(plan->tree, storageDir_));
  for (const auto &qry : plan->tree) {
    if (!qry.isDeclaration()) continue;
    std::error_code ignored;
    std::filesystem::remove(storageDir / (qry.id + ".desc"), ignored);
    std::filesystem::remove(storageDir / (qry.id + ".meta"), ignored);
  }

  // Jak `xretractor -u`: brak zawijania musi wejsc do modelu PRZED jego budowa, bo isOneShot
  // trafia do fabryki akcesorow w konstruktorze streamInstance.
  if (untilEof)
    for (auto &qry : plan->tree)
      if (qry.isDeclaration()) qry.isOneShot = true;

  // Model dostaje sklep MEMORY TEGO silnika - to jest chwila, w ktorej izolacja z fazy 2
  // obejmuje plan, a nie tylko magazyn otwarty przez openStorage().
  plan->model = std::make_unique<dataModel>(plan->tree, &memory_);
  // Po budowie modelu, tak jak w petli demona: dataModel usuwa dyrektywy z drzewa, a os
  // czasu ma powstac z samych strumieni.
  plan->timeline = std::make_unique<CRationalStreamMath::TimeLine>(plan->tree.getAvailableTimeIntervals());

  plan_ = std::move(plan);
}

std::optional<std::uint64_t> Engine::step() {
  Plan &plan = requirePlan("step");
  if (plan.endOfInput) return std::nullopt;

  if (!plan.bootstrapped) {
    plan.model->processZeroStep();
    plan.bootstrapped = true;
  }

  // Cialo slotu z executorsm::run(), bez czekania na zegar i bez rozglaszania IPC:
  // os czasu -> zbior strumieni oczekujacych w tym slocie -> przeliczenie.
  const boost::rational<int> slot = plan.timeline->getNextTimeSlot();
  std::set<std::string> awaited;
  for (const auto &qry : plan.tree)
    if (plan.timeline->isThisDeltaAwaitCurrentTimeSlot(qry.rInterval)) awaited.insert(qry.id);

  plan.model->processRows(awaited, slot);
  plan.time                 = slot;
  const std::uint64_t index = plan.slotsDone++;

  // Deklaracje czytane sa na koncu slotu, a ich rekord konsumuje dopiero slot nastepny,
  // wiec koniec wejscia wykryty TERAZ zostawia policzone rekordy poprawne i zatrzymuje
  // dopiero nastepne wywolanie - ta sama regula co przy --until-eof w petli demona.
  if (plan.untilEof && !plan.model->exhaustedInputStream().empty()) plan.endOfInput = true;

  return index;
}

std::uint64_t Engine::slotsDone() const noexcept { return plan_ ? plan_->slotsDone : 0; }

boost::rational<int> Engine::time() const noexcept { return plan_ ? plan_->time : boost::rational<int>(0); }

bool Engine::endOfInput() const noexcept { return plan_ != nullptr && plan_->endOfInput; }

std::vector<std::string> Engine::streams() const {
  const Plan &plan = requirePlan("streams");
  std::vector<std::string> retVal;
  retVal.reserve(plan.tree.size());
  for (const auto &qry : plan.tree)
    retVal.push_back(qry.id);
  return retVal;
}

const Descriptor &Engine::schema(const std::string &stream) const { return streamStorage(stream, "schema").descriptor; }

bool Engine::isDeclared(const std::string &stream) const { return streamStorage(stream, "isDeclared").isDeclared(); }

std::size_t Engine::recordCount(const std::string &stream) const {
  return streamStorage(stream, "recordCount").getRecordsCount();
}

std::size_t Engine::retainedFrom(const std::string &stream) const {
  storage &store          = streamStorage(stream, "retainedFrom");
  const std::size_t count = store.getRecordsCount();
  std::size_t retained    = count;
  if (store.isDeclared()) {
    retained = store.historySize();
  } else if (const auto [type, retention] = store.descriptor.storagePolicy(); type == "MEMORY" && retention != 0) {
    // Strumien MEMORY (VOLATILE) jest pierscieniem o rozmiarze wyznaczonym przez kompilator
    // z potrzeb konsumentow - czesto 1. memoryFile::read(pozycja) liczy slot modulo ten
    // rozmiar, wiec indeks spoza pierscienia oddalby cicho CUDZY rekord, nie blad.
    retained = std::min(count, retention);
  }
  return count - retained;
}

payload Engine::record(const std::string &stream, const std::size_t index) {
  storage &store          = streamStorage(stream, "record");
  const std::size_t count = store.getRecordsCount();
  if (index >= count) {
    throw ConfigError(std::format("Engine::record: stream '{}' has {} records, index {} is out of range", stream, count, index));
  }
  if (const std::size_t oldest = retainedFrom(stream); index < oldest) {
    throw ConfigError(std::format("Engine::record: stream '{}' retains only records {}..{} of its {}; record {} is gone", stream,
                                  oldest, count - 1, count, index));
  }

  if (store.isDeclared()) {
    // Zrodlo deklarowane nie ma pliku do odczytu wstecz - tylko bufor historii. history()
    // jest const i niczego w zrodle nie rusza; revRead(0) na zrodle w stanie flux WCZYTALBY
    // nastepny rekord, czyli zjadl wejscie.
    return store.history(count - 1 - index);
  }

  // Do WLASNEGO bufora: wewnetrzny payload magazynu jest stanem modelu, a jedyne, co read()
  // w nim zmienia przy podanym celu, to mapa NULL - i wlasnie ja stad przepisujemy.
  payload out(store.descriptor);
  store.read(index, out.span().data());
  out.setNullBitset(store.getPayload()->getNullBitset());
  return out;
}

std::vector<double> Engine::project(const std::string &stream, const std::vector<int> &flatFields, const std::size_t first,
                                    const std::size_t count) {
  const storage &store    = streamStorage(stream, "project");
  const std::size_t total = store.getRecordsCount();
  if (first > total || count > total - first) {
    throw ConfigError(std::format("Engine::project: stream '{}' has {} records, range [{}, {}) is out of range", stream, total,
                                  first, first + count));
  }
  const int flatCount = store.descriptor.flatElementCount();
  for (const int flat : flatFields) {
    if (flat < 0 || flat >= flatCount) {
      throw ConfigError(
          std::format("Engine::project: stream '{}' has {} flat elements, index {} is out of range", stream, flatCount, flat));
    }
  }

  std::vector<double> block;
  block.reserve(count * flatFields.size());
  for (std::size_t row = 0; row < count; ++row) {
    const payload item = record(stream, first + row);
    for (const int flat : flatFields) {
      const auto value = item.getItemVT(flat);
      if (!value.has_value()) {
        block.push_back(std::numeric_limits<double>::quiet_NaN());
        continue;
      }
      block.push_back(std::visit(Overload{
                                     [](const std::monostate &) { return std::numeric_limits<double>::quiet_NaN(); },
                                     [](const std::uint8_t arg) { return static_cast<double>(arg); },
                                     [](const int arg) { return static_cast<double>(arg); },
                                     [](const unsigned int arg) { return static_cast<double>(arg); },
                                     [](const float arg) { return static_cast<double>(arg); },
                                     [](const double arg) { return arg; },
                                     [](const boost::rational<int> &arg) { return boost::rational_cast<double>(arg); },
                                     [&](const std::pair<int, int> &) -> double {
                                       throw ConfigError(std::format(
                                           "Engine::project: stream '{}' element {} is an INTPAIR, not a number", stream, flat));
                                     },
                                     [&](const std::pair<std::string, int> &) -> double {
                                       throw ConfigError(std::format(
                                           "Engine::project: stream '{}' element {} is an IDXPAIR, not a number", stream, flat));
                                     },
                                     [&](const std::string &) -> double {
                                       throw ConfigError(std::format(
                                           "Engine::project: stream '{}' element {} is a STRING, not a number", stream, flat));
                                     },
                                 },
                                 value.value()));
    }
  }
  return block;
}

void Engine::close() noexcept {
  // Destruktor magazynu potrafi zglosic blad (np. zapis deskryptora) - z close() i z ~Engine
  // wyjatek nie ma prawa wyjsc, bo host wola je ze swojego __del__ / __exit__.
  try {
    plan_.reset();
  } catch (...) {  // NOLINT(bugprone-empty-catch)
  }
}

Engine::Plan &Engine::requirePlan(const char *operation) const {
  if (!plan_) throw ConfigError(std::format("Engine::{}: no plan - call compile() first", operation));
  return *plan_;
}

storage &Engine::streamStorage(const std::string &stream, const char *operation) const {
  Plan &plan       = requirePlan(operation);
  const auto found = plan.model->qSet.find(stream);
  if (found == plan.model->qSet.end()) {
    throw ConfigError(std::format("Engine::{}: no stream '{}' in the plan", operation, stream));
  }
  return *found->second->outputPayload;
}

}  // namespace rdb::embed
