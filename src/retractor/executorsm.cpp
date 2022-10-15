#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <memory>

#include "CRSMath.h"
#include "Processor.h"
#include "QStruct.h"
#include "config.h"  // Add an automatically generated configuration file

// This defines is required to remove deprecation of boost/bind.hpp
// some boost libraries still didn't remove dependency to boost bin
// remove this is boost will clean up on own side.
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <spdlog/spdlog.h>

#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/process/environment.hpp>  // boost::this_process::get_id()
#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>

#include "spdlog/sinks/basic_file_sink.h"  // support for basic file logging

namespace IPC = boost::interprocess;

// Define for IPC purposes - maps & strings (most important IPCString i IPCMap)
typedef IPC::managed_shared_memory::segment_manager segment_manager_t;

typedef IPC::allocator<char, segment_manager_t> CharAllocator;
typedef IPC::basic_string<char, std::char_traits<char>, CharAllocator>
    IPCString;
typedef IPC::allocator<IPCString, segment_manager_t> StringAllocator;

typedef int KeyType;
typedef std::pair<const int, IPCString> ValueType;

typedef IPC::allocator<ValueType, segment_manager_t> ShmemAllocator;
typedef IPC::map<KeyType, IPCString, std::less<KeyType>, ShmemAllocator> IPCMap;

using namespace CRationalStreamMath;

// Segment and allocator for string exchange
// IPC::managed_shared_memory strSegment(IPC::open_or_create,
// "RetractorShmemStr", 65536); const StringAllocator allocatorShmemStrInstance
// (strSegment.get_segment_manager());

// Map stores realtions processId -> sended stream
std::map<const int, std::string> id2StreamName_Relation;

std::vector<IPC::message_queue> qset;

// Object coreInstance in QStruct.cpp
extern "C" qTree coreInstance;

extern Processor *pProc;

// varialbe connected with tlimitqry (-m) parameter
// when it will be set thread will exit by given time (testing purposes)
int iTimeLimitCnt(0);

typedef boost::property_tree::ptree ptree;

extern int iTest();

std::map<std::string, ptree> streamTable;

int _kbhit(void) {
  struct termios oldt, newt;
  int ch;
  int oldf;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }
  return 0;
}

int _getch() { return getchar(); }

std::set<boost::rational<int>> getListFromCore() {
  std::set<boost::rational<int>> lstTimeIntervals;
  for (const auto &it : coreInstance) lstTimeIntervals.insert(it.rInterval);
  return lstTimeIntervals;
}

void dumpCore(std::ostream &xout) {
  xout << "Seqence\tItrval\tQuery id" << std::endl;
  for (const auto &it : coreInstance) {
    xout << getSeqNr(it.id) << "\t";
    xout << it.rInterval << "\t";
    xout << it.id;
    xout << std::endl;
  }
}

std::set<std::string> getAwaitedStreamsSet(TimeLine &tl) {
  std::set<std::string> retVal;
  while (pProc == nullptr)
    boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
  for (const auto &it : coreInstance) {
    boost::rational<int> slot = it.rInterval;
    if (!tl.isThisDeltaAwaitCurrentTimeSlot(slot)) continue;
    retVal.insert(it.id);
  }
  return retVal;
}

void showAwaitedStreams(TimeLine &tl, std::string streamName = "") {
  std::set<std::string> strSet = getAwaitedStreamsSet(tl);
  for (const auto &str : strSet) std::cout << "-" << getSeqNr(str) << "-";
}

ptree commandProcessor(ptree ptInval) {
  ptree ptRetval;
  std::string command = ptInval.get("db.message", "");
  try {
    //
    // This command return stream idenifiers
    //
    if (command == "get" && pProc != nullptr) {
      SPDLOG_DEBUG("get cmd rcv.");
      for (auto &q : coreInstance) {
        ptRetval.put(std::string("db.stream.") + q.id, q.id);
        ptRetval.put(
            std::string("db.stream.") + q.id + std::string(".duration"),
            boost::lexical_cast<std::string>(q.rInterval));
        long recordsCount = -1;
        if (!q.isDeclaration()) recordsCount = streamStoredSize(q.id);
        ptRetval.put(std::string("db.stream.") + q.id + std::string(".size"),
                     boost::lexical_cast<std::string>(recordsCount));
        ptRetval.put(std::string("db.stream.") + q.id + std::string(".count"),
                     boost::lexical_cast<std::string>(getStreamCount(q.id)));
        ptRetval.put(
            std::string("db.stream.") + q.id + std::string(".location"),
            q.filename);
      }
    }
    //
    // This command return what stream contains of
    //
    if (command == "detail" && pProc != nullptr) {
      std::string streamName = ptInval.get("db.argument", "");
      assert(streamName != "");
      SPDLOG_DEBUG("got detail {} rcv.", streamName);
      for (const auto &s : coreInstance[streamName].lSchema)
        ptRetval.put(std::string("db.field.") + s.fieldName, s.fieldName);
    }
    //
    // This command will add stream to list of transmited streams
    // there are created next queue with stream for client
    // and map indentifier with this stream
    //
    if (command == "show" && pProc != nullptr) {
      std::string streamName = ptInval.get("db.argument", "");
      // Probably someone calls show w/o stream name
      assert(streamName != "");
      // check if command present id of process
      assert(ptInval.get("db.id", "") != "");
      // Testing purposes only - get it off after testing
      SPDLOG_DEBUG("got show {} rcv.", streamName);
      // Here we set that for porcess of given id we send apropriate data stream
      int streamId = boost::lexical_cast<int>(ptInval.get("db.id", ""));
      id2StreamName_Relation[streamId] = streamName;
      // Create a message_queue
      std::string queueName = "brcdbr" + ptInval.get("db.id", "");
      // let's assuem that we have 1/10 duration
      // that menas 10 elements are going in second
      // so - we need 10 elements for one second buffer
      int maxElements =
          boost::rational_cast<int>(1 / coreInstance[streamName].rInterval);
      maxElements = (maxElements < 2) ? 2 : maxElements;
      IPC::message_queue mq(IPC::open_or_create  // open or crate
                            ,
                            queueName.c_str()  // name
                            ,
                            maxElements  // max message number
                            ,
                            1024  // max message size
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
      // write_json(strstream, ptRetval) ;
      // write_xml(strstream, ptRetval);
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
void commmandProcessorLoop() {
  try {
    IPC::message_queue::remove("RetractorQueryQueue");
    IPC::shared_memory_object::remove("RetractorShmemMap");
    // Segment and allogator for map purposes
    IPC::managed_shared_memory mapSegment(IPC::open_or_create,
                                          "RetractorShmemMap", 65536);
    const ShmemAllocator allocatorShmemMapInstance(
        mapSegment.get_segment_manager());
    // Create a message_queue.
    IPC::message_queue mq(IPC::open_or_create  // open or crate
                          ,
                          "RetractorQueryQueue"  // name
                          ,
                          1000  // max message number
                          ,
                          1000  // max message size
    );
    IPCMap *mymap = mapSegment.construct<IPCMap>("MyMap")  // object name
                    (std::less<int>(), allocatorShmemMapInstance);
    //
    // This need to be clean up - There are some mess.
    //
    char message[1000];
    unsigned int priority;
    IPC::message_queue::size_type recvd_size;
    while (true) {
      while (mq.try_receive(message, 1000, recvd_size, priority)) {
        message[recvd_size] = 0;
        std::stringstream strstream;
        strstream << message;
        memset(message, 0, 1000);
        ptree pt;
        // read_json(strstream, pt) ;
        // read_xml(strstream, pt);
        read_info(strstream, pt);
        ptree pt_retval = commandProcessor(pt);
        int clientProcessId = boost::lexical_cast<int>(pt.get("db.id", ""));
        // Sending answer
        std::stringstream response_stream;
        // write_json(response_stream, pt_retval) ;
        // write_xml(response_stream, pt_retval);
        write_info(response_stream, pt_retval);
        IPCString response(allocatorShmemMapInstance);
        response = response_stream.str().c_str();
        mymap->insert(std::pair<KeyType, IPCString>(clientProcessId, response));
      }
      boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
    }
  } catch (IPC::interprocess_exception &ex) {
    std::cout << ex.what() << std::endl << "catch on server" << std::endl;
  }
}

std::string printRowValue(const std::string query_name) {
  using boost::property_tree::ptree;
  ptree pt;
  pt.put("stream", query_name);
  pt.put("count",
         boost::lexical_cast<std::string>(getQuery(query_name).lSchema.size()));
  int i = 0;
  for (auto value : pProc->getRow(query_name, 0)) {
    //
    // There is part of communication format - here data are formated for
    // transmission via internal queue.
    //
    std::stringstream retVal;
    retVal << boost::rational_cast<double>(value);
    pt.put(boost::lexical_cast<std::string>(i++), retVal.str());
  }
  std::stringstream strstream;
  // write_json(strstream, pt);
  // write_xml(strstream, pt);
  write_info(strstream, pt);
  return strstream.str();
}

int main(int argc, char *argv[]) {
  // Clarification: When gcc has been upgraded to 9.x version some tests fails.
  // Bug appear when data are passing to program via script .sh
  // additional 13 (\r) character was append - this code normalize argv list.
  // C99: The parameters argc and argv and the strings pointed to by the argv
  // array shall be modifiable by the program, and retain their last-stored
  // values between program startup and program termination.
  auto retVal = system::errc::success;
  for (int i = 0; i < argc; i++) {
    auto len = strlen(argv[i]);
    if (len > 0)
      if (argv[i][len - 1] == 13) argv[i][len - 1] = 0;
  }
  auto filelog = spdlog::basic_logger_mt("log", std::string(argv[0]) + ".log");
  spdlog::set_default_logger(filelog);
  spdlog::set_pattern(common_log_pattern);
  spdlog::flush_on(spdlog::level::trace);
  SPDLOG_INFO("{} start  [-------------------]", argv[0]);
  thread bt(commmandProcessorLoop);  // Sending service in thread
  // This line - delay is ugly fix for slow machine on CI !
  boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
  try {
    namespace po = boost::program_options;
    std::string sInputFile;
    std::string sQuery;
    std::string sDumpFile;
    po::options_description desc("Supported program options");
    desc.add_options()                        //
        ("help,h", "show help")               //
        ("compiler", "show compiler config")  //
        ("infile,i",
         po::value<std::string>(&sInputFile)->default_value("query.qry"),
         "input query plan")  //
        ("display,s", po::value<std::string>(&sQuery),
         "process single query")  //
        ("dump,d",
         po::value<std::string>(&sDumpFile)->default_value("query.dmp"),
         "dump file name")  //
        ("tlimitqry,m", po::value<int>(&iTimeLimitCnt)->default_value(0),
         "query limit, 0 - no limit")           //
        ("waterfall,f", "show waterfall mode")  //
        ("verbose,v", "Dump diagnostic info on screen while work");
    // Assume that infile is the first option
    po::positional_options_description p;
    p.add("infile", -1);
    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(p).run(),
        vm);
    po::notify(vm);
    if (vm.count("verbose"))
      std::cerr << argv[0] << " - query plan executor." << std::endl;
    if (vm.count("help")) {
      std::cout << desc;
      std::cout << config_line;
      return system::errc::success;
    }
    if (vm.count("compiler")) {
      std::cout << config_line;
      return system::errc::success;
    }
    if (!boost::filesystem::exists(sInputFile)) {
      std::cout << argv[0] << ": fatal error: no input file" << std::endl;
      std::cout << "query processing terminated." << std::endl;
      return EPERM;  // ERROR defined in errno-base.h
    }
    if (vm.count("verbose")) std::cerr << "Input :" << sInputFile << std::endl;
    std::ifstream ifs(sInputFile.c_str(), std::ios::binary);
    if (!ifs) {
      std::cerr << sInputFile << " - no input file" << std::endl;
      return system::errc::invalid_argument;
    }
    //
    // Load of compiled query from file
    //
    boost::archive::text_iarchive ia(ifs);
    ia >> coreInstance;
    //
    // Special parameters support in qurey set
    // fetch all ':*' - and remove them from coreInstance
    //
    std::string storagePath("");
    for (auto qry : coreInstance) {
      if (qry.id == ":STORAGE") storagePath = qry.filename;
    }
    auto new_end =
        std::remove_if(coreInstance.begin(), coreInstance.end(),
                       [](const query &qry) { return qry.id[0] == ':'; });
    coreInstance.erase(new_end, coreInstance.end());
    // setStorageLocation(storagePath);
    //
    // End of special parameters support
    //
    Processor proc;
    if (vm.count("verbose")) {
      std::cerr << "Objects:" << std::endl;
      dumpCore(std::cerr);
      std::cerr << "Storage location:" << storagePath << std::endl;
    }
    TimeLine tl(getListFromCore());
    //
    // Main loop of data processing
    //
    // When this value is 0 - means we are waiting for key - other way watchdog
    //
    if (vm.count("verbose")) {
      std::cout << ((iTimeLimitCnt == 0)
                        ? "Press any key to stop."
                        : "Query limit (-m) waiting for fullfil")
                << std::endl;
    }
    boost::rational<int> prev_interval(0);
    while (!_kbhit() && iTimeLimitCnt != 1) {
      if (iTimeLimitCnt != 0) {
        if (iTimeLimitCnt != 1)
          iTimeLimitCnt--;
        else
          break;
      }
      //
      // Inner time is counted in miliseconds
      // probably can be increased in faster machines
      //
      const int msInSec = 1000;
      boost::rational<int> interval(tl.getNextTimeSlot() *
                                    msInSec /* sec->ms */);
      int period(rational_cast<int>(interval - prev_interval));  // miliseconds
      prev_interval = interval;
      //
      // When you add additional parameter -w numbers of queries appear on
      // screen if additional -w -s str2 (when -s means display) only given one
      // query will appear
      //
      if (vm.count("waterfall")) {
        std::cout << period << "\t";
        showAwaitedStreams(tl, vm.count("display") ? sQuery : "");
        std::cout << std::endl;
      }
      std::set<std::string> inSet = getAwaitedStreamsSet(tl);
      proc.processRows(inSet);
      //
      // Data broadcast - main loop
      //
      for (auto queryName : getAwaitedStreamsSet(tl)) {
        std::string row = printRowValue(queryName);
        std::list<int> eraseList;
        for (const auto &element : id2StreamName_Relation) {
          if (element.second == queryName) {
            using namespace boost::interprocess;
            //
            // Query discovery. queues are created by show command
            //
            std::string queueName =
                "brcdbr" + boost::lexical_cast<std::string>(element.first);
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
          if (vm.count("verbose"))
            std::cout << "queue erased on timeout, procId=" << element
                      << std::endl;
        }
      }
      //
      // Waiting given miliseconds time that is computed
      //
      boost::this_thread::sleep_for(boost::chrono::milliseconds(period));
      //
      // End of loop while( ! _kbhit() )
      //
    }
    //
    // End of main processing loop
    //
    if (iTimeLimitCnt != 1) {
      _getch();  // no wait ... feed key from kbhit
    } else {
      if (vm.count("verbose"))
        std::cout << "Query limit (-m) waiting for fullfil" << std::endl;
    }
  } catch (IPC::interprocess_exception &ex) {
    std::cerr << ex.what() << std::endl
              << "IPC::interprocess exception" << std::endl;
    retVal = system::errc::no_child_process;
  } catch (std::exception &e) {
    std::cerr << "Fail";
    std::cerr << "." << std::endl;
    SPDLOG_ERROR("catch exception: {}", e.what());
    retVal = system::errc::interrupted;
  }
  bt.interrupt();
  bt.join();
  IPC::shared_memory_object::remove("RetractorShmemMap");
  IPC::message_queue::remove("RetractorQueryQueue");
  for (const auto &element : id2StreamName_Relation) {
    std::string queueName =
        "brcdbr" + boost::lexical_cast<std::string>(element.first);
    IPC::message_queue::remove(queueName.c_str());
  }
  return retVal;
}
