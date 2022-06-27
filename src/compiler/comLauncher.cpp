#include <spdlog/spdlog.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/stacktrace.hpp>
#include <boost/system/error_code.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#include "QStruct.h"
#include "compiler.hpp"
#include "config.h"  // Add an automatically generated configuration file
#include "spdlog/sinks/basic_file_sink.h"  // support for basic file logging

using namespace boost;

using boost::lexical_cast;

extern std::string parser(std::string sInputFile);
extern std::string storeParseResult(std::string sOutputFile);

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
  SPDLOG_INFO("{} start  [-------------------]", argv[0]);
  try {
    std::unique_ptr<std::ostream> p_ofs;
    namespace po = boost::program_options;
    std::string sOutputFile;
    std::string sInputFile;
    po::options_description desc("Avaiable options");
    desc.add_options()                      // formating
        ("help,h", "Show program options")  //
        ("queryfile,q", po::value<std::string>(&sInputFile),
         "query set file")  //
        ("outfile,o",
         po::value<std::string>(&sOutputFile)->default_value("query.qry"),
         "output file")  //
        ("dumpcross,d",
         "dump diagnostic cross "
         "compilation forms");
    po::positional_options_description
        p;  // Assume that infile is the first option
    p.add("queryfile", -1);
    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(p).run(),
        vm);
    po::notify(vm);
    if (vm.count("help")) {
      std::cout << desc;
      std::cout << config_line;
      return system::errc::success;
    }
    if (vm.count("queryfile") == 0) {
      std::cout << argv[0] << ": fatal error: no input file" << std::endl;
      std::cout << "compilation terminated." << std::endl;
      return EPERM;  // ERROR defined in errno-base.h
    }
    std::string sQueryFile = vm["queryfile"].as<std::string>();
    std::string sOutFile = vm["outfile"].as<std::string>();
    std::cout << "Input file:" << sQueryFile << std::endl;
    std::cout << "Compile result:" << parser(sQueryFile) << std::endl;
    //
    // Main Algorithm body
    //
    std::istringstream iss(dumpInstance(""), std::ios::binary);
    boost::archive::text_iarchive ia(iss);
    ia >> coreInstance;
    if (coreInstance.empty())
      throw std::out_of_range("No queries to process found");
    if (vm.count("dumpcross")) {
      std::string sOutFileLog1 = vm["outfile"].as<std::string>() + ".lg1";
      dumpInstance(sOutFileLog1);
    }
    std::string response;
    response = simplifyLProgram();
    assert(response == "OK");
    if (vm.count("dumpcross")) {
      std::string sOutFileLog2 = vm["outfile"].as<std::string>() + ".lg2";
      dumpInstance(sOutFileLog2);
    }
    response = prepareFields();
    assert(response == "OK");
    if (vm.count("dumpcross")) {
      std::string sOutFileLog3 = vm["outfile"].as<std::string>() + ".lg3";
      dumpInstance(sOutFileLog3);
    }
    response = intervalCounter();
    assert(response == "OK");
    response = convertReferences();
    assert(response == "OK");
    response = replicateIDX();
    assert(response == "OK");
    dumpInstance(sOutFile);
  } catch (std::exception& e) {
    std::cerr << e.what() << "\n";
    // cerr << boost::stacktrace::stacktrace() << endl ;
    return system::errc::interrupted;
  }
  return system::errc::success;
}
