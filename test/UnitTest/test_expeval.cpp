#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "retractor/expressionEvaluator.h"

// ctest -R '^unittest-test-expeval'

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

  ASSERT_TRUE(result.index() == rdb::DOUBLE);

  ASSERT_TRUE(std::get<double>(result) == 3.1);
}

TEST(xExpressionEval, add_string_int) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(PUSH_VAL, rdb::descFldVT("data1")));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::STRING);

  ASSERT_TRUE(std::get<std::string>(result) == "data11");
}

TEST(xExpressionEval, add_rational_int) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(PUSH_VAL, boost::rational<int>(1, 2)));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::RATIONAL);

  ASSERT_TRUE(std::get<boost::rational<int>>(result) == boost::rational<int>(3, 2));
}

TEST(xExpressionEval, add_rational_string) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT("test")));
  program.push_back(token(PUSH_VAL, boost::rational<int>(1, 2)));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::STRING);

  ASSERT_TRUE(std::get<std::string>(result) == "1/2test");
}

TEST(xExpressionEval, sub_int_int) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(SUBTRACT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::INTEGER);

  ASSERT_TRUE(std::get<int>(result) == 1);
}

TEST(xExpressionEval, sub_int_dobule) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2.1));
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(SUBTRACT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::DOUBLE);

  ASSERT_TRUE(std::get<double>(result) == 1.1);
}

TEST(xExpressionEval, mul_int_dobule) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2.1));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(MULTIPLY));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::DOUBLE);
  // std::cerr << std::get<double>(result) << std::endl;
  ASSERT_TRUE(std::get<double>(result) == 4.2);
}

TEST(xExpressionEval, div_int_dobule) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(PUSH_VAL, 2.1));
  program.push_back(token(DIVIDE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::DOUBLE);
  // std::cerr << std::get<double>(result) << std::endl;
  ASSERT_TRUE(std::get<double>(result) == 1.05);
}

TEST(xExpressionEval, neg_dobule) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2.1));
  program.push_back(token(NEGATE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::DOUBLE);
  ASSERT_TRUE(std::get<double>(result) == -2.1);
}

TEST(xExpressionEval, neg_byte_as_xor_ff) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, static_cast<uint8_t>(0x0f)));
  program.push_back(token(NEGATE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::BYTE);
  ASSERT_TRUE(std::get<uint8_t>(result) == 0xf0);
}

TEST(xExpressionEval, call_floor_function_double) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1.25));
  program.push_back(token(CALL, std::string("floor")));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::DOUBLE);
  ASSERT_TRUE(std::get<double>(result) == 1);
}

TEST(xExpressionEval, call_floor_function_rational) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, boost::rational<int>(3 / 2)));
  program.push_back(token(CALL, std::string("floor")));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::RATIONAL);
  ASSERT_TRUE(std::get<boost::rational<int>>(result) == 1);
}

TEST(xExpressionEval, call_ceil_function_double) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1.25));
  program.push_back(token(CALL, std::string("ceil")));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::DOUBLE);
  ASSERT_TRUE(std::get<double>(result) == 2);
}

TEST(xExpressionEval, long_program) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1.25));
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(ADD));
  program.push_back(token(MULTIPLY));
  program.push_back(token(CALL, std::string("ceil")));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(result.index() == rdb::DOUBLE);
  ASSERT_TRUE(std::get<double>(result) == 4);
}
