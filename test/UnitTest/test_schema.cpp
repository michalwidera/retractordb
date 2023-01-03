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
  auto dataDescriptorInternal{
      rdb::Descriptor("A[1]", rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", rdb::INTEGER) |  //
      rdb::Descriptor("A[3]", rdb::INTEGER)    //
  };

  auto dataDescriptorExternal{
      rdb::Descriptor("A[1]", rdb::INTEGER) |  //
      rdb::Descriptor("A[2]", rdb::INTEGER)    //
  };

  streamInstance data{"file_A.dat", dataDescriptorInternal, dataDescriptorExternal};

  ASSERT_TRUE(true);
};
