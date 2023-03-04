#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "retractor/expressionEvaluator.h"

TEST(xExpressionEval, check_compile_result_1) {
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

  ASSERT_TRUE(std::get<boost::rational<int>>(result) == 3);
}

TEST(xExpressionEval, check_compile_result_2) {
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

  ASSERT_TRUE(std::get<boost::rational<int>>(result) == 3);
}
