#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <sstream>

#include "config.h"  // Add an automatically generated configuration file
#include "qry.hpp"
#include "uxSysTermTools.h"

using namespace boost;
using boost::property_tree::ptree;

namespace IPC = boost::interprocess;

int main(int argc, char *argv[]) {
  if (argc > 0 && argv != nullptr) fixArgcv(argc, argv);

  auto filelog = spdlog::basic_logger_mt("log", std::string(argv[0]) + ".log");
  spdlog::set_default_logger(filelog);
  constexpr auto common_log_pattern = "%C%m%d %T.%e %^%s:%# [%L] %v%$";
  spdlog::set_pattern(common_log_pattern);
  spdlog::flush_on(spdlog::level::trace);

  try {
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    int timeLimit{0};
    std::string sInputStream{""};
    desc.add_options()                                                                                    //
        ("select,s", po::value<std::string>(&sInputStream), "show this stream")                           //
        ("detail,t", po::value<std::string>(&sInputStream), "show details of this stream")                //
        ("tlimitqry,m", po::value<int>(&timeLimit)->default_value(0), "limit of elements, 0 - no limit")  //
        ("hello,l", "diagnostic - hello db world")                                                        //
        ("kill,k", "kill xretractor server")                                                              //
        ("dir,d", "list of queries")                                                                      //
        ("graphite,g", "graphite output mode")                                                            //
        ("influxdb,f", "influxDB output mode")                                                            //
        ("raw,r", "raw mode (default)")                                                                   //
        ("help,h", "show options")                                                                        //
        ("needctrlc,c", "force ctl+c for stop this tool");
    po::positional_options_description p;  // Assume that select is the first option
    p.add("select", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);
    setbuf(stdout, nullptr);
    if (vm.count("graphite")) setmode("GRAPHITE");
    if (vm.count("raw")) setmode("RAW");
    if (vm.count("influxdb")) setmode("INFLUXDB");
    if (vm.count("help")) {
      std::cout << argv[0] << " - data query tool." << std::endl << std::endl;
      std::cout << "Usage: " << argv[0] << " [option]" << std::endl << std::endl;
      std::cout << desc;
      std::cout << config_line << std::endl;
      std::cout << warranty << std::endl;
      return system::errc::success;
    }
    if (vm.count("hello"))
      return hello();
    else if (vm.count("kill")) {
      ptree pt = netClient("kill", "");
      printf("kill sent.\n");
    } else if (vm.count("dir"))
      dir();
    else if (vm.count("detail")) {
      if (!detailShow(sInputStream)) return system::errc::no_such_file_or_directory;
    } else if (vm.count("select") && sInputStream != "none") {
      if (!select(vm.count("needctrlc"), timeLimit, sInputStream)) return system::errc::no_such_file_or_directory;
    } else {
      std::cout << argv[0] << ": fatal error: no argument" << std::endl;
      SPDLOG_ERROR("stop - error, no argument.");
      return EPERM;  // ERROR defined in errno-base.h
    }
  } catch (IPC::interprocess_exception &ex) {
    SPDLOG_ERROR("stop - IPC qry catch client:{}", ex.what());
    return system::errc::no_child_process;
  } catch (std::exception &e) {
    SPDLOG_ERROR("stop - Exception catch client:{}", e.what());
    return system::errc::interrupted;
  }
  std::cout << "ok." << std::endl;
  SPDLOG_INFO("stop");
  return system::errc::success;
}
