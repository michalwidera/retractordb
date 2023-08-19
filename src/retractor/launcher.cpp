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

#include "config.h"  // Add an automatically generated configuration file
#include "lib/QStruct.h"
#include "lib/compiler.h"
#include "lib/dumper.h"
#include "lib/executorsm.h"
#include "uxSysTermTools.h"

using namespace boost;

using boost::lexical_cast;

extern std::string parserFile(qTree &coreInstance, std::string sInputFile);

static int iTimeLimitCntParam{0};

int main(int argc, char *argv[]) {
  qTree coreInstance;

  fixArgcv(argc, argv);
  setupLoggerMain(std::string(argv[0]));

  namespace po = boost::program_options;
  po::variables_map vm;
  po::options_description desc("Available options");

  bool onlyCompile{false};
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--onlycompile") == 0) onlyCompile = true;
  }

  try {
    std::string sInputFile{""};
    if (onlyCompile) {
      desc.add_options()                                                          //
          ("help,h", "show help options")                                         //
          ("queryfile,q", po::value<std::string>(&sInputFile), "query set file")  //
          ("quiet,r", "no output on screen, skip dumper")                         //
          ("dot,d", "create dot output")                                          //
          ("csv,m", "create csv output")                                          // c->m
          ("fields,f", "show fields in dot file")                                 //
          ("tags,t", "show tags in dot file")                                     //
          ("streamprogs,s", "show stream programs in dot file")                   //
          ("onlycompile,c", "compile only mode");                                 // linking inheritance from launcher
    } else {
      desc.add_options()                                                          //
          ("help,h", "Show program options")                                      //
          ("queryfile,q", po::value<std::string>(&sInputFile), "query set file")  //
          ("verbose,v", "verbose mode (show stream params)")                      //
          ("tlimitqry,m", po::value<int>(&iTimeLimitCntParam)->default_value(0),  //
           "query limit, 0 - no limit")                                           //
          ("onlycompile,c", "compile only mode");                                 // linking inheritance from launcher
    }
    po::positional_options_description p;  // Assume that infile is the first option
    p.add("queryfile", -1);
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

    po::notify(vm);

    if (vm.count("help")) {
      std::cout << argv[0] << " - compiler & data processing tool." << std::endl << std::endl;
      std::cout << "Usage: " << argv[0] << " queryfile [option]" << std::endl << std::endl;
      std::cout << desc;
      std::cout << config_line << std::endl;
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

    auto parseOut = parserFile(coreInstance, sInputFile);

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
    compiler cm(coreInstance);
    response = cm.simplifyLProgram();
    assert(response == "OK");
    response = cm.prepareFields();
    assert(response == "OK");
    response = cm.intervalCounter();
    assert(response == "OK");
    response = cm.convertReferences();
    assert(response == "OK");
    response = cm.replicateIDX();
    assert(response == "OK");
    response = cm.convertRemotes();
    assert(response == "OK");

    coreInstance.maxCapacity = cm.countBuffersCapacity();

    response = cm.applyConstraints();
    if (response != "OK") {
      std::cerr << "Input file:" << sInputFile << std::endl  //
                << "Check result:" << response << std::endl;
      return system::errc::protocol_error;
    }

    if (onlyCompile) {
      if (!vm.count("quiet")) {
        dumper dm(coreInstance);
        return dm.run(vm);
      }
      return system::errc::success;
    }

  } catch (std::exception &e) {
    std::cerr << e.what() << "\n";
    return system::errc::interrupted;
  }

  executorsm exec(coreInstance);
  return exec.run(vm.count("verbose"), iTimeLimitCntParam);
}
