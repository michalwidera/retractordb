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
SELECT core0[0],core0[1] STREAM str1 FROM core0#core1
SELECT * STREAM test1 FROM core@(-1,10)
SELECT * STREAM test2 FROM core@(1,10)
*/

// ctest -R unittest-test-schema

TEST(xschema, check_test1) {
  /*
  query qry;
  qry.id = "file_1";
  std::list<token> lSchema;
  lSchema.push_back(token(PUSH_TSCAN));
  qry.lSchema.push_back(field("*", lSchema, rdb::BAD, "*"));
  coreInstance.push_back(qry);
  */

  coreInstance.clear();
  auto compiled = parser("ut_example.rql") == "OK";

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
    dataInstance data{"file_A.dat", dataStorageDescriptor, dataInternalDesciptor};

    data.internalPayload->setItem(0, 123);
    data.internalPayload->setItem(1, 345);

    auto v1 = data.internalPayload->getItem(0);

    data.storagePayload->setItem(0, 234);
    data.storagePayload->setItem(1, 456);
    data.storage->write();
  }

  {
    dataInstance data{"file_A.dat", dataStorageDescriptor, dataInternalDesciptor};

    data.internalPayload->setItem(0, 123);
    data.internalPayload->setItem(1, 345);

    auto v1 = data.internalPayload->getItem(0);

    data.storagePayload->setItem(0, 234);
    data.storagePayload->setItem(1, 456);
    data.storage->write();
  }

  // for (auto q : coreInstance) {
  //  SPDLOG_INFO("coreInstance[] {}, {}", q.id, q.filename);
  //}

  dataInstance data(coreInstance["str1"]);

  ASSERT_TRUE(true);
};
