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
  /// Slot zwrocil blad: model moze stac w polowie przeliczenia, wiec dalsze step() odmawia.
  /// Trzyma tresc pierwszego bledu, zeby kolejna odmowa mowila, CO sie stalo, a nie tylko ze.
  std::optional<Error> failure;
  boost::rational<int> time{0};
  /// Maska strumieni naleznych w slocie, pozycyjna wzgledem drzewa - ta sama, ktora buduje
  /// executorsm::collectAwaitedStreams. Pole planu, a nie zmienna slotu: pojemnosc zostaje
  /// miedzy taktami, wiec krok nie alokuje.
  std::vector<char> dueMask;
};

Engine::Engine(std::string storageDir) : storageDir_(std::move(storageDir)) {}

Engine::~Engine() { close(); }

Result<std::unique_ptr<storage>> Engine::openStorage(const std::string_view qryID,         //
                                                     const std::string_view fileName,      //
                                                     const std::string_view storageParam,  //
                                                     const std::string_view storageType,   //
                                                     const bool oneShot,                   //
                                                     const bool isHold,                    //
                                                     const int percounter) {
  return storage::create(qryID, fileName, storageParam, storageType, oneShot, isHold, percounter, &memory_);
}

Result<> Engine::compile(const std::string_view rql, const bool untilEof) {
  // Poprzedni plan schodzi PRZED budowa nowego: oba pisalyby do tych samych plikow magazynu.
  close();

  auto plan      = std::make_unique<Plan>();
  plan->untilEof = untilEof;

  // Ta sama droga, ktora idzie launcher (parsePlanText + compiler::compile) i przeladowanie
  // planu w locie (`xqry --reset`); status "OK" albo tresc bledu. Status z przedrostkiem
  // kInternalCompilerError to nie blad planu, tylko blad w parserze/kompilatorze (dawny
  // LogicError) - host ma go dostac jako InternalError, nie jako "popraw swoj RQL".
  const PlanSource loaded = parsePlanText(plan->tree, std::string(rql));
  if (loaded.status != "OK") return fail(isInternalCompilerError(loaded.status) ? Errc::Logic : Errc::Syntax, loaded.status);
  if (plan->tree.empty()) return fail(Errc::Compile, "plan holds no statements");

  // Odwolanie do strumienia, ktorego plan nie deklaruje (`FROM nosuch`), do 2026-10 konczylo
  // kompilacje std::logic_error z qTree::getQuery, lapanym tu i zamienianym na CompileError.
  // Teraz odrzuca je przebieg compiler::checkStreamReferences statusem - tym samym kanalem co
  // kazdy inny blad planu.
  if (const std::string response = plan->cm.compile(); response != "OK") {
    return fail(isInternalCompilerError(response) ? Errc::Logic : Errc::Compile, response);
  }

  // Trzy rzeczy, ktorych silnik osadzony NIE MA, odrzucane tutaj, a nie w polowie slotu:
  //  - :ROTATION czyta licznik rotacji z globalnego pCounterPtr demona (streamInstance.cpp);
  //    bez niego dyrektywa bylaby po cichu ignorowana, a plan pisalby bez rotacji;
  //  - regula DUMP siega po model przez globalny pProc (dumpManager.cpp), ktorego tu nie ma -
  //    zamiast bledu o pustym wskazniku w srodku slotu uzytkownik dostaje odpowiedz
  //    przy compile();
  //  - regula SYSTEM wykonuje ::system() w procesie hosta; roadmapa (iOS faza 3, pkt 5)
  //    kaze ja bramkowac wywolaniem zwrotnym hosta, DOMYSLNIE WYLACZONYM. Bramki jeszcze nie
  //    ma, wiec obowiazuje "wylaczone".
  for (const auto &qry : plan->tree) {
    if (qry.id == ":ROTATION") {
      return fail(Errc::Compile,
                  ":ROTATION is not available in the embedded engine - the rotation counter belongs to the daemon");
    }
    for (const auto &item : qry.lRules) {
      if (item.action == rule::DUMP) {
        return fail(Errc::Compile, std::format("stream '{}', rule '{}': DUMP actions are not available in the embedded engine",
                                               qry.id, item.name));
      }
      if (item.action == rule::SYSTEM) {
        return fail(Errc::Compile, std::format("stream '{}', rule '{}': SYSTEM actions are not available in the embedded engine",
                                               qry.id, item.name));
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
  dropStalePlanArtifacts(plan->tree);
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
  RDB_TRY_ASSIGN(plan->model, dataModel::create(plan->tree, &memory_));
  // Po budowie modelu, tak jak w petli demona: dataModel usuwa dyrektywy z drzewa, a os
  // czasu ma powstac z samych strumieni.
  RDB_TRY_ASSIGN(const auto intervals, plan->tree.getAvailableTimeIntervals());
  // Os czasu bez ani jednego interwalu to niezmiennik TimeLine (RDB_ASSERT). Plan z samymi
  // dyrektywami przechodzi kompilacje, wiec odmowa nalezy do granicy - tutaj.
  if (intervals.empty()) return fail(Errc::Compile, "plan holds no streams - only directives");
  plan->timeline = std::make_unique<CRationalStreamMath::TimeLine>(intervals);

  plan_ = std::move(plan);
  return {};
}

Result<std::optional<std::uint64_t>> Engine::step() {
  RDB_TRY_ASSIGN(Plan *const plan, requirePlan("step"));
  if (plan->failure.has_value()) {
    return fail(Errc::Logic, std::format("Engine::step: the plan stopped at slot {} with: {} - compile it again",
                                         plan->slotsDone, plan->failure->message()));
  }
  auto done = stepPlan(*plan);
  // Kopia bledu, nie przeniesienie: oryginal idzie do wolajacego, kopia zostaje w planie jako
  // przyczyna kazdej nastepnej odmowy. Sciezka bez bledu niczego nie kopiuje.
  if (!done) [[unlikely]]
    plan->failure = done.error();
  return done;
}

Result<std::optional<std::uint64_t>> Engine::stepPlan(Plan &plan) {
  if (plan.endOfInput) return std::nullopt;

  if (!plan.bootstrapped) {
    RDB_TRY(plan.model->processZeroStep());
    plan.bootstrapped = true;
  }

  // Cialo slotu z executorsm::run(), bez czekania na zegar i bez rozglaszania IPC:
  // os czasu -> maska strumieni oczekujacych w tym slocie -> przeliczenie.
  const boost::rational<int> slot = plan.timeline->getNextTimeSlot();
  plan.dueMask.assign(plan.tree.size(), 0);
  std::size_t position = 0;
  for (const auto &qry : plan.tree) {
    if (plan.timeline->isThisDeltaAwaitCurrentTimeSlot(qry.rInterval)) plan.dueMask[position] = 1;
    ++position;
  }

  RDB_TRY(plan.model->processRows(plan.dueMask, slot));
  plan.time                 = slot;
  const std::uint64_t index = plan.slotsDone++;

  // Deklaracje czytane sa na koncu slotu, a ich rekord konsumuje dopiero slot nastepny,
  // wiec koniec wejscia wykryty TERAZ zostawia policzone rekordy poprawne i zatrzymuje
  // dopiero nastepne wywolanie - ta sama regula co przy --until-eof w petli demona.
  if (plan.untilEof && !plan.model->exhaustedInputStream().empty()) plan.endOfInput = true;

  return index;
}

bool Engine::failed() const noexcept { return plan_ != nullptr && plan_->failure.has_value(); }

std::uint64_t Engine::slotsDone() const noexcept { return plan_ ? plan_->slotsDone : 0; }

boost::rational<int> Engine::time() const noexcept { return plan_ ? plan_->time : boost::rational<int>(0); }

bool Engine::endOfInput() const noexcept { return plan_ != nullptr && plan_->endOfInput; }

Result<std::vector<std::string>> Engine::streams() const {
  RDB_TRY_ASSIGN(const Plan *const plan, requirePlan("streams"));
  std::vector<std::string> retVal;
  retVal.reserve(plan->tree.size());
  for (const auto &qry : plan->tree)
    retVal.push_back(qry.id);
  return retVal;
}

Result<const Descriptor *> Engine::schema(const std::string &stream) const {
  RDB_TRY_ASSIGN(const storage *const store, streamStorage(stream, "schema"));
  return &store->descriptor;
}

Result<bool> Engine::isDeclared(const std::string &stream) const {
  RDB_TRY_ASSIGN(const storage *const store, streamStorage(stream, "isDeclared"));
  return store->isDeclared();
}

Result<std::size_t> Engine::recordCount(const std::string &stream) const {
  RDB_TRY_ASSIGN(const storage *const store, streamStorage(stream, "recordCount"));
  return store->getRecordsCount();
}

Result<std::size_t> Engine::retainedFrom(const std::string &stream) const {
  RDB_TRY_ASSIGN(storage *const found, streamStorage(stream, "retainedFrom"));
  storage &store          = *found;
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

Result<payload> Engine::record(const std::string &stream, const std::size_t index) {
  RDB_TRY_ASSIGN(storage *const found, streamStorage(stream, "record"));
  storage &store          = *found;
  const std::size_t count = store.getRecordsCount();
  if (index >= count) {
    return fail(Errc::Config,
                std::format("Engine::record: stream '{}' has {} records, index {} is out of range", stream, count, index));
  }
  RDB_TRY_ASSIGN(const std::size_t oldest, retainedFrom(stream));
  if (index < oldest) {
    return fail(Errc::Config, std::format("Engine::record: stream '{}' retains only records {}..{} of its {}; record {} is gone",
                                          stream, oldest, count - 1, count, index));
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
  // Status odczytu bez znaczenia: zakres [retainedFrom, count) sprawdzony wyzej, wiec NoSuchRecord
  // nie zapada. Blad odczytu (nosnik, uprawnienia) wraca do hosta.
  RDB_TRY(store.read(index, out.span().data()));
  out.setNullBitset(store.getPayload()->getNullBitset());
  return out;
}

Result<std::vector<double>> Engine::project(const std::string &stream, const std::vector<int> &flatFields,
                                            const std::size_t first, const std::size_t count) {
  RDB_TRY_ASSIGN(const storage *const store, streamStorage(stream, "project"));
  const std::size_t total = store->getRecordsCount();
  if (first > total || count > total - first) {
    return fail(Errc::Config, std::format("Engine::project: stream '{}' has {} records, range [{}, {}) is out of range", stream,
                                          total, first, first + count));
  }
  const int flatCount = store->descriptor.flatElementCount();
  for (const int flat : flatFields) {
    if (flat < 0 || flat >= flatCount) {
      return fail(Errc::Config, std::format("Engine::project: stream '{}' has {} flat elements, index {} is out of range",
                                            stream, flatCount, flat));
    }
  }

  std::vector<double> block;
  block.reserve(count * flatFields.size());
  // Typ elementu, ktory nie jest liczba - zapamietany w wizytatorze, zgloszony po nim.
  const char *notNumber = nullptr;
  for (std::size_t row = 0; row < count; ++row) {
    RDB_TRY_ASSIGN(const payload item, record(stream, first + row));
    for (const int flat : flatFields) {
      const auto value = item.getItemVT(flat);
      if (!value.has_value()) {
        block.push_back(std::numeric_limits<double>::quiet_NaN());
        continue;
      }
      constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
      block.push_back(std::visit(Overload{
                                     [](const std::monostate &) { return kNaN; },
                                     [](const std::uint8_t arg) { return static_cast<double>(arg); },
                                     [](const int arg) { return static_cast<double>(arg); },
                                     [](const unsigned int arg) { return static_cast<double>(arg); },
                                     [](const float arg) { return static_cast<double>(arg); },
                                     [](const double arg) { return arg; },
                                     [](const boost::rational<int> &arg) { return boost::rational_cast<double>(arg); },
                                     [&notNumber](const std::pair<int, int> &) {
                                       notNumber = "an INTPAIR";
                                       return kNaN;
                                     },
                                     [&notNumber](const std::pair<std::string, int> &) {
                                       notNumber = "an IDXPAIR";
                                       return kNaN;
                                     },
                                     [&notNumber](const std::string &) {
                                       notNumber = "a STRING";
                                       return kNaN;
                                     },
                                 },
                                 value.value()));
      if (notNumber != nullptr) [[unlikely]] {
        return fail(Errc::Config,
                    std::format("Engine::project: stream '{}' element {} is {}, not a number", stream, flat, notNumber));
      }
    }
  }
  return block;
}

void Engine::close() noexcept {
  // Destruktory modelu i magazynow nie rzucaja (rdzen buduje sie z -fno-exceptions), wiec
  // close() i ~Engine, wolane przez hosta z __del__ / __exit__, nie potrzebuja juz try/catch.
  // Blad zapisu przy zamykaniu (np. .desc) magazyn loguje sam.
  plan_.reset();
}

Result<Engine::Plan *> Engine::requirePlan(const char *operation) const {
  if (!plan_) return fail(Errc::Config, std::format("Engine::{}: no plan - call compile() first", operation));
  return plan_.get();
}

Result<storage *> Engine::streamStorage(const std::string &stream, const char *operation) const {
  RDB_TRY_ASSIGN(Plan *const plan, requirePlan(operation));
  const auto found = plan->model->qSet.find(stream);
  if (found == plan->model->qSet.end()) {
    return fail(Errc::Config, std::format("Engine::{}: no stream '{}' in the plan", operation, stream));
  }
  return found->second->outputPayload.get();
}

}  // namespace rdb::embed
