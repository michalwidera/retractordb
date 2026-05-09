#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

#include "config.h"
#include "rdb/fainterface.hpp"
#include "rdb/payload.hpp"
#include "rdb/storageacc.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/qTree.hpp"  // coreInstance

// ctest -R '^ut-test_dataModel' -V

extern std::string parserRQLFile_4Test(qTree &coreInstance, std::string sInputFile);
extern dataModel *pProc;

qTree coreInstance;

/*
file path: test/UnitTest/Data/dataModel/ut_example_schema.rql

DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE 'datafile1.txt'
DECLARE c INTEGER,d INTEGER STREAM core1, 0.5 FILE 'datafile2.txt'
SELECT str1[0],str1[1] STREAM str1 FROM core0#core1
SELECT str2[0]+5 STREAM str2 FROM core0
SELECT str3[0] STREAM str3 FROM core0+core1
SELECT str4[0] STREAM str4 FROM core0%2
SELECT str5[0] STREAM str5 FROM core0>1
SELECT str6[0] STREAM str6 FROM core0-1/2
SELECT str7[0] STREAM str7 FROM core0.max
*/
namespace {

std::unique_ptr<dataModel> dataArea;

class xschema : public ::testing::Test {
 protected:
  xschema() {
    SPDLOG_INFO("Constructor");
    std::vector<std::string> cleanFilesSet = {
        "core0.desc",   //
        "core1.desc",   //
        "str1",         //
        "str1.desc",    //
        "str2",         //
        "str2.desc",    //
        "str1a",        //
        "str1a.desc",   //
        "file_A",       //
        "file_A.desc",  //
        "file_B",       //
        "file_B.desc"   //
    };

    for (auto i : cleanFilesSet)
      if (std::filesystem::exists(i)) {
        std::filesystem::remove(i);
        SPDLOG_INFO("Drop file {}", i);
      } else
        SPDLOG_WARN("Not found {}", i);

    // This simplified dataModel::load
    coreInstance.clear();
    auto compiled = parserRQLFile_4Test(coreInstance, "ut_example_schema.rql");
    dataArea      = std::make_unique<dataModel>(coreInstance);

    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(0, 11);
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(1, 12);
    dataArea->qSet["str1"]->outputPayload->write();

    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(0, 13);
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(1, 14);
    dataArea->qSet["str1"]->outputPayload->write();

    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(0, 15);
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(1, 16);
    dataArea->qSet["str1"]->outputPayload->write();

    dataArea->qSet["str2"]->outputPayload->getPayload()->setItem(0, 111);
    dataArea->qSet["str2"]->outputPayload->write();

    dataArea->qSet["str2"]->outputPayload->getPayload()->setItem(0, 222);
    dataArea->qSet["str2"]->outputPayload->write();

    dataArea->qSet["str2"]->outputPayload->getPayload()->setItem(0, 333);
    dataArea->qSet["str2"]->outputPayload->write();

    for (auto i : coreInstance)
      if (!i.isDeclaration()) dataArea->constructInputPayload(i.id);

    pProc = dataArea.get();
  }

  virtual ~xschema() override { pProc = nullptr; }

  void SetUp() override { SPDLOG_INFO("SetUp"); }

  void TearDown() override { SPDLOG_INFO("TearDown"); }
};

TEST_F(xschema, check_construct_payload) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16

  // @(4,1)  15,14,13,12
  // @(-4,1) 12,13,14,15
  {
    std::unique_ptr<rdb::payload> payload = std::make_unique<rdb::payload>(data.constructAgsePayload(4, 1, "str1", 2));
    std::stringstream coutstring1;
    coutstring1 << rdb::singleLineFormat << payload.get()->descriptor;
    std::stringstream coutstring2;
    coutstring2 << rdb::singleLineFormat << *(payload.get());
    std::cerr << rdb::singleLineFormat << *(payload.get()) << std::endl;

    EXPECT_TRUE(coutstring2.str() == "{ str1_0:11 str1_1:null str1_2:null str1_3:null }");
    EXPECT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str1_2 INTEGER str1_3 }");
  }
}

TEST_F(xschema, check_construct_payload_mirror) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16

  // @(4,1)  15,14,13,12
  // @(-4,1) 12,13,14,15
  {
    std::unique_ptr<rdb::payload> payload = std::make_unique<rdb::payload>(data.constructAgsePayload(-4, 1, "str1", 2));
    std::stringstream coutstring1;
    coutstring1 << rdb::singleLineFormat << payload.get()->descriptor;

    std::stringstream coutstring2;
    coutstring2 << rdb::singleLineFormat << *(payload.get());
    std::cout << rdb::singleLineFormat << *(payload.get()) << std::endl;

    EXPECT_TRUE(coutstring2.str() == "{ str1_0:null str1_1:null str1_2:null str1_3:11 }");
    EXPECT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str1_2 INTEGER str1_3 }");
  }
}

TEST_F(xschema, check_sum) {
  streamInstance dataStr1{coreInstance, coreInstance["str1"]};
  dataStr1.outputPayload->setDisposable(false);
  dataStr1.outputPayload->revRead(0);

  streamInstance dataStr2{coreInstance, coreInstance["str2"]};
  dataStr2.outputPayload->setDisposable(false);
  dataStr2.outputPayload->revRead(0);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16 <-

  // str2
  // 11
  // 22
  // 33 <-
  // 44
  {
    auto payload = *(dataStr1.outputPayload->getPayload()) + *(dataStr2.outputPayload->getPayload());

    std::stringstream coutstring1;
    coutstring1 << rdb::singleLineFormat << payload.descriptor;
    std::cout << coutstring1.str() << std::endl;

    std::stringstream coutstring2;
    coutstring2 << rdb::singleLineFormat << payload;
    std::cout << "!" << coutstring2.str() << std::endl;

    EXPECT_TRUE(coutstring2.str() == "{ str1_0:15 str1_1:16 str2_0:333 }");
    EXPECT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str2_0 }");
  }
}

auto print(const std::vector<rdb::descFldVT> &row) {
  std::string res("{ ");
  for (auto &v : row) {
    std::stringstream coutstring;

    std::visit(Overload{                                                                                                    //
                        [&coutstring](std::monostate) { coutstring << "null"; },                                            //
                        [&coutstring](uint8_t a) { coutstring << (unsigned)a; },                                            //
                        [&coutstring](int a) { coutstring << a; },                                                          //
                        [&coutstring](unsigned a) { coutstring << a; },                                                     //
                        [&coutstring](float a) { coutstring << a; },                                                        //
                        [&coutstring](double a) { coutstring << a; },                                                       //
                        [&coutstring](std::pair<int, int> a) { coutstring << a.first << "," << a.second; },                 //
                        [&coutstring](std::pair<std::string, int> a) { coutstring << a.first << "[" << a.second << "]"; },  //
                        [&coutstring](const std::string &a) { coutstring << a; },                                           //
                        [&coutstring](boost::rational<int> a) { coutstring << a; }},
               v);

    coutstring << " ";
    res.append(coutstring.str());
  }
  res.append("}");
  return res;
}

TEST_F(xschema, getRow_1) {
  /* datafile1.txt contents:
  20 31
  21 32
  22 33
  */
  dataArea->qSet["core0"]->outputPayload->resetForUnitTest();

  std::set<std::string> rowSet = {"core0"};
  dataArea->processZeroStep();
  auto row1 = dataArea->getRow("core0", 0);
  dataArea->processRows(rowSet);
  auto row2 = dataArea->getRow("core0", 1);

  std::string res1 = print(row1);
  std::string res2 = print(row2);

  EXPECT_TRUE("{ 20 31 }" == res1);
  EXPECT_TRUE("{ 21 32 }" == res2);

  dataArea->qSet["core0"]->outputPayload->resetForUnitTest();
}
TEST_F(xschema, constructAggregate_max) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → MAX = 16
  auto result = data.constructAggregate(STREAM_MAX, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:16 }");
}

TEST_F(xschema, constructAggregate_min) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → MIN = 15
  auto result = data.constructAggregate(STREAM_MIN, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:15 }");
}

TEST_F(xschema, constructAggregate_sum) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → SUM = 31
  auto result = data.constructAggregate(STREAM_SUM, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:31 }");
}

TEST_F(xschema, constructAggregate_avg) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → AVG = 31/2 = 15 (obcięcie do int)
  auto result = data.constructAggregate(STREAM_AVG, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:15 }");
}

TEST_F(xschema, constructOutputPayload_expression) {
  // str2: SELECT str2[0]+5 FROM core0 → core0.a=20, result=25
  dataArea->processZeroStep();
  dataArea->constructInputPayload("str2");
  dataArea->qSet["str2"]->constructOutputPayload(coreInstance["str2"].lSchema);
  std::stringstream ss;
  ss << rdb::singleLineFormat << *(dataArea->qSet["str2"]->outputPayload->getPayload());
  EXPECT_EQ(ss.str(), "{ str2_0:25 }");
}

TEST_F(xschema, constructRulesAndUpdate_empty_rules) {
  // str2 has no RULE declarations → constructRulesAndUpdate is a no-op (no crash)
  dataArea->qSet["str2"]->constructRulesAndUpdate(coreInstance["str2"]);
  SUCCEED();
}

TEST_F(xschema, constructAggregate_single_field) {
  streamInstance data{coreInstance, coreInstance["str2"]};
  data.outputPayload->setDisposable(false);
  // str2 last record: {333} → single-field aggregate
  auto result = data.constructAggregate(STREAM_MAX, "str2");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str2:333 }");
}

std::unique_ptr<dataModel> dataArea_rules;

class xschema_rules : public ::testing::Test {
 protected:
  xschema_rules() {
    for (auto f : {"rule_marker1.txt", "rule_marker2.txt", "str_rule", "str_rule.desc", "core0.desc"})
      if (std::filesystem::exists(f)) std::filesystem::remove(f);

    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_rules_schema.rql");
    dataArea_rules = std::make_unique<dataModel>(coreInstance);
    pProc = dataArea_rules.get();
  }
  ~xschema_rules() override { pProc = nullptr; }
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(xschema_rules, constructRulesAndUpdate_system_rule_fires) {
  // core0 first row: a=20 → str_rule[0]=20 > 0 → rule1 fires, rule2 does not
  dataArea_rules->processZeroStep();
  dataArea_rules->processRows({"str_rule"});
  EXPECT_TRUE(std::filesystem::exists("rule_marker1.txt")) << "rule1 (>0) should fire for positive data";
  EXPECT_FALSE(std::filesystem::exists("rule_marker2.txt")) << "rule2 (<0) should not fire for positive data";
}

}  // namespace
