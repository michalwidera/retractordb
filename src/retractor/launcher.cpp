#include <signal.h>
#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "config.h"  // Add an automatically generated configuration file
#include "lib/QStruct.h"
#include "lib/compiler.h"
#include "lib/executorsm.h"
#include "lib/lockManager.hpp"
#include "lib/presenter.h"
#include "uxSysTermTools.hpp"

using namespace boost;

using boost::lexical_cast;

extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, std::string sInputFile);

extern int iTimeLimitCnt;

extern std::vector<std::pair<std::string, std::string>> processedLines;

static void handleSignal(int signum) {
  switch (signum) {
    case SIGINT:
      SPDLOG_INFO("Received SIGINT, initiating shutdown...");
      break;
    case SIGTERM:
      SPDLOG_INFO("Received SIGTERM, initiating shutdown...");
      break;
    case SIGHUP:
      SPDLOG_INFO("Received SIGHUP, initiating shutdown...");
      break;
    default:
      SPDLOG_INFO("Received unknown signal: {}", signum);
      break;
  }

  // This will cause the main loop to exit
  iTimeLimitCnt = executorsm::stop_now;
}

int main(int argc, char *argv[]) {
  qTree coreInstance;
  compiler cm(coreInstance);

  fixArgcv(argc, argv);
  const auto tempLocation = setupLoggerMain(std::string(argv[0]));

  namespace po = boost::program_options;
  po::variables_map vm;
  po::options_description desc("Available options");

  bool onlyCompile{false};
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--onlycompile") == 0) onlyCompile = true;
  }

  const std::string serviceName = std::string(argv[0]) + "_service";
  FlockServiceGuard guard(serviceName);

  try {
    std::string sInputFile{""};
    std::string sDiagram{""};
    if (onlyCompile) {
      desc.add_options()                                                             //
          ("help,h", "show help options")                                            //
          ("queryfile,q", po::value<std::string>(&sInputFile), "query set file")     //
          ("quiet,r", "no output on screen, skip presenter")                         //
          ("dot,d", "create dot output")                                             //
          ("csv,m", "create csv output")                                             // c->m
          ("fields,f", "show fields in dot file")                                    //
          ("tags,t", "show tags in dot file")                                        //
          ("streamprogs,s", "show stream programs in dot file")                      //
          ("rules,u", "show rules in dot file")                                      //
          ("transparent,p", "make dot background transparent")                       //
          ("diagram,w", po::value<std::string>(&sDiagram), "create diagram output")  //
          ("onlycompile,c", "compile only mode");                                    // linking inheritance from launcher
    } else {
      desc.add_options()                                                                             //
          ("help,h", "Show program options")                                                         //
          ("status,s", "check service status")                                                       //
          ("queryfile,q", po::value<std::string>(&sInputFile), "query set file")                     //
          ("verbose,v", "verbose mode (show stream params)")                                         //
          ("tlimitqry,m", po::value<int>(&iTimeLimitCnt)->default_value(executorsm::inifitie_loop),  //
           "query limit, 0 - no limit")                                                              //
          ("onlycompile,c", "compile only mode");  // linking inheritance from launcher
    }
    po::positional_options_description p;  // Assume that infile is the first option
    p.add("queryfile", -1);
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

    po::notify(vm);

    if (vm.count("status")) {
      std::cout << "Checking service status." << std::endl;
      bool isRunning = guard.isAnotherInstanceRunning();
      std::cout << serviceName << ": " << (isRunning ? "Running" : "Stopped") << std::endl;
      return isRunning ? system::errc::no_lock_available : system::errc::success;
    }

    if (vm.count("help")) {
      std::cout << argv[0] << " - compiler & data processing tool." << std::endl << std::endl;
      std::cout << "Usage: " << argv[0] << " queryfile [option]" << std::endl << std::endl;
      std::cout << desc;
      std::cout << config_line << std::endl;
      std::cout << "Log: " << tempLocation << std::endl;
      std::cout << warranty << std::endl;
      return system::errc::success;
    }

    if (!vm.count("queryfile")) {
      std::cout << argv[0] << ": fatal error: no input file" << std::endl;
      return EPERM;  // ERROR defined in errno-base.h
    }
    if (!std::filesystem::exists(sInputFile)) {
      std::cout << argv[0] << ": fatal error: file " << sInputFile << " does not exist." << std::endl;
      return EPERM;  // ERROR defined in errno-base.h
    }

    std::ifstream file(sInputFile);
    if (!file.is_open()) {
      std::cerr << "Error: Unable to open file!" << std::endl;
      return system::errc::protocol_error;
    }

    std::string parseOut = "Empty file.";
    std::string line;
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#') continue;  // Skip empty lines and comments
      auto [status, first_keyword, stream_name] = parserRQLString(coreInstance, line);
      parseOut                                  = status;
      if (status != "OK") break;  // Return error if parsing fails
      processedLines.push_back({stream_name, line});
    }

    file.close();

    if (parseOut != "OK") {
      std::cerr << "Input file:" << sInputFile << std::endl  //
                << "Parse result:" << parseOut << std::endl;
      return system::errc::protocol_error;
    }

    //
    // Compile part
    //
    if (coreInstance.empty()) throw std::out_of_range("No queries to process found");

    std::string response;

    response = cm.run();

    if (response != "OK") {
      std::cerr << "Input file:" << sInputFile << std::endl  //
                << "Check result:" << response << std::endl;
      return system::errc::protocol_error;
    }
    assert(response == "OK");

    if (onlyCompile) {
      if (!vm.count("quiet")) {
        presenter dm(coreInstance);
        return dm.run(vm);
      }
      return system::errc::success;
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << "\n";
    return system::errc::interrupted;
  }

  signal(SIGINT, handleSignal);   // Ctrl+C
  signal(SIGTERM, handleSignal);  // Terminate
  signal(SIGHUP, handleSignal);   // Hangup

  if (!guard.acquireLock()) {
    SPDLOG_ERROR("Cannot acquire service lock, another instance might be running.");
    return system::errc::no_lock_available;
  };

  SPDLOG_INFO("Service lock acquired successfully.");
  SPDLOG_INFO("Current process PID: {}", getpid());

  executorsm exec;
  return exec.run(coreInstance, vm.count("verbose"), guard, cm);
}
