#include "executorsm.hpp"

#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "bus.hpp"
#include "dataModel.hpp"
#include "executorsmState.hpp"
#include "fatalError.hpp"
#include "planSource.hpp"
#include "RQLParser.hpp"

// Kanal ad-hoc: dolaczenie pojedynczej instrukcji SELECT/DECLARE albo reguly do planu, ktory
// juz dziala. Stan wspolny opisuje executorsmState.hpp.
using namespace esm;

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
