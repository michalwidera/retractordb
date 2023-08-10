#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <iomanip>
#include <variant>

#include "config.h"
#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"
#include "retractor/lib/CRSMath.h"
#include "retractor/lib/QStruct.h"  // coreInstance
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/dataModel.h"

// ctest -R '^ut-test_crsMath' -V

// https://github.com/google/googletest/blob/main/docs/index.md

using namespace CRationalStreamMath;

extern std::string parserFile(std::string sInputFile);

extern "C" qTree coreInstance;

const int TEST_COUNT = 15;

// std::unique_ptr<dataModel> dataArea;

struct crsMathTestInit {
  crsMathTestInit() {
    assert(std::filesystem::exists("ut_crsmath.rql") && "file ut_crsmath.rql does not exist!");
    std::ifstream infl("ut_crsmath.rql");
    for (std::string line; std::getline(infl, line);) std::cout << line << std::endl;
  }

  ~crsMathTestInit() {}

} crsMathTestInstance_;

namespace {
class crsMathTest : public ::testing::Test {
 protected:
  crsMathTest() {}

  virtual ~crsMathTest() {}

  virtual void SetUp() {
    auto compiled = parserFile("ut_crsmath.rql") == "OK";
    assert(compiled && "Query set malformed according to grammar.");
    // dataArea = std::make_unique<dataModel>(coreInstance);

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

    coreInstance.maxCapacity = countBuffersCapacity();

    response = applyConstraints();
    assert(response == "OK");
  }

  virtual void TearDown() {
    coreInstance.clear();

    // This magic is clearing all files that have .desc and are .desc - so called artifacts
    std::system("find ./*.desc | sed 's/\\.[^.]*$//' | cut -c 3- | xargs rm -f ; rm -f *.desc");
  }
};

TEST_F(crsMathTest, Only_nine_items_in_query) { ASSERT_TRUE(coreInstance.size() == 9); }

const std::vector<std::string> allStreams = {"cx", "s1x", "s2x", "s3x", "s4x", "s5x", "s6x", "s7x", "s8x"};

TEST_F(crsMathTest, check_if_streams_sequence_are_correct) {
  const auto colSize = 4;
  const auto expectedResult =
      "Dlt: { 1/1}{ 1/3}{ 2/3}{ 1/3}{ 1/1}{ 1/1}{ 1/1}{ 2/3}{ 2/3}\n"
      " 000:{  cx}{    }{    }{    }{    }{    }{    }{    }{    }\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}\n"
      " 333 {  cx}{ s1x}{    }{ s3x}{ s4x}{ s5x}{ s6x}{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }\n"
      " 333 {  cx}{ s1x}{ s2x}{ s3x}{ s4x}{ s5x}{ s6x}{ s7x}{ s8x}\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}\n"
      " 333 {  cx}{ s1x}{    }{ s3x}{ s4x}{ s5x}{ s6x}{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }\n"
      " 333 {  cx}{ s1x}{ s2x}{ s3x}{ s4x}{ s5x}{ s6x}{ s7x}{ s8x}\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}\n";

  std::stringstream strstream;

  dataModel proc(coreInstance);
  TimeLine tl(coreInstance.getAvailableTimeIntervals());
  boost::rational<int> prev_interval(0);

  strstream << std::setw(colSize) << "Dlt: ";

  // Delta presentation

  for (const auto &x : allStreams) strstream << "{" << std::setw(colSize) << coreInstance.getDelta(x) << "}";
  strstream << std::endl;

  // Init row - process all declaration

  std::set<std::string> initSet;
  for (const auto &it : coreInstance)
    if (it.isDeclaration()) initSet.insert(it.id);

  proc.processRows(initSet);

  strstream << std::setw(colSize) << " 000:";
  for (const auto &x : allStreams) strstream << "{" << std::setw(colSize) << (initSet.contains(x) ? x : "") << "}";

  strstream << std::endl;

  // Process declarations and queries in time slots

  auto queryCounter{TEST_COUNT};
  while (queryCounter-- != 1) {
    const int msInSec = 1000;
    boost::rational<int> interval(tl.getNextTimeSlot() * msInSec /* sec->ms */);
    int period(rational_cast<int>(interval - prev_interval));  // miliseconds
    prev_interval = interval;

    strstream << std::setw(colSize) << period << " ";

    std::set<std::string> procSet;
    for (const auto &it : coreInstance)
      if (tl.isThisDeltaAwaitCurrentTimeSlot(it.rInterval)) procSet.insert(it.id);

    for (const auto &x : allStreams) strstream << "{" << std::setw(colSize) << (procSet.contains(x) ? x : "") << "}";

    strstream << std::endl;

    proc.processRows(procSet);
  }

  ASSERT_TRUE(strstream.str() == expectedResult);
}

std::string print(std::string query_name, dataModel &proc) {
  std::stringstream coutstring;
  auto cnt = proc.getPayload(query_name)->getDescriptor().sizeFlat();
  for (auto value : proc.getRow(query_name, 0)) {
    std::visit(Overload{                                                                                                    //
                        [&coutstring](uint8_t a) { coutstring << (unsigned)a; },                                            //
                        [&coutstring](int a) { coutstring << a; },                                                          //
                        [&coutstring](unsigned a) { coutstring << a; },                                                     //
                        [&coutstring](float a) { coutstring << a; },                                                        //
                        [&coutstring](double a) { coutstring << a; },                                                       //
                        [&coutstring](std::pair<int, int> a) { coutstring << a.first << "," << a.second; },                 //
                        [&coutstring](std::pair<std::string, int> a) { coutstring << a.first << "[" << a.second << "]"; },  //
                        [&coutstring](const std::string &a) { coutstring << a; },                                           //
                        [&coutstring](boost::rational<int> a) { coutstring << a; }},
               value);
    if (--cnt) coutstring << ",";
  }  // endfor
  return coutstring.str();
}

TEST_F(crsMathTest, check_if_streams_values_are_correct) {
  const auto colSize = 8;
  const auto expectedResult = "";

  std::stringstream strstream;

  dataModel proc(coreInstance);
  TimeLine tl(coreInstance.getAvailableTimeIntervals());
  boost::rational<int> prev_interval(0);

  // Delta presentation
  std::cout << std::setw(4) << " Dlt:";
  for (const auto &x : allStreams) std::cout << "|" << std::setw(colSize) << coreInstance.getDelta(x);
  std::cout << "|" << std::endl;

  // Names
  std::cout << std::setw(4) << "Name:";
  for (const auto &x : allStreams) std::cout << "|" << std::setw(colSize) << x;
  std::cout << "|" << std::endl;

  // Init row - process all declaration

  std::set<std::string> initSet;
  for (const auto &it : coreInstance)
    if (it.isDeclaration()) initSet.insert(it.id);

  proc.processRows(initSet);

  std::cout << std::setw(4) << " 000 ";
  for (const auto &x : allStreams) std::cout << "|" << std::setw(colSize) << (initSet.contains(x) ? print(x, proc) : "");

  std::cout << "|" << std::endl;

  // Process declarations and queries in time slots

  auto queryCounter{TEST_COUNT};
  while (queryCounter-- != 1) {
    const int msInSec = 1000;
    boost::rational<int> interval(tl.getNextTimeSlot() * msInSec /* sec->ms */);
    int period(rational_cast<int>(interval - prev_interval));  // miliseconds
    prev_interval = interval;

    std::cout << std::setw(4) << period << " ";

    std::set<std::string> procSet;
    for (const auto &it : coreInstance)
      if (tl.isThisDeltaAwaitCurrentTimeSlot(it.rInterval)) procSet.insert(it.id);

    for (const auto &x : allStreams) std::cout << "|" << std::setw(colSize) << (procSet.contains(x) ? print(x, proc) : "");

    std::cout << "|" << std::endl;

    proc.processRows(procSet);
  }

  // ASSERT_TRUE(strstream.str() == expectedResult);
}

}  // Namespace
