#include "executorsm.h"

#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <array>
#include <boost/chrono.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

#include "config.h"  // Add an automatically generated configuration file
#include "dataModel.h"
#include "uxSysTermTools.hpp"

namespace IPC = boost::interprocess;

// Define for IPC purposes - maps & strings (most important IPCString i IPCMap)
typedef IPC::managed_shared_memory::segment_manager segment_manager_t;

typedef IPC::allocator<char, segment_manager_t> CharAllocator;
typedef IPC::basic_string<char, std::char_traits<char>, CharAllocator> IPCString;
typedef IPC::allocator<IPCString, segment_manager_t> StringAllocator;

typedef std::pair<const int, IPCString> ValueType;

typedef IPC::allocator<ValueType, segment_manager_t> ShmemAllocator;
typedef IPC::map<int, IPCString, std::less<int>, ShmemAllocator> IPCMap;

using namespace CRationalStreamMath;

// Segment and allocator for string exchange
// IPC::managed_shared_memory strSegment(IPC::open_or_create,
// "RetractorShmemStr", 65536); const StringAllocator allocatorShmemStrInstance
// (strSegment.get_segment_manager());

// Map stores relations processId -> sended stream
static std::map<const int, std::string> id2StreamName_Relation;

dataModel *pProc = nullptr;

// variable connected with tlimitqry (-m) parameter
// when it will be set thread will exit by given time (testing purposes)
int iTimeLimitCnt(0);

qTree *executorsm::coreInstancePtr = nullptr;

std::set<std::string> executorsm::getAwaitedStreamsSet(TimeLine &tl) {
  assert(coreInstancePtr != nullptr);
  std::set<std::string> retVal;
  for (const auto &it : *coreInstancePtr)
    if (tl.isThisDeltaAwaitCurrentTimeSlot(it.rInterval)) retVal.insert(it.id);

  return retVal;
}

ptree executorsm::collectStreamsParameters() {
  assert(coreInstancePtr != nullptr);
  ptree ptRetval;
  assert(pProc != nullptr && "pProc must be checked before procedure call.");
  SPDLOG_DEBUG("get cmd rcv.");
  for (auto &q : *coreInstancePtr) {
    ptRetval.put(std::string("db.stream.") + q.id, q.id);
    ptRetval.put(std::string("db.stream.") + q.id + std::string(".duration"), boost::lexical_cast<std::string>(q.rInterval));
    long recordsCount = -1;
    if (!q.isDeclaration()) recordsCount = pProc->streamStoredSize(q.id);
    ptRetval.put(std::string("db.stream.") + q.id + std::string(".size"), boost::lexical_cast<std::string>(recordsCount));
    ptRetval.put(std::string("db.stream.") + q.id + std::string(".count"),
                 boost::lexical_cast<std::string>(pProc->getStreamCount(q.id)));
    ptRetval.put(std::string("db.stream.") + q.id + std::string(".location"), q.filename);
    ptRetval.put(std::string("db.stream.") + q.id + std::string(".cap"), (*coreInstancePtr).maxCapacity[q.id]);
  }
  return ptRetval;
}

ptree executorsm::commandProcessor(ptree ptInval) {
  assert(coreInstancePtr != nullptr);
  ptree ptRetval;
  std::string command = ptInval.get("db.message", "");
  try {
    //
    // This command return stream identifiers
    //
    if (command == "get" && pProc != nullptr) ptRetval = collectStreamsParameters();

    //
    // This command return what stream contains of
    //
    if (command == "detail" && pProc != nullptr) {
      std::string streamName = ptInval.get("db.argument", "");
      assert(streamName != "");
      SPDLOG_DEBUG("got detail {} rcv.", streamName);
      for (const auto &s : (*coreInstancePtr)[streamName].lSchema)
        ptRetval.put(std::string("db.field.") + s.field_.rname, s.field_.rname);
    }
    //
    // This command will add stream to list of transmitted streams
    // there are created next queue with stream for client
    // and map identifier with this stream
    //
    if (command == "show" && pProc != nullptr) {
      std::string streamName = ptInval.get("db.argument", "");
      // Probably someone calls show w/o stream name
      assert(streamName != "");
      // check if command present id of process
      assert(ptInval.get("db.id", "") != "");
      // Testing purposes only - get it off after testing
      SPDLOG_DEBUG("got show {} rcv.", streamName);
      // Here we set that for process of given id we send appropriate data stream
      int streamId                     = boost::lexical_cast<int>(ptInval.get("db.id", ""));
      id2StreamName_Relation[streamId] = streamName;
      // Create a message_queue
      std::string queueName = "brcdbr" + ptInval.get("db.id", "");
      // let's assume that we have 1/10 duration
      // that means 10 elements are going in second
      // so - we need 10 elements for one second buffer
      int maxElements = boost::rational_cast<int>(1 / (*coreInstancePtr)[streamName].rInterval);
      maxElements     = (maxElements < 2) ? 2 : maxElements;
      IPC::message_queue mq(IPC::open_or_create,  // open or crate
                            queueName.c_str(),    // name
                            maxElements,          // max message number
                            1024                  // max message size
      );
      boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
    }
    //
    // This command stop (kills) server process
    //
    if (command == "kill") {
      SPDLOG_DEBUG("got kill rcv.");
      iTimeLimitCnt = 1;
    }
    //
    // Diagnostic method
    //
    if (command == "hello") {
      SPDLOG_DEBUG("got hello.");
      ptRetval.put(std::string("db"), std::string("world"));
      using boost::property_tree::ptree;
      std::stringstream strstream;
      write_info(strstream, ptRetval);
      SPDLOG_DEBUG("reply: {}", strstream.str());
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
  assert(coreInstancePtr != nullptr);
  try {
    IPC::message_queue::remove("RetractorQueryQueue");
    IPC::shared_memory_object::remove("RetractorShmemMap");
    // Segment and allocator for map purposes
    IPC::managed_shared_memory mapSegment(IPC::open_or_create, "RetractorShmemMap", 65536);
    const ShmemAllocator allocatorShmemMapInstance(mapSegment.get_segment_manager());
    // Create a message_queue.
    IPC::message_queue mq(IPC::open_or_create,    // open or crate
                          "RetractorQueryQueue",  // name
                          1000,                   // max message number
                          1000                    // max message size
    );
    IPCMap *mymap = mapSegment.construct<IPCMap>("MyMap")  // object name
                    (std::less<int>(), allocatorShmemMapInstance);
    //
    // This need to be clean up - There are some mess.
    //
    std::array<char, 1000> message;
    unsigned int priority;
    IPC::message_queue::size_type recvd_size;
    while (true) {
      while (mq.try_receive(message.data(), 1000, recvd_size, priority)) {
        message[recvd_size] = 0;
        std::stringstream strstream;
        strstream << message.data();
        memset(message.data(), 0, 1000);
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
        mymap->insert(std::pair<int, IPCString>(clientProcessId, ipcResponse));
      }
      boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
    }
  } catch (IPC::interprocess_exception &ex) {
    std::cout << ex.what() << std::endl << "catch on server" << std::endl;
  }
}

std::string executorsm::printRowValue(const std::string &query_name) {
  using boost::property_tree::ptree;
  if (pProc == nullptr) return "";
  ptree pt;
  pt.put("stream", query_name);
  pt.put("count", boost::lexical_cast<std::string>(coreInstance.getQuery(query_name).lSchema.size()));
  int i = 0;
  for (auto value : pProc->getRow(query_name, 0)) {
    //
    // There is part of communication format - here data are formatted for
    // transmission via internal queue.
    //
    // std::stringstream retVal;
    // retVal << boost::rational_cast<double>(value); - now it's more complicated due types.

    std::stringstream coutstring;

    std::visit(Overload{                                                                                                    //
                        [&coutstring](uint8_t a) { coutstring << (unsigned)a; },                                            //
                        [&coutstring](int a) { coutstring << a; },                                                          //
                        [&coutstring](unsigned a) { coutstring << a; },                                                     //
                        [&coutstring](float a) { coutstring << a; },                                                        //
                        [&coutstring](double a) { coutstring << a; },                                                       //
                        [&coutstring](std::pair<int, int> a) { coutstring << a.first << "," << a.second; },                 //
                        [&coutstring](std::pair<std::string, int> a) { coutstring << a.first << "[" << a.second << "]"; },  //
                        [&coutstring](const std::string &a) { coutstring << a; },                                           //
                        [&coutstring](boost::rational<int> a) { coutstring << a; }},
               value);

    pt.put(boost::lexical_cast<std::string>(i++), coutstring.str());
  }
  std::stringstream strstream;
  write_info(strstream, pt);
  return strstream.str();
}

int executorsm::run(bool verbose, int iTimeLimitCntParam, FlockServiceGuard &guard) {
  executorsm::coreInstancePtr = &coreInstance;

  iTimeLimitCnt = iTimeLimitCntParam;
  auto retVal   = system::errc::success;
  thread bt(executorsm::commandProcessorLoop);  // Sending service in thread
  // This line - delay is ugly fix for slow machine on CI !
  boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
  try {
    dataModel proc(coreInstance);
    pProc = &proc;

    if (verbose) coreInstance.dumpCore();

    TimeLine tl(coreInstance.getAvailableTimeIntervals());
    //
    // Main loop of data processing
    //
    // When this value is 0 - means we are waiting for key - other way watchdog
    //
    if (iTimeLimitCnt == 0 && verbose) std::cout << "Press any key to stop.\n";

    // ZERO-step

    std::set<std::string> initSet;
    for (const auto &it : coreInstance)
      if (it.isDeclaration()) initSet.insert(it.id);

    proc.processRows(initSet);

    // End of ZERO-step

    // Loop of data processing

    boost::rational<int> prev_interval(0);
    while (!_kbhit() && iTimeLimitCnt != 1) {
      if (iTimeLimitCnt != 0) {
        if (iTimeLimitCnt != 1)
          iTimeLimitCnt--;
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
      boost::this_thread::sleep_for(boost::chrono::milliseconds(period));

      const std::set<std::string> inSet = getAwaitedStreamsSet(tl);

      proc.processRows(inSet);

      //
      // Data broadcast - main loop
      //
      for (const auto queryName : inSet) {
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

      // End of loop while( ! _kbhit() )
    }
    //
    // End of data processing loop
    //
    if (iTimeLimitCnt != 1) _getch();  // no wait ... feed key from kbhit
  } catch (IPC::interprocess_exception &ex) {
    std::cerr << ex.what() << std::endl << "IPC::interprocess exception" << std::endl;
    retVal = system::errc::no_child_process;
  } catch (std::exception &e) {
    std::cerr << "IPC Fail." << std::endl;
    std::cerr << e.what() << std::endl;
    SPDLOG_ERROR("catch exception: {}", e.what());
    retVal = system::errc::interrupted;
  }
  bt.interrupt();
  bt.join();
  IPC::shared_memory_object::remove("RetractorShmemMap");
  IPC::message_queue::remove("RetractorQueryQueue");
  for (const auto &element : id2StreamName_Relation) {
    std::string queueName = "brcdbr" + boost::lexical_cast<std::string>(element.first);
    IPC::message_queue::remove(queueName.c_str());
  }
  return retVal;
}
