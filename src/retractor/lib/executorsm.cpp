#include "executorsm.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <ctime>  // kotwica osi czasu pętli: clock_gettime, timespec
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <print>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>
#include <boost/interprocess/exceptions.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/system/error_code.hpp>

#include "bus.hpp"
#include "constants.hpp"
#include "dataModel.hpp"
#include "executor_rt.hpp"
#include "fatalError.hpp"
#include "ipcServer.hpp"
#include "persistentCounter.hpp"
#include "planSource.hpp"
#include "rdb/convertTypes.hpp"
#include "rdb/probe.hpp"  // sondy E1/E2E, K6, E4
#include "serviceControl.hpp"
#include "shmBudget.hpp"
#include "uxSysTermTools.hpp"

// #include "antlr4-runtime/tree/ParseTree.h"

// extern antlr4::tree::ParseTree *pTree;

namespace IPC = boost::interprocess;

using namespace CRationalStreamMath;

namespace {
constexpr std::chrono::milliseconds kIdleLoopSleep{100};

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

/// Zycie EPOKI planu. Obiekt `dataModel` z petli epok i tresc `*coreInstancePtr` istnieja
/// tylko miedzy opublikowaniem pProc a jego zgaszeniem. core_mutex tego nie pilnuje: chroni
/// pojedyncza zmiane stanu, a handler komendy zwalnia go, ZANIM siegnie po model, i czyta
/// globalny pProc na nowo przy kazdym uzyciu. Watek przetwarzania zdazyl w tym oknie zgasic
/// wskaznik i rozebrac model -- `xqry -d` rownolegle z `xqry --reset` konczylo sie SIGSEGV
/// w dataModel::streamStoredSize (this=0x0), w polowie petli po strumieniach.
/// Ten muteks trzyma epoke w miejscu przez CALY czas obslugi komendy, a wymiana epoki czeka
/// na jego zwolnienie. Wystarcza muteks zwykly, bo handlery i tak sa szeregowane -- IpcServer
/// prowadzi dokladnie jedna petle odbioru komend. Kolejnosc zagniezdzenia jest zawsze
/// plan_epoch_mutex -> core_mutex.
std::mutex plan_epoch_mutex;
}  // namespace

extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &sInputFile,
                                                                         std::vector<std::string> &statementKeywords);

std::unique_ptr<PersistentCounter> pCounterPtr;

extern std::mutex core_mutex;

std::condition_variable cv;  // multithreading condition variable

std::vector<std::pair<std::string, std::string>> processedLines;

dataModel *pProc = nullptr;
std::atomic<bool> dataModelExpected{false};
/// Zatrzask bramki --xqrywait: czy watek komunikacyjny odebral juz JAKAKOLWIEK komende.
/// Osobny od iLoopLimitCnt swiadomie -- patrz komentarz przy bramce w run().
std::atomic<bool> firstQueryReceived{false};
std::atomic<std::uint64_t> adHocPlanRevision{0};
bool untilEofMode{false};

/// Zadanie przeladowania planu przyjete przez kanal IPC. Podnosi je resetCommit() po pelnej
/// walidacji, zdejmuje applyPendingPlan(). Petla epok traktuje je jak warunek konca epoki —
/// dokladnie tak samo jak `stop_now`, tyle ze po niej zaczyna sie epoka nastepna, nie koniec
/// procesu.
std::atomic<bool> planResetRequested{false};
/// Tresc przyjetego zestawu RQL. Chroniona przez core_mutex.
std::string pendingPlanText;

/// Czy trwa wymiana planu: od PRZYJECIA zestawu az do aktywacji jego rezerwacji na magistrali.
/// Osobna od planResetRequested, bo tamta gasnie na POCZATKU wymiany, a rezerwacja zyje jeszcze
/// przez cale budowanie planu. Roznica byla dziura: gniazdo magistrali trzyma DOKLADNIE JEDNA
/// rezerwacje, wiec reset przyjety w oknie miedzy zabraniem tekstu a activateReservedPlan()
/// nadpisywal rezerwacje planu wlasnie wchodzacego. Odchodzacy plan aktywowal wtedy cudza
/// rezerwacje (oglaszajac na magistrali nazwy, ktorych nie liczy), a nastepna epoka nie miala
/// juz czego aktywowac i konczyla sie FatalError -- w trybie --service takze wyczyszczeniem
/// pliku zapytan, czyli restartem uslugi BEZ planu. Odtworzone dwoma rownoleglymi `xqry -q`.
///
/// Flaga ma dokladnie jednego pisarza z kazdej strony: podnosi ja watek komunikacyjny
/// (IpcServer prowadzi jedna petle komend), zdejmuje watek glowny i dopiero PO aktywacji.
/// Odczyt "false" znaczy wiec, ze ani rezerwacja nie wisi, ani wymiana nie trwa -- sprawdzenie
/// w resetCommit() nie potrzebuje muteksu.
std::atomic<bool> planSwapInFlight{false};

/// Plik zapytan uslugi, do ktorego trafia przyjety plan i ktory jest oprozniany po bledzie
/// krytycznym. PUSTY dla instancji, ktora usluga nie jest — plik operatora uruchamiajacego
/// xretractor z terminala nie jest stanem uslugi i nie wolno go nadpisywac.
std::string serviceQueryFilePath;

// variable connected with llimitqry (-m) parameter
// counts remaining loop iterations; 0 = stop, inifitie_loop = run forever
std::atomic<int> iLoopLimitCnt{executorsm::inifitie_loop};

qTree *executorsm::coreInstancePtr = nullptr;
compiler *executorsm::cmPtr        = nullptr;
std::atomic<bool> executorsm::ipcReady{false};
std::atomic<bool> executorsm::ipcFailed{false};
int executorsm::cfgQueueBufferSeconds = appcfg::kDefaultIpcQueueBufferSeconds;
int executorsm::cfgMinQueueElements   = appcfg::kDefaultIpcMinQueueElements;
int executorsm::cfgRtPriority         = appcfg::kDefaultSchedulingRtPriority;
std::string executorsm::cfgStorageDir;
std::string executorsm::activeStorageDir;

// Transport IPC serwera. Obiekt o statycznym czasie zycia, bo sprzatanie musi byc
// osiagalne z handlera atexit (cleanup ponizej): std::exit nie uruchamia destruktorow
// obiektow automatycznych, a destruktory obiektow statycznych wykonuja sie PO
// handlerach zarejestrowanych pozniej niz ich konstrukcja.
static IpcServer ipcServer;

/// Straznik blokady uslugi — wskaznik wazny WYLACZNIE na czas trwania executorsm::run().
///
/// std::exit — przez ktory konczy sie FatalError — nie uruchamia destruktorow obiektow
/// AUTOMATYCZNYCH. Przy bledzie krytycznym cleanup() jest jedynym miejscem, ktore jeszcze
/// dziala, wiec to on musi zwolnic flock. Stabilny plik blokady pozostaje na dysku celowo.
///
/// Zerowany przed powrotem z run() (patrz lockGuardScope), i to jest wymog poprawnosci:
/// handlery atexit wykonuja sie PO zakonczeniu main, a straznik jest tam obiektem
/// automatycznym — po normalnym wyjsciu wskaznik wskazywalby na obiekt juz zniszczony.
static FlockServiceGuard *serviceGuardPtr = nullptr;

/// Magistrala xrdbbus — wskaznik wazny na tych samych zasadach co serviceGuardPtr powyzej.
/// Slot instancji musi zniknac takze na sciezce FatalError, inaczej martwy wpis blokowalby
/// nazwy strumieni az do chwili, gdy ktos go zauwazy i sprzatnie.
static bus::Bus *busPtr = nullptr;

void cleanup() {
  // Blad krytyczny w usludze systemd: plan, ktory zabil proces, nie moze wrocic przy
  // restarcie. Plik zapytan zostaje oprozniony, wiec jednostka wstaje w trybie bezczynnym
  // i czeka na kolejne `xqry --reset`. Bez tego Restart=on-failure zapetla start na tym
  // samym planie — a stan zerowy jest jedynym stanem, o ktorym wiadomo, ze wstanie.
  // Pierwsza czynnosc sprzatania: dalej zwalniamy blokade, po ktorej moze juz wystartowac
  // nastepna instancja i przeczytac ten plik.
  if (fatalErrorRaised.load(std::memory_order_acquire) && !serviceQueryFilePath.empty()) {
    if (servicecontrol::writeQueryFile("", serviceQueryFilePath))
      SPDLOG_CRITICAL("Fatal error: query file '{}' cleared; the service unit will restart with no plan.", serviceQueryFilePath);
    else
      SPDLOG_CRITICAL("Fatal error: could NOT clear query file '{}'; the service unit may restart into the same plan.",
                      serviceQueryFilePath);
  }
  {
    std::scoped_lock lock(core_mutex);
    if (iLoopLimitCnt != executorsm::stop_now) {
      SPDLOG_WARN("Cleanup: Setting iLoopLimitCnt to stop_now.");
      iLoopLimitCnt = executorsm::stop_now;
      std::cout << "Cleanup!" << '\n';
    }
  }
  cv.notify_all();
  ipcServer.shutdownFromExitHandler();
  // Slot magistrali przed blokada, w tej samej kolejnosci co reszta sprzatania: dopiero
  // zwolniona blokada wpuszcza kolejna instancje, a ta czyta magistrale.
  if (busPtr != nullptr) busPtr->release();
  // Blokada uslugi na koncu: po niej moze juz wystartowac kolejna instancja, wiec
  // zwalniamy ja dopiero, gdy IPC jest posprzatane. releaseLock() jest idempotentny,
  // wiec pozniejszy destruktor straznika na sciezce normalnej nie zrobi nic drugi raz.
  if (serviceGuardPtr != nullptr) serviceGuardPtr->releaseLock();
}

std::set<std::string> executorsm::getAwaitedStreamsSet(TimeLine &tl, qTree *coreInstancePtr) {
  if (coreInstancePtr == nullptr) FatalError("executorsm::getAwaitedStreamsSet: coreInstancePtr is null");
  std::set<std::string> retVal;
  for (const auto &it : *coreInstancePtr)
    if (tl.isThisDeltaAwaitCurrentTimeSlot(it.rInterval)) retVal.insert(it.id);

  return retVal;
}

ptree executorsm::collectStreamsParameters() {
  if (coreInstancePtr == nullptr) FatalError("executorsm::collectStreamsParameters: coreInstancePtr is null");
  ptree ptRetval;
  if (pProc == nullptr) FatalError("executorsm::collectStreamsParameters: pProc is null");
  // Hak diagnostyczny testu regresyjnego it_service_reset_race, ta sama droga co RDB_FAULT_SHOW.
  //
  // Zwykle opoznienie tu nie wystarcza i zostalo odrzucone po probie: okno, w ktorym pProc jest
  // juz zgaszony, a nowa epoka jeszcze nie opublikowana, trwa kilkanascie milisekund, wiec
  // handler uspiony na stale dwie sekundy budzil sie PO wymianie i konczyl poprawnie takze na
  // silniku bez naprawy. Hak czeka wiec na SAM FAKT, a nie na uplyw czasu: krecac sie do
  // wyczerpania budzetu (wartosc zmiennej w ms), az pProc zgasnie.
  //
  // Obie odpowiedzi sa jednoznaczne. Bez naprawy rozbiorka epoki przechodzi obok handlera,
  // pProc gasnie, petla ponizej dereferencuje nulla -- SIGSEGV, dokladnie ten z rdzenia
  // (dataModel::streamStoredSize, this=0x0). Z naprawa rozbiorka czeka na plan_epoch_mutex
  // trzymany przez ten handler, wiec pProc NIE MA JAK zgasnac: hak wyczerpuje budzet i komenda
  // konczy sie normalnie, na modelu odchodzacej epoki. Odczyt pProc bez blokady jest tu
  // swiadomy -- to jest wlasnie badany odczyt.
  if (const char *budgetMs = std::getenv("RDB_FAULT_GET_AWAIT_EPOCH_SWAP"); budgetMs != nullptr) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::atoi(budgetMs));
    while (pProc != nullptr && std::chrono::steady_clock::now() < deadline)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  for (auto &q : *coreInstancePtr) {
    ptRetval.put(std::string("db.stream.") + q.id, q.id);

    auto duration = q.rInterval;
    if (duration.denominator() == 1)
      ptRetval.put(std::string("db.stream.") + q.id + std::string(".duration"),
                   boost::lexical_cast<std::string>(duration.numerator()));
    else
      ptRetval.put(std::string("db.stream.") + q.id + std::string(".duration"), boost::lexical_cast<std::string>(duration));

    long recordsCount = -1;
    if (!q.isDeclaration()) recordsCount = static_cast<long>(pProc->streamStoredSize(q.id));
    ptRetval.put(std::string("db.stream.") + q.id + std::string(".size"), boost::lexical_cast<std::string>(recordsCount));
    ptRetval.put(std::string("db.stream.") + q.id + std::string(".count"),
                 boost::lexical_cast<std::string>(pProc->getStreamCount(q.id)));
    ptRetval.put(std::string("db.stream.") + q.id + std::string(".location"), q.filename);
    ptRetval.put(std::string("db.stream.") + q.id + std::string(".cap"), (*coreInstancePtr).maxCapacity[q.id]);
  }
  return ptRetval;
}

/// Dolaczenie reguly do zywego planu — droga rozlaczna z importem strumienia.
///
/// Regula nie powoluje zadnej nazwy: wisi na strumieniu, ktory juz istnieje. Nie ma wiec czego
/// zaimportowac (compiler::importFrom przenosi WYLACZNIE wezly o nowych identyfikatorach, wiec
/// dla reguly jego lista wyjsciowa bylaby pusta), nie ma czego zgloszic na magistrali i nie ma
/// po co przebudowywac osi czasu — zbior interwalow planu zostaje ten sam, dlatego nie rusza
/// tez adHocPlanRevision.
ptree executorsm::attachAdHocRule(qTree &coreInstanceCopy, const std::string &streamName) {
  ptree ptRetval;
  const auto refuse = [&ptRetval](const std::string &message) {
    ptRetval.put(std::string("db"), "Rejected: " + message);
    SPDLOG_ERROR("AdHoc RULE rejected: {}", message);
    return ptRetval;
  };

  // Istnienie celu, jego typ (nie deklaracja), niepusty zakres DUMP i unikalnosc nazwy reguly
  // sprawdzil juz parser — bez tego nie byloby tu parseOut == "OK". Regula jest dokladnie jedna
  // (statementKeywords.size() == 1), wiec parser dopisal ja na koniec listy celu.
  query &copyTarget       = coreInstanceCopy.getQuery(streamName);
  const rule parsed       = copyTarget.lRules.back();
  const auto targetPolicy = copyTarget.policy;  // przed kompilacja kopii == polityka zywego strumienia

  // SYSTEM przez kanal ad-hoc bylby wykonaniem dowolnego polecenia powloki na serwerze przez
  // kazdego, kto otworzy segment IPC. W pliku planu autorem reguly jest ten, kto uruchamia
  // usluge — i to jest cala roznica. Akcja zostaje, kanal nie.
  if (parsed.action != rule::DUMP) return refuse("AdHoc RULE supports DO DUMP only; DO SYSTEM stays available in the plan file");

  const long int historyDepth = parsed.dumpRange.first < 0 ? -parsed.dumpRange.first : 0;

  // Pojemnosci nie da sie podniesc w locie: polityka trafia do deskryptora przy tworzeniu
  // streamInstance, a storage::setCapacity() dla strumienia niedeklarowanego nic nie robi.
  // Magazyn MEMORY jest pierscieniem o rozmiarze policy.second — glebszej historii tam nie ma
  // i nie bedzie, wiec odmawiamy zamiast uzbrajac regule, ktora czekalaby w nieskonczonosc.
  if (historyDepth > 0 && targetPolicy.first == "MEMORY" && std::cmp_greater(historyDepth, targetPolicy.second))
    return refuse("stream '" + streamName + "' keeps only " + std::to_string(targetPolicy.second) +
                  " record(s) in memory, so a dump range reaching " + std::to_string(historyDepth) +
                  " record(s) back cannot be served");

  compiler localCompiler(coreInstanceCopy);
  const auto response = localCompiler.compile();
  if (response != "OK") {
    ptRetval.put(std::string("db"), "Fail local chain compiler:" + response);
    SPDLOG_ERROR("Compile chain of adhoc rule failed: {}", response);
    return ptRetval;
  }

  query &compiledTarget     = coreInstanceCopy.getQuery(streamName);
  rule attached             = compiledTarget.lRules.back();
  const auto compiledLayout = compiledTarget.descriptorStorage();

  {
    std::scoped_lock scoped_lock(core_mutex);
    query &live = coreInstancePtr->getQuery(streamName);
    // Warunek reguly adresuje rekord wyjsciowy celu po indeksie plaskim, wiec wolno go dolaczyc
    // tylko wtedy, gdy rekord ma w obu planach ten sam ksztalt. Kompilacja kopii przebiega
    // niezaleznie od tej, ktora zbudowala plan dzialajacy, a rownosc deskryptorow jest jedynym
    // uczciwym sprawdzianem, ze indeks znaczy po obu stronach to samo.
    if (!(compiledLayout == live.descriptorStorage()))
      return refuse("stream '" + streamName + "' has a different record layout in the recompiled plan");

    // Granica historii: regula rusza dopiero, gdy PO dolaczeniu przybedzie tyle rekordow, ile
    // siega jej zakres. Do tej chwili zostaje nieuzbrojona — patrz rule::armAtCount.
    attached.armAtCount = pProc->getStreamCount(streamName) + static_cast<size_t>(historyDepth);
    live.lRules.push_back(std::move(attached));
  }

  ptRetval.put(std::string("db"), "OK");
  return ptRetval;
}

ptree executorsm::getAdHoc(const std::string &adHocQuery) {
  ptree ptRetval;

  qTree coreInstanceCopy = *coreInstancePtr;

  std::vector<std::string> statementKeywords;
  auto [parseOut, first_keyword, stream_name] = parserRQLString(coreInstanceCopy, adHocQuery, statementKeywords);

  // Blad skladni rozstrzygamy PRZED first_keyword. Po bledzie parser zwraca "UNRECOGNIZED",
  // a kontrole slowa kluczowego koncza sie ponizej FatalError-em, czyli smiercia serwera —
  // tego samego, przed ktora broni usuniecie exit(EPERM) z listenerow (patrz RQLParser.cpp).
  // Zalozenie "slowo kluczowe zawsze rozpoznane" bylo prawdziwe wylacznie dlatego, ze blad
  // parsowania konczyl proces, zanim ta kontrola zdazyla je sprawdzic.
  if (parseOut != "OK") {
    ptRetval.put(std::string("db"), "Fail parse:" + parseOut);
    SPDLOG_ERROR("Parse adhoc query failed: {}", parseOut);
    return ptRetval;
  }

  if (first_keyword == "UNRECOGNIZED") {
    ptRetval.put(std::string("db"), "Unrecognized command. AdHoc query must start with SELECT");
    SPDLOG_ERROR("Unrecognized command in AdHoc query");
    return ptRetval;
  }

  // Parser przyjmuje caly program RQL. Kanal ad-hoc publikuje jednak jedna transakcje
  // SELECT albo DECLARE; sprawdzenie tylko pierwszego slowa pozwalalo ukryc zakazana
  // instrukcje jako drugi element programu zaczynajacego sie od SELECT.
  if (statementKeywords.size() != 1) {
    ptRetval.put(std::string("db"), "Fail parse: AdHoc accepts exactly one SELECT or DECLARE statement");
    SPDLOG_ERROR("Parse adhoc query failed: expected one statement, got {}", statementKeywords.size());
    return ptRetval;
  }

  if (first_keyword == "RULE") return attachAdHocRule(coreInstanceCopy, stream_name);

  if (first_keyword == "STORAGE" ||   //
      first_keyword == "SUBSTRAT" ||  //
      first_keyword == "PERCOUTNER") {
    ptRetval.put(std::string("db"), "Fail parse: AdHoc STORAGE, SUBSTRAT or PERCOUTNER not supported");
    SPDLOG_ERROR("Parse adhoc query failed: AdHoc STORAGE, SUBSTRAT or PERCOUTNER not supported");
    return ptRetval;
  }

  if (first_keyword == "DECLARE" && coreInstancePtr->exists(stream_name)) {
    ptRetval.put(std::string("db"), "Rejected: stream '" + stream_name + "' already exists in this instance");
    SPDLOG_ERROR("AdHoc DECLARE rejected: stream '{}' already exists", stream_name);
    return ptRetval;
  }

  if (first_keyword != "SELECT" && first_keyword != "DECLARE") {
    FatalError("executorsm::getAdHoc: unexpected first_keyword '{}' after filtering — parser logic error", first_keyword);
  }

  // --until-eof jest trybem calego przebiegu. Deklaracja dolaczona pozniej musi
  // odziedziczyc ONESHOT tak samo jak deklaracje planu startowego.
  if (first_keyword == "DECLARE" && untilEofMode) coreInstanceCopy[stream_name].isOneShot = true;

  compiler localCompiler(coreInstanceCopy);
  auto response = localCompiler.compile();

  if (response != "OK") {
    ptRetval.put(std::string("db"), "Fail local chain compiler:" + response);
    SPDLOG_ERROR("Compile chain of adhoc failed: {}", response);
    return ptRetval;
  }

  // Roszczenie nazw powolanych ad-hoc: PO lokalnej kompilacji (dopiero wtedy znane sa takze
  // wezly posrednie, ktore kompilator dolozyl do planu) i PRZED importFrom, czyli przed
  // jakakolwiek zmiana planu dzialajacego serwera. Bez tego nazwa dodana w locie zylaby
  // w drugiej instancji bez roszczenia, a rdb::StoragePaths nadpisalby jej <qryID>.desc
  // we wspolnym katalogu magazynu — ta sama fizyczna kolizja, przed ktora broni start.
  //
  // Zbior nowych nazw wyznaczamy dokladnie ta sama regula co compiler::importFrom:
  // wezly nie bedace dyrektywa, ktorych plan serwera jeszcze nie zna.
  std::vector<std::string> adHocStreams;
  for (const auto &q : coreInstanceCopy) {
    if (q.isCompilerDirective()) continue;
    if (coreInstancePtr->exists(q.id)) continue;
    adHocStreams.push_back(q.id);
  }

  // Sciezki magazynow bierzemy z CALEGO planu po scaleniu, a nie z samych nowych wezlow:
  // claimAdditional pomija to, co juz stoi we wlasnym slocie, wiec zbior jest ten sam, a regula
  // "co jest magazynem" zostaje w jednym miejscu (planStorePaths).
  if (busPtr != nullptr && !adHocStreams.empty()) {
    const bus::ClaimResult claimed = busPtr->claimAdditional(adHocStreams, planStorePaths(coreInstanceCopy, activeStorageDir));
    switch (claimed.status) {
      case bus::ClaimStatus::Claimed:
        break;
      case bus::ClaimStatus::Conflict: {
        const std::string owner   = claimed.ownerName.empty() ? "the unnamed instance" : "instance '" + claimed.ownerName + "'";
        const std::string message = "Rejected: stream '" + claimed.stream + "' is already served by " + owner + " (pid " +
                                    std::to_string(claimed.ownerPid) + ")";
        ptRetval.put(std::string("db"), message);
        SPDLOG_ERROR("AdHoc rejected: {}", message);
        return ptRetval;
      }
      case bus::ClaimStatus::StoreConflict: {
        const std::string owner   = claimed.ownerName.empty() ? "the unnamed instance" : "instance '" + claimed.ownerName + "'";
        const std::string message = "Rejected: storage file '" + claimed.detail + "' is already written by " + owner + " (pid " +
                                    std::to_string(claimed.ownerPid) + ")";
        ptRetval.put(std::string("db"), message);
        SPDLOG_ERROR("AdHoc rejected: {}", message);
        return ptRetval;
      }
      case bus::ClaimStatus::CounterConflict:
        // Nieosiagalne: ad-hoc nie przyjmuje :ROTATION (getAdHoc odrzuca dyrektywy wyzej),
        // wiec claimAdditional nigdy nie porownuje sciezki licznika.
        FatalError("executorsm::getAdHoc: bus reported a rotation counter conflict for an adhoc query");
        break;
      case bus::ClaimStatus::ServiceConflict:
        // Nieosiagalne: tryb pracy jest wlasnoscia URUCHOMIENIA i trafia do slotu wylacznie
        // w claim(); claimAdditional dopisuje nazwy strumieni i maski trybow nie oglada.
        FatalError("executorsm::getAdHoc: bus reported a service mode conflict for an adhoc query");
        break;
      case bus::ClaimStatus::TooLarge:
      case bus::ClaimStatus::NoFreeSlot: {
        const std::string message = "Rejected: cannot register adhoc streams on the xrdbbus bus: " + claimed.detail;
        ptRetval.put(std::string("db"), message);
        SPDLOG_ERROR("AdHoc rejected: {}", message);
        return ptRetval;
      }
      case bus::ClaimStatus::Unavailable:
        // Spojnie ze sciezka startowa: niedostepna magistrala nie zatrzymuje pracy, cena jest
        // wypisana wprost — rozlacznosc nazw nie jest wtedy egzekwowana.
        SPDLOG_WARN("xrdbbus unavailable ({}); adhoc stream name uniqueness is NOT enforced.", claimed.detail);
        break;
    }
  }

  std::vector<std::string> mergedIds;
  std::string compileChainResult;
  std::string addFailedId;
  if (cmPtr == nullptr) FatalError("executorsm::getAdHoc: cmPtr is null");
  if (pProc == nullptr) FatalError("executorsm::getAdHoc: pProc is null");

  // Publish the compiled tree and its runtime stream instances atomically with respect
  // to the execution loop.
  {
    std::scoped_lock scoped_lock(core_mutex);
    mergedIds = cmPtr->importFrom(coreInstanceCopy);
    if (!mergedIds.empty()) adHocPlanRevision.fetch_add(1, std::memory_order_release);
    compileChainResult = cmPtr->compile();
    if (compileChainResult == "OK") {
      pProc->syncDeclaredCapacities();
      for (const auto &id : mergedIds)
        if (!pProc->addQueryToModel(id)) {
          addFailedId = id;
          break;
        }
    }
  }

  if (compileChainResult != "OK") {
    ptRetval.put(std::string("db"), "Compile chain failed:" + response);
    SPDLOG_ERROR("Compile chain failed: {}", compileChainResult);
    return ptRetval;
  }

  if (!addFailedId.empty()) {
    ptRetval.put(std::string("db"), "dataModel::addQueryToModel FAILED:" + addFailedId);
    SPDLOG_ERROR("dataModel::addQueryToModel FAILED, stream {}", addFailedId);
    return ptRetval;
  }

  for (const auto &id : mergedIds)
    processedLines.emplace_back(id, adHocQuery);

  ptRetval.put(std::string("db"), "OK");
  return ptRetval;
}

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

ptree executorsm::commandProcessor(const ptree &ptInval) {
  if (coreInstancePtr == nullptr) FatalError("executorsm::commandProcessor: coreInstancePtr is null");
  ptree ptRetval;
  std::string command = ptInval.get("db.message", "");
  try {
    const bool requiresDataModel = command == "get" || command == "adhoc" || command == "detail" || command == "show";
    // Blokada epoki zyje az do wyjscia z handlera -- patrz plan_epoch_mutex. Brana dopiero PO
    // przebudzeniu na core_mutex, nigdy przed: czekanie na model pod blokada epoki zamykaloby
    // droge watkowi, ktory ten model ma dopiero opublikowac.
    std::unique_lock<std::mutex> epochLock;
    if (requiresDataModel) {
      if (dataModelExpected.load()) {
        // Predykat obejmuje takze ZNIKNIECIE modelu: przeladowanie planu zdejmuje
        // dataModelExpected pod tym samym muteksem, wiec komenda, ktora trafila w rozbiorke
        // epoki, budzi sie zamiast czekac na model, ktory juz nie powstanie.
        std::unique_lock<std::mutex> lock(core_mutex);
        cv.wait(lock, [] { return pProc != nullptr || iLoopLimitCnt == executorsm::stop_now || !dataModelExpected.load(); });
      }
      epochLock = std::unique_lock<std::mutex>(plan_epoch_mutex);
      // Brak modelu jest ODPOWIEDZIA, nie cisza. Do 2026-09-05 instancja bez planu
      // przepuszczala komendy przez wszystkie `if`-y i odsylala PUSTE ptree, a klient
      // meldowal brak kolejki odpowiedzi i wskazywal winnego po drugiej stronie IPC.
      if (pProc == nullptr) {
        ptRetval.put("db", iLoopLimitCnt == executorsm::stop_now ? std::string(constants::kServerStoppingReply)
                                                                 : std::string(constants::kNoActivePlanReply));
        return ptRetval;
      }
    }

    //
    // This command return stream identifiers
    //
    if (command == "get" && pProc != nullptr) ptRetval = collectStreamsParameters();

    if (command == "adhoc" && pProc != nullptr) ptRetval = getAdHoc(ptInval.get("db.argument", ""));
    //
    // This command return what stream contains of
    //
    if (command == "detail" && pProc != nullptr) {
      std::string streamName = ptInval.get("db.argument", "");
      if (streamName.empty()) {
        SPDLOG_ERROR("commandProcessor: 'detail' command missing stream name");
        ptRetval.put("db", "error: missing stream name");
        return ptRetval;
      }
      for (const auto &s : (*coreInstancePtr)[streamName].lSchema) {
        ptRetval.put(std::string("db.field.") + s.field_.rname, s.field_.rname);
        ptRetval.put(std::string("db.field_type.") + s.field_.rname, GetStringdescFld(s.field_.rtype));
      }
      ptRetval.put(std::string("db.stream"), streamName);
      ptRetval.put(std::string("db.count"), boost::lexical_cast<std::string>((*coreInstancePtr)[streamName].lSchema.size()));

      auto duration = (*coreInstancePtr)[streamName].rInterval;
      if (duration.denominator() == 1)
        ptRetval.put(std::string("db.duration"), boost::lexical_cast<std::string>(duration.numerator()));
      else
        ptRetval.put(std::string("db.duration"), boost::lexical_cast<std::string>(duration));

      ptRetval.put(std::string("db.location"), (*coreInstancePtr)[streamName].filename);
      ptRetval.put(std::string("db.cap"), (*coreInstancePtr).maxCapacity[streamName]);
      ptRetval.put(std::string("db.size"), boost::lexical_cast<std::string>(pProc->streamStoredSize(streamName)));
      ptRetval.put(std::string("db.count_records"), boost::lexical_cast<std::string>(pProc->getStreamCount(streamName)));
      ptRetval.put(std::string("db.is_declaration"), ((*coreInstancePtr)[streamName].isDeclaration() ? "true" : "false"));
      ptRetval.put(std::string("db.is_generated"), ((*coreInstancePtr)[streamName].isGenerated() ? "true" : "false"));
      ptRetval.put(std::string("db.query"),
                   boost::lexical_cast<std::string>((*coreInstancePtr)[streamName].lProgram.size()) + " tokens");
      auto it =
          std::ranges::find_if(processedLines,  //
                               [&streamName](const std::pair<std::string, std::string> &p) { return p.first == streamName; });
      std::string queryLine = (it != processedLines.end()) ? it->second : "{not found}";
      ptRetval.put(std::string("db.processed_line"), queryLine);
    }
    //
    // This command will add stream to list of transmitted streams
    // there are created next queue with stream for client
    // and map identifier with this stream
    //
    if (command == "show" && pProc != nullptr) {
      std::string streamName = ptInval.get("db.argument", "");
      if (streamName.empty()) {
        SPDLOG_ERROR("commandProcessor: 'show' command missing stream name");
        ptRetval.put("db", "error: missing stream name");
        return ptRetval;
      }
      if (ptInval.get("db.id", "").empty()) {
        SPDLOG_ERROR("commandProcessor: 'show' command missing db.id");
        ptRetval.put("db", "error: missing db.id");
        return ptRetval;
      }
      // Here we set that for process of given id we send appropriate data stream
      int streamId = boost::lexical_cast<int>(ptInval.get("db.id", ""));
      // 10-second buffer to prevent overflow on loaded systems
      // (1/delta gives elements/sec; multiply by 10 for 10s headroom)
      //
      // Wzor mieszka w shmBudget, bo ta sama liczba wycenia kolejke w raporcie `-c --shmbudget`
      // i w strazy miejsca w IpcServer::subscribe. Kopia wzoru rozjezdzalaby wycene z faktem.
      const int maxElements =
          shmbudget::responseQueueElements((*coreInstancePtr)[streamName].rInterval, cfgQueueBufferSeconds, cfgMinQueueElements);
      // Hak diagnostyczny testu regresyjnego it_show_handler_failure. Awaria handlera
      // 'show' na CI (2026-09-04) byla nieodtwarzalna lokalnie, a jej jedynym skutkiem
      // widocznym dla klienta byla ODPOWIEDZ WYGLADAJACA NA POPRAWNA — bo blok ponizej
      // nie wpisuje do ptRetval niczego takze wtedy, gdy sie powiedzie. Test musi wiec
      // umiec wymusic wyjatek, zamiast czekac na warunki wyscigu.
      if (std::getenv("RDB_FAULT_SHOW") != nullptr)
        throw std::runtime_error("RDB_FAULT_SHOW: wstrzyknieta awaria handlera 'show'");
      ipcServer.subscribe(streamId, streamName, maxElements);
      // Odstep na ustanie kolejki nie potrzebuje juz ani modelu, ani planu, a blokada epoki
      // wstrzymuje w tym czasie slot. Zdejmujemy ja przed czekaniem, zeby subskrypcja nie
      // dokladala tego milisekunda do kazdego slotu, w ktory trafi komenda `show`.
      epochLock.unlock();
      std::this_thread::sleep_for(ipc::kQueuePollInterval);
    }
    //
    // Przeladowanie calego planu: transfer porcjami, potem walidacja i publikacja zadania.
    // Dziala takze przy pProc == nullptr — to jest cala rzecz, po ktora ten kanal istnieje:
    // instancja bezczynna musi umiec przyjac pierwszy plan.
    //
    if (command == "reset-begin") ptRetval = resetBegin(ptInval);
    if (command == "reset-chunk") ptRetval = resetChunk(ptInval);
    if (command == "reset-commit") ptRetval = resetCommit(ptInval);
    //
    // This command stop (kills) server process
    //
    if (command == "kill") {
      {
        std::scoped_lock lock(core_mutex);
        iLoopLimitCnt = executorsm::stop_now;
      }
      cv.notify_all();
    }
    //
    // Diagnostic method
    //
    if (command == "hello") {
      ptRetval.put(std::string("db"), std::string("world"));
    }
  } catch (const boost::property_tree::ptree_error &e) {
    SPDLOG_ERROR("ptree fail: {}", e.what());
    ptRetval.put("error.response", std::string("ptree fail: ") + e.what());
  } catch (std::exception &e) {
    // Bez tego wpisu awaria handlera jest dla klienta NIEODROZNIALNA od powodzenia:
    // 'show' nie wypelnia ptRetval nawet po udanej subskrypcji, wiec pusta odpowiedz
    // znaczyla naraz "zrobione" i "wywrocilo sie". Klient dostawal komunikat o braku
    // kolejki odpowiedzi, a zdanie nazywajace przyczyne zostawalo w logu serwera.
    SPDLOG_ERROR("Command processor failure: {}", e.what());
    ptRetval.put("error.response", std::string("command processor failure: ") + e.what());
  }
  return ptRetval;  // sub for a while
}

std::string executorsm::printRowValue(const std::string &query_name) {
  using boost::property_tree::ptree;
  if (pProc == nullptr) return "";
  if (coreInstancePtr == nullptr) FatalError("executorsm::printRowValue: coreInstancePtr is null");
  auto *payload = pProc->getPayload(query_name, 0);
  if (payload == nullptr) FatalError("executorsm::printRowValue: getPayload returned null");

  ptree pt;
  pt.put("stream", query_name);
  const auto fields = payload->descriptor.dataFields();
  pt.put("count", boost::lexical_cast<std::string>(fields.size()));

  std::string nullmap;
  nullmap.reserve(fields.size());

  int i = 0;
  for (const auto &field : fields) {
    //
    // There is part of communication format - here data are formatted for
    // transmission via internal queue.
    //
    // std::stringstream retVal;
    // retVal << boost::rational_cast<double>(value); - now it's more complicated due types.

    auto valueOpt = payload->getItem(i);
    auto value    = valueOpt.has_value() ? any_to_variant_cast(valueOpt.value()) : nullFallbackValue(field.rtype);
    nullmap.push_back(valueOpt.has_value() ? '0' : '1');

    std::stringstream coutstring;

    std::visit(
        Overload{                                                                                                           //
                 [&coutstring](std::monostate) { coutstring << "null"; },                                                   //
                 [&coutstring](uint8_t a) { coutstring << (unsigned)a; },                                                   //
                 [&coutstring](int a) { coutstring << a; },                                                                 //
                 [&coutstring](unsigned a) { coutstring << a; },                                                            //
                 [&coutstring](float a) { coutstring << a; },                                                               //
                 [&coutstring](double a) { coutstring << a; },                                                              //
                 [&coutstring](std::pair<int, int> a) { coutstring << a.first << "," << a.second; },                        //
                 [&coutstring](const std::pair<std::string, int> &a) { coutstring << a.first << "[" << a.second << "]"; },  //
                 [&coutstring](const std::string &a) { coutstring << a; },                                                  //
                 [&coutstring](boost::rational<int> a) { coutstring << a; }},
        value);

    pt.put(boost::lexical_cast<std::string>(i++), coutstring.str());
  }
  pt.put("nullmap", nullmap);
  std::stringstream strstream;
  write_info(strstream, pt);
  return strstream.str();
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

int executorsm::run(qTree &coreInstance, FlockServiceGuard &guard, bus::Bus &xrdbbus, compiler &cm, vm_map &vm,
                    const AppConfig &cfg, std::string_view serverName, std::string_view systemdUnit) {
  executorsm::coreInstancePtr       = &coreInstance;
  executorsm::cmPtr                 = &cm;
  executorsm::cfgQueueBufferSeconds = cfg.ipcQueueBufferSeconds;
  executorsm::cfgMinQueueElements   = cfg.ipcMinQueueElements;
  executorsm::cfgRtPriority         = cfg.schedulingRtPriority;
  executorsm::cfgStorageDir         = cfg.storageDir;
  // Dyrektywy sa jeszcze w drzewie: dataModel usunie je dopiero przy budowie modelu.
  executorsm::activeStorageDir = planStorageDir(coreInstance, cfg.storageDir);
  dataModelExpected            = !coreInstance.empty();
  untilEofMode                 = vm.contains("until-eof");
  // Plik zapytan uslugi. Nadpisuje go przyjety plan i oprozniaja skutki bledu krytycznego,
  // wiec wskazuje go WYLACZNIE instancja bedaca jednostka systemd: plik `.rql` operatora,
  // ktory uruchomil xretractor z terminala, jest jego wlasnoscia, a nie stanem uslugi.
  if (!systemdUnit.empty()) {
    serviceQueryFilePath = guard.getServiceQueryFile().empty() ? cfg.serviceQueryFile : guard.getServiceQueryFile();
    SPDLOG_INFO("Service unit '{}': plan reloads are persisted to '{}'.", systemdUnit, serviceQueryFilePath);
  }

  // Zakres waznosci wskaznika na straznika — patrz komentarz przy serviceGuardPtr.
  // RAII, a nie zerowanie przy kazdym `return`, bo run() ma ich kilka.
  struct LockGuardScope {
    explicit LockGuardScope(FlockServiceGuard &g) { serviceGuardPtr = &g; }
    ~LockGuardScope() { serviceGuardPtr = nullptr; }
  } lockGuardScope(guard);

  struct BusScope {
    explicit BusScope(bus::Bus &b) { busPtr = &b; }
    ~BusScope() { busPtr = nullptr; }
  } busScope(xrdbbus);

  // Launcher musi wejsc tutaj z przejeta blokada i roszczeniem magistrali. To jest granica
  // transakcji startowej: oba zasoby zostaly zdobyte przed kasowaniem artefaktow i pozostaja
  // wazne do konca executora. Brak blokady oznacza blad kolejnosci wywolan, nie zwykla kolizje.
  if (!guard.isLockActive()) {
    SPDLOG_ERROR("Executor started without an active instance lock.");
    return system::errc::state_not_recoverable;
  }

  ipcServer.setServerName(serverName);
  if (!serverName.empty()) SPDLOG_INFO("Instance name: {}", serverName);

  // atexit dopiero po przejeciu blokady i slotu. Proces, ktory odpadl w launcherze, nie moze
  // miec handlera kasujacego IPC lub zwalniajacego cudze zasoby.
  std::atexit(cleanup);

  std::string percounterFilename{"{notinitialized}"};
  for (const auto &it : coreInstance)
    if (it.id == ":ROTATION") {
      percounterFilename = it.filename;
    }

  if (percounterFilename != "{notinitialized}") pCounterPtr = std::make_unique<PersistentCounter>(percounterFilename);

  auto retVal = system::errc::success;

  // Sending service in thread. Warstwa protokolu wchodzi do transportu przez te
  // cztery wywolania zwrotne -- IpcServer nie zna qTree, dataModel ani compilera.
  ipcServer.start({
      .onCommand = [](const ptree &pt) { return executorsm::commandProcessor(pt); },
      // Stan predykatu musi zmienic sie POD core_mutex. Watek glowny czeka na ipcReady
      // pod tym samym muteksem (ponizej), a cv.wait zwalnia go dopiero w chwili
      // zablokowania. Ustawienie flagi bez muteksu pozwalalo trafic w okno miedzy
      // sprawdzeniem predykatu a zasnieciem watku glownego -- powiadomienie przepadalo
      // i start wisial na zawsze, nie reagujac nawet na SIGTERM.
      .onReady =
          [] {
            {
              std::scoped_lock lock(core_mutex);
              executorsm::ipcReady = true;
            }
            cv.notify_all();
          },
      // Sciezka blizniacza do onReady i ta sama regula muteksu: predykat zmienia sie pod
      // core_mutex, inaczej powiadomienie trafia w okno miedzy sprawdzeniem a zasnieciem.
      .onFailure =
          [] {
            {
              std::scoped_lock lock(core_mutex);
              executorsm::ipcFailed = true;
            }
            cv.notify_all();
          },
      .onMessageReceived =
          [] {
            // Fakt "przyszla pierwsza komenda" zapisujemy BEZWARUNKOWO i w osobnym
            // zatrzasku. Poprzednia wersja podnosila bramke tylko wtedy, gdy widziala juz
            // iLoopLimitCnt == waitForXqry, a te flage watek glowny ustawia dopiero PO
            // zbudowaniu dataModel -- czyli dlugo po opublikowaniu blokady, na ktora czeka
            // klient. Komenda z tego okna gubila pobudke i serwer stal na bramce az do
            // nastepnej komendy (odtworzone 5/5 planem o 120 strumieniach).
            {
              std::scoped_lock lock(core_mutex);
              firstQueryReceived = true;
            }
            cv.notify_all();
          },
      .shouldStop = [] { return iLoopLimitCnt == executorsm::stop_now; },
  });

  {
    std::unique_lock<std::mutex> lock(core_mutex);
    cv.wait(lock, [] { return executorsm::ipcReady.load() || executorsm::ipcFailed.load(); });
  }

  // Bez zasobow IPC instancja nie ma jak przyjac ani jednej komendy: nie odpowie na `xqry`,
  // nie przyjmie planu i nie da sie jej zatrzymac inaczej niz sygnalem. Konczymy wiec od razu
  // i z komunikatem, zamiast liczyc dla nikogo. Najczestsza przyczyna jest mierzalna z wyprzedzeniem
  // -- `xretractor -c --shmbudget <plan>` pokazuje, ile miejsca w pamieci dzielonej potrzeba.
  if (executorsm::ipcFailed.load()) {
    const shmbudget::Space fs = shmbudget::space();
    std::println(std::cerr, "xretractor: cannot create IPC resources; fixed reservation needs {}{}",  //
                 shmbudget::humanBytes(shmbudget::fixedReservationBytes()),
                 fs.known ? std::format(", shared memory has {} free", shmbudget::humanBytes(fs.available)) : std::string{});
    SPDLOG_ERROR("Startup aborted: IPC resources unavailable.");
    ipcServer.stop();
    ipcServer.removeAllObjects();
    return system::errc::no_buffer_space;
  }

  // Blokade mamy od poczatku run(), ale jej TRESC publikujemy dopiero teraz. Linia
  // "PID: <pid>" w pliku blokady jest dla klientow i dla testow sygnalem "serwer gotowy"
  // (kontrakt server_start w test/IntegrationTest/serverlib.sh), wiec nie moze
  // pojawic sie, zanim segment i kolejka komend beda istniec.
  guard.publishLockInfo();

  try {
    // Zatrzymanie klawiszem nalezy wylacznie do przebiegu nieograniczonego -- tylko dla niego
    // drukowany jest ponizej komunikat "Press any key to stop". Przebieg z zadeklarowanym
    // budzetem slotow konczy sie po tym budzecie i po niczym innym, bo jego wynik ma byc
    // powtarzalny. Bez tego warunku bajt czekajacy na terminalu konczyl petle PRZED pierwszym
    // slotem: proces wychodzil kodem 0, deskryptor juz istnial (powstaje przed petla), a plik
    // danych zostawal pusty -- tak padl it_agse_array na CI (2026-09-04). Ta sama pulapka, co
    // opisana w qry.cpp dla xqry (issue_215): na CI stdin bywa terminalem z bajtem w buforze.
    // Ctrl+C (SIGINT) zatrzymuje przebieg bez zmian, obiema drogami.
    //
    // O ograniczeniu przebiegu decyduje WARTOSC licznika, nie obecnosc opcji: --llimitqry ma
    // default_value, wiec vm.contains("llimitqry") jest zawsze prawda. Licznik ograniczony
    // liczy w dol do stop_now i nigdy nie przyjmuje wartosci inifitie_loop.
    const bool boundedRun   = iLoopLimitCnt != executorsm::inifitie_loop;
    const bool ignoreanykey = vm.contains("noanykey") || boundedRun;

    // Petla EPOK planu. Jedna epoka to jeden plan: pusty (tryb bezczynny) albo policzalny
    // (dataModel + os czasu + petla slotow). Epoka konczy sie zatrzymaniem procesu, klawiszem,
    // wyczerpaniem budzetu — albo przyjeta komenda `reset`, i tylko wtedy zaczyna sie nastepna.
    // Bez tej petli tryb bezczynny byl slepym zaulkiem: instancja bez planu nie miala jak go
    // przyjac inaczej niz przez restart procesu.
    while (iLoopLimitCnt != executorsm::stop_now) {
      dataModelExpected = !coreInstancePtr->empty();

      if (coreInstancePtr->empty()) {
        //
        // Tryb bezczynny (idle): brak zapytań — nie budujemy dataModel ani TimeLine
        // (uniknięcie FatalError). Czekamy na zatrzymanie (SIGTERM / klawisz / limit iteracji),
        // utrzymując wątek komunikacyjny i blokadę usługi. pProc pozostaje null —
        // wątek komunikacyjny obsługuje to (komendy działają tylko gdy pProc != nullptr).
        //
        SPDLOG_INFO("Idle mode: no queries to process, waiting for a plan or a shutdown signal.");
        while (!_kbhit(ignoreanykey) && iLoopLimitCnt != executorsm::stop_now &&
               !planResetRequested.load(std::memory_order_acquire)) {
          if (iLoopLimitCnt != executorsm::inifitie_loop) {
            if (iLoopLimitCnt != executorsm::stop_now)
              iLoopLimitCnt--;
            else
              break;
          }
          if (!guard.isLockActive()) {
            SPDLOG_ERROR("CRITICAL ERROR: Lost service lock!");
            break;
          }
          std::this_thread::sleep_for(kIdleLoopSleep);
        }
      } else {
        // Tryb liczenia do konca wejscia: zrodla deklarowane czytamy bez zawijania, tak jakby kazda
        // deklaracja niosla ONESHOT. Bez tego pytanie "czy wejscie sie skonczylo" nie ma odpowiedzi —
        // zrodlo zawijane po koncu pliku wraca na jego poczatek i produkuje rekordy z danych, ktore
        // juz raz przeszly. Ustawienie musi nastapic PRZED konstrukcja dataModel, bo to ona tworzy
        // magazyny i przekazuje isOneShot do fabryki akcesorow.
        const bool until_eof_mode = untilEofMode;
        if (until_eof_mode)
          for (auto &q : *coreInstancePtr)
            if (q.isDeclaration()) q.isOneShot = true;

        dataModel proc(*coreInstancePtr);
        {
          // Publikacja pod obiema blokadami: blokada epoki wpuszcza handlery dopiero do
          // modelu gotowego, core_mutex niesie powiadomienie do czekajacych na cv.
          std::scoped_lock lock(plan_epoch_mutex, core_mutex);
          pProc = &proc;
        }
        cv.notify_all();

        // Czy bramke --xqrywait zdjelo zatrzymanie procesu, a nie komenda klienta. Osobna
        // zmienna, a nie odczyt iLoopLimitCnt nizej: `stop_now` to wartosc 1, czyli dokladnie
        // to, co w liczniku zostawia `-m 1`, wiec warunek na liczniku zmienialby zachowanie
        // przebiegu z budzetem jednego slotu — a ten z bramka nie ma nic wspolnego.
        bool gateStoppedProcess = false;

        if (vm.contains("xqrywait")) {
          if (vm.contains("verbose")) std::cout << "Waiting for first query to start process.\n";
          // Warunek na zatrzasku, a nie na liczniku petli. Licznik niesie budzet slotow
          // z --llimitqry, wiec uzycie go jako flagi bramki kasowalo ten budzet: po
          // podniesieniu bramki wracala wartosc inifitie_loop, a nie zadane N. Skutek byl
          // wprost mierzalny -- `xretractor -m 5` konczyl sie sam, `xretractor -x -m 5`
          // chodzil bez konca. Zatrzask ustawiony PRZED wejsciem tutaj przepuszcza od razu,
          // wiec komenda z okna startowego nie ginie.
          //
          // Czekanie jest TERMINOWE, bo bramke musi zdejmowac takze zatrzymanie procesu, a
          // sygnalu nie da sie tu uslyszec inaczej. handleSignal() ustawia wylacznie
          // iLoopLimitCnt (notify_all nie jest async-signal-safe), wiec czekanie bezterminowe
          // nie mialo kto przerwac: `xretractor -x` bez ani jednej komendy przezywal SIGTERM
          // i schodzil dopiero na SIGKILL -- systemd czekal na to caly TimeoutStopSec.
          // Rozszerzenie samego predykatu nic by nie dalo, potrzebna jest wlasnie pobudka
          // z zegara. Takt 100 ms jest ponizej kazdego rozsadnego limitu zatrzymania.
          std::unique_lock<std::mutex> scoped_lock(core_mutex);
          while (!firstQueryReceived.load() && iLoopLimitCnt != executorsm::stop_now)
            cv.wait_for(scoped_lock, kIdleLoopSleep);
          gateStoppedProcess = !firstQueryReceived.load();
          if (vm.contains("verbose") && !gateStoppedProcess) std::cout << "First query received, starting processing loop.\n";
        }

        if (vm.contains("verbose")) coreInstancePtr->dumpCore();

        std::set<boost::rational<int>> timeIntervals;
        std::uint64_t observedAdHocPlanRevision;
        {
          std::scoped_lock lock(core_mutex);
          timeIntervals             = coreInstancePtr->getAvailableTimeIntervals();
          observedAdHocPlanRevision = adHocPlanRevision.load(std::memory_order_relaxed);
        }
        TimeLine tl(timeIntervals);
        //
        // Main loop of data processing
        //
        // When this value is 0 - means we are waiting for key - other way watchdog
        //
        if (iLoopLimitCnt == executorsm::inifitie_loop && vm.contains("verbose")) std::cout << "Press any key to stop.\n";

        // Formatowanie wiersza jest warstwa protokolu, transport dostaje je jako callback.
        const IpcServer::RowFormatter formatRow = [this](const std::string &name) { return printRowValue(name); };

        // ZERO-step
        std::set<std::string> inSet;
        for (const auto &it : *coreInstancePtr)
          if (it.isDeclaration()) inSet.insert(it.id);
        // Zatrzymanie, ktore zdjelo bramke --xqrywait, nie ma prawa policzyc ani jednego kroku:
        // proces konczony sygnalem zapisalby wtedy rekord zerowy do magazynu, choc nikt o niego
        // nie prosil. Sama petla ponizej i tak nie wykona obrotu (warunek stop_now), a wyjscia
        // `break` w tym miejscu byc nie moze -- ominieloby zgaszenie pProc na koncu epoki i
        // zostawiloby watkowi komunikacyjnemu wskaznik na rozbierany dataModel.
        if (!gateStoppedProcess) {
          proc.processZeroStep();
          ipcServer.broadcast(inSet, formatRow);
        }
        // End of ZERO-step

        // Loop of data processing
        boost::rational<int> prev_interval(0);

        // Sonda E1/E2E: czas obliczeń slotu i latencja end-to-end (rdb/probe.hpp).
        // Uzbrajana dopiero zmienną RDB_BENCH_CSV; bez wkompilowanej sondy znika w całości.
        rdb::probe::slotProbe slotBench;

        struct timespec loop_anchor{};
        const bool rt_mode = vm.contains("realtime");
        // Tryb offline: oś czasu planu (interwały, wyrównanie slotów, ogon) pozostaje nietknięta —
        // znika wyłącznie czekanie na zegar ścienny, więc ciąg wyliczonych rekordów jest ten sam
        // co w przebiegu taktowanym. Wyklucza się z rt_mode; sprzeczność odrzuca launcher.
        const bool no_clock_mode = vm.contains("no-clock");
        if (rt_mode) {
          if (rtCheckAndPrint()) {
            rtActivate(cfgRtPriority);
            // Dopiero TERAZ znana jest maska wątku RT, więc dopiero teraz można z
            // niej wyliczyć rdzenie dla wątku komunikacyjnego. Bez tego przy
            // obciążeniu powyżej 100 % slotu wątek komunikacyjny nie dostaje CPU
            // i żaden klient nie zdąży się zarejestrować (issue_217, badanie W8).
            rtKeepThreadOffRtCpus(ipcServer.threadHandle());
          }
        }

        // Sonda E1/E2E otwierana PRZED kotwicą osi czasu: koszt otwarcia pliku nie może
        // obciążyć budżetu pierwszych slotów (transjent startowy ~20-47 ms w wake_lag --
        // sledztwo ~40 ms, JOURNAL.md 2026-07-18, Faza 3). Tak samo rtActivate
        // (mlockall/SCHED_FIFO) musi wykonać się przed kotwicą.
        slotBench.open();
        clock_gettime(CLOCK_MONOTONIC, &loop_anchor);
        slotBench.anchor(loop_anchor);

        while (!_kbhit(ignoreanykey) && iLoopLimitCnt != executorsm::stop_now &&
               !planResetRequested.load(std::memory_order_acquire)) {
          if (iLoopLimitCnt != executorsm::inifitie_loop) {
            if (iLoopLimitCnt != executorsm::stop_now)
              iLoopLimitCnt--;
            else
              break;
          }

          // Check if system service lock is still active
          if (!guard.isLockActive()) {
            SPDLOG_ERROR("CRITICAL ERROR: Lost service lock!");
            break;
          }

          // Szybka ścieżka wykonuje tylko odczyt atomowy. Pełny skan planu i
          // przebudowa osi następują wyłącznie po opublikowaniu importu ad hoc.
          const auto currentAdHocPlanRevision = adHocPlanRevision.load(std::memory_order_acquire);
          if (currentAdHocPlanRevision != observedAdHocPlanRevision) {
            std::scoped_lock lock(core_mutex);
            auto availableTimeIntervals = coreInstancePtr->getAvailableTimeIntervals();
            if (availableTimeIntervals != timeIntervals) {
              tl.updateTimeIntervals(availableTimeIntervals);
              timeIntervals = std::move(availableTimeIntervals);
            }
            // Import również publikuje rewizję pod core_mutex. Ponowny odczyt
            // pod blokadą obejmuje wszystkie importy zakończone przed tym skanem.
            observedAdHocPlanRevision = adHocPlanRevision.load(std::memory_order_relaxed);
          }

          //
          // Inner time is counted in miliseconds
          // probably can be increased in faster machines
          //
          const int msInSec                          = 1000;
          const boost::rational<int> currentTimeSlot = tl.getNextTimeSlot();
          boost::rational<int> interval(currentTimeSlot * msInSec /* sec->ms */);
          int period(rational_cast<int>(interval - prev_interval));  // miliseconds
          prev_interval = interval;

          //
          // Waiting given miliseconds time that is computed
          //
          if (rt_mode)
            rtAbsoluteSleep(loop_anchor, rational_cast<long>(interval));
          else if (!no_clock_mode)
            std::this_thread::sleep_for(std::chrono::milliseconds(period));

          slotBench.beginSlot(rational_cast<long>(interval));
          {
            // Kompilator ad hoc modyfikuje qTree pod tym samym muteksem. Bez blokady
            // iteracja getAwaitedStreamsSet mogłaby ścigać się z importem nowych węzłów.
            std::scoped_lock lock(core_mutex);
            inSet = getAwaitedStreamsSet(tl, coreInstancePtr);
          }
          {
            // Slot liczy sie pod blokada epoki, bo MUTUJE model: processRows przepisuje payloady,
            // a broadcast siega po nie przez getPayload (releaseOnHold/revRead). Handler komendy
            // czyta te same liczniki i te sama mape qSet, wiec bez tego wykluczenia `xqry -d`
            // czytalby stan w trakcie zmiany. Blokada jest brana PRZED beginCompute: czekanie na
            // komende w locie nie ma prawa wejsc do mierzonego rdzenia E1, a samo zajecie
            // nieobciazonego muteksu to kilkadziesiat nanosekund. endSlot zostaje w srodku, zeby
            // zwolnienie blokady wypadlo poza pomiarem.
            std::scoped_lock epoch(plan_epoch_mutex);
            slotBench.beginCompute();
            proc.processRows(inSet, currentTimeSlot);  // mierzony rdzeń obliczeń jednego interwału (E1)
            slotBench.endCompute();
            ipcServer.broadcast(inSet, formatRow);
            slotBench.endSlot();
          }

          // Deklaracje sa czytane na koncu slotu, a ich rekord konsumuje dopiero slot nastepny.
          // Wyjscie z petli w tym miejscu wypada wiec dokladnie przed pierwszym rekordem, ktory
          // powstalby z all-null wstawionego za koniec wejscia.
          if (until_eof_mode) {
            const auto exhausted = proc.exhaustedInputStream();
            if (!exhausted.empty()) {
              SPDLOG_INFO("End of input on declared stream '{}' — stopping (--until-eof).", exhausted);
              if (vm.contains("verbose")) std::cout << "End of input on stream '" << exhausted << "'. Stopping.\n";
              // Ta sama droga wyjscia co przy wyczerpaniu --llimitqry: stop_now zdejmuje czekanie
              // na klawisz ponizej petli, wiec przebieg wsadowy konczy sie sam.
              {
                std::scoped_lock lock(core_mutex);
                iLoopLimitCnt = executorsm::stop_now;
              }
              break;
            }
          }
          // End of loop while( ! _kbhit(ignoreanykey) )
        }

        // Raport liczników runtime (K6 materializacja, E4 praca na slot) po zakończeniu
        // mierzonej pętli, żeby zliczanie nie obciążało budżetu slotu.
        rdb::probe::reportRuntimeCounters();
        //
        // End of data processing loop
        //

        // Koniec epoki: model znika, zanim `proc` wyjdzie z zakresu. Kolejnosc jest wymogiem
        // poprawnosci — watek komunikacyjny siega po pProc bez wlasnej wiedzy o epokach, wiec
        // wskaznik musi zgasnac POD BLOKADA EPOKI, a nie tylko pod core_mutex. Sam core_mutex
        // zatrzymywal wylacznie komendy jeszcze nieprzebudzone; ta, ktora byla juz w srodku
        // handlera, czytala pProc na nowo i dostawala nulla albo zniszczony model.
        // plan_epoch_mutex czeka tu na handler w locie, a po jego zwolnieniu pProc jest juz
        // nullem, wiec destrukcja `proc` ponizej nie ma komu wyrwac obiektu spod rak.
        {
          std::scoped_lock lock(plan_epoch_mutex, core_mutex);
          pProc             = nullptr;
          dataModelExpected = false;
        }
        cv.notify_all();
        ipcServer.broadcastOutOfBusiness();
      }

      if (!planResetRequested.load(std::memory_order_acquire)) break;
      applyPendingPlan(guard, xrdbbus, cfg);
    }
    // Klawisz, ktory zakonczyl OSTATNIA epoke, zdejmujemy raz — epoka przerwana
    // przeladowaniem planu nie konczy sie klawiszem, wiec nie ma tam czego pobierac.
    if (iLoopLimitCnt != executorsm::stop_now) _getch();  // no wait ... feed key from kbhit
  } catch (IPC::interprocess_exception &ex) {
    std::cerr << ex.what() << '\n' << "IPC::interprocess exception" << '\n';
    retVal = system::errc::no_child_process;
  } catch (std::exception &e) {
    std::cerr << "IPC Fail." << '\n';
    std::cerr << e.what() << '\n';
    SPDLOG_ERROR("catch exception: {}", e.what());
    retVal = system::errc::interrupted;
  }
  {
    std::scoped_lock lock(core_mutex);
    iLoopLimitCnt = executorsm::stop_now;
  }
  cv.notify_all();
  ipcServer.broadcastOutOfBusiness();
  ipcServer.stop();
  ipcServer.removeAllObjects();
  return retVal;
}
