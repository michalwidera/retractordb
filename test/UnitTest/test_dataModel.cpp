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
#include "rdb/storage.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/qTree.hpp"  // coreInstance

// ctest -R '^ut-test_dataModel' -V

extern std::string parserRQLFile_4Test(qTree &coreInstance, const std::string &sInputFile);
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
    parserRQLFile_4Test(coreInstance, "ut_example_schema.rql");
    dataArea = std::make_unique<dataModel>(coreInstance);

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

    for (const auto &i : coreInstance)
      if (!i.isDeclaration()) dataArea->constructInputPayload(i.id);

    pProc = dataArea.get();
  }

  ~xschema() override { pProc = nullptr; }

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
    coutstring1 << rdb::singleLineFormat << payload->descriptor;
    std::stringstream coutstring2;
    coutstring2 << rdb::singleLineFormat << *payload;
    std::cerr << rdb::singleLineFormat << *(payload) << '\n';

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
    coutstring1 << rdb::singleLineFormat << payload->descriptor;

    std::stringstream coutstring2;
    coutstring2 << rdb::singleLineFormat << *payload;
    std::cout << rdb::singleLineFormat << *payload << '\n';

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
    std::cout << coutstring1.str() << '\n';

    std::stringstream coutstring2;
    coutstring2 << rdb::singleLineFormat << payload;
    std::cout << "!" << coutstring2.str() << '\n';

    EXPECT_TRUE(coutstring2.str() == "{ str1_0:15 str1_1:16 str2_0:333 }");
    EXPECT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str2_0 }");
  }
}

auto print(const std::vector<rdb::descFldVT> &row) {
  std::string res("{ ");
  for (const auto &v : row) {
    std::stringstream coutstring;

    std::visit(
        Overload{                                                                                                           //
                 [&coutstring](std::monostate) { coutstring << "null"; },                                                   //
                 [&coutstring](uint8_t a) { coutstring << (unsigned)a; },                                                   //
                 [&coutstring](int a) { coutstring << a; },                                                                 //
                 [&coutstring](unsigned a) { coutstring << a; },                                                            //
                 [&coutstring](float a) { coutstring << a; },                                                               //
                 [&coutstring](double a) { coutstring << a; },                                                              //
                 [&coutstring](const std::pair<int, int> &a) { coutstring << a.first << "," << a.second; },                 //
                 [&coutstring](const std::pair<std::string, int> &a) { coutstring << a.first << "[" << a.second << "]"; },  //
                 [&coutstring](const std::string &a) { coutstring << a; },                                                  //
                 [&coutstring](const boost::rational<int> &a) { coutstring << a; }},
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
TEST_F(xschema, reduceFieldsToPayload_max) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → MAX = 16
  auto result = data.reduceFieldsToPayload(STREAM_MAX, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:16 }");
}

TEST_F(xschema, reduceFieldsToPayload_min) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → MIN = 15
  auto result = data.reduceFieldsToPayload(STREAM_MIN, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:15 }");
}

TEST_F(xschema, reduceFieldsToPayload_sum) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → SUM = 31
  auto result = data.reduceFieldsToPayload(STREAM_SUM, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:31 }");
}

TEST_F(xschema, reduceFieldsToPayload_avg) {
  streamInstance data{coreInstance, coreInstance["str1"]};
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → AVG = 31/2 = 15 (obcięcie do int)
  auto result = data.reduceFieldsToPayload(STREAM_AVG, "str1");
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

TEST_F(xschema, reduceFieldsToPayload_single_field) {
  streamInstance data{coreInstance, coreInstance["str2"]};
  data.outputPayload->setDisposable(false);
  // str2 last record: {333} → single-field aggregate
  auto result = data.reduceFieldsToPayload(STREAM_MAX, "str2");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str2:333 }");
}

std::unique_ptr<dataModel> dataArea_rules;

class xschema_rules : public ::testing::Test {
 protected:
  xschema_rules() {
    for (const auto *f :
         {"rule_marker1.txt", "rule_marker2.txt", "str_rule", "str_rule.desc", "rules_core0.desc", "datafile1.txt.desc"})
      if (std::filesystem::exists(f)) std::filesystem::remove(f);

    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_rules_schema.rql");
    dataArea_rules = std::make_unique<dataModel>(coreInstance);
    pProc          = dataArea_rules.get();
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

std::unique_ptr<dataModel> dataArea_null;

class xschema_all_null : public ::testing::Test {
 protected:
  xschema_all_null() {
    // Destroy first so its metaData flushes before files are removed
    dataArea_null.reset();
    for (const auto *f : {"core0.desc", "core1.desc", "str1", "str1.meta", "str1.desc", "str2", "str2.desc"})
      if (std::filesystem::exists(f)) std::filesystem::remove(f);
    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_example_schema.rql");
    dataArea_null = std::make_unique<dataModel>(coreInstance);
    dataArea_null->qSet["str1"]->outputPayload->getPayload()->setItem(0, std::nullopt);
    dataArea_null->qSet["str1"]->outputPayload->getPayload()->setItem(1, std::nullopt);
    dataArea_null->qSet["str1"]->outputPayload->write();
    pProc = dataArea_null.get();
  }
  ~xschema_all_null() override { pProc = nullptr; }
  void SetUp() override {}
  void TearDown() override {}
};

// Logika trójwartościowa: agregacja na samych NULL-ach → wynik NULL
// Null bity są w currentEntry_ metaData tej samej instancji storage,
// więc revRead(0) odczyta je poprawnie bez potrzeby flushu na dysk.
TEST_F(xschema_all_null, reduceFieldsToPayload_all_null_sum) {
  auto result = dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_SUM, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

TEST_F(xschema_all_null, reduceFieldsToPayload_all_null_min) {
  auto result = dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_MIN, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

TEST_F(xschema_all_null, reduceFieldsToPayload_all_null_max) {
  auto result = dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_MAX, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

TEST_F(xschema_all_null, reduceFieldsToPayload_all_null_avg) {
  auto result = dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_AVG, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

class xschema_partial_null : public ::testing::Test {
 protected:
  xschema_partial_null() {
    dataArea_null.reset();
    for (const auto *f : {"core0.desc", "core1.desc", "str1", "str1.meta", "str1.desc", "str2", "str2.desc"})
      if (std::filesystem::exists(f)) std::filesystem::remove(f);
    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_example_schema.rql");
    dataArea_null = std::make_unique<dataModel>(coreInstance);
    // pole 0 = NULL, pole 1 = 10 (nie-NULL)
    dataArea_null->qSet["str1"]->outputPayload->getPayload()->setItem(0, std::nullopt);
    dataArea_null->qSet["str1"]->outputPayload->getPayload()->setItem(1, 10);
    dataArea_null->qSet["str1"]->outputPayload->write();
    pProc = dataArea_null.get();
  }
  ~xschema_partial_null() override { pProc = nullptr; }
  void SetUp() override {}
  void TearDown() override {}
};

// Logika trójwartościowa: NULL ignorowany, agregacja tylko na wartościach niezerowych
TEST_F(xschema_partial_null, reduceFieldsToPayload_partial_null_sum) {
  auto result = dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_SUM, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10 }");
}

TEST_F(xschema_partial_null, reduceFieldsToPayload_partial_null_avg) {
  // AVG: tylko 1 pole niezerowe (10), mianownik = 1, wynik = 10
  auto result = dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_AVG, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10 }");
}

TEST_F(xschema_partial_null, reduceFieldsToPayload_partial_null_min) {
  auto result = dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_MIN, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10 }");
}

TEST_F(xschema_partial_null, reduceFieldsToPayload_partial_null_max) {
  auto result = dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_MAX, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10 }");
}

// Regression: null bits must survive flush to disk so a second storage reader
// sees them correctly. Before fix: second reader saw totalRecords()==0 →
// all-non-null fallback → SUM(null,null)=0 instead of null.
TEST_F(xschema_all_null, null_bits_flushed_to_disk_second_reader_sum) {
  streamInstance second{coreInstance, coreInstance["str1"]};
  second.outputPayload->setDisposable(false);
  auto result = second.reduceFieldsToPayload(STREAM_SUM, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

TEST_F(xschema_partial_null, null_bits_flushed_to_disk_second_reader_ignores_null) {
  streamInstance second{coreInstance, coreInstance["str1"]};
  second.outputPayload->setDisposable(false);
  auto result = second.reduceFieldsToPayload(STREAM_SUM, "str1");
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10 }");
}

// Restore fixture: the null-test fixtures above delete and recreate str1/str2,
// leaving them with fewer records than pattern.txt expects.  This fixture runs
// last and writes back the canonical 3-record state that ut-dataModel-compare
// compares against.
class xschema_compare_restore : public ::testing::Test {
 protected:
  xschema_compare_restore() {
    dataArea_null.reset();
    for (const auto *f : {"str1", "str1.meta", "str1.desc", "str2", "str2.desc"})
      if (std::filesystem::exists(f)) std::filesystem::remove(f);
    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_example_schema.rql");
    dataArea = std::make_unique<dataModel>(coreInstance);

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

    pProc = dataArea.get();
  }
  ~xschema_compare_restore() override { pProc = nullptr; }
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(xschema_compare_restore, state_restored_for_compare_test) { SUCCEED(); }

}  // namespace
