#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <boost/system/error_code.hpp>
#include <fstream>
#include <string>
#include <vector>

#include "retractor/lib/QStruct.h"
#include "retractor/lib/compiler.h"

// ctest -R '^ut-test_compiler' -V

extern std::string parserRQLFile_4Test(qTree &coreInstance, std::string sInputFile);
extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, std::string sInputFile);

qTree coreInstance;

bool compiled = false;

bool check_compile_function() {
  // assure compile only once
  std::ifstream infl("ut_compiler.rql");
  for (std::string line; std::getline(infl, line);) {
    SPDLOG_INFO("{}", line);
  }

  if (!compiled) {
    coreInstance.clear();
    compiled = parserRQLFile_4Test(coreInstance, "ut_compiler.rql") == "OK";
  }
  return compiled;
}

// ut_compiler.rql:
// DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE '/dev/urandom'
// DECLARE c INTEGER,d BYTE STREAM core1, 0.5 FILE '/dev/urandom'
// SELECT core0[0],core0[1] STREAM str1 FROM core0#core1
// SELECT * STREAM test1 FROM core@(1,-10)
// SELECT * STREAM test2 FROM core@(1,10)

TEST(xparser, check_compile) { ASSERT_TRUE(check_compile_function()); }

TEST(xparser, check_compile_result) {
  ASSERT_TRUE(check_compile_function());

  SPDLOG_INFO("coreInstance.size() {}", coreInstance.size());

  for (auto q : coreInstance) {
    SPDLOG_INFO("coreInstance[] {}, {}", q.id, q.filename);
    if (q.id == "test1") {
      ASSERT_TRUE(q.isDeclaration() == false);
    }
  }
}

TEST(xparser, check_parserRQLString) {
  auto [result, first_keyword, stream_name] =
      parserRQLString(coreInstance, "DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE '/dev/urandom'");
  ASSERT_TRUE(result == "OK");
  ASSERT_TRUE(first_keyword == "DECLARE");
  ASSERT_TRUE(stream_name == "core0");
}

TEST(xparser, check_topological_sort) { qTree myInstance; }
