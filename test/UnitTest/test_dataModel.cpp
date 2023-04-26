#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

#include "QStruct.h"  // coreInstance
#include "config.h"
#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"
#include "retractor/dataModel.h"

// ctest -R '^ut-dataModel' -V

extern std::string parserFile(std::string sInputFile);

extern "C" qTree coreInstance;

/*
file path: test/UnitTest/Data/dataModel/ut_example_schema.rql

DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE '/dev/urandom'
DECLARE c INTEGER,d BYTE STREAM core1, 0.5 FILE '/dev/urandom'
SELECT str1[0],str1[1] STREAM str1 FROM core0#core1
SELECT str1[0]+5 STREAM str2 FROM core0
*/
namespace {

std::unique_ptr<dataModel> dataArea;

class xschema : public ::testing::Test {
 protected:
  xschema() {}

  virtual ~xschema() override {}

  void SetUp() override { SPDLOG_INFO("SetUp"); }

  void TearDown() override { SPDLOG_INFO("TearDown"); }
};

// ctest -R ut-dataModel

TEST_F(xschema, check_test0) {
  std::vector<std::string> cleanFilesSet = {
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

  auto dataInternalDesciptor{
      rdb::Descriptor("A[1]", rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", rdb::INTEGER) |  //
      rdb::Descriptor("A[3]", rdb::INTEGER)    //
  };

  auto dataStorageDescriptor{
      rdb::Descriptor("A[1]", rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", rdb::INTEGER)    //
  };

  {
    streamInstance data{"file_A", "file_A.dat", dataStorageDescriptor, dataInternalDesciptor};

    data.fromPayload->setItem(0, 123);
    data.fromPayload->setItem(1, 345);

    auto v1 = data.fromPayload->getItem(0);

    data.storage->getPayload()->setItem(0, 234);
    data.storage->getPayload()->setItem(1, 456);
    data.storage->write();

    ASSERT_TRUE(data.storage->getRecordsCount() == 1);
  }

  {
    streamInstance data{"file_B", "file_B.dat", dataStorageDescriptor, dataInternalDesciptor};

    data.fromPayload->setItem(0, 123);
    data.fromPayload->setItem(1, 345);

    auto v1 = data.fromPayload->getItem(0);

    data.storage->getPayload()->setItem(0, 234);
    data.storage->getPayload()->setItem(1, 456);
    data.storage->write();

    ASSERT_TRUE(data.storage->getRecordsCount() == 1);
  }
}

TEST_F(xschema, check_test1) {
  std::vector<std::string> cleanFilesSet = {"core0.desc",  //
                                            "core1.desc",  //
                                            "str1",        //
                                            "str1.desc",   //
                                            "str2",        //
                                            "str2.desc",   //
                                            "str1a",       //
                                            "str1a.desc"};

  for (auto i : cleanFilesSet)
    if (std::filesystem::exists(i)) {
      std::filesystem::remove(i);
      SPDLOG_INFO("Drop file {}", i);
    } else
      SPDLOG_WARN("Not found {}", i);

  // This simplified dataModel::load
  coreInstance.clear();
  auto compiled = parserFile("ut_example_schema.rql");
  ASSERT_TRUE(compiled == "OK");
  dataArea = std::make_unique<dataModel>(coreInstance);

  // Todo
  dataArea->qSet["str1"]->storage->getPayload()->setItem(0, 2);
  dataArea->qSet["str1"]->storage->getPayload()->setItem(1, static_cast<uint8_t>(3));
  dataArea->qSet["str1"]->storage->write();

  dataArea->qSet["str1"]->storage->getPayload()->setItem(0, 3);
  dataArea->qSet["str1"]->storage->getPayload()->setItem(1, static_cast<uint8_t>(4));
  dataArea->qSet["str1"]->storage->write();

  dataArea->qSet["str1"]->storage->getPayload()->setItem(0, 4);
  dataArea->qSet["str1"]->storage->getPayload()->setItem(1, static_cast<uint8_t>(5));
  dataArea->qSet["str1"]->storage->write();

  SPDLOG_INFO("Records in str1 {}", dataArea->qSet["str1"]->storage->getRecordsCount());

  ASSERT_TRUE(dataArea->qSet["str1"]->storage->getRecordsCount() == 3);

  ASSERT_TRUE(coreInstance.size() == dataArea->qSet.size());
}

TEST_F(xschema, create_struct_local) {
  SPDLOG_INFO("Create struct on LOCAL ARTIFACTS");

  auto dataStorageAndInternalDesciptor{rdb::Descriptor("A", rdb::INTEGER) |  //
                                       rdb::Descriptor("B", rdb::INTEGER)};

  streamInstance q("str1a",                          // storage and descriptor are the same name
                   dataStorageAndInternalDesciptor,  //
                   dataStorageAndInternalDesciptor);

  q.storage->getPayload()->setItem(0, 2);
  q.storage->getPayload()->setItem(1, atoi(build_id));
  q.storage->write();
  q.storage->setRemoveOnExit(false);

  std::cerr << q.storage->getRecordsCount() << std::endl;

  ASSERT_TRUE(q.storage->getRecordsCount() == 1);
}

TEST_F(xschema, check_construct_payload) {
  auto dataInternalDesciptor{
      rdb::Descriptor("str1_0", rdb::INTEGER) |  //
      rdb::Descriptor("str1_1", rdb::BYTE)       //
  };

  auto dataStorageDescriptor{
      rdb::Descriptor("str1_0", rdb::INTEGER) |  //
      rdb::Descriptor("str1_1", rdb::BYTE)       //
  };

  streamInstance data{"str1", dataStorageDescriptor, dataInternalDesciptor};
  data.storage->setRemoveOnExit(false);

  {
    std::unique_ptr<rdb::payload> payload = std::make_unique<rdb::payload>(data.constructAgsePayload(5, 1));
    std::string expectedOutDesc = "{ BYTE str1_1 INTEGER str1_2 BYTE str1_3 INTEGER str1_4 BYTE str1_5 }";
    std::string expectedOutData = "{ str1_1:0 str1_2:3 str1_3:4 str1_4:2 str1_5:3 }";
    std::stringstream coutstring1;
    coutstring1 << rdb::flat << payload.get()->getDescriptor();
    std::stringstream coutstring2;
    coutstring2 << rdb::flat << *(payload.get());
    // std::cerr << rdb::flat << payload.get()->getDescriptor();
    // std::cerr << rdb::flat << *(payload.get()) ;

    ASSERT_TRUE(expectedOutData == coutstring2.str());
    ASSERT_TRUE(expectedOutDesc == coutstring1.str());
  }

  {
    std::unique_ptr<rdb::payload> payload = std::make_unique<rdb::payload>(data.constructAgsePayload(-5, 1));
    std::string expectedOutDesc = "{ BYTE str1_1 INTEGER str1_2 BYTE str1_3 INTEGER str1_4 BYTE str1_5 }";
    std::string expectedOutData = "{ str1_1:3 str1_2:2 str1_3:4 str1_4:3 str1_5:5 }";
    std::stringstream coutstring1;
    coutstring1 << rdb::flat << payload.get()->getDescriptor();
    std::stringstream coutstring2;
    coutstring2 << rdb::flat << *(payload.get());
    // std::cerr << rdb::flat << payload.get()->getDescriptor();
    // std::cerr << rdb::flat << *(payload.get()) ;

    ASSERT_TRUE(expectedOutData == coutstring2.str());
    ASSERT_TRUE(expectedOutDesc == coutstring1.str());
  }
}

}  // namespace
