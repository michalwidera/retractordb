#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <filesystem>

#include "config.h"
#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"
#include "retractor/lib/QStruct.h"  // coreInstance
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/dataModel.h"

// ctest -R '^ut-buffManagement' -V

// https://github.com/google/googletest/blob/main/docs/index.md

extern std::string parserFile(std::string sInputFile);

extern "C" qTree coreInstance;

std::unique_ptr<dataModel> dataArea;

struct buffMgTestInit {
  buffMgTestInit() {
    assert(std::filesystem::exists("ut_agse.rql") && "file ut_agse.rql does not exist!");
    std::ifstream infl("ut_agse.rql");
    for (std::string line; std::getline(infl, line);) std::cout << line << std::endl;
  }

  ~buffMgTestInit() {}

} buffMgTestInstance_;

namespace {
class buffMgTest : public ::testing::Test {
 protected:
  buffMgTest() {}

  virtual ~buffMgTest() {}

  virtual void SetUp() {
    auto compiled = parserFile("ut_agse.rql") == "OK";
    assert(compiled && "Query set malformed.");
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
    coreInstance.clear();
    dataArea->qSet["serial1"]->outputPayload->reset();  // This removes artifacts
  }
};

TEST_F(buffMgTest, Only_two_items_in_query) { ASSERT_TRUE(coreInstance.size() == 2); }

TEST_F(buffMgTest, Sole_core_is_flowing) {
  auto expectedResult =
      "{ a:10 b:11 c:12 }"
      "{ a:13 b:14 c:15 }"
      "{ a:16 b:17 c:18 }"
      "{ a:19 b:20 c:21 }"
      "{ a:22 b:23 c:24 }"
      "{ a:25 b:26 c:27 }"
      "{ a:28 b:29 c:30 }"
      "{ a:10 b:11 c:12 }"
      "{ a:13 b:14 c:15 }"
      "{ a:16 b:17 c:18 }";

  std::stringstream strstream;

  std::set<std::string> rowSet = {"core"};
  for (auto i = 0; i < 10; i++) {
    dataArea->processRows(rowSet);
    strstream << rdb::flat << *(dataArea->qSet["core"]->inputPayload.get());
  }

  ASSERT_TRUE(strstream.str() == expectedResult);
}

TEST_F(buffMgTest, Core_agse_1_1_is_correctly_picked) {
  auto expectedResult =
      "{ a:10 b:11 c:12 }"
      "{ serial1_0:10 }"
      "{ a:10 b:11 c:12 }"
      "{ serial1_0:11 }"
      "{ a:10 b:11 c:12 }"
      "{ serial1_0:12 }"
      "{ a:13 b:14 c:15 }"
      "{ serial1_0:13 }";

  std::stringstream strstream;

  std::vector<std::set<std::string>> vRowSet = {{"core", "serial1"},  //
                                                {"serial1"},          //
                                                {"serial1"},          //
                                                {"core", "serial1"}};

  for (auto i : vRowSet) {
    dataArea->processRows(i);
    strstream << rdb::flat << rdb::flat << *(dataArea->qSet["core"]->inputPayload.get());
    strstream << rdb::flat << *(dataArea->qSet["serial1"]->inputPayload.get());
  }

  ASSERT_TRUE(strstream.str() == expectedResult);
}

}  // Namespace
