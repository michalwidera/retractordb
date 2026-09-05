#include "executorsm.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <ctime>  // kotwica osi czasu pętli: clock_gettime, timespec
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
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
#include "rdb/convertTypes.hpp"
#include "rdb/probe.hpp"  // sondy E1/E2E, K6, E4
#include "uxSysTermTools.hpp"

// #include "antlr4-runtime/tree/ParseTree.h"

// extern antlr4::tree::ParseTree *pTree;

namespace IPC = boost::interprocess;

using namespace CRationalStreamMath;

namespace {
constexpr std::chrono::milliseconds kIdleLoopSleep{100};
}  // namespace

extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &sInputFile);
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

// variable connected with llimitqry (-m) parameter
// counts remaining loop iterations; 0 = stop, inifitie_loop = run forever
std::atomic<int> iLoopLimitCnt{executorsm::inifitie_loop};

qTree *executorsm::coreInstancePtr = nullptr;
compiler *executorsm::cmPtr        = nullptr;
std::atomic<bool> executorsm::ipcReady{false};
int executorsm::cfgQueueBufferSeconds = appcfg::kDefaultIpcQueueBufferSeconds;
int executorsm::cfgMinQueueElements   = appcfg::kDefaultIpcMinQueueElements;
int executorsm::cfgRtPriority         = appcfg::kDefaultSchedulingRtPriority;

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
  if (historyDepth > 0 && targetPolicy.first == "MEMORY" && static_cast<size_t>(historyDepth) > targetPolicy.second)
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

  if (busPtr != nullptr && !adHocStreams.empty()) {
    const bus::ClaimResult claimed = busPtr->claimAdditional(adHocStreams);
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
      case bus::ClaimStatus::CounterConflict:
        // Nieosiagalne: ad-hoc nie przyjmuje :ROTATION (getAdHoc odrzuca dyrektywy wyzej),
        // wiec claimAdditional nigdy nie porownuje sciezki licznika.
        FatalError("executorsm::getAdHoc: bus reported a rotation counter conflict for an adhoc query");
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

ptree executorsm::commandProcessor(const ptree &ptInval) {
  if (coreInstancePtr == nullptr) FatalError("executorsm::commandProcessor: coreInstancePtr is null");
  ptree ptRetval;
  std::string command = ptInval.get("db.message", "");
  try {
    const bool requiresDataModel = command == "get" || command == "adhoc" || command == "detail" || command == "show";
    if (requiresDataModel && dataModelExpected.load()) {
      std::unique_lock<std::mutex> lock(core_mutex);
      cv.wait(lock, [] { return pProc != nullptr || iLoopLimitCnt == executorsm::stop_now; });
      if (pProc == nullptr) {
        ptRetval.put("db", "server stopping");
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
      int maxElements = boost::rational_cast<int>(1 / (*coreInstancePtr)[streamName].rInterval) * cfgQueueBufferSeconds;
      maxElements     = std::max(maxElements, cfgMinQueueElements);
      // Hak diagnostyczny testu regresyjnego it_show_handler_failure. Awaria handlera
      // 'show' na CI (2026-09-04) byla nieodtwarzalna lokalnie, a jej jedynym skutkiem
      // widocznym dla klienta byla ODPOWIEDZ WYGLADAJACA NA POPRAWNA — bo blok ponizej
      // nie wpisuje do ptRetval niczego takze wtedy, gdy sie powiedzie. Test musi wiec
      // umiec wymusic wyjatek, zamiast czekac na warunki wyscigu.
      if (std::getenv("RDB_FAULT_SHOW") != nullptr)
        throw std::runtime_error("RDB_FAULT_SHOW: wstrzyknieta awaria handlera 'show'");
      ipcServer.subscribe(streamId, streamName, maxElements);
      std::this_thread::sleep_for(ipc::kQueuePollInterval);
    }
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

int executorsm::run(qTree &coreInstance, FlockServiceGuard &guard, bus::Bus &xrdbbus, compiler &cm, vm_map &vm,
                    const AppConfig &cfg, std::string_view serverName) {
  executorsm::coreInstancePtr       = &coreInstance;
  executorsm::cmPtr                 = &cm;
  executorsm::cfgQueueBufferSeconds = cfg.ipcQueueBufferSeconds;
  executorsm::cfgMinQueueElements   = cfg.ipcMinQueueElements;
  executorsm::cfgRtPriority         = cfg.schedulingRtPriority;
  dataModelExpected                 = !coreInstance.empty();
  untilEofMode                      = vm.contains("until-eof");

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
    cv.wait(lock, [] { return executorsm::ipcReady.load(); });
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

    if (coreInstancePtr->empty()) {
      //
      // Tryb bezczynny (idle): brak zapytań — nie budujemy dataModel ani TimeLine
      // (uniknięcie FatalError). Czekamy na zatrzymanie (SIGTERM / klawisz / limit iteracji),
      // utrzymując wątek komunikacyjny i blokadę usługi. pProc pozostaje null —
      // wątek komunikacyjny obsługuje to (komendy działają tylko gdy pProc != nullptr).
      //
      SPDLOG_INFO("Idle mode: no queries to process, waiting for shutdown signal.");
      while (!_kbhit(ignoreanykey) && iLoopLimitCnt != executorsm::stop_now) {
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
      if (iLoopLimitCnt != executorsm::stop_now) _getch();
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
        std::scoped_lock lock(core_mutex);
        pProc = &proc;
      }
      cv.notify_all();

      if (vm.contains("xqrywait")) {
        if (vm.contains("verbose")) std::cout << "Waiting for first query to start process.\n";
        // Warunek na zatrzasku, a nie na liczniku petli. Licznik niesie budzet slotow
        // z --llimitqry, wiec uzycie go jako flagi bramki kasowalo ten budzet: po
        // podniesieniu bramki wracala wartosc inifitie_loop, a nie zadane N. Skutek byl
        // wprost mierzalny -- `xretractor -m 5` konczyl sie sam, `xretractor -x -m 5`
        // chodzil bez konca. Zatrzask ustawiony PRZED wejsciem tutaj przepuszcza od razu,
        // wiec komenda z okna startowego nie ginie.
        std::unique_lock<std::mutex> scoped_lock(core_mutex);
        cv.wait(scoped_lock, [] { return firstQueryReceived.load(); });
        if (vm.contains("verbose")) std::cout << "First query received, starting processing loop.\n";
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
      proc.processZeroStep();
      ipcServer.broadcast(inSet, formatRow);
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

      while (!_kbhit(ignoreanykey) && iLoopLimitCnt != executorsm::stop_now) {
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
        slotBench.beginCompute();
        proc.processRows(inSet, currentTimeSlot);  // mierzony rdzeń obliczeń jednego interwału (E1)
        slotBench.endCompute();
        ipcServer.broadcast(inSet, formatRow);
        slotBench.endSlot();

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
      if (iLoopLimitCnt != executorsm::stop_now) _getch();  // no wait ... feed key from kbhit
    }
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
