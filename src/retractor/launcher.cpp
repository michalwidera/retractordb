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

#include "QStruct.h"
#include "compiler.hpp"
#include "config.h"  // Add an automatically generated configuration file

using namespace boost;

using boost::lexical_cast;

extern std::string parserFile(std::string sInputFile);
extern int main_retractor(bool verbose, bool waterfall, int iTimeLimitCntParam);
extern int dumper(boost::program_options::variables_map& vm);

int iTimeLimitCntParam{0};

extern qTree coreInstance;

int main(int argc, char* argv[]) {
  // Clarification: When gcc has been upgraded to 9.x version some tests fails.
  // Bug appear when data are passing to program via script .sh
  // additional 13 (\r) character was append - this code normalize argv list.
  // C99: The parameters argc and argv and the strings pointed to by the argv
  // array shall be modifiable by the program, and retain their last-stored
  // values between program startup and program termination.
  for (int i = 0; i < argc; i++) {
    auto len = strlen(argv[i]);
    if (len > 0)
      if (argv[i][len - 1] == 13) argv[i][len - 1] = 0;
  }
  auto filelog = spdlog::basic_logger_mt("log", std::string(argv[0]) + ".log");
  spdlog::set_default_logger(filelog);
  spdlog::set_pattern(common_log_pattern);
  spdlog::flush_on(spdlog::level::trace);
  std::string sInputFile;
  namespace po = boost::program_options;
  po::variables_map vm;

  po::options_description desc("Available options");

  bool onlyCompile{false};
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--onlycompile") == 0) onlyCompile = true;
  }

  try {
    if (onlyCompile) {
      desc.add_options()                                           //
          ("queryfile,q", po::value<std::string>(&sInputFile),     //
           "query set file")                                       //
          ("help,h", "show help options")                          //
          ("verbose,r", "verbose mode")                            //
          ("dot,d", "create dot file")                             //
          ("csv,m", "create csv file")                             // c->m
          ("view,v", "create dot file and then call dot process")  //
          ("fields,f", "show fields in dot file")                  //
          ("tags,t", "show tags in dot file")                      //
          ("streamprogs,s", "show stream programs in dot file")    //
          ("sdump,p", "take as input file executor dump")          //
          ("leavedot,e", "dont delete temporary dot file")         //
          ("onlycompile,c", "compile only mode");                  // linking inheritance from launcher
    } else {
      desc.add_options()                                                          //
          ("help,h", "Show program options")                                      //
          ("queryfile,q", po::value<std::string>(&sInputFile), "query set file")  //
          ("waterfall,f", "show waterfall mode")                                  //
          ("verbose,v", "Dump diagnostic info on screen while work")              //
          ("tlimitqry,m", po::value<int>(&iTimeLimitCntParam)->default_value(0),  //
           "query limit, 0 - no limit");                                          //
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

    auto parseOut = parserFile(sInputFile);

    //
    // Compile part
    //
    if (coreInstance.empty()) throw std::out_of_range("No queries to process found");

    std::string response;
    response = simplifyLProgram();
    assert(response == "OK");
    response = prepareFields();
    assert(response == "OK");
    response = intervalCounter();
    assert(response == "OK");
    response = convertReferences();
    assert(response == "OK");
    response = replicateIDX();
    assert(response == "OK");
    response = convertRemotes();
    assert(response == "OK");

    if (parseOut != "OK")
      std::cerr << "Input file:" << sInputFile << std::endl  //
                << "Compile result:" << parseOut << std::endl;

    if (onlyCompile) {
      dumper(vm);
      return system::errc::success;
    }

  } catch (std::exception& e) {
    std::cerr << e.what() << "\n";
    return system::errc::interrupted;
  }

  return main_retractor(vm.count("verbose"), vm.count("waterfall"), iTimeLimitCntParam);
}
