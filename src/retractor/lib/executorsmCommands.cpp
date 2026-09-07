#include "executorsm.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "constants.hpp"
#include "dataModel.hpp"
#include "executorsmState.hpp"
#include "fatalError.hpp"
#include "ipcServer.hpp"
#include "rdb/convertTypes.hpp"
#include "shmBudget.hpp"

// Dyspozytor komend kanalu IPC i formatowanie odpowiedzi. Stan wspolny opisuje
// executorsmState.hpp; kanaly `adhoc` i `reset-*` maja wlasne jednostki.
using namespace esm;

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
