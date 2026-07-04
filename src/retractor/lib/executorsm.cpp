#include "executorsm.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdio>   // sonda benchmarku E1 (RDB_BENCH_CSV): std::fopen/fprintf/fclose
#include <cstdlib>  // sonda benchmarku E1: std::getenv
#include <ctime>    // sonda benchmarku E1: clock_gettime, timespec
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>
#include <boost/container/map.hpp>
#include <boost/container/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/system/error_code.hpp>

#include "constants.hpp"
#include "dataModel.hpp"
#include "executor_rt.hpp"
#include "fatalError.hpp"
#include "persistentCounter.hpp"
#include "rdb/convertTypes.hpp"
#include "uxSysTermTools.hpp"

// #include "antlr4-runtime/tree/ParseTree.h"

// extern antlr4::tree::ParseTree *pTree;

namespace IPC = boost::interprocess;

// Define for IPC purposes - maps & strings (most important IPCString i IPCMap)
using segment_manager_t = IPC::managed_shared_memory::segment_manager;

using CharAllocator   = IPC::allocator<char, segment_manager_t>;
using IPCString       = boost::container::basic_string<char, std::char_traits<char>, CharAllocator>;
using StringAllocator = IPC::allocator<IPCString, segment_manager_t>;

using ValueType = std::pair<const int, IPCString>;

using ShmemAllocator = IPC::allocator<ValueType, segment_manager_t>;
using IPCMap         = boost::container::map<int, IPCString, std::less<>, ShmemAllocator>;

using namespace CRationalStreamMath;

namespace {
constexpr std::chrono::milliseconds kIdleLoopSleep{100};
}  // namespace

extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &sInputFile);

std::unique_ptr<PersistentCounter> pCounterPtr;

// Segment and allocator for string exchange
// IPC::managed_shared_memory strSegment(IPC::open_or_create,
// "RetractorShmemStr", 65536); const StringAllocator allocatorShmemStrInstance
// (strSegment.get_segment_manager());

// Map stores relations processId -> sended stream
static std::map<const int, std::string> id2StreamName_Relation;

extern std::mutex core_mutex;

std::condition_variable cv;  // multithreading condition variable

std::vector<std::pair<std::string, std::string>> processedLines;

dataModel *pProc = nullptr;

// variable connected with llimitqry (-m) parameter
// counts remaining loop iterations; 0 = stop, inifitie_loop = run forever
std::atomic<int> iLoopLimitCnt{executorsm::inifitie_loop};

qTree *executorsm::coreInstancePtr = nullptr;
compiler *executorsm::cmPtr        = nullptr;
std::atomic<bool> executorsm::ipcReady{false};
int executorsm::cfgQueueBufferSeconds = appcfg::kDefaultIpcQueueBufferSeconds;
int executorsm::cfgMinQueueElements   = appcfg::kDefaultIpcMinQueueElements;
int executorsm::cfgRtPriority         = appcfg::kDefaultSchedulingRtPriority;

static std::thread bt;

void cleanup() {
  if (iLoopLimitCnt != executorsm::stop_now) {
    SPDLOG_WARN("Cleanup: Setting iLoopLimitCnt to stop_now.");
    iLoopLimitCnt = executorsm::stop_now;
    std::cout << "Cleanup!" << '\n';
  }
  if (bt.joinable()) bt.join();
  IPC::shared_memory_object::remove("RetractorShmemMap");
  IPC::message_queue::remove("RetractorQueryQueue");
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

ptree executorsm::getAdHoc(const std::string &adHocQuery) {
  qTree coreCopy;
  ptree ptRetval;

  qTree coreInstanceCopy = *coreInstancePtr;

  auto [parseOut, first_keyword, stream_name] = parserRQLString(coreInstanceCopy, adHocQuery);

  if (first_keyword == "UNRECOGNIZED") {
    ptRetval.put(std::string("db"), "Unrecognized command. AdHoc query must start with DECLARE or SELECT");
    SPDLOG_ERROR("Unrecognized command in AdHoc query");
    return ptRetval;
  }

  if (first_keyword == "RULE") {
    ptRetval.put(std::string("db"), "Fail parse: AdHoc RULE not yet supported");
    SPDLOG_ERROR("Parse adhoc query failed: AdHoc RULE not yet supported");
    return ptRetval;
  }

  if (first_keyword == "STORAGE" ||   //
      first_keyword == "SUBSTRAT" ||  //
      first_keyword == "PERCOUTNER") {
    ptRetval.put(std::string("db"), "Fail parse: AdHoc STORAGE, SUBSTRAT or PERCOUTNER not supported");
    SPDLOG_ERROR("Parse adhoc query failed: AdHoc STORAGE, SUBSTRAT or PERCOUTNER not supported");
    return ptRetval;
  }

  if (first_keyword != "SELECT" && first_keyword != "DECLARE") {
    FatalError("executorsm::getAdHoc: unexpected first_keyword '{}' after filtering — parser logic error", first_keyword);
  }

  if (parseOut != "OK") {
    ptRetval.put(std::string("db"), "Fail parse:" + parseOut);
    SPDLOG_ERROR("Parse adhoc query failed: {}", parseOut);
    return ptRetval;
  }

  compiler localCompiler(coreInstanceCopy);
  auto response = localCompiler.compile();

  if (response != "OK") {
    ptRetval.put(std::string("db"), "Fail local chain compiler:" + response);
    SPDLOG_ERROR("Compile chain of adhoc failed: {}", response);
    return ptRetval;
  }

  std::vector<std::string> mergedIds;
  std::string compileChainResult;
  if (cmPtr == nullptr) FatalError("executorsm::getAdHoc: cmPtr is null");

  // These brackets are important - we need to lock coreInstancePtr as less as possible
  {
    std::scoped_lock scoped_lock(core_mutex);
    mergedIds          = cmPtr->importFrom(coreInstanceCopy);
    compileChainResult = cmPtr->compile();
  }

  if (compileChainResult != "OK") {
    ptRetval.put(std::string("db"), "Compile chain failed:" + response);
    SPDLOG_ERROR("Compile chain failed: {}", compileChainResult);
    return ptRetval;
  }

  for (const auto &id : mergedIds) {
    auto result = pProc->addQueryToModel(id);
    if (!result) {
      ptRetval.put(std::string("db"), "dataModel::addQueryToModel FAILED:" + id);
      SPDLOG_ERROR("dataModel::addQueryToModel FAILED, stream {}", id);
      return ptRetval;
    }
    processedLines.emplace_back(id, adHocQuery);
  }

  ptRetval.put(std::string("db"), "OK");
  return ptRetval;
}

ptree executorsm::commandProcessor(const ptree &ptInval) {
  if (coreInstancePtr == nullptr) FatalError("executorsm::commandProcessor: coreInstancePtr is null");
  ptree ptRetval;
  std::string command = ptInval.get("db.message", "");
  try {
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
      int streamId                     = boost::lexical_cast<int>(ptInval.get("db.id", ""));
      id2StreamName_Relation[streamId] = streamName;
      // Create a message_queue
      std::string queueName = std::string(ipc::kResponseQueuePrefix) + ptInval.get("db.id", "");
      // 10-second buffer to prevent overflow on loaded systems
      // (1/delta gives elements/sec; multiply by 10 for 10s headroom)
      int maxElements = boost::rational_cast<int>(1 / (*coreInstancePtr)[streamName].rInterval) * cfgQueueBufferSeconds;
      maxElements     = std::max(maxElements, cfgMinQueueElements);
      IPC::message_queue mq(IPC::open_or_create,               // open or crate
                            queueName.c_str(),                 // name
                            maxElements,                       // max message number
                            ipc::kResponseQueueMaxMessageSize  // max message size
      );
      std::this_thread::sleep_for(ipc::kQueuePollInterval);
    }
    //
    // This command stop (kills) server process
    //
    if (command == "kill") {
      iLoopLimitCnt = executorsm::stop_now;
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
  } catch (std::exception &e) {
    SPDLOG_ERROR("Command processor failure: {}", e.what());
  }
  return ptRetval;  // sub for a while
}

// Thread procedure
void executorsm::commandProcessorLoop() {
  if (coreInstancePtr == nullptr) FatalError("executorsm::commandProcessorLoop: coreInstancePtr is null");
  try {
    IPC::message_queue::remove(std::string(ipc::kQueryQueue).c_str());
    IPC::shared_memory_object::remove(std::string(ipc::kShmemSegment).c_str());
    IPC::named_mutex::remove(std::string(ipc::kMapMutex).c_str());
    // Segment and allocator for map purposes
    IPC::managed_shared_memory mapSegment(IPC::open_or_create, std::string(ipc::kShmemSegment).c_str(), ipc::kShmemSegmentSize);
    const ShmemAllocator allocatorShmemMapInstance(mapSegment.get_segment_manager());
    IPC::named_mutex mapMutex(IPC::open_or_create, std::string(ipc::kMapMutex).c_str());
    // Create a message_queue.
    IPC::message_queue mq(IPC::open_or_create,                    // open or crate
                          std::string(ipc::kQueryQueue).c_str(),  // name
                          ipc::kQueryQueueMaxMessages,            // max message number
                          ipc::kQueryQueueMaxMessageSize          // max message size
    );
    IPCMap *mymap = mapSegment.construct<IPCMap>(std::string(ipc::kMapObject).c_str())  // object name
                    (std::less<>(), allocatorShmemMapInstance);
    ipcReady = true;
    cv.notify_all();
    //
    // This need to be clean up - There are some mess.
    //
    std::array<char, ipc::kQueryQueueMaxMessageSize> message;
    unsigned int priority;
    IPC::message_queue::size_type recvd_size;

    bool loopRunning = true;
    while (loopRunning) {
      while (mq.try_receive(message.data(), ipc::kQueryQueueMaxMessageSize, recvd_size, priority)) {
        if (iLoopLimitCnt == executorsm::waitForXqry) {
          // Notify main thread that first query is received
          iLoopLimitCnt = executorsm::inifitie_loop;
          cv.notify_all();
        }

        message[recvd_size] = 0;
        std::stringstream strstream;
        strstream << message.data();
        memset(message.data(), 0, ipc::kQueryQueueMaxMessageSize);
        ptree pt;
        read_info(strstream, pt);
        ptree pt_retval     = commandProcessor(pt);
        int clientProcessId = boost::lexical_cast<int>(pt.get("db.id", ""));
        // Sending answer
        std::stringstream response_stream;
        write_info(response_stream, pt_retval);
        IPCString ipcResponse(allocatorShmemMapInstance);
        ipcResponse = response_stream.str().c_str();
        // cppcheck-suppress danglingTemporaryLifetime
        {
          IPC::scoped_lock<IPC::named_mutex> lock(mapMutex);
          mymap->insert(std::pair<int, IPCString>(clientProcessId, ipcResponse));
        }
      }
      std::this_thread::sleep_for(ipc::kQueuePollInterval);

      if (iLoopLimitCnt == executorsm::stop_now) loopRunning = false;
    }
  } catch (IPC::interprocess_exception &ex) {
    std::cout << "Exception on server." << '\n' << ex.what() << '\n';
  }
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

void executorsm::boradcastOutOfBussiness() {
  if (executorsm::coreInstancePtr == nullptr) FatalError("executorsm::boradcastOutOfBussiness: coreInstancePtr is null");
  for (const auto &element : id2StreamName_Relation) {
    using namespace boost::interprocess;
    //
    // Queue may have been removed earlier (try_send overflow in boradcast).
    // Wrap in try-catch to avoid crash on open_only failure.
    //
    std::string queueName = "brcdbr" + boost::lexical_cast<std::string>(element.first);
    try {
      IPC::message_queue mq(IPC::open_only, queueName.c_str());
      //
      // Sending out-of-bussiness message
      //
      ptree pt;
      pt.put("stream", constants::Reserved_id_oob);
      std::stringstream strstream;
      write_info(strstream, pt);
      std::string row = strstream.str();

      mq.try_send(row.c_str(), row.length(), 0);
      message_queue::remove(queueName.c_str());
      SPDLOG_WARN("queue erased on out-of-business, procId={}", element.first);
    } catch (IPC::interprocess_exception &e) {
      SPDLOG_WARN("boradcastOutOfBussiness: queue {} already removed, procId={}: {}", queueName, element.first, e.what());
    }
  }
  id2StreamName_Relation.clear();
}

void executorsm::boradcast(const std::set<std::string> &inSet) {
  if (executorsm::coreInstancePtr == nullptr) FatalError("executorsm::boradcast: coreInstancePtr is null");
  for (const auto &queryName : inSet) {
    std::string row = printRowValue(queryName);
    std::list<int> eraseList;
    for (const auto &element : id2StreamName_Relation) {
      if (element.second == queryName) {
        using namespace boost::interprocess;
        //
        // Query discovery. queues are created by show command
        //
        std::string queueName = "brcdbr" + boost::lexical_cast<std::string>(element.first);
        IPC::message_queue mq(IPC::open_only, queueName.c_str());
        //
        // If send queue is full - means no one is listening and queue is
        // going to remove
        //
        if (!mq.try_send(row.c_str(), row.length(), 0)) {
          message_queue::remove(queueName.c_str());
          eraseList.push_back(element.first);
        }
      }
    }
    //
    // cleaning form clients map that are not receiving data from queue
    //
    for (const auto &element : eraseList) {
      id2StreamName_Relation.erase(element);
      SPDLOG_WARN("queue erased on timeout, procId={}", element);
    }
  }
}

int executorsm::run(qTree &coreInstance, FlockServiceGuard &guard, compiler &cm, vm_map &vm, const AppConfig &cfg) {
  executorsm::coreInstancePtr       = &coreInstance;
  executorsm::cmPtr                 = &cm;
  executorsm::cfgQueueBufferSeconds = cfg.ipcQueueBufferSeconds;
  executorsm::cfgMinQueueElements   = cfg.ipcMinQueueElements;
  executorsm::cfgRtPriority         = cfg.schedulingRtPriority;

  std::atexit(cleanup);

  std::string percounterFilename{"{notinitialized}"};
  for (const auto &it : coreInstance)
    if (it.id == ":ROTATION") {
      percounterFilename = it.filename;
    }

  if (percounterFilename != "{notinitialized}") pCounterPtr = std::make_unique<PersistentCounter>(percounterFilename);

  auto retVal = system::errc::success;
  bt          = std::thread(executorsm::commandProcessorLoop);  // Sending service in thread

  {
    std::unique_lock<std::mutex> lock(core_mutex);
    cv.wait(lock, [] { return executorsm::ipcReady.load(); });
  }

  if (!guard.acquireLock()) {
    SPDLOG_ERROR("Cannot acquire service lock, another instance might be running.");
    iLoopLimitCnt = executorsm::stop_now;
    bt.join();
    return system::errc::no_lock_available;
  }
  try {
    bool ignoreanykey = vm.contains("noanykey");

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
      dataModel proc(*coreInstancePtr);
      pProc = &proc;

      if (vm.contains("xqrywait")) {
        if (vm.contains("verbose")) std::cout << "Waiting for first query to start process.\n";
        std::unique_lock<std::mutex> scoped_lock(core_mutex);
        iLoopLimitCnt = executorsm::waitForXqry;
        cv.wait(scoped_lock, [this] { return iLoopLimitCnt != executorsm::waitForXqry; });
        if (vm.contains("verbose")) std::cout << "First query received, starting processing loop.\n";
      }

      if (vm.contains("verbose")) coreInstancePtr->dumpCore();

      TimeLine tl(coreInstancePtr->getAvailableTimeIntervals());
      //
      // Main loop of data processing
      //
      // When this value is 0 - means we are waiting for key - other way watchdog
      //
      if (iLoopLimitCnt == executorsm::inifitie_loop && vm.contains("verbose")) std::cout << "Press any key to stop.\n";

      // ZERO-step
      std::set<std::string> inSet;
      for (const auto &it : *coreInstancePtr)
        if (it.isDeclaration()) inSet.insert(it.id);
      proc.processZeroStep();
      boradcast(inSet);
      // End of ZERO-step

      // Loop of data processing
      boost::rational<int> prev_interval(0);

#ifdef __linux__
      struct timespec loop_anchor{};
      clock_gettime(CLOCK_MONOTONIC, &loop_anchor);
      const bool rt_mode = vm.contains("realtime");
      if (rt_mode) {
        if (rtCheckAndPrint()) rtActivate(cfgRtPriority);
      }

#ifdef RDB_BENCH_PROBE
      //
      // Sonda pomiarowa E1 + E2E (benchmark budżetu czasowego i latencji end-to-end).
      // Cały kod sondy jest kompilowany tylko przy -DRDB_BENCH_PROBE=ON
      // (scripts/buildrdb.sh probe); w zwykłej kompilacji znika bez śladu.
      // Dodatkowo aktywna w runtime tylko, gdy ustawiona jest zmienna środowiskowa
      // RDB_BENCH_CSV wskazująca plik wyjściowy. W normalnym działaniu (brak zmiennej)
      // benchFile == nullptr i sonda nie ma żadnego wpływu na przetwarzanie.
      // Format CSV analizuje examples/ecg/e1_stats.py. Kolumny:
      //   compute_ns  — czas processRows() (E1, czysty rdzeń obliczeń),
      //   wake_lag_ns — spóźnienie pobudki względem deadline'u interwału
      //                 (loop_anchor + interval; ten sam cel, do którego dąży
      //                 rtAbsoluteSleep) — jitter planisty,
      //   e2e_ns      — od deadline'u (nominalny moment pojawienia się krotki
      //                 wejściowej w modelu czasowym) do końca boradcast()
      //                 (emisja wyniku do kolejek IPC klientów).
      // Uwaga: bez -t (realtime) pętla śpi względnie, więc dryf kumuluje się
      // w wake_lag/e2e — do CDF E2E miarodajny jest przebieg z -t.
      //
      const char *benchPath = std::getenv("RDB_BENCH_CSV");
      std::FILE *benchFile  = benchPath ? std::fopen(benchPath, "w") : nullptr;
      if (benchFile) std::fprintf(benchFile, "iter,compute_ns,wake_lag_ns,e2e_ns\n");
      long benchIter             = 0;
      const long loop_anchor_ns  = loop_anchor.tv_sec * 1'000'000'000L + loop_anchor.tv_nsec;
      constexpr auto benchTimeNs = [](const struct timespec &ts) { return ts.tv_sec * 1'000'000'000L + ts.tv_nsec; };
#endif
#endif

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

        //
        // Inner time is counted in miliseconds
        // probably can be increased in faster machines
        //
        const int msInSec = 1000;
        boost::rational<int> interval(tl.getNextTimeSlot() * msInSec /* sec->ms */);
        int period(rational_cast<int>(interval - prev_interval));  // miliseconds
        prev_interval = interval;

        //
        // Waiting given miliseconds time that is computed
        //
#ifdef __linux__
        if (rt_mode)
          rtAbsoluteSleep(loop_anchor, rational_cast<long>(interval));
        else
#endif
          std::this_thread::sleep_for(std::chrono::milliseconds(period));

#if defined(__linux__) && defined(RDB_BENCH_PROBE)
        struct timespec t0{}, t1{}, tWake{};
        long deadline_ns = 0;
        if (benchFile) {
          clock_gettime(CLOCK_MONOTONIC, &tWake);
          deadline_ns = loop_anchor_ns + rational_cast<long>(interval) * 1'000'000L;  // ms -> ns
        }
#endif
        inSet = getAwaitedStreamsSet(tl, coreInstancePtr);
#if defined(__linux__) && defined(RDB_BENCH_PROBE)
        if (benchFile) clock_gettime(CLOCK_MONOTONIC, &t0);
#endif
        proc.processRows(inSet);  // mierzony rdzeń obliczeń jednego interwału (E1)
#if defined(__linux__) && defined(RDB_BENCH_PROBE)
        if (benchFile) clock_gettime(CLOCK_MONOTONIC, &t1);
#endif
        boradcast(inSet);
#if defined(__linux__) && defined(RDB_BENCH_PROBE)
        if (benchFile) {
          struct timespec tEmit{};
          clock_gettime(CLOCK_MONOTONIC, &tEmit);  // koniec emisji wyniku (E2E)
          std::fprintf(benchFile, "%ld,%ld,%ld,%ld\n", benchIter++, benchTimeNs(t1) - benchTimeNs(t0),
                       benchTimeNs(tWake) - deadline_ns, benchTimeNs(tEmit) - deadline_ns);
        }
#endif
        // End of loop while( ! _kbhit(ignoreanykey) )
      }
#if defined(__linux__) && defined(RDB_BENCH_PROBE)
      if (benchFile) std::fclose(benchFile);  // domknięcie sondy E1/E2E
#endif
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
  iLoopLimitCnt = executorsm::stop_now;
  boradcastOutOfBussiness();
  bt.join();
  IPC::shared_memory_object::remove("RetractorShmemMap");
  IPC::message_queue::remove("RetractorQueryQueue");
  IPC::named_mutex::remove("RetractorMapMutex");
  for (const auto &element : id2StreamName_Relation) {
    std::string queueName = "brcdbr" + boost::lexical_cast<std::string>(element.first);
    IPC::message_queue::remove(queueName.c_str());
  }
  return retVal;
}
