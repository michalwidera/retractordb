#include "rdb/embed/engine.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
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

#include "fatalError.hpp"
#include "fldType.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/CRSMath.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/planSource.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/query.hpp"
#include "retractor/lib/rule.hpp"

namespace rdb::embed {

namespace {

[[noreturn]] void refuse(const Refused::Stage stage, std::string message) {
  const ErrorClass cls = stage == Refused::Stage::Request ? ErrorClass::Request : ErrorClass::Plan;
  throw Refused(stage, Refusal{.cls = cls, .message = std::move(message)});
}

}  // namespace

/// Wszystko, co zyje od compile() do close(). Kolejnosc pol jest kolejnoscia zaleznosci: kompilator,
/// model i os czasu trzymaja referencje do drzewa, wiec drzewo stoi pierwsze i niszczy sie ostatnie -
/// ta sama zasada, ktorej pilnuje EpochPublication w executorsm.cpp, tutaj egzekwowana ukladem
/// struktury.
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
  /// Maska strumieni naleznych w slocie, pozycyjna wzgledem drzewa - ta sama, ktora buduje
  /// executorsm::collectAwaitedStreams. Pole planu, a nie zmienna slotu: pojemnosc zostaje miedzy
  /// taktami.
  std::vector<char> dueMask;
  /// Tresc pierwszego bledu w slocie; obecna = plan zatrzasniety (patrz Engine).
  std::optional<std::string> failure;
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
  const ThrowOnFatalScope throwOnFatal;
  return std::make_unique<storage>(qryID, fileName, storageParam, storageType, oneShot, isHold, percounter, &memory_);
}

void Engine::compile(const std::string_view rql, const bool untilEof) {
  // Poprzedni plan schodzi PRZED budowa nowego: oba pisalyby do tych samych plikow magazynu.
  close();

  const ThrowOnFatalScope throwOnFatal;
  auto plan      = std::make_unique<Plan>();
  plan->untilEof = untilEof;

  // Ta sama droga, ktora idzie launcher (parsePlanText + compiler::compile) i przeladowanie planu
  // w locie (`xqry --reset`); status "OK" albo tresc odmowy.
  const PlanSource loaded = parsePlanText(plan->tree, std::string(rql));
  if (loaded.status != "OK") refuse(Refused::Stage::Parse, loaded.status);
  if (plan->tree.empty()) refuse(Refused::Stage::Check, "plan holds no statements");

  std::string response;
  try {
    response = plan->cm.compile();
  } catch (const std::logic_error &error) {
    // Odwolanie do strumienia, ktorego plan nie deklaruje (`FROM nosuch`), kompilator zglasza
    // rzutem std::logic_error z qTree::getQuery, nie statusem "Check result:" - usluga konczy sie
    // wtedy kodem EINTR. Dla planu podanego przez hosta to blad WEJSCIA, wiec odmowa etapu Check.
    // EngineFatal nie dziedziczy po std::exception i przechodzi tedy bez zmian.
    refuse(Refused::Stage::Check, error.what());
  }
  if (response != "OK") refuse(Refused::Stage::Check, response);

  for (const auto &qry : plan->tree) {
    if (qry.id == ":ROTATION") {
      refuse(Refused::Stage::Check,
             ":ROTATION is not available in the embedded engine - the rotation counter belongs to the service");
    }
    // Faza DEVICE (#347) zbiera bajty zrodla przed processRows z terminem z konfiguracji uslugi;
    // step() jej nie ma, wiec zrodlo dawaloby po cichu same rekordy all-null.
    if (qry.kind == sourceKind::device) {
      refuse(Refused::Stage::Check, std::format("stream '{}': DEVICE sources are not available in the embedded engine", qry.id));
    }
    for (const auto &item : qry.lRules) {
      // DUMP siega po model przez globalny pProc (dumpManager.cpp), ktorego silnik osadzony nie
      // publikuje; SYSTEM wykonalby ::system() w procesie hosta.
      if (item.action == rule::DUMP) {
        refuse(Refused::Stage::Check,
               std::format("stream '{}', rule '{}': DUMP actions are not available in the embedded engine", qry.id, item.name));
      }
      if (item.action == rule::SYSTEM) {
        refuse(
            Refused::Stage::Check,
            std::format("stream '{}', rule '{}': SYSTEM actions are not available in the embedded engine", qry.id, item.name));
      }
    }
  }

  // Domyslny katalog magazynu, gdy plan nie niesie :STORAGE - jak launcher z `[storage] dir`: RQL
  // ma pierwszenstwo, katalog silnika jest tylko domyslny.
  if (!storageDir_.empty() && std::ranges::none_of(plan->tree, [](const query &qry) { return qry.id == ":STORAGE"; })) {
    query directive;
    directive.id       = ":STORAGE";
    directive.filename = storageDir_;
    plan->tree.push_back(directive);
  }

  // Zachowanie z galezi feature/jupyter-integration, do decyzji w #450: .desc i .meta deklaracji
  // schodza PRZED kontrola magazynu. Usluga ich nie kasuje, a checkDescriptorFiles odrzuca zastany
  // .desc deklaracji niezgodny z planem (#265); w notatniku, ktory uruchamia komorki wielokrotnie
  // w jednym katalogu, zmiana pliku zrodlowego miedzy uruchomieniami jest przypadkiem normalnym,
  // wiec plan jest tu jedynym zrodlem prawdy o deklaracji.
  const std::filesystem::path storagePath(planStorageDir(plan->tree, storageDir_));
  for (const auto &qry : plan->tree) {
    if (!qry.isDeclaration()) continue;
    std::error_code ignored;
    std::filesystem::remove(storagePath / (qry.id + ".desc"), ignored);
    std::filesystem::remove(storagePath / (qry.id + ".meta"), ignored);
  }

  // Ten sam preflight co start uslugi (#473): katalog magazynu, zrodla deklarowane, zachowane .desc.
  // Klasa odmowy zostaje: Environment (np. nieczytelny katalog) binding tlumaczy inaczej niz Plan.
  if (Refusal refusal = checkPlanStorage(plan->tree, storageDir_); !refusal.message.empty()) {
    throw Refused(Refused::Stage::Check, std::move(refusal));
  }

  // Artefakty poprzedniego przebiegu, jak przed startem uslugi i przed `xqry --reset`: magazyn
  // SELECT-a zastany na dysku liczylby dalej od starych rekordow.
  dropStalePlanArtifacts(plan->tree);

  // Jak `xretractor -u`: brak zawijania musi wejsc do modelu PRZED jego budowa, bo isOneShot trafia
  // do fabryki akcesorow w konstruktorze streamInstance.
  if (untilEof)
    for (auto &qry : plan->tree)
      if (qry.isDeclaration()) qry.isOneShot = true;

  // Model dostaje Memory Store TEGO silnika - w tej chwili izolacja obejmuje plan, a nie tylko
  // magazyn otwarty przez openStorage().
  plan->model = std::make_unique<dataModel>(plan->tree, &memory_);
  if (!plan->model->initializationError.message.empty()) {
    throw Refused(Refused::Stage::Check, plan->model->initializationError);
  }

  // Po budowie modelu, tak jak w petli uslugi: dataModel usuwa dyrektywy z drzewa, a os czasu ma
  // powstac z samych strumieni. Plan z samymi dyrektywami przechodzi kompilacje, a os czasu bez
  // interwalu nie istnieje - odmowa nalezy do granicy.
  const std::set<boost::rational<int>> intervals = plan->tree.getAvailableTimeIntervals();
  if (intervals.empty()) refuse(Refused::Stage::Check, "plan holds no streams - only directives");
  plan->timeline = std::make_unique<CRationalStreamMath::TimeLine>(intervals);

  plan_ = std::move(plan);
}

std::optional<std::uint64_t> Engine::step() {
  Plan &plan = requirePlan("step");
  if (plan.failure.has_value()) {
    throw EngineFatal{.cls     = ErrorClass::Internal,
                      .message = std::format("Engine::step: the plan stopped at slot {} with: {} - compile it again",
                                             plan.slotsDone, *plan.failure),
                      .where   = std::source_location::current()};
  }
  const ThrowOnFatalScope throwOnFatal;
  try {
    return stepPlan(plan);
  } catch (const EngineFatal &fault) {
    plan.failure = fault.message;
    throw;
  } catch (const std::exception &error) {
    plan.failure = error.what();
    throw;
  }
}

std::optional<std::uint64_t> Engine::stepPlan(Plan &plan) {
  if (plan.endOfInput) return std::nullopt;

  if (!plan.bootstrapped) {
    plan.model->processZeroStep();
    plan.bootstrapped = true;
  }

  // Cialo slotu z executorsm::run(), bez czekania na zegar i bez rozglaszania IPC:
  // os czasu -> maska strumieni naleznych w tym slocie -> przeliczenie.
  const boost::rational<int> slot = plan.timeline->getNextTimeSlot();
  plan.dueMask.assign(plan.tree.size(), 0);
  std::size_t position = 0;
  for (const auto &qry : plan.tree) {
    if (plan.timeline->isThisDeltaAwaitCurrentTimeSlot(qry.rInterval)) plan.dueMask[position] = 1;
    ++position;
  }

  plan.model->processRows(plan.dueMask, slot);
  plan.time                 = slot;
  const std::uint64_t index = plan.slotsDone++;

  // Deklaracje czytane sa na koncu slotu, a ich rekord konsumuje dopiero slot nastepny, wiec koniec
  // wejscia wykryty TERAZ zostawia policzone rekordy poprawne i zatrzymuje dopiero nastepne
  // wywolanie - ta sama regula co przy --until-eof w petli uslugi.
  if (plan.untilEof && !plan.model->exhaustedInputStream().empty()) plan.endOfInput = true;

  return index;
}

std::uint64_t Engine::slotsDone() const noexcept { return plan_ ? plan_->slotsDone : 0; }

boost::rational<int> Engine::time() const noexcept { return plan_ ? plan_->time : boost::rational<int>(0); }

bool Engine::endOfInput() const noexcept { return plan_ != nullptr && plan_->endOfInput; }

bool Engine::failed() const noexcept { return plan_ != nullptr && plan_->failure.has_value(); }

std::vector<std::string> Engine::streams() const {
  const Plan &plan = requirePlan("streams");
  std::vector<std::string> retVal;
  retVal.reserve(plan.tree.size());
  for (const auto &qry : plan.tree)
    retVal.push_back(qry.id);
  return retVal;
}

const Descriptor &Engine::schema(const std::string &stream) const { return streamStorage(stream, "schema").descriptor; }

bool Engine::isDeclared(const std::string &stream) const {
  storage &store = streamStorage(stream, "isDeclared");
  const ThrowOnFatalScope throwOnFatal;
  return store.isDeclared();
}

std::size_t Engine::recordCount(const std::string &stream) const {
  storage &store = streamStorage(stream, "recordCount");
  const ThrowOnFatalScope throwOnFatal;
  return store.getRecordsCount();
}

std::size_t Engine::retainedFrom(const std::string &stream) const {
  storage &store = streamStorage(stream, "retainedFrom");
  const ThrowOnFatalScope throwOnFatal;
  const std::size_t count = store.getRecordsCount();
  std::size_t retained    = count;
  if (store.isDeclared()) {
    retained = store.historySize();
  } else if (const auto [type, retention] = store.descriptor.storagePolicy(); type == "MEMORY" && retention != 0) {
    // Strumien MEMORY (VOLATILE) jest pierscieniem o rozmiarze wyznaczonym przez kompilator z
    // potrzeb konsumentow - czesto 1. memoryFile::read(pozycja) liczy slot modulo ten rozmiar, wiec
    // indeks spoza pierscienia oddalby cicho CUDZY rekord, nie blad.
    retained = std::min(count, retention);
  }
  return count - retained;
}

payload Engine::record(const std::string &stream, const std::size_t index) {
  storage &store          = streamStorage(stream, "record");
  const std::size_t count = recordCount(stream);
  if (index >= count) {
    refuse(Refused::Stage::Request,
           std::format("Engine::record: stream '{}' has {} records, index {} is out of range", stream, count, index));
  }
  if (const std::size_t oldest = retainedFrom(stream); index < oldest) {
    refuse(Refused::Stage::Request,
           std::format("Engine::record: stream '{}' retains only records {}..{} of its {}; record {} is gone", stream, oldest,
                       count - 1, count, index));
  }

  const ThrowOnFatalScope throwOnFatal;
  if (store.isDeclared()) {
    // Zrodlo deklarowane nie ma pliku do odczytu wstecz - tylko bufor historii. history() jest
    // const i niczego w zrodle nie rusza; revRead(0) na zrodle w stanie flux WCZYTALBY nastepny
    // rekord, czyli zjadl wejscie.
    return store.history(count - 1 - index);
  }

  // Do WLASNEGO bufora: wewnetrzny payload magazynu jest stanem modelu, a jedyne, co read() w nim
  // zmienia przy podanym celu, to mapa NULL - i wlasnie ja stad przepisujemy. Status bez znaczenia:
  // zakres [retainedFrom, count) sprawdzony wyzej, a NoSuchRecord i tak zostawia mape all-null.
  payload out(store.descriptor);
  static_cast<void>(store.read(index, out.span().data()));
  out.setNullBitset(store.getPayload()->getNullBitset());
  return out;
}

std::vector<double> Engine::project(const std::string &stream, const std::vector<int> &flatFields, const std::size_t first,
                                    const std::size_t count) {
  const std::size_t total = recordCount(stream);
  if (first > total || count > total - first) {
    refuse(Refused::Stage::Request, std::format("Engine::project: stream '{}' has {} records, range [{}, {}) is out of range",
                                                stream, total, first, first + count));
  }
  const int flatCount = schema(stream).flatElementCount();
  for (const int flat : flatFields) {
    if (flat < 0 || flat >= flatCount) {
      refuse(Refused::Stage::Request, std::format("Engine::project: stream '{}' has {} flat elements, index {} is out of range",
                                                  stream, flatCount, flat));
    }
  }

  constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> block;
  block.reserve(count * flatFields.size());
  for (std::size_t row = 0; row < count; ++row) {
    const payload item = record(stream, first + row);
    for (const int flat : flatFields) {
      const auto value = item.getItemVT(flat);
      if (!value.has_value()) {
        block.push_back(kNaN);
        continue;
      }
      const auto notNumber = [&](const char *what) -> double {
        refuse(Refused::Stage::Request,
               std::format("Engine::project: stream '{}' element {} is {}, not a number", stream, flat, what));
      };
      block.push_back(std::visit(Overload{
                                     [](const std::monostate &) { return kNaN; },
                                     [](const std::uint8_t arg) { return static_cast<double>(arg); },
                                     [](const int arg) { return static_cast<double>(arg); },
                                     [](const unsigned int arg) { return static_cast<double>(arg); },
                                     [](const float arg) { return static_cast<double>(arg); },
                                     [](const double arg) { return arg; },
                                     [](const boost::rational<int> &arg) { return boost::rational_cast<double>(arg); },
                                     [&](const std::pair<int, int> &) { return notNumber("an INTPAIR"); },
                                     [&](const std::pair<std::string, int> &) { return notNumber("an IDXPAIR"); },
                                     [&](const std::string &) { return notNumber("a STRING"); },
                                 },
                                 value.value()));
    }
  }
  return block;
}

void Engine::close() noexcept {
  // Bez ThrowOnFatalScope: close() i ~Engine sa noexcept (host wola je z __del__ / __exit__), wiec
  // K1/K4 z destruktora modelu albo magazynu konczy proces jak w usludze. Pelna polityka - #476.
  plan_.reset();
}

Engine::Plan &Engine::requirePlan(const char *operation) const {
  if (!plan_) refuse(Refused::Stage::Request, std::format("Engine::{}: no plan - call compile() first", operation));
  return *plan_;
}

storage &Engine::streamStorage(const std::string &stream, const char *operation) const {
  Plan &plan       = requirePlan(operation);
  const auto found = plan.model->qSet.find(stream);
  if (found == plan.model->qSet.end()) {
    refuse(Refused::Stage::Request, std::format("Engine::{}: no stream '{}' in the plan", operation, stream));
  }
  return *found->second->outputPayload;
}

}  // namespace rdb::embed
