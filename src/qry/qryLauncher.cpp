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

    qry obj;

    if (vm.count("graphite")) obj.setmode("GRAPHITE");
    if (vm.count("raw")) obj.setmode("RAW");
    if (vm.count("influxdb")) obj.setmode("INFLUXDB");
    if (vm.count("help")) {
      std::cout << argv[0] << " - data query tool." << std::endl << std::endl;
      std::cout << "Usage: " << argv[0] << " [option]" << std::endl << std::endl;
      std::cout << desc;
      std::cout << config_line << std::endl;
      std::cout << "Log: " << tempLocation << std::endl;
      std::cout << warranty << std::endl;
      return system::errc::success;
    }
    if (vm.count("hello"))
      return obj.hello();
    else if (vm.count("kill")) {
      ptree pt = obj.netClient("kill", "");
      SPDLOG_INFO("kill sent");
    } else if (vm.count("dir"))
      obj.dir();
    else if (vm.count("detail")) {
      if (!obj.detailShow(sInputStream)) return system::errc::no_such_file_or_directory;
    } else if (vm.count("select") && sInputStream != "none") {
      if (!obj.select(vm.count("needctrlc"), timeLimit, sInputStream)) return system::errc::no_such_file_or_directory;
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
  SPDLOG_INFO("ok");

  return system::errc::success;
}
