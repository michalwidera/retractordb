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
#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"
#include "retractor/QStruct.h"  // coreInstance
#include "retractor/dataModel.h"

// ctest -R '^ut-dataModel' -V
// ctest -R ut-dataModel

extern std::string parserFile(std::string sInputFile);

extern "C" qTree coreInstance;

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
        "file_A.dat",   //
        "file_A.desc",  //
        "file_B.dat",   //
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
    auto compiled = parserFile("ut_example_schema.rql");
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

    for (auto i : coreInstance)
      if (!i.isDeclaration()) dataArea->constructInputPayload(i.id);
  }

  virtual ~xschema() override {}

  void SetUp() override { SPDLOG_INFO("SetUp"); }

  void TearDown() override { SPDLOG_INFO("TearDown"); }
};

TEST_F(xschema, check_test0) {
  auto dataInternalDescriptor{
      rdb::Descriptor("A[1]", 4, 1, rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", 4, 1, rdb::INTEGER) |  //
      rdb::Descriptor("A[3]", 4, 1, rdb::INTEGER)    //
  };

  auto dataStorageDescriptor{
      rdb::Descriptor("A[1]", 4, 1, rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", 4, 1, rdb::INTEGER)    //
  };

  {
    streamInstance data{"file_A", "file_A.dat", dataStorageDescriptor, dataInternalDescriptor};

    data.inputPayload->setItem(0, 123);
    data.inputPayload->setItem(1, 345);

    auto v1 = data.inputPayload->getItem(0);

    data.outputPayload->getPayload()->setItem(0, 234);
    data.outputPayload->getPayload()->setItem(1, 456);
    data.outputPayload->write();

    ASSERT_TRUE(data.outputPayload->getRecordsCount() == 1);
  }

  {
    streamInstance data{"file_B", "file_B.dat", dataStorageDescriptor, dataInternalDescriptor};

    data.inputPayload->setItem(0, 123);
    data.inputPayload->setItem(1, 345);

    auto v1 = data.inputPayload->getItem(0);

    data.outputPayload->getPayload()->setItem(0, 234);
    data.outputPayload->getPayload()->setItem(1, 456);
    data.outputPayload->write();

    ASSERT_TRUE(data.outputPayload->getRecordsCount() == 1);
  }
}

TEST_F(xschema, check_test_check_constructor) {
  SPDLOG_INFO("Records in str1 {}", dataArea->qSet["str1"]->outputPayload->getRecordsCount());

  ASSERT_TRUE(dataArea->qSet["str1"]->outputPayload->getRecordsCount() == 3);

  ASSERT_TRUE(coreInstance.size() == dataArea->qSet.size());
}

TEST_F(xschema, create_struct_local_str1a) {
  SPDLOG_INFO("Create struct on LOCAL ARTIFACTS");

  auto dataDescriptor{rdb::Descriptor("A", 4, 1, rdb::INTEGER) |  //
                      rdb::Descriptor("B", 4, 1, rdb::INTEGER)};

  streamInstance q("str1a",         // storage and descriptor are the same name
                   dataDescriptor,  //
                   dataDescriptor);

  q.outputPayload->getPayload()->setItem(0, 2);
  q.outputPayload->getPayload()->setItem(1, atoi(build_id));
  q.outputPayload->write();
  q.outputPayload->setRemoveOnExit(false);

  ASSERT_TRUE(q.outputPayload->getRecordsCount() == 1);
}

TEST_F(xschema, check_construct_payload) {
  streamInstance data{coreInstance["str1"]};
  data.outputPayload->setRemoveOnExit(false);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16

  // @(4,1)  15,14,13,12
  // @(-4,1) 12,13,14,15
  {
    std::unique_ptr<rdb::payload> payload = std::make_unique<rdb::payload>(data.constructAgsePayload(4, 1));
    std::stringstream coutstring1;
    coutstring1 << rdb::flat << payload.get()->getDescriptor();
    std::stringstream coutstring2;
    coutstring2 << rdb::flat << *(payload.get());

    ASSERT_TRUE(coutstring2.str() == "{ str1_0:15 str1_1:14 str1_2:13 str1_3:12 }");
    ASSERT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str1_2 INTEGER str1_3 }");
  }
}

TEST_F(xschema, check_construct_payload_mirror) {
  auto dataDescriptor{
      rdb::Descriptor("str1_0", 4, 1, rdb::INTEGER) |  //
      rdb::Descriptor("str1_1", 1, 1, rdb::BYTE)       //
  };

  streamInstance data{coreInstance["str1"]};
  data.outputPayload->setRemoveOnExit(false);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16

  // @(4,1)  15,14,13,12
  // @(-4,1) 12,13,14,15
  {
    std::unique_ptr<rdb::payload> payload = std::make_unique<rdb::payload>(data.constructAgsePayload(-4, 1));
    std::stringstream coutstring1;
    coutstring1 << rdb::flat << payload.get()->getDescriptor();
    std::stringstream coutstring2;
    coutstring2 << rdb::flat << *(payload.get());

    ASSERT_TRUE(coutstring2.str() == "{ str1_0:12 str1_1:13 str1_2:14 str1_3:15 }");
    ASSERT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str1_2 INTEGER str1_3 }");
  }
}

TEST_F(xschema, check_sum) {
  streamInstance dataStr1{coreInstance["str1"]};
  dataStr1.outputPayload->setRemoveOnExit(false);
  dataStr1.outputPayload->read(0);

  streamInstance dataStr2{coreInstance["str2"]};
  dataStr2.outputPayload->setRemoveOnExit(false);
  dataStr2.outputPayload->read(0);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16

  // str2
  // 11
  // 22
  // 33
  // 44
  {
    auto payload = *(dataStr1.outputPayload->getPayload()) + *(dataStr2.outputPayload->getPayload());
    std::stringstream coutstring1;
    coutstring1 << rdb::flat << payload.getDescriptor();
    std::stringstream coutstring2;
    coutstring2 << rdb::flat << payload;

    ASSERT_TRUE(coutstring2.str() == "{ str1_0:11 str1_1:12 str2_0:111 }");
    ASSERT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str2_0 }");
  }
}

TEST_F(xschema, compute_instance_1) {
  // SELECT str7[0] STREAM str7 FROM core0.max

  // datafile1.txt
  // 20 31
  // 21 32
  // 22 33
  {
    auto payload = *(dataArea->qSet["str7"]->inputPayload);
    std::stringstream coutstring;
    coutstring << rdb::flat << payload;
    std::cerr << coutstring.str() << std::endl;
    ASSERT_TRUE("{ str7_0:31 }" == coutstring.str());
  }
  std::set<std::string> rowSet = {"str7"};
  dataArea->processRows(rowSet);
  {
    auto payload = *(dataArea->qSet["str7"]->inputPayload);
    std::stringstream coutstring;
    coutstring << rdb::flat << payload;
    std::cerr << coutstring.str() << std::endl;
    ASSERT_TRUE("{ str7_0:32 }" == coutstring.str());
  }
}

TEST_F(xschema, getRow_1) {
  /* datafile1.txt contents:
  20 31
  21 32
  22 33
  */
  dataArea->qSet["core0"]->outputPayload->reset();

  std::set<std::string> rowSet = {"core0"};
  dataArea->processRows(rowSet);

  auto row = dataArea->getRow("core0", 0);

  std::string res("{ ");
  for (auto &v : row) {
    std::stringstream coutstring;

    std::visit(Overload{                                                                                                    //
                        [&coutstring](uint8_t a) { coutstring << (unsigned)a; },                                            //
                        [&coutstring](int a) { coutstring << a; },                                                          //
                        [&coutstring](unsigned a) { coutstring << a; },                                                     //
                        [&coutstring](float a) { coutstring << a; },                                                        //
                        [&coutstring](double a) { coutstring << a; },                                                       //
                        [&coutstring](std::pair<int, int> a) { coutstring << a.first << "," << a.second; },                 //
                        [&coutstring](std::pair<std::string, int> a) { coutstring << a.first << "[" << a.second << "]"; },  //
                        [&coutstring](std::string a) { coutstring << a; },                                                  //
                        [&coutstring](boost::rational<int> a) { coutstring << a; }},
               v);

    coutstring << " ";
    res.append(coutstring.str());
  }
  res.append("}");

  std::cerr << res << std::endl;
  ASSERT_TRUE("{ 20 31 }" == res);

  dataArea->qSet["core0"]->outputPayload->reset();
}

TEST_F(xschema, process_rows_1) {
  // This creates 4 records in str1 and str2 - checked here & in Data/dataModel/pattern.txt

  dataArea->qSet["str1"]->outputPayload->reset();
  dataArea->qSet["str2"]->outputPayload->reset();
  dataArea->qSet["core0"]->outputPayload->reset();
  dataArea->qSet["core1"]->outputPayload->reset();

  ASSERT_TRUE(dataArea->qSet["str1"]->outputPayload->getRecordsCount() == 0);
  ASSERT_TRUE(dataArea->qSet["str2"]->outputPayload->getRecordsCount() == 0);

  std::set<std::string> rowSet = {"str1", "str2", "core0", "core1"};
  dataArea->processRows(rowSet);

  ASSERT_TRUE(dataArea->qSet["str1"]->outputPayload->getRecordsCount() == 1);
  ASSERT_TRUE(dataArea->qSet["str2"]->outputPayload->getRecordsCount() == 1);

  {
    auto payload = *(dataArea->qSet["str2"]->inputPayload);
    std::stringstream coutstring;
    coutstring << rdb::flat << payload;
    std::cerr << coutstring.str() << std::endl;
    ASSERT_TRUE("{ str2_0:20 str2_1:31 }" == coutstring.str());  // core0 == {20 31}, str2 => {20,31}
  }

  {
    auto payload = *(dataArea->qSet["str2"]->outputPayload->getPayload());
    std::stringstream coutstring;
    coutstring << rdb::flat << payload;
    std::cerr << coutstring.str() << std::endl;
    ASSERT_TRUE("{ str2_0:25 }" == coutstring.str());  // str1[0]+5 => 20 + 5 => 25
  }

  {
    auto payload = *(dataArea->qSet["str1"]->inputPayload);
    std::stringstream coutstring;
    coutstring << rdb::flat << payload;
    std::cerr << coutstring.str() << std::endl;
    ASSERT_TRUE("{ str1_0:10 str1_1:20 }" == coutstring.str());
  }

  {
    auto payload = *(dataArea->qSet["str1"]->outputPayload->getPayload());
    std::stringstream coutstring;
    coutstring << rdb::flat << payload;
    std::cerr << coutstring.str() << std::endl;
    ASSERT_TRUE("{ str1_0:10 str1_1:20 }" == coutstring.str());
  }

  dataArea->processRows(rowSet);
  dataArea->processRows(rowSet);
  dataArea->processRows(rowSet);

  ASSERT_TRUE(dataArea->qSet["str1"]->outputPayload->getRecordsCount() == 4);
  ASSERT_TRUE(dataArea->qSet["str2"]->outputPayload->getRecordsCount() == 4);

  dataArea->qSet["str1"]->outputPayload->reset();
  dataArea->qSet["str2"]->outputPayload->reset();
  dataArea->qSet["core0"]->outputPayload->reset();
  dataArea->qSet["core1"]->outputPayload->reset();
}

}  // namespace
