#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <locale>
#include <string>

#include "QStruct.h"  // coreInstance
#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"
#include "retractor/dataModel.h"
#include "spdlog/spdlog.h"

extern std::string parser(std::string sInputFile);

extern "C" qTree coreInstance;

/*
DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE '/dev/urandom'
DECLARE c INTEGER,d BYTE STREAM core1, 0.5 FILE '/dev/urandom'
SELECT str1[0],str1[1] STREAM str1 FROM core0#core1
SELECT str1[0]+5 STREAM str2 FROM core0
*/

// ctest -R unittest-test-schema
TEST(xschema, check_test0) {

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
TEST(xschema, check_test1) {
  /*
  query qry;
  qry.id = "file_1";
  std::list<token> lTempProgram;
  lTempProgram.push_back(token(PUSH_TSCAN));
  qry.lSchema.push_back(field("*", lTempProgram, rdb::BAD, "*"));
  coreInstance.push_back(qry);
  */

  coreInstance.clear();
  auto compiled = parser("ut_example_schema.rql") == "OK";

  std::map<std::string, std::unique_ptr<dataInstance>> qSet;

  for (auto& q1 : coreInstance) qSet.emplace(q1.id, std::make_unique<dataInstance>(q1));
  for (auto const& [key, val] : qSet) val->storage->setRemoveOnExit(false);

  
  // Todo
  qSet["str1"]->storagePayload->setItem(0,2);
  qSet["str1"]->storagePayload->setItem(1,2);
  qSet["str1"]->storage->write();

  ASSERT_TRUE( coreInstance.size() == qSet.size());
};
