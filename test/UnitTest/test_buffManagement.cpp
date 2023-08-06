#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <filesystem>

#include "config.h"
#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"
#include "retractor/QStruct.h"  // coreInstance
#include "retractor/compiler.hpp"
#include "retractor/dataModel.h"

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

  virtual void TearDown() { coreInstance.clear(); }
};

TEST_F(buffMgTest, Only_two_items_in_query) { ASSERT_TRUE(coreInstance.size() == 2); }

TEST_F(buffMgTest, CanDoBaz) {
  std::set<std::string> rowSet1 = {"core", "serial1"};
  dataArea->processRows(rowSet1);
  std::cout << rdb::flat << *(dataArea->qSet["core"]->inputPayload.get()) << std::endl;
  std::cout << rdb::flat << *(dataArea->qSet["serial1"]->inputPayload.get()) << std::endl;

  std::set<std::string> rowSet2 = {"serial1"};
  dataArea->processRows(rowSet2);
  std::cout << rdb::flat << *(dataArea->qSet["core"]->inputPayload.get()) << std::endl;
  std::cout << rdb::flat << *(dataArea->qSet["serial1"]->inputPayload.get()) << std::endl;

  std::set<std::string> rowSet3 = {"serial1"};
  dataArea->processRows(rowSet3);
  std::cout << rdb::flat << *(dataArea->qSet["core"]->inputPayload.get()) << std::endl;
  std::cout << rdb::flat << *(dataArea->qSet["serial1"]->inputPayload.get()) << std::endl;

  std::set<std::string> rowSet4 = {"serial1"};
  dataArea->processRows(rowSet4);
  std::cout << rdb::flat << *(dataArea->qSet["core"]->inputPayload.get()) << std::endl;
  std::cout << rdb::flat << *(dataArea->qSet["serial1"]->inputPayload.get()) << std::endl;
}

}  // Namespace
