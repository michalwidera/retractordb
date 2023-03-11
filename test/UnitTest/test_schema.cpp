#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <locale>
#include <string>

#include "QStruct.h"  // coreInstance
#include "config.h"
#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"
#include "retractor/dataModel.h"

extern std::string parser(std::string sInputFile);

extern "C" qTree coreInstance;

/*
DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE '/dev/urandom'
DECLARE c INTEGER,d BYTE STREAM core1, 0.5 FILE '/dev/urandom'
SELECT str1[0],str1[1] STREAM str1 FROM core0#core1
SELECT str1[0]+5 STREAM str2 FROM core0
*/
namespace {

class xschema : public ::testing::Test {
 protected:
  xschema() {}

  virtual ~xschema() override {}

  void SetUp() override { SPDLOG_INFO("SetUp"); }

  void TearDown() override { SPDLOG_INFO("TearDown"); }
};

// ctest -R unittest-test-schema
TEST_F(xschema, check_test0) {
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
    dataInstance data{"file_A", "file_A.dat", dataStorageDescriptor, dataInternalDesciptor};

    data.internalPayload->setItem(0, 123);
    data.internalPayload->setItem(1, 345);

    auto v1 = data.internalPayload->getItem(0);

    data.storagePayload->setItem(0, 234);
    data.storagePayload->setItem(1, 456);
    data.storage->write();

    ASSERT_TRUE(data.storage->getRecordsCount() == 1);
  }

  {
    dataInstance data{"file_B", "file_B.dat", dataStorageDescriptor, dataInternalDesciptor};

    data.internalPayload->setItem(0, 123);
    data.internalPayload->setItem(1, 345);

    auto v1 = data.internalPayload->getItem(0);

    data.storagePayload->setItem(0, 234);
    data.storagePayload->setItem(1, 456);
    data.storage->write();

    ASSERT_TRUE(data.storage->getRecordsCount() == 1);
  }
}

TEST_F(xschema, check_test1) {
  /*
  query qry;
  qry.id = "file_1";
  std::list<token> lTempProgram;
  lTempProgram.push_back(token(PUSH_TSCAN));
  qry.lSchema.push_back(field("*", lTempProgram, rdb::BAD, "*"));
  coreInstance.push_back(qry);
  */
  std::vector<std::string> cleanFilesSet = {"core0.desc",  //
                                            "core1.desc",  //
                                            "str1",        //
                                            "str1.desc",   //
                                            "str2",        //
                                            "str2.desc"};

  for (auto i : cleanFilesSet)
    if (std::filesystem::exists(i)) {
      std::filesystem::remove(i);
      SPDLOG_INFO("Drop file {}", i);
    } else
      SPDLOG_WARN("Not found {}", i);

  // This simplified dataModel::load
  coreInstance.clear();
  auto compiled = parser("ut_example_schema.rql") == "OK";
  ASSERT_TRUE(compiled == true);

  dataModel dataArea ;

  dataArea.prepare();

  // Todo
  dataArea.qSet["str1"]->storagePayload->setItem(0, 2);
  dataArea.qSet["str1"]->storagePayload->setItem(1, 3);
  dataArea.qSet["str1"]->storage->write();

  SPDLOG_INFO("Records in str1 {}", dataArea.qSet["str1"]->storage->getRecordsCount());

  SPDLOG_INFO("Create struct on LOCAL ARTIFACTS");

  auto dataStorageAndInternalDesciptor{rdb::Descriptor("A", rdb::INTEGER) |  //
                                       rdb::Descriptor("B", rdb::INTEGER)};

  dataInstance q("str1a",                          // storage and descriptor are the same name
                 dataStorageAndInternalDesciptor,  //
                 dataStorageAndInternalDesciptor);

  q.storagePayload->setItem(0, 2);
  q.storagePayload->setItem(1, atoi(build_id));
  q.storage->write();
  q.storage->setRemoveOnExit(false);

  ASSERT_TRUE(coreInstance.size() == dataArea.qSet.size());
};
}  // namespace
