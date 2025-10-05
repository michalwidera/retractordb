#include <spdlog/spdlog.h>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <sstream>
#include <utility>

#include "config.h"  // Add an automatically generated configuration file
#include "qry.hpp"
#include "uxSysTermTools.hpp"

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
    std::string sDetailStream{""};
    std::string sAdHoc{""};
    std::string sGnuplotDim{""};
    std::pair<int, int> gnuplotDim{0, 0};
    desc.add_options()                                                                                    //
        ("select,s", po::value<std::string>(&sInputStream), "show this stream")                           //
        ("detail,t", po::value<std::string>(&sDetailStream), "show details of this stream")               //
        ("adhoc,a", po::value<std::string>(&sAdHoc), "adhoc query mode")                                  //
        ("tlimitqry,m", po::value<int>(&timeLimit)->default_value(0), "limit of elements, 0 - no limit")  //
        ("hello,l", "diagnostic - hello db world")                                                        //
        ("kill,k", "kill xretractor server")                                                              //
        ("dir,d", "list of queries")                                                                      //
        ("raw,r", "raw output mode (default)")                                                            //
        ("graphite,g", "graphite output mode")                                                            //
        ("influxdb,f", "influxDB output mode")                                                            //
        ("gnuplot,p", po::value<std::string>(&sGnuplotDim), "x,y - gnuplot output mode")                  //
        ("help,h", "produce help message")                                                                //
        ("needctrlc,c", "force ctl+c for stop this tool");
    po::positional_options_description p;  // Assume that select is the first option
    p.add("select", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);
    setbuf(stdout, nullptr);

    qry obj;

    if (vm.count("graphite") + vm.count("raw") + vm.count("influxdb") + vm.count("gnuplot") > 1) {
      std::cout << "Only one output format could be selected." << std::endl;
      return system::errc::invalid_argument;
    }
    if (vm.count("graphite")) obj.outputFormatMode = formatMode::GRAPHITE;
    if (vm.count("raw")) obj.outputFormatMode = formatMode::RAW;
    if (vm.count("influxdb")) obj.outputFormatMode = formatMode::INFLUXDB;
    if (vm.count("gnuplot")) {
      obj.outputFormatMode = formatMode::GNUPLOT;
      std::stringstream ss(sGnuplotDim);
      char c;
      int x, y;
      ss >> x >> c >> y;  // expected format is x,y or x:y
      if (ss.fail() || !ss.eof() || (c != ',' && c != ':')) {
        std::cout << "gnuplot mode need x and y parameters.";
        return system::errc::invalid_argument;
      }
      if (x <= 0 || y <= 0) {
        std::cout << "gnuplot mode need x and y > 0.";
        return system::errc::invalid_argument;
      }
      gnuplotDim = {x, y};
    }
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
      SPDLOG_INFO("kill sent to server");
    } else if (vm.count("dir")) {
      std::cout << obj.dir();
    } else if (vm.count("adhoc") && sAdHoc != "") {
      if (!obj.adhoc(sAdHoc)) return system::errc::no_such_file_or_directory;
    } else if (vm.count("detail")) {
      auto ret = obj.detailShow(sDetailStream);
      if (ret != "")
        std::cout << ret;
      else
        return system::errc::no_such_file_or_directory;
    } else if (vm.count("select") && sInputStream != "none") {
      if (!obj.select(vm, timeLimit, sInputStream, gnuplotDim)) return system::errc::no_such_file_or_directory;
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
  SPDLOG_INFO("ok.");

  return system::errc::success;
}
