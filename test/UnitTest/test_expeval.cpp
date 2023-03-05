#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "retractor/expressionEvaluator.h"

TEST(xExpressionEval, add_int_int) {
  /*
  PUSH_VAL(1)
  PUSH_VAL(2)
  ADD
  */
  std::list<token> program;
  program.push_back(token(PUSH_VAL, "", 1));
  program.push_back(token(PUSH_VAL, "", 2));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == 2);  // int in Token

  ASSERT_TRUE(std::get<int>(result) == 3);  // (int)3 in Token
}

TEST(xExpressionEval, add_int_int_new_token) {
  /*
  PUSH_VAL(1)
  PUSH_VAL(2)
  ADD
  */
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == 2);  // int in Token

  ASSERT_TRUE(std::get<int>(result) == 3);
}

TEST(xExpressionEval, add_double_int) {
  /*
  PUSH_VAL(1.1)
  PUSH_VAL(2)
  ADD
  */
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1.1));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::DOUBLE);  // int in Token

  ASSERT_TRUE(std::get<double>(result) == 3.1);
}

/* Work in progress...
TEST(xExpressionEval, add_string_int) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, std::string("data1")));
  program.push_back(token(PUSH_VAL, std::string("data2")));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  //ASSERT_TRUE(result.index() == rdb::DOUBLE);  // int in Token

  //ASSERT_TRUE(std::get<double>(result) == 3.1);
}
*/