#include <chrono>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "config.h"  // Add an automatically generated configuration file
#include "constants.hpp"
#include "qry.hpp"
#include "uxSysTermTools.hpp"

using namespace boost;
using boost::property_tree::ptree;

namespace IPC = boost::interprocess;

// Domyślny czas oczekiwania na gotowość serwera xretractor przed wykonaniem komendy.
constexpr int kDefaultServerWaitSeconds = 30;

// Interwał odpytywania IPC podczas czekania na serwer — kompromis między
// latencją startu a obciążeniem CPU przy czekaniu.
constexpr int kServerWaitPollIntervalMs = 100;

static bool waitForServer(int maxSeconds = kDefaultServerWaitSeconds) {
  const int maxAttempts = maxSeconds * 1000 / kServerWaitPollIntervalMs;
  for (int i = 0; i < maxAttempts; ++i) {
    try {
      IPC::managed_shared_memory seg(IPC::open_only, std::string(ipc::kShmemSegment).c_str());
      IPC::message_queue mq(IPC::open_only, std::string(ipc::kQueryQueue).c_str());
      return true;
    } catch (...) {
      std::this_thread::sleep_for(std::chrono::milliseconds(kServerWaitPollIntervalMs));
    }
  }
  return false;
}

void cleanup() {
  spdlog::shutdown();  // flush logs on disk
}

int main(int argc, char *argv[]) {
  fixArgcv(argc, argv);
  const auto tempLocation = setupLoggerMain(std::string(argv[0]), true);
  /* const int result_atexit = */
  std::atexit(cleanup);

  try {
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    int elemLimit{0};
    std::string sInputStream;
    std::string sDetailStream;
    std::string sAdHoc;
    std::string sGnuplotDim;
    std::tuple<int, int, int> gnuplotDim{0, 0, 0};
    desc.add_options()                                                                                    //
        ("select,s", po::value<std::string>(&sInputStream), "show this stream")                           //
        ("detail,t", po::value<std::string>(&sDetailStream), "show details of this stream")               //
        ("adhoc,a", po::value<std::string>(&sAdHoc), "adhoc query mode")                                  //
        ("elimitqry,m", po::value<int>(&elemLimit)->default_value(0), "limit of elements, 0 - no limit")  //
        ("null,n", "if null row appear - skip it in output")                                              //
        ("hello,l", "diagnostic - hello db world")                                                        //
        ("kill,k", "kill xretractor server")                                                              //
        ("dir,d", "list of queries")                                                                      //
        ("diryaml,y", "list of queries in yaml format")                                                   //
        ("raw,r", "raw output mode (default)")                                                            //
        ("graphite,g", "graphite output mode")                                                            //
        ("influxdb,f", "influxDB output mode")                                                            //
        ("gnuplot,p", po::value<std::string>(&sGnuplotDim), "x,y - gnuplot output mode")                  //
        ("gnuplot-rtl", "gnuplot output: newest samples on the right (right-to-left scroll)")             //
        ("help,h", "produce help message")                                                                //
        ("needctrlc,c", "force ctl+c for stop this tool")                                                 //
        ("wait-server,w", "poll until xretractor server is available before executing command");
    po::positional_options_description p;  // Assume that select is the first option
    p.add("select", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);
    (void)setvbuf(stdout, nullptr, _IONBF, 0);

    qry obj;

    if (vm.count("graphite") + vm.count("raw") + vm.count("influxdb") + vm.count("gnuplot") > 1) {
      std::println("Only one output format could be selected.");
      return system::errc::invalid_argument;
    }
    if (vm.contains("graphite")) obj.outputFormatMode = formatMode::GRAPHITE;
    if (vm.contains("raw")) obj.outputFormatMode = formatMode::RAW;
    if (vm.contains("influxdb")) obj.outputFormatMode = formatMode::INFLUXDB;
    if (vm.contains("gnuplot")) {
      obj.outputFormatMode   = formatMode::GNUPLOT;
      obj.gnuplotRightToLeft = vm.contains("gnuplot-rtl");
      std::stringstream ss(sGnuplotDim);

      auto delimetersCnt = std::count_if(sGnuplotDim.begin(), sGnuplotDim.end(), [](char c) { return c == ',' || c == ':'; });

      char c;
      int x    = 0;
      int ymax = 0;
      int ymin = 0;

      if (delimetersCnt == 1) {
        ss >> x >> c >> ymax;  // expected format is x,y or x:y
      } else if (delimetersCnt == 2) {
        ss >> x >> c >> ymin >> c >> ymax;  // expected format is x,ymin,ymax or x:ymin:ymax
      } else {
        std::print(std::cerr, "gnuplot mode need {{x,y}} or {{x,ymin,ymax}} parameters.");
        return system::errc::invalid_argument;
      }

      if (ss.fail() || !ss.eof()) {
        std::print(std::cerr, "gnuplot mode need {{x,y}} or {{x,ymin,ymax}} parameters.");
        return system::errc::invalid_argument;
      }
      if (x <= 0) {
        std::print("gnuplot mode need x > 0.");
        return system::errc::invalid_argument;
      }
      if (ymin >= ymax) {
        std::print("gnuplot mode need ymin < ymax.");
        return system::errc::invalid_argument;
      }
      gnuplotDim = std::make_tuple(x, ymin, ymax);
    }
    if (vm.contains("gnuplot-rtl") && !vm.contains("gnuplot")) {
      std::print(std::cerr, "--gnuplot-rtl requires --gnuplot/-p mode.");
      return system::errc::invalid_argument;
    }
    if (vm.contains("wait-server") && !vm.contains("help")) {
      if (!waitForServer()) {
        SPDLOG_ERROR("server not available after {} seconds", kDefaultServerWaitSeconds);
        return system::errc::no_child_process;
      }
    }
    if (vm.contains("help")) {
      std::println("{} - data query tool.\n", argv[0]);
      std::println("Usage: {} [option]\n", argv[0]);
      std::cout << desc;
      std::println("{}", config_line);
      std::println("Log: {}", tempLocation);
      std::println("{}", warranty);
      return system::errc::success;
    }
    if (vm.contains("hello")) return obj.hello();
    if (vm.contains("kill") && elemLimit == 0) {
      obj.netClient("kill", "");
    } else if (vm.contains("dir")) {
      std::print("{}", obj.dir());
    } else if (vm.contains("diryaml")) {
      std::print("{}", obj.dirYaml());
    } else if (vm.contains("adhoc") && !sAdHoc.empty()) {
      if (obj.adhoc(sAdHoc)) return system::errc::no_such_file_or_directory;
    } else if (vm.contains("detail")) {
      auto ret = obj.detailShow(sDetailStream);
      if (!ret.empty()) {
        std::print("{}", ret);
      } else
        return system::errc::no_such_file_or_directory;
    } else if (vm.contains("select") && sInputStream != "none") {
      if (!obj.select(vm, elemLimit, sInputStream, gnuplotDim, obj.gnuplotRightToLeft))
        return system::errc::no_such_file_or_directory;
    } else {
      SPDLOG_ERROR("no argument.");
      return EPERM;  // ERROR defined in errno-base.h
    }
  } catch (IPC::interprocess_exception &ex) {
    SPDLOG_ERROR("IPC: {}", ex.what());
    return system::errc::no_child_process;
  } catch (std::exception &e) {
    SPDLOG_ERROR("Std: {}", e.what());
    return system::errc::interrupted;
  }
  return system::errc::success;
}
