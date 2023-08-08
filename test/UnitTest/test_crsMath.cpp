#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <iomanip>

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

TEST_F(crsMathTest, Only_two_items_in_query) { ASSERT_TRUE(coreInstance.size() == 13); }

TEST_F(crsMathTest, test1) {}

// Work in progress

TEST_F(crsMathTest, test2) {
  dataModel proc(coreInstance);
  TimeLine tl(coreInstance.getAvailableTimeIntervals());
  boost::rational<int> prev_interval(0);

  auto queryCounter{15};
  while (queryCounter-- != 1) {
    const int msInSec = 1000;
    boost::rational<int> interval(tl.getNextTimeSlot() * msInSec /* sec->ms */);
    int period(rational_cast<int>(interval - prev_interval));  // miliseconds
    prev_interval = interval;

    std::set<std::string> inSet;
    for (const auto &it : coreInstance)
      if (tl.isThisDeltaAwaitCurrentTimeSlot(it.rInterval)) inSet.insert(it.id);

    std::cout << period << "\t";
    for (const auto &str : inSet) {
      std::cout << "{" << std::setw(4) << str << "}";
    }
    std::cout << std::endl;

    proc.processRows(inSet);
  }
}

}  // Namespace
