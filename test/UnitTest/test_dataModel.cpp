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
// ctest -R ut-dataModel

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

    dataArea->qSet["str1"]->storage->getPayload()->setItem(0, 11);
    dataArea->qSet["str1"]->storage->getPayload()->setItem(1, 12);
    dataArea->qSet["str1"]->storage->write();

    dataArea->qSet["str1"]->storage->getPayload()->setItem(0, 13);
    dataArea->qSet["str1"]->storage->getPayload()->setItem(1, 14);
    dataArea->qSet["str1"]->storage->write();

    dataArea->qSet["str1"]->storage->getPayload()->setItem(0, 15);
    dataArea->qSet["str1"]->storage->getPayload()->setItem(1, 16);
    dataArea->qSet["str1"]->storage->write();

    dataArea->qSet["str2"]->storage->getPayload()->setItem(0, 11);
    dataArea->qSet["str2"]->storage->write();

    dataArea->qSet["str2"]->storage->getPayload()->setItem(0, 22);
    dataArea->qSet["str2"]->storage->write();

    dataArea->qSet["str2"]->storage->getPayload()->setItem(0, 33);
    dataArea->qSet["str2"]->storage->write();
  }

  virtual ~xschema() override {}

  void SetUp() override { SPDLOG_INFO("SetUp"); }

  void TearDown() override { SPDLOG_INFO("TearDown"); }
};

TEST_F(xschema, check_test0) {
  auto dataInternalDescriptor{
      rdb::Descriptor("A[1]", rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", rdb::INTEGER) |  //
      rdb::Descriptor("A[3]", rdb::INTEGER)    //
  };

  auto dataStorageDescriptor{
      rdb::Descriptor("A[1]", rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", rdb::INTEGER)    //
  };

  {
    streamInstance data{"file_A", "file_A.dat", dataStorageDescriptor, dataInternalDescriptor};

    data.fromPayload->setItem(0, 123);
    data.fromPayload->setItem(1, 345);

    auto v1 = data.fromPayload->getItem(0);

    data.storage->getPayload()->setItem(0, 234);
    data.storage->getPayload()->setItem(1, 456);
    data.storage->write();

    ASSERT_TRUE(data.storage->getRecordsCount() == 1);
  }

  {
    streamInstance data{"file_B", "file_B.dat", dataStorageDescriptor, dataInternalDescriptor};

    data.fromPayload->setItem(0, 123);
    data.fromPayload->setItem(1, 345);

    auto v1 = data.fromPayload->getItem(0);

    data.storage->getPayload()->setItem(0, 234);
    data.storage->getPayload()->setItem(1, 456);
    data.storage->write();

    ASSERT_TRUE(data.storage->getRecordsCount() == 1);
  }
}

TEST_F(xschema, check_test_check_constructor) {
  SPDLOG_INFO("Records in str1 {}", dataArea->qSet["str1"]->storage->getRecordsCount());

  ASSERT_TRUE(dataArea->qSet["str1"]->storage->getRecordsCount() == 3);

  ASSERT_TRUE(coreInstance.size() == dataArea->qSet.size());
}

TEST_F(xschema, create_struct_local_str1a) {
  SPDLOG_INFO("Create struct on LOCAL ARTIFACTS");

  auto dataDescriptor{rdb::Descriptor("A", rdb::INTEGER) |  //
                      rdb::Descriptor("B", rdb::INTEGER)};

  streamInstance q("str1a",         // storage and descriptor are the same name
                   dataDescriptor,  //
                   dataDescriptor);

  q.storage->getPayload()->setItem(0, 2);
  q.storage->getPayload()->setItem(1, atoi(build_id));
  q.storage->write();
  q.storage->setRemoveOnExit(false);

  ASSERT_TRUE(q.storage->getRecordsCount() == 1);
}

TEST_F(xschema, check_construct_payload) {
  auto dataDescriptor{
      rdb::Descriptor("str1_0", rdb::INTEGER) |  //
      rdb::Descriptor("str1_1", rdb::BYTE)       //
  };

  streamInstance data{"str1", dataDescriptor, dataDescriptor};
  data.storage->setRemoveOnExit(false);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16

  // @(4,1)  15,14,13,12
  // @(-4,1) 12,13,14,15
  {
    std::unique_ptr<rdb::payload> payload = std::make_unique<rdb::payload>(data.constructAgsePayload(4, 1));
    std::string expectedOutDesc = "{ INTEGER str1_1 INTEGER str1_2 INTEGER str1_3 INTEGER str1_4 }";
    std::string expectedOutData = "{ str1_1:15 str1_2:14 str1_3:13 str1_4:12 }";
    std::stringstream coutstring1;
    coutstring1 << rdb::flat << payload.get()->getDescriptor();
    std::stringstream coutstring2;
    coutstring2 << rdb::flat << *(payload.get());

    // payload->specialDebug = true;
    // std::cerr << "t " << rdb::flat << payload.get()->getDescriptor() << std::endl;
    // std::cerr << "t " << rdb::flat << *(payload.get()) << std::endl;

    ASSERT_TRUE(expectedOutData == coutstring2.str());
    ASSERT_TRUE(expectedOutDesc == coutstring1.str());
  }
}

TEST_F(xschema, check_construct_payload_mirror) {
  auto dataDescriptor{
      rdb::Descriptor("str1_0", rdb::INTEGER) |  //
      rdb::Descriptor("str1_1", rdb::BYTE)       //
  };

  streamInstance data{"str1", dataDescriptor, dataDescriptor};
  data.storage->setRemoveOnExit(false);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16

  // @(4,1)  15,14,13,12
  // @(-4,1) 12,13,14,15
  {
    std::unique_ptr<rdb::payload> payload = std::make_unique<rdb::payload>(data.constructAgsePayload(-4, 1));
    std::string expectedOutDesc = "{ INTEGER str1_1 INTEGER str1_2 INTEGER str1_3 INTEGER str1_4 }";
    std::string expectedOutData = "{ str1_1:12 str1_2:13 str1_3:14 str1_4:15 }";
    std::stringstream coutstring1;
    coutstring1 << rdb::flat << payload.get()->getDescriptor();
    std::stringstream coutstring2;
    coutstring2 << rdb::flat << *(payload.get());

    ASSERT_TRUE(expectedOutData == coutstring2.str());
    ASSERT_TRUE(expectedOutDesc == coutstring1.str());
  }
}

TEST_F(xschema, check_sum) {
  auto dataDescriptorStr1{
      rdb::Descriptor("str1_0", rdb::INTEGER) |  //
      rdb::Descriptor("str1_1", rdb::BYTE)       //
  };

  auto dataDescriptorStr2Storage{
      rdb::Descriptor("str2_0", rdb::INTEGER)
  };
  auto dataDescriptorStr2Internal{
      rdb::Descriptor("str2_0", rdb::INTEGER) |  //
      rdb::Descriptor("str2_1", rdb::BYTE)       //
  };

  streamInstance dataStr1{"str1", dataDescriptorStr1, dataDescriptorStr1};
  dataStr1.storage->setRemoveOnExit(false);

  // Tutaj trzeba sprawdzic co tu jest storage a co internal - bo chyba doszlo do zamiany
  streamInstance dataStr2{"str2", dataDescriptorStr2Internal, dataDescriptorStr2Storage }; 
  dataStr2.storage->setRemoveOnExit(false);

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
    auto payload = *(dataStr1.storage->getPayload()) + *(dataStr2.storage->getPayload());
    std::string expectedOutDesc = "{ INTEGER str1_0 BYTE str1_1 INTEGER str2_0 BYTE str2_1 }";
    std::string expectedOutData = "{ str1_0:0 str1_1:0 str2_0:0 str2_1:0 }";
    std::stringstream coutstring1;
    coutstring1 << rdb::flat << payload.getDescriptor();
    std::stringstream coutstring2;
    coutstring2 << rdb::flat << payload;

    ASSERT_TRUE(expectedOutData == coutstring2.str());
    ASSERT_TRUE(expectedOutDesc == coutstring1.str());
  }
}

}  // namespace
