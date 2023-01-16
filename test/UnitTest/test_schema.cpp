#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <locale>
#include <string>

#include "QStruct.h"  // coreInstance
#include "rdb/dsacc.h"
#include "rdb/fainterface.h"
#include "rdb/payloadacc.h"
#include "retractor/dataModel.h"
#include "spdlog/spdlog.h"

extern "C" qTree coreInstance;

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
  auto dataInternalDesciptor{
      rdb::Descriptor("A[1]", rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", rdb::INTEGER) |  //
      rdb::Descriptor("A[3]", rdb::INTEGER)    //
  };

  auto dataStorageDescriptor{
      rdb::Descriptor("A[1]", rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", rdb::INTEGER)    //
  };

  streamInstance data{"file_A.dat", dataStorageDescriptor, dataInternalDesciptor};

  data.accessorInternal->setItem(0, 123);
  data.accessorInternal->setItem(1, 345);

  auto v1 = data.accessorInternal->getItem(0);

  data.accessorStorage->setItem(0, 123);
  data.accessorStorage->setItem(1, 345);
  data.storage->write();

  ASSERT_TRUE(true);
};
