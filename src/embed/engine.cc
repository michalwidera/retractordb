#include "rdb/embed/engine.hpp"

#include <fcntl.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
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

#include "fldType.hpp"
#include "rdb/embed/storageGuard.hpp"
#include "rdb/error.hpp"
#include "rdb/storageFile.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/CRSMath.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/planSource.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/query.hpp"
#include "retractor/lib/rule.hpp"

namespace rdb::embed {

/// Wszystko, co zyje od compile() do close(). Kolejnosc pol jest kolejnoscia zaleznosci: kompilator,
/// model i os czasu trzymaja referencje do drzewa, wiec drzewo stoi pierwsze i niszczy sie ostatnie -
/// ta sama zasada, ktorej pilnuje EpochPublication w executorsm.cpp, tutaj egzekwowana ukladem
/// struktury.
struct Engine::Plan {
  // Zwalnia sie PO modelu i wszystkich jego magazynach.
  std::unique_ptr<StorageGuard> guard;
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
  /// Pierwszy blad w slocie; obecny = plan zatrzasniety (patrz Engine). Klasa zostaje klasa tego
  /// bledu: zatrzask po K4 zglaszany jako K1 kazalby hostowi uznac za zepsuty caly silnik.
  std::optional<Error> failure;
};

Engine::Engine(OfflinePolicy offline, std::string storageDir, const bool temporaryDir)
    : offline_(std::move(offline)),
      storageDir_(std::move(storageDir)),
      temporaryDir_(temporaryDir) {}

Result<std::unique_ptr<Engine>> Engine::create(std::string storageDir, const std::string &lockDir,
                                               const std::string &configPath) {
  RDB_TRY_ASSIGN(auto offline, offlinePolicy(lockDir, configPath));
  const bool temporary = storageDir.empty();
  if (temporary) {
    RDB_TRY_ASSIGN(storageDir, temporaryStorageDirectory());
  }
  return std::unique_ptr<Engine>(new Engine(std::move(offline), std::move(storageDir), temporary));
}

Engine::~Engine() {
  close();
  if (temporaryDir_) removeTemporaryStorage(storageDir_);
}

const std::string &Engine::storageDir() const noexcept { return plan_ ? plan_->guard->directory() : storageDir_; }

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
  warnings_.clear();

  auto plan      = std::make_unique<Plan>();
  plan->untilEof = untilEof;

  // Ta sama droga, ktora idzie launcher (parsePlanText + compiler::compile) i przeladowanie planu
  // w locie (`xqry --reset`); status "OK" albo tresc odmowy.
  const PlanSource loaded = parsePlanText(plan->tree, std::string(rql));
  if (loaded.status != "OK") return fail(isInternalCompilerError(loaded.status) ? Errc::Logic : Errc::Syntax, loaded.status);
  if (plan->tree.empty()) return fail(Errc::Compile, "plan holds no statements");

  // Ostrzezenia K6 zbierane PRZED kompilacja: kompilator dopisuje do drzewa wezly posrednie, ktore
  // moglyby powtorzyc ostrzezenie zapytania, z ktorego powstaly. Odkladane do konca compile(), zeby
  // odmowa nie zostawila ostrzezen planu, ktorego nie ma.
  std::vector<std::string> warnings = deprecatedFileWarnings(plan->tree);
  for (const auto &qry : plan->tree)
    warnings.insert(warnings.end(), qry.deprecatedNotations.begin(), qry.deprecatedNotations.end());

  if (const std::string response = plan->cm.compile(); response != "OK")
    return fail(isInternalCompilerError(response) ? Errc::Logic : Errc::Compile, response);

  for (const auto &qry : plan->tree) {
    if (qry.id == ":ROTATION") {
      return fail(Errc::Compile,
                  ":ROTATION is not available in the embedded engine - the rotation counter belongs to the service");
    }
    // Faza DEVICE (#347) zbiera bajty zrodla przed processRows z terminem z konfiguracji uslugi;
    // step() jej nie ma, wiec zrodlo dawaloby po cichu same rekordy all-null.
    if (qry.kind == sourceKind::device) {
      return fail(Errc::Compile, std::format("stream '{}': DEVICE sources are not available in the embedded engine", qry.id));
    }
    for (const auto &item : qry.lRules) {
      // DUMP siega po model przez globalny pProc (dumpManager.cpp), ktorego silnik osadzony nie
      // publikuje; SYSTEM wykonalby ::system() w procesie hosta.
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

  // Domyslny katalog magazynu, gdy plan nie niesie :STORAGE - jak launcher z `[storage] dir`: RQL
  // ma pierwszenstwo, katalog silnika jest tylko domyslny.
  const bool defaultDir = std::ranges::none_of(plan->tree, [](const query &qry) { return qry.id == ":STORAGE"; });
  if (!storageDir_.empty() && defaultDir) {
    query directive;
    directive.id       = ":STORAGE";
    directive.filename = storageDir_;
    plan->tree.push_back(directive);
  }

  RDB_TRY_ASSIGN(const auto availableIntervals, plan->tree.getAvailableTimeIntervals());
  if (availableIntervals.empty()) return fail(Errc::Compile, "plan holds no streams - only directives");
  // Katalog z mkdtemp nie moze byc katalogiem serwisu, wiec blokady serwisu go nie dotycza.
  if (!(temporaryDir_ && defaultDir)) RDB_TRY(checkOffline(offline_));
  if (Refusal refusal = checkStorageDirectory(plan->tree, storageDir_); !refusal.message.empty())
    return fail(std::move(refusal));
  if (Refusal refusal = checkDeclaredSources(plan->tree); !refusal.message.empty()) return fail(std::move(refusal));
  if (Refusal refusal = checkOutputFilesOpenable(plan->tree, storageDir_); !refusal.message.empty())
    return fail(std::move(refusal));

  RDB_TRY_ASSIGN(plan->guard, StorageGuard::create(planStorageDir(plan->tree, storageDir_), true));
  const std::filesystem::path storagePath(plan->guard->directory());
  // Zamrozona sciezka bezwzgledna obowiazuje tez model, nie tylko kontrole sciezek.
  for (auto &qry : plan->tree)
    if (qry.id == ":STORAGE") qry.filename = storagePath.string();
  for (const auto &qry : plan->tree) {
    if (qry.isCompilerDirective()) continue;
    RDB_TRY(plan->guard->checkPath((storagePath / (qry.id + ".desc")).string()));
    if (qry.isDeclaration()) {
      // Zrodla poza katalogiem wynikow: zadne czyszczenie artefaktow nie moze ich usunac.
      const std::string source = absolutePathOf(qry.filename);
      const std::filesystem::path sourcePath(source);
      if (std::ranges::mismatch(storagePath, sourcePath).in1 == storagePath.end())
        return fail(Errc::Compile, "source must be outside the Engine storage directory: " + source);
      // Serwis daje przy braku zrodla rekordy NULL (#346); granica osadzona odmawia (#451), bo w
      // Release ostrzezenie facctxtsrc jest wyciete i kolumna NULL nie zostawilaby sladu.
      const StorageFd input(source, O_RDONLY | O_NONBLOCK, kStreamFileMode, true);
      if (const int err = errno; !input.isOpen()) {
        if (err == ENOENT || err == ENOTDIR)
          return fail(Errc::IO, std::format("stream '{}': source file does not exist: {}", qry.id, source), err);
        return fail(Errc::IO, "cannot open embedded source: " + source, err);
      }
      RDB_TRY(plan->guard->checkPath((storagePath / (qry.id + ".meta")).string()));
      continue;
    }
    const std::filesystem::path data = storagePath / (qry.filename.empty() ? qry.id : qry.filename);
    for (const auto &suffix : {"", ".shadow", ".meta", ".meta.shadow"})
      RDB_TRY(plan->guard->checkPath(data.string() + suffix));
  }

  // #450, wariant A: jedynie we WLASNYM zablokowanym katalogu plan zastepuje deklaracje.
  // Kontrole niewymagajace zmiany plikow sa juz zakonczone, wiec odmowa nie kasuje starego .desc.
  for (const auto &qry : plan->tree) {
    if (!qry.isDeclaration()) continue;
    for (const auto &suffix : {".desc", ".meta"}) {
      const auto file = storagePath / (qry.id + suffix);
      std::error_code ec;
      std::filesystem::remove(file, ec);
      if (ec) return fail(Errc::IO, "cannot remove owned artifact: " + file.string(), ec.value());
    }
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
  RDB_TRY_ASSIGN(plan->model, dataModel::create(plan->tree, &memory_));

  // Po budowie modelu, tak jak w petli uslugi: dataModel usuwa dyrektywy z drzewa, a os czasu ma
  // powstac z samych strumieni. Plan z samymi dyrektywami przechodzi kompilacje, a os czasu bez
  // interwalu nie istnieje - odmowa nalezy do granicy.
  RDB_TRY_ASSIGN(const auto intervals, plan->tree.getAvailableTimeIntervals());
  if (intervals.empty()) return fail(Errc::Compile, "plan holds no streams - only directives");
  plan->timeline = std::make_unique<CRationalStreamMath::TimeLine>(intervals);

  plan_     = std::move(plan);
  warnings_ = std::move(warnings);
  return {};
}

Result<std::optional<std::uint64_t>> Engine::step() {
  RDB_TRY_ASSIGN(Plan *const plan, requirePlan("step"));
  if (plan->failure.has_value()) {
    return fail(plan->failure->code(),
                std::format("Engine::step: the plan stopped at slot {} with: {} - compile it again", plan->slotsDone,
                            plan->failure->message()),
                plan->failure->err());
  }
  // Hak testu api/python/tests/test_error_classes.py (#511): wyjatek biblioteki standardowej w srodku
  // wywolania rdzenia, jak bad_alloc przy braku pamieci. Rdzen nie ma `throw`, wiec wyjatek powstaje
  // tam, gdzie powstaje naprawde - w libstdc++ (length_error z reserve).
  if (std::getenv("RDB_FAULT_STD_EXCEPTION_IN_STEP") != nullptr) {
    std::vector<char> fault;
    fault.reserve(fault.max_size() + 1);
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
  // Nieodwracalne naruszenie niezmiennika nadal konczy proces przez fatal().
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
