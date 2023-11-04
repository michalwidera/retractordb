/*
How xqry terminal works

1. Take a command line argument ie. xqry list
2. Connect to server
3. Execute command
4. Show result (Show result,....) (wait for ctrl+c)
5. End of process
*/

#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <array>
#include <boost/config.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/regex.hpp>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>
#include <thread>

// for: BOOST_FOREACH( cmd_e i, commands | map_keys )
#include <algorithm>
#include <boost/assign.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/process/environment.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm.hpp>
#include <ctime>

#include "qry.hpp"
#include "uxSysTermTools.h"

using namespace boost;
using namespace boost::assign;
using namespace boost::placeholders;

using boost::property_tree::ptree;

namespace IPC = boost::interprocess;

// definitions for IPC - maps i strings (most important IPCString i IPCMap)
typedef IPC::managed_shared_memory::segment_manager segment_manager_t;

typedef IPC::allocator<char, segment_manager_t> CharAllocator;
typedef IPC::basic_string<char, std::char_traits<char>, CharAllocator> IPCString;
typedef IPC::allocator<IPCString, segment_manager_t> StringAllocator;

typedef int KeyType;
typedef std::pair<const int, IPCString> ValueType;

typedef IPC::allocator<ValueType, segment_manager_t> ShmemAllocator;
typedef IPC::map<KeyType, IPCString, std::less<KeyType>, ShmemAllocator> IPCMap;

boost::lockfree::spsc_queue<ptree, boost::lockfree::capacity<1024>> spsc_queue;

std::atomic<bool> done{false};

std::map<std::string, boost::property_tree::ptree> streamTable;
int timeLimitCntQry{0};  // testing purposes - time limit query (-m)
enum class formatMode { RAW, GRAPHITE, INFLUXDB };
formatMode outputFormatMode{formatMode::RAW};

// Graphite embedded schema in format "path.to.data value timestamp"
boost::property_tree::ptree schema;

std::string sInputStream{""};

void qry::setmode(std::string const &mode) {
  if (mode == "RAW")
    outputFormatMode = formatMode::RAW;
  else if (mode == "GRAPHITE")
    outputFormatMode = formatMode::GRAPHITE;
  else if (mode == "INFLUXDB")
    outputFormatMode = formatMode::INFLUXDB;
  else
    assert(false);
}

//
// Consumer process asynchronously fetch data from query and puts on
// screen/output
//
void qry::consumer() {
  ptree e_value;
  while (!done) {
    while (spsc_queue.pop(e_value)) {
      std::string stream = e_value.get("stream", "");
      using namespace boost::adaptors;
      BOOST_FOREACH (std::string w, streamTable | map_keys)
        if (w == stream) {
          int count = std::stoi(e_value.get("count", ""));
          if (outputFormatMode == formatMode::RAW) {
            for (int i = 0; i < count; i++) printf("%s ", e_value.get(std::to_string(i), "").c_str());
            printf("\r\n");
          }
          if (outputFormatMode == formatMode::GRAPHITE) {
            int i = 0;
            for (const auto &v : schema.get_child("db.field")) {
              printf("%s.%s %s %llu\n", sInputStream.c_str(), v.second.get<std::string>("").c_str(),
                     e_value.get(std::to_string(i++), "").c_str(), (unsigned long long)time(nullptr));
            }
          }
          // https://docs.influxdata.com/influxdb/v1.5/write_protocols/line_protocol_tutorial/
          if (outputFormatMode == formatMode::INFLUXDB) {
            using namespace std::chrono;
            int i = 0;
            printf("%s ", sInputStream.c_str());
            bool firstValNoComma(true);
            for (const auto &v : schema.get_child("db.field")) {
              if (firstValNoComma)
                firstValNoComma = false;
              else
                printf(",");
              printf("%s=%s", v.second.get<std::string>("").c_str(), e_value.get(std::to_string(i++), "").c_str());
            }
            printf(" %ld\n", duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count());
          }
          // This part is time limited (-m) resposbile
          if (timeLimitCntQry > 1) --timeLimitCntQry;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  while (spsc_queue.pop(e_value)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void qry::producer() {
  try {
    std::string queueName = "brcdbr" + std::to_string(boost::this_process::get_id());
    IPC::message_queue mq(IPC::open_only, queueName.c_str());
    std::array<char, 1024> message;
    unsigned int priority;
    IPC::message_queue::size_type recvd_size = 1024;
    while (!done) {
      bool messageReceived = false;
      while (!messageReceived && !done) {
        messageReceived = mq.try_receive(message.data(), 1024, recvd_size, priority);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      if (done) continue;
      message[recvd_size] = 0;
      std::stringstream strstream;
      strstream << message.data();
      memset(message.data(), 0, 1000);
      ptree pt;
      // read_json(strstream, pt) ;
      // read_xml(strstream, pt);
      read_info(strstream, pt);
      while (!spsc_queue.push(pt)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  } catch (IPC::interprocess_exception &ex) {
    std::cerr << ex.what() << std::endl << "catch on producer queue" << std::endl;
    std::cerr << "queue:"
              << "brcdbr" + std::to_string(boost::this_process::get_id()) << std::endl;
    done = true;
    return;
  }
}

ptree qry::netClient(std::string netCommand, const std::string &netArgument) {
  ptree pt_response;
  ptree pt_request;
  try {
    IPC::managed_shared_memory mapSegment(IPC::open_only, "RetractorShmemMap");
    const ShmemAllocator allocatorShmemMapInstance(mapSegment.get_segment_manager());
    pt_request.put("db.message", netCommand);
    pt_request.put("db.id", boost::this_process::get_id());
    if (netArgument != "") pt_request.put("db.argument", netArgument);
    //
    // request part
    //
    IPC::message_queue mq(IPC::open_only, "RetractorQueryQueue");
    std::stringstream request_stream;
    // write_json(request_stream, pt_request) ;
    // write_xml(request_stream, pt_request);
    write_info(request_stream, pt_request);
    mq.send(request_stream.str().c_str(), request_stream.str().length(), 0);
    // Send the request.
    //
    // Response part
    //
    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    // Check that response is OK.
    std::pair<IPCMap *, std::size_t> ret = mapSegment.find<IPCMap>("MyMap");
    IPCMap *mymap                        = ret.first;
    assert(mymap);
    std::size_t processId = boost::this_process::get_id();
    auto it               = mymap->find(processId);

    // When server works under valgrid - must be 10 probes x 10ms
    constexpr int maxAcceptableFails = 10;

    int cntr(maxAcceptableFails);
    while (it == mymap->end() && cntr) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      it = mymap->find(processId);
      --cntr;
    }
    if (it == mymap->end()) {
      printf("server not found\n");
      SPDLOG_ERROR("server not found");
      done = true;
      pt_response.put("error.response", "server not found");
      return pt_response;
    }
    std::stringstream strstream;
    strstream << it->second;
    mymap->erase(processId);
    // read_json(strstream, pt_response) ;
    // read_xml(strstream, pt_response);
    read_info(strstream, pt_response);
  } catch (IPC::interprocess_exception &e) {
    SPDLOG_ERROR("IPC::interprocess_exception catch {}", e.what());
    done = true;
  } catch (boost::system::system_error &e) {
    SPDLOG_ERROR("boost::system::system_error catch {}", e.what());
    done = true;
  } catch (std::exception &e) {
    SPDLOG_ERROR("std::exception catch {}", e.what());
    pt_response.put("error.response", e.what());
    done = true;
  }
  return pt_response;
}

bool qry::select(bool noneedctrlc, int iTimeLimit, const std::string &input) {
  timeLimitCntQry = iTimeLimit;  // set value from Launcher.
  sInputStream    = input;       // this is required for consumer process.
  ptree pt        = netClient("get", "");

  auto stream      = pt.get_child("db.stream");
  const bool found = std::any_of(stream.begin(), stream.end(), [input, this](const auto &node) {
    const ptree &v = node.second;
    bool ret       = (input == v.get<std::string>(""));
    if (ret) streamTable[sInputStream] = netClient("show", sInputStream);
    return ret;
  });

  if (!found) {
    SPDLOG_ERROR("not found: {}", input);
    return found;
  }
  schema = netClient("detail", sInputStream);
  //
  // Function in this thread will start listner on udp
  //
  std::jthread producer_thread(producer);
  //
  // Function in this thrade will start fetching data from queue
  //
  std::jthread consumer_thread(consumer);
  do {
    if (noneedctrlc) {
      // If this option appear - any key will not stop process
    } else {
      if (_kbhit()) break;
    }
    if (done) break;
    if (timeLimitCntQry == 1) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  } while (true);
  if (timeLimitCntQry != 1 && !done) {
    _getch();  // no wait ... feed key from kbhit
  }
  done = true;
  producer_thread.join();
  consumer_thread.join();
  assert(found == true);
  return found;
}

int qry::hello() {
  ptree pt = netClient("hello", "");
  SPDLOG_INFO("snd: hello");

  std::string rcv("fail.");
  BOOST_FOREACH (ptree::value_type &v, pt) {
    rcv = v.second.get<std::string>("");
    SPDLOG_INFO("rcv: {} {}", v.first.c_str(), rcv.c_str());
  }
  if (rcv != "world") {
    SPDLOG_ERROR("{}", rcv.c_str());
    return system::errc::protocol_error;
  }
  return system::errc::success;
  // Fail in this part of code could means that server in in json mode
  // and query is going xml. (or otherwise)
  // catches in regressions
}

void qry::dir() {
  ptree pt                       = netClient("get", "");
  std::vector<std::string> vcols = {"", "duration", "size", "count", "location", "cap"};
  std::stringstream ss;
  for (auto nName : vcols) {
    auto stream    = pt.get_child("db.stream");
    auto maxSizeIt = std::max_element(stream.begin(), stream.end(), [&nName](const auto &node1, const auto &node2) {
      const ptree &v1 = node1.second;
      const ptree &v2 = node2.second;

      return v1.get<std::string>(nName).length() < v2.get<std::string>(nName).length();
    });
    ss << "|%";
    ss << maxSizeIt->second.get<std::string>(nName).length();
    ss << "s";
  }
  ss << "|\n";
  for (const auto &v : pt.get_child("db.stream")) {
    printf(ss.str().c_str(), v.second.get<std::string>("").c_str(), v.second.get<std::string>("duration").c_str(),
           v.second.get<std::string>("size").c_str(), v.second.get<std::string>("count").c_str(),
           v.second.get<std::string>("location").c_str(), v.second.get<std::string>("cap").c_str());
  }
}

bool qry::detailShow(const std::string &input) {
  ptree pt = netClient("get", "");
  SPDLOG_INFO("got answer");

  const auto streams = pt.get_child("db.stream");
  bool found         = std::any_of(streams.begin(), streams.end(), [&input](const auto &node) {
    const ptree &v = node.second;
    return input == v.get<std::string>("");
  });

  if (found) {
    ptree ptsh = netClient("detail", input);
    for (const auto &v : ptsh.get_child("db.field")) printf("%s.%s\n", input.c_str(), v.second.get<std::string>("").c_str());
  } else
    SPDLOG_ERROR("not found");

  return found;
}
