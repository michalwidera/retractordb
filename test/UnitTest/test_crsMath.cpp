#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <filesystem>

#include "config.h"
#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"
#include "retractor/lib/CRSMath.h"
#include "retractor/lib/QStruct.h"  // coreInstance
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/dataModel.h"

// ctest -R '^ut-buffManagement' -V

// https://github.com/google/googletest/blob/main/docs/index.md

extern std::string parserFile(std::string sInputFile);

extern "C" qTree coreInstance;

std::unique_ptr<dataModel> dataArea;

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
    dataArea = std::make_unique<dataModel>(coreInstance);

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
    for (auto q : coreInstance)
      if (!q.isDeclaration()) dataArea->qSet[q.id]->outputPayload->reset();  // This removes artifacts/ i.e. data files
    coreInstance.clear();
  }
};

TEST_F(crsMathTest, Only_two_items_in_query) { ASSERT_TRUE(coreInstance.size() == 13); }

TEST_F(crsMathTest, test1) {}

TEST_F(crsMathTest, test2) {}

}  // Namespace
