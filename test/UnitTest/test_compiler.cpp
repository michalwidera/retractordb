#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <boost/system/error_code.hpp>

#include "retractor/lib/compiler.hpp"
#include "retractor/lib/qTree.hpp"

// ctest -R '^ut-test_compiler' -V

extern std::string parserRQLFile_4Test(qTree &coreInstance, const std::string &sInputFile);
extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &sInputFile);

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

TEST(xparser, check_compile) { EXPECT_TRUE(check_compile_function()); }

TEST(xparser, check_compile_result) {
  EXPECT_TRUE(check_compile_function());

  SPDLOG_INFO("coreInstance.size() {}", coreInstance.size());

  for (auto q : coreInstance) {
    SPDLOG_INFO("coreInstance[] {}, {}", q.id, q.filename);
    if (q.id == "test1") {
      EXPECT_TRUE(q.isDeclaration() == false);
    }
  }
}

TEST(xparser, check_parserRQLString) {
  auto [result, first_keyword, stream_name] =
      parserRQLString(coreInstance, "DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE '/dev/urandom'");
  EXPECT_TRUE(result == "OK");
  EXPECT_TRUE(first_keyword == "DECLARE");
  EXPECT_TRUE(stream_name == "core0");
}

TEST(xparser, check_topological_sort) { qTree myInstance; }

TEST(xparser, check_multiline_backslash) {
  qTree instance;
  auto result = parserRQLFile_4Test(instance, "ut_multiline.rql");
  EXPECT_EQ(result, "OK");
  EXPECT_TRUE(instance.exists("core0"));
  EXPECT_TRUE(instance.exists("str1"));
}

TEST(xcompiler, shares_commutative_add_select_computation) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        SELECT a[_]*b[_] STREAM c1 FROM a+b
        SELECT a[_]*b[_] STREAM c2 FROM b+a
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");
  ASSERT_TRUE(instance.exists("c1"));
  ASSERT_TRUE(instance.exists("c2"));

  auto &c1 = instance.getQuery("c1");
  auto &c2 = instance.getQuery("c2");
  ASSERT_EQ(c1.lProgram.size(), 1);
  ASSERT_EQ(c2.lProgram.size(), 1);

  const auto sharedId = c1.lProgram.front().getStr_();
  EXPECT_EQ(c2.lProgram.front().getStr_(), sharedId);
  EXPECT_TRUE(sharedId.starts_with("STREAM_SELECT_"));
  ASSERT_TRUE(instance.exists(sharedId));
  EXPECT_TRUE(instance.getQuery(sharedId).isSubstrat);
  EXPECT_EQ(instance.getQuery(sharedId).lSchema.size(), 2);
}

TEST(xcompiler, does_not_share_order_sensitive_selects) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        SELECT * STREAM d1 FROM a+b
        SELECT * STREAM d2 FROM b+a
        SELECT a[0],b[1] STREAM e1 FROM a+b
        SELECT b[1],a[0] STREAM e2 FROM b+a
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  const auto sharedCount =
      std::ranges::count_if(instance, [](const query &qry) { return qry.id.starts_with("STREAM_SELECT_"); });
  EXPECT_EQ(sharedCount, 0);
  EXPECT_EQ(instance.getQuery("d1").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("d2").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("e1").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("e2").lProgram.size(), 3);
}

TEST(xcompiler, does_not_share_different_result_shapes) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        SELECT to_string(a[0]:8) STREAM narrow FROM a+b
        SELECT to_string(a[0]:16) STREAM wide FROM b+a
        SELECT to_float(a[0]) STREAM as_float FROM a+b
        SELECT to_double(a[0]) STREAM as_double FROM b+a
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  const auto sharedCount =
      std::ranges::count_if(instance, [](const query &qry) { return qry.id.starts_with("STREAM_SELECT_"); });
  EXPECT_EQ(sharedCount, 0);
  EXPECT_EQ(instance.getQuery("narrow").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("wide").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("as_float").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("as_double").lProgram.size(), 3);
}

TEST(xcompiler, preserves_add_grouping_in_computation_fingerprint) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        DECLARE cx INTEGER, cy INTEGER STREAM c, 3 FILE 'c.txt'
        SELECT a[_]*b[_]+c[_] STREAM x1 FROM (a+b)+c
        SELECT a[_]*b[_]+c[_] STREAM x2 FROM (b+a)+c
        SELECT a[_]*b[_]+c[_] STREAM x3 FROM (c+b)+a
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  auto &x1 = instance.getQuery("x1");
  auto &x2 = instance.getQuery("x2");
  auto &x3 = instance.getQuery("x3");
  ASSERT_EQ(x1.lProgram.size(), 1);
  ASSERT_EQ(x2.lProgram.size(), 1);
  EXPECT_EQ(x1.lProgram.front().getStr_(), x2.lProgram.front().getStr_());
  EXPECT_EQ(x3.lProgram.size(), 3);
  EXPECT_FALSE(instance.exists("STREAM_ADD_b_a"));
  EXPECT_TRUE(instance.exists("STREAM_ADD_c_b"));
}

TEST(xcompiler, does_not_rewrite_existing_queries_during_import) {
  qTree live;
  auto [baseParseResult, baseKeyword, baseStream] = parserRQLString(live, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        SELECT a[_]*b[_] STREAM c1 FROM a+b
      )");
  ASSERT_EQ(baseParseResult, "OK");

  compiler liveCompiler(live);
  ASSERT_EQ(liveCompiler.compile(), "OK");
  ASSERT_EQ(live.getQuery("c1").lProgram.size(), 3);

  qTree importedPlan                                    = live;
  auto [importParseResult, importKeyword, importStream] = parserRQLString(importedPlan, "SELECT a[_]*b[_] STREAM c2 FROM b+a");
  ASSERT_EQ(importParseResult, "OK");

  compiler importedCompiler(importedPlan);
  ASSERT_EQ(importedCompiler.compile(), "OK");
  auto importedIds = liveCompiler.importFrom(importedPlan);
  ASSERT_FALSE(importedIds.empty());
  ASSERT_EQ(liveCompiler.compile(), "OK");

  EXPECT_EQ(live.getQuery("c1").lProgram.size(), 3);
  ASSERT_TRUE(live.exists("c2"));
  EXPECT_EQ(live.getQuery("c2").lProgram.size(), 3);
}
