#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "retractor/expressionEvaluator.h"

TEST(xExpressionEval, check_compile_result) {
  /*
  PUSH_VAL(1)
  PUSH_VAL(2)
  ADD
  */
  std::list<token> program;
  program.push_back(token(PUSH_VAL, "", 1));
  program.push_back(token(PUSH_VAL, "", 2));
  program.push_back(token(ADD));

  // expressionEvaluator test;
  // test.eval(program);
}
