#include "executorsm.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>

#include "bus.hpp"
#include "executorsmState.hpp"
#include "fatalError.hpp"
#include "persistentCounter.hpp"
#include "planSource.hpp"
#include "serviceControl.hpp"

// Przeladowanie calego planu (`xqry --reset`): trzyczesciowy transfer tekstu, jego walidacja
// na kopii i wymiana epoki. Stan wspolny opisuje executorsmState.hpp.
using namespace esm;

namespace {
/// Gorna granica transferu planu kanalem `reset`. Nie jest to limit rozmiaru planu jako
/// takiego, tylko zabezpieczenie przed niedomknietym transferem: zle zachowany klient nie
/// moze rosnac serwerowi w pamieci bez konca. 512 porcji to okolo 200 kB tekstu RQL.
constexpr int kResetMaxChunks{512};

/// Gorna granica sklejonego tekstu jednego transferu. Sama liczba porcji jej nie wyznacza:
/// rozmiar porcji wybiera KLIENT, a kolejka komend przepuszcza do ipc::kQueryQueueMaxMessageSize
/// bajtow, wiec 512 porcji po 1000 B to juz pol megabajta, a nie zapowiedziane 200 kB. Wartosc
/// odpowiada 512 porcjom po 400 B, czyli temu, co naprawde wysyla xqry (kResetChunkBytes).
constexpr std::size_t kResetMaxPlanBytes{204800};

/// Ile transferow moze byc rozpoczetych naraz. Klucz mapy to PID klienta, wiec bez tego limitu
/// wystarczy wolac `reset-begin` z kolejnych procesow i nigdy nie domykac: kazdy zostawia wpis
/// do konca zycia serwera. Osiem transferow po 200 kB to 1,6 MB i tyle wynosi cala pamiec,
/// ktora ten kanal jest w stanie zajac.
constexpr std::size_t kResetMaxTransfers{8};

/// Po tym czasie bez ani jednej porcji transfer jest uznawany za porzucony. Klient zabity
/// miedzy `reset-begin` a `reset-commit` nie ma jak po sobie posprzatac, a serwer nie ma jak sie
/// o tym dowiedziec: kanal IPC nie niesie rozlaczenia.
constexpr std::chrono::seconds kResetTransferTtl{60};

/// Transfer planu w toku, per klient (db.id). Trzymany BEZ muteksu, bo dotyka go wylacznie
/// watek komunikacyjny — IpcServer prowadzi dokladnie jedna petle odbioru komend.
struct ResetTransfer {
  int expectedChunks{0};
  int receivedChunks{0};
  std::string text;
  std::chrono::steady_clock::time_point lastActivity{std::chrono::steady_clock::now()};
};
std::map<int, ResetTransfer> resetTransfers;

/// Usuwa transfery, ktore od ostatniej porcji milcza dluzej niz kResetTransferTtl. Wolane
/// wylacznie z resetBegin: sprzatac warto tam, gdzie o miejsce sie prosi, a osobny zegar dla
/// kanalu uzywanego raz na wiele godzin bylby kosztem bez pokrycia.
void purgeStaleResetTransfers(std::chrono::steady_clock::time_point now) {
  for (auto it = resetTransfers.begin(); it != resetTransfers.end();) {
    if (now - it->second.lastActivity > kResetTransferTtl) {
      SPDLOG_WARN("reset transfer of client {} abandoned; {} of {} chunks dropped", it->first, it->second.receivedChunks,
                  it->second.expectedChunks);
      it = resetTransfers.erase(it);
    } else {
      ++it;
    }
  }
}
}  // namespace

std::string executorsm::validatePlanText(const std::string &planText) {
  qTree candidate;
  const PlanSource loaded = parsePlanText(candidate, planText);
  if (loaded.status != "OK") return "Fail parse:" + loaded.status;

  // Zestaw bez ani jednej instrukcji jest LEGALNY: tak sprowadza sie usluge do stanu
  // zerowego, w ktorym czeka na nastepny plan. To ta sama droga, ktora idzie start
  // z pustym plikiem zapytan.
  if (!candidate.empty()) {
    compiler localCompiler(candidate);
    if (const std::string response = localCompiler.compile(); response != "OK") return "Fail compile:" + response;
  }

  // Limity i rozlacznosc nazw sa rozstrzygane atomowo w magistrali, ZANIM resetCommit()
  // opublikuje zadanie wymiany. Udane roszczenie jest zarazem rezerwacja nazw nowego planu:
  // inna instancja nie moze ich zajac w oknie miedzy odpowiedzia dla klienta a granica epoki.
  // Odmowa nie zmienia slotu, wiec stary plan zachowuje takze swoje dotychczasowe roszczenie.
  if (busPtr != nullptr) {
    const bus::ClaimResult claimed =
        busPtr->reservePlan(planStreamNames(candidate), planCounterPath(candidate), planStorePaths(candidate, cfgStorageDir));
    switch (claimed.status) {
      case bus::ClaimStatus::Claimed:
        break;
      case bus::ClaimStatus::Conflict: {
        const std::string owner = claimed.ownerName.empty() ? "the unnamed instance" : "instance '" + claimed.ownerName + "'";
        return "Rejected: stream '" + claimed.stream + "' is already served by " + owner + " (pid " +
               std::to_string(claimed.ownerPid) + ")";
      }
      case bus::ClaimStatus::CounterConflict: {
        const std::string owner = claimed.ownerName.empty() ? "the unnamed instance" : "instance '" + claimed.ownerName + "'";
        return "Rejected: rotation counter file '" + claimed.detail + "' is already used by " + owner + " (pid " +
               std::to_string(claimed.ownerPid) + ")";
      }
      case bus::ClaimStatus::StoreConflict: {
        const std::string owner = claimed.ownerName.empty() ? "the unnamed instance" : "instance '" + claimed.ownerName + "'";
        return "Rejected: storage file '" + claimed.detail + "' is already written by " + owner + " (pid " +
               std::to_string(claimed.ownerPid) + ")";
      }
      case bus::ClaimStatus::ServiceConflict:
        // Nieosiagalne z tego samego powodu co w getAdHoc: reservePlan nie dotyka maski trybow.
        FatalError("executorsm::validatePlanText: bus reported a service mode conflict");
        break;
      case bus::ClaimStatus::TooLarge:
      case bus::ClaimStatus::NoFreeSlot:
        return "Rejected: cannot register the replacement plan on the xrdbbus bus: " + claimed.detail;
      case bus::ClaimStatus::Unavailable:
        if (busPtr->attached()) return "Rejected: cannot reserve the replacement plan on the xrdbbus bus: " + claimed.detail;
        SPDLOG_WARN("xrdbbus unavailable ({}); replacement plan stream name uniqueness is NOT enforced.", claimed.detail);
        break;
    }
  }
  return {};
}

ptree executorsm::resetBegin(const ptree &ptInval) {
  ptree ptRetval;
  const int clientId = ptInval.get("db.id", 0);
  const int chunks   = ptInval.get("db.argument", -1);
  if (chunks < 0 || chunks > kResetMaxChunks) {
    ptRetval.put("db", "Rejected: chunk count out of range (0.." + std::to_string(kResetMaxChunks) + ")");
    SPDLOG_ERROR("reset-begin rejected: chunk count {} out of range", chunks);
    return ptRetval;
  }
  purgeStaleResetTransfers(std::chrono::steady_clock::now());
  // Odmowa, a nie usuniecie najstarszego transferu: cudzy zestaw w polowie drogi nalezy do
  // klienta, ktory nadal czeka na odpowiedz, a odebranie mu miejsca zamienialoby jego transfer
  // w niezrozumiale "no plan transfer in progress" przy nastepnej porcji. Odmowa jest przy tym
  // samoleczaca -- najdalej po kResetTransferTtl miejsce zwalnia sprzatanie powyzej.
  if (!resetTransfers.contains(clientId) && resetTransfers.size() >= kResetMaxTransfers) {
    ptRetval.put("db", "Rejected: too many plan transfers in progress (" + std::to_string(kResetMaxTransfers) + ")");
    SPDLOG_ERROR("reset-begin rejected: {} plan transfers already in progress", resetTransfers.size());
    return ptRetval;
  }
  // Nadpisanie transferu w toku jest zamierzone: klient, ktory zaczyna od nowa, przerwal
  // poprzedni. Bez tego porzucony transfer blokowalby nastepny az do konca procesu.
  resetTransfers[clientId] = ResetTransfer{.expectedChunks = chunks, .receivedChunks = 0, .text = {}};
  ptRetval.put("db", "OK");
  return ptRetval;
}

ptree executorsm::resetChunk(const ptree &ptInval) {
  ptree ptRetval;
  const int clientId = ptInval.get("db.id", 0);
  const auto it      = resetTransfers.find(clientId);
  if (it == resetTransfers.end()) {
    ptRetval.put("db", "Rejected: no plan transfer in progress");
    SPDLOG_ERROR("reset-chunk rejected: no transfer in progress for client {}", clientId);
    return ptRetval;
  }
  // Kolejnosc porcji gwarantuje sam protokol: klient wysyla nastepna dopiero po odpowiedzi
  // na poprzednia, a kolejka komend zachowuje kolejnosc. Sprawdzana jest za to LICZBA —
  // nadmiarowa porcja znaczy, ze po drugiej stronie dzieje sie cos innego niz zapowiedziany
  // transfer, a wtedy plan nie moze zostac sklejony "prawie dobrze".
  if (it->second.receivedChunks >= it->second.expectedChunks) {
    const int expected = it->second.expectedChunks;
    resetTransfers.erase(it);
    ptRetval.put("db", "Rejected: more chunks than the announced " + std::to_string(expected));
    SPDLOG_ERROR("reset-chunk rejected: more chunks than the announced {} for client {}", expected, clientId);
    return ptRetval;
  }
  // Limit bajtow, a nie tylko porcji: rozmiar porcji nalezy do klienta, wiec zapowiedziane
  // 512 porcji moze znaczyc i 200 kB, i pol megabajta -- patrz kResetMaxPlanBytes.
  const std::string chunk = ptInval.get("db.argument", "");
  if (it->second.text.size() + chunk.size() > kResetMaxPlanBytes) {
    resetTransfers.erase(it);
    ptRetval.put("db", "Rejected: plan text exceeds " + std::to_string(kResetMaxPlanBytes) + " bytes");
    SPDLOG_ERROR("reset-chunk rejected: plan text of client {} exceeds {} bytes", clientId, kResetMaxPlanBytes);
    return ptRetval;
  }
  it->second.text += chunk;
  ++it->second.receivedChunks;
  it->second.lastActivity = std::chrono::steady_clock::now();
  ptRetval.put("db", "OK");
  return ptRetval;
}

ptree executorsm::resetCommit(const ptree &ptInval) {
  ptree ptRetval;
  const int clientId = ptInval.get("db.id", 0);
  const auto it      = resetTransfers.find(clientId);
  if (it == resetTransfers.end()) {
    ptRetval.put("db", "Rejected: no plan transfer in progress");
    SPDLOG_ERROR("reset-commit rejected: no transfer in progress for client {}", clientId);
    return ptRetval;
  }
  if (it->second.receivedChunks != it->second.expectedChunks) {
    const int got      = it->second.receivedChunks;
    const int expected = it->second.expectedChunks;
    resetTransfers.erase(it);
    ptRetval.put("db", "Rejected: incomplete transfer (" + std::to_string(got) + " of " + std::to_string(expected) + ")");
    SPDLOG_ERROR("reset-commit rejected: incomplete transfer, {} of {} chunks", got, expected);
    return ptRetval;
  }
  std::string planText = std::move(it->second.text);
  resetTransfers.erase(it);

  // Jedna wymiana naraz. Sprawdzenie musi wypasc PRZED validatePlanText(), bo to ona rezerwuje
  // zasoby w gniezdzie magistrali, a rezerwacja jest tam pojedyncza — patrz planSwapInFlight.
  // Odmowa jest przy okazji uczciwsza od poprzedniego "ostatni wygrywa": zestaw przyjety
  // i nadpisany przez nastepny nigdy nie ruszal, a jego klient dostawal "OK".
  if (planSwapInFlight.load(std::memory_order_acquire)) {
    ptRetval.put("db", "Rejected: a plan reload is already in progress");
    SPDLOG_ERROR("reset-commit rejected: a plan reload is already in progress");
    return ptRetval;
  }

  if (const std::string refusal = validatePlanText(planText); !refusal.empty()) {
    ptRetval.put("db", refusal);
    SPDLOG_ERROR("reset-commit rejected: {}", refusal);
    return ptRetval;
  }

  {
    std::scoped_lock lock(core_mutex);
    pendingPlanText = std::move(planText);
  }
  // Kolejnosc zapisow jest istotna: znacznik wymiany PRZED zadaniem wymiany. Odwrotnie watek
  // glowny zdazylby przeprowadzic cala wymiane i zdjac flage, ktora dopiero potem zostalaby
  // podniesiona — i zostalaby podniesiona na zawsze, odrzucajac kazdy nastepny reset.
  planSwapInFlight.store(true, std::memory_order_release);
  planResetRequested.store(true, std::memory_order_release);
  cv.notify_all();
  SPDLOG_INFO("Plan reload accepted; the running plan will be replaced.");
  ptRetval.put("db", "OK");
  return ptRetval;
}

void executorsm::applyPendingPlan(FlockServiceGuard &guard, bus::Bus &xrdbbus, const AppConfig &cfg) {
  std::string planText;
  {
    std::scoped_lock lock(core_mutex);
    planText = std::move(pendingPlanText);
    pendingPlanText.clear();
  }
  planResetRequested.store(false, std::memory_order_release);

  // Hak diagnostyczny testu regresyjnego it_service_reset_double, ta sama droga co
  // RDB_FAULT_GET_AWAIT_EPOCH_SWAP. Rozciaga (o podana liczbe ms) DOKLADNIE to okno, w ktorym
  // tekst planu jest juz zabrany, a jego rezerwacja na magistrali jeszcze nie aktywowana.
  // Wyscigu z dwoma klientami nie da sie zamowic — bez haka trafienie wymagalo omiatania
  // przesuniecia miedzy dwoma `xqry -q` (trafienie 1 na 14 prob).
  if (const char *delayMs = std::getenv("RDB_FAULT_PLAN_SWAP_DELAY"); delayMs != nullptr)
    std::this_thread::sleep_for(std::chrono::milliseconds(std::atoi(delayMs)));

  // Licznik rotacji nalezy do planu, ktory wlasnie odszedl. Nowy powstanie nizej, o ile
  // nowy plan w ogole niesie :ROTATION.
  pCounterPtr.reset();

  {
    std::scoped_lock lock(core_mutex);
    // Plan wymienia sie PRZEZ ZAWARTOSC tego samego obiektu: `cm` i wszystkie wskazniki na
    // qTree sa zwiazane z nim na stale. Przypisanie pustego drzewa czysci takze maxCapacity
    // i stan sortowania topologicznego — czego samo clear() na wektorze bazowym nie robi.
    *coreInstancePtr = qTree{};
    cmPtr->reset();
    processedLines.clear();
    adHocPlanRevision.store(0, std::memory_order_release);

    const PlanSource loaded = parsePlanText(*coreInstancePtr, planText);
    if (loaded.status != "OK") {
      // Nieosiagalne przez kanal `reset`: resetCommit() przepuszcza wylacznie zestaw, ktory
      // przeszedl te sama droge na kopii. Gdyby jednak tu wyladowalo, epoka ma byc PUSTA,
      // a nie polowiczna — instancja z na wpol wczytanym planem liczylaby cos, czego nikt
      // nie zamowil.
      SPDLOG_ERROR("Plan reload failed at parse stage: {}; falling back to idle mode.", loaded.status);
      *coreInstancePtr = qTree{};
      cmPtr->reset();
    } else {
      processedLines = loaded.lines;
      if (!coreInstancePtr->empty()) {
        if (const std::string response = cmPtr->compile(); response != "OK") {
          SPDLOG_ERROR("Plan reload failed at compile stage: {}; falling back to idle mode.", response);
          *coreInstancePtr = qTree{};
          cmPtr->reset();
          processedLines.clear();
        }
      }
    }

    // Domyslny katalog storage z konfiguracji — ta sama regula co przy starcie: dyrektywa
    // :STORAGE z RQL ma pierwszenstwo, a w planie pustym nie ma czego kierowac.
    if (!cfg.storageDir.empty() && !coreInstancePtr->empty() &&
        std::ranges::none_of(*coreInstancePtr, [](const auto &it) { return it.id == ":STORAGE"; })) {
      query storageDirective;
      storageDirective.id       = ":STORAGE";
      storageDirective.filename = cfg.storageDir;
      coreInstancePtr->push_back(storageDirective);
    }

    // Ostatnia chwila, w ktorej `:STORAGE` jest jeszcze w drzewie — dataModel usunie dyrektywy
    // przy budowie modelu, a zapytania ad-hoc nastepnej epoki potrzebuja tego katalogu.
    activeStorageDir = planStorageDir(*coreInstancePtr, cfg.storageDir);
  }

  // Oczekiwanie na model podniesione PRZED ogloszeniem planu na magistrali. Kolejnosc jest
  // istotna: activateReservedPlan() czyni nazwy strumieni widocznymi dla klientow, a model
  // powstaje dopiero na poczatku nastepnej epoki. Klient, ktory trafil w to okno, nie mial na
  // czym czekac -- predykat cv widzial dataModelExpected == false z epoki bezczynnej i komenda
  // wracala z "no active plan", choc plan zostal juz przyjety i ogloszony.
  dataModelExpected.store(!coreInstancePtr->empty(), std::memory_order_release);

  const bus::ClaimResult activated = xrdbbus.activateReservedPlan();
  if (activated.status != bus::ClaimStatus::Claimed && xrdbbus.attached())
    FatalError("Cannot activate the reserved bus resources: {}", activated.detail);

  // Rezerwacja jest zuzyta, wiec od tej chwili wolno przyjac nastepny reset. Ani chwili
  // wczesniej: az dotad kolejne reservePlan() nadpisywaloby rezerwacje wlasnie aktywowana.
  planSwapInFlight.store(false, std::memory_order_release);

  dropStalePlanArtifacts(*coreInstancePtr, *cmPtr, processedLines);

  for (const auto &it : *coreInstancePtr)
    if (it.id == ":ROTATION") pCounterPtr = std::make_unique<PersistentCounter>(it.filename);

  // Trwalosc planu: usluga, ktora zostanie zrestartowana, ma wstac z tym, co faktycznie
  // liczy, a nie z zestawem sprzed przeladowania. Niepowodzenie zapisu nie zatrzymuje
  // pracy — plan juz dziala, traci sie wylacznie jego przetrwanie restartu.
  if (!serviceQueryFilePath.empty()) {
    if (servicecontrol::writeQueryFile(planText, serviceQueryFilePath))
      SPDLOG_INFO("Plan persisted to '{}'.", serviceQueryFilePath);
    else
      SPDLOG_WARN("Plan is running but could NOT be persisted to '{}'; a restart would lose it.", serviceQueryFilePath);
  }

  guard.publishLockInfo();
  SPDLOG_INFO("Plan reloaded: {} node(s) in the new plan.", coreInstancePtr->size());
}
