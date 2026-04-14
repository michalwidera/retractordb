#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <stdexcept>

#include "rdb/payload.hpp"
#include "retractor/lib/expressionEvaluator.hpp"

// ctest -R '^ut-expeval' -V

TEST(xExpressionEval, add_int_int) {
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

  EXPECT_TRUE(result.index() == rdb::INTEGER);  // int in Token

  EXPECT_TRUE(std::get<int>(result) == 3);  // (int)3 in Token
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

  EXPECT_TRUE(result.index() == rdb::INTEGER);  // int in Token

  EXPECT_TRUE(std::get<int>(result) == 3);
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

  EXPECT_TRUE(result.index() == rdb::DOUBLE);

  EXPECT_TRUE(std::get<double>(result) == 3.1);
}

TEST(xExpressionEval, add_string_int) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(PUSH_VAL, rdb::descFldVT("data1")));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::STRING);

  EXPECT_TRUE(std::get<std::string>(result) == "data11");
}

TEST(xExpressionEval, add_rational_int) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(PUSH_VAL, boost::rational<int>(1, 2)));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::RATIONAL);

  EXPECT_TRUE(std::get<boost::rational<int>>(result) == boost::rational<int>(3, 2));
}

TEST(xExpressionEval, add_rational_string) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT("test")));
  program.push_back(token(PUSH_VAL, boost::rational<int>(1, 2)));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::STRING);

  EXPECT_TRUE(std::get<std::string>(result) == "1/2test");
}

TEST(xExpressionEval, sub_int_int) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(SUBTRACT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::INTEGER);

  EXPECT_TRUE(std::get<int>(result) == 1);
}

TEST(xExpressionEval, sub_int_dobule) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2.1));
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(SUBTRACT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);

  EXPECT_TRUE(std::get<double>(result) == 1.1);
}

TEST(xExpressionEval, mul_int_dobule) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2.1));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(MULTIPLY));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  // std::cerr << std::get<double>(result) << std::endl;
  EXPECT_TRUE(std::get<double>(result) == 4.2);
}

TEST(xExpressionEval, div_int_dobule) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(PUSH_VAL, 2.5));
  program.push_back(token(DIVIDE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  // std::cerr << std::get<double>(result) << std::endl;
  EXPECT_TRUE(std::get<double>(result) == 0.8);  // 2 / 2.5 = 0.8
}

TEST(xExpressionEval, neg_dobule) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2.1));
  program.push_back(token(NEGATE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == -2.1);
}

TEST(xExpressionEval, neg_byte_as_xor_ff) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, static_cast<uint8_t>(0x0f)));
  program.push_back(token(NEGATE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::BYTE);
  EXPECT_TRUE(std::get<uint8_t>(result) == 0xf0);
}

TEST(xExpressionEval, neg_rational_preserves_type) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, boost::rational<int>(3, 2)));
  program.push_back(token(NEGATE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::RATIONAL);
  EXPECT_EQ(std::get<boost::rational<int>>(result), boost::rational<int>(-3, 2));
}

TEST(xExpressionEval, call_floor_function_double) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1.25));
  program.push_back(token(CALL, std::string("floor")));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == 1);
}

TEST(xExpressionEval, call_floor_function_rational) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, boost::rational<int>(3 / 2)));
  program.push_back(token(CALL, std::string("floor")));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::RATIONAL);
  EXPECT_TRUE(std::get<boost::rational<int>>(result) == 1);
}

TEST(xExpressionEval, call_ceil_function_double) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1.25));
  program.push_back(token(CALL, std::string("ceil")));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == 2);
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

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == 4);  // ceil ( (1 + 2) * 1.25 ) == ceil( 3.75 ) == 4
}

TEST(xExpressionEval, add_null_int_propagates_null) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(ADD));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

TEST(xExpressionEval, and_false_null_is_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 0));
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(AND));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, or_true_null_is_true) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(OR));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, and_true_null_is_null) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(AND));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

TEST(xExpressionEval, not_true_is_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(NOT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, not_null_is_null) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(NOT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

TEST(xExpressionEval, divide_by_zero_throws) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 5));
  program.push_back(token(PUSH_VAL, 0));
  program.push_back(token(DIVIDE));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::domain_error);
}

TEST(xExpressionEval, malformed_stack_throws) {
  std::list<token> program;
  program.push_back(token(ADD));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

TEST(xExpressionEval, cmp_equal_true) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(CMP_EQUAL));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_equal_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 4));
  program.push_back(token(CMP_EQUAL));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_not_equal_true) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 4));
  program.push_back(token(CMP_NOT_EQUAL));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_lt_true) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(CMP_LT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 2 < 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_lt_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(CMP_LT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 < 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_gt_true) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(CMP_GT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 > 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_gt_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(CMP_GT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 2 > 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_le_true_less) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(CMP_LE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 2 <= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_le_true_equal) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(CMP_LE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 <= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_le_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(CMP_LE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 <= 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_ge_true_greater) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(CMP_GE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 >= 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_ge_true_equal) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(CMP_GE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 >= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_ge_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 2));
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(CMP_GE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 2 >= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// --- null propagation in arithmetic ---

TEST(xExpressionEval, sub_null_propagates_null) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(PUSH_VAL, 5));
  program.push_back(token(SUBTRACT));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, mul_null_propagates_null) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(PUSH_VAL, 5));
  program.push_back(token(MULTIPLY));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, div_null_propagates_null) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(PUSH_VAL, 5));
  program.push_back(token(DIVIDE));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- neg ---

TEST(xExpressionEval, neg_int) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 7));
  program.push_back(token(NEGATE));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), -7);
}

TEST(xExpressionEval, neg_null_propagates_null) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(NEGATE));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- null propagation in comparisons ---

TEST(xExpressionEval, cmp_equal_null_propagates_null) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(CMP_EQUAL));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, cmp_not_equal_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(PUSH_VAL, 3));
  program.push_back(token(CMP_NOT_EQUAL));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 != 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// --- three-value logic ---

TEST(xExpressionEval, or_false_null_is_null) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 0));
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::monostate{})));
  program.push_back(token(OR));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, or_false_false_is_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 0));
  program.push_back(token(PUSH_VAL, 0));
  program.push_back(token(OR));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, and_true_true_is_true) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(AND));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

// --- error handling ---

TEST(xExpressionEval, call_unsupported_function_throws) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, 1.0));
  program.push_back(token(CALL, std::string("unsupported_fn")));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

TEST(xExpressionEval, empty_program_throws) {
  std::list<token> program;

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

TEST(xExpressionEval, sub_string_string_throws) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::string("a"))));
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::string("b"))));
  program.push_back(token(SUBTRACT));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

TEST(xExpressionEval, mul_string_string_throws) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::string("a"))));
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::string("b"))));
  program.push_back(token(MULTIPLY));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

TEST(xExpressionEval, div_string_string_throws) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::string("a"))));
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::string("b"))));
  program.push_back(token(DIVIDE));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

// --- toLogicValue: string and pair handling ---

TEST(xExpressionEval, not_nonempty_string_is_false) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::string("hello"))));
  program.push_back(token(NOT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "0");
}

TEST(xExpressionEval, not_empty_string_is_true) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::string(""))));
  program.push_back(token(NOT));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, and_string_int_uses_string_truthiness) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::string("text"))));
  program.push_back(token(PUSH_VAL, 1));
  program.push_back(token(AND));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  // "text" is truthy, 1 is truthy → true; result type matches first operand (string)
  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, not_intpair_throws) {
  std::list<token> program;
  program.push_back(token(PUSH_VAL, rdb::descFldVT(std::pair<int, int>(1, 2))));
  program.push_back(token(NOT));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

// --- PUSH_ID from payload ---

TEST(xExpressionEval, push_id_reads_value_from_payload) {
  auto desc = rdb::Descriptor("x", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);
  p.setItem(0, 42);

  std::list<token> program;
  program.push_back(token(PUSH_ID, rdb::descFldVT(std::pair<std::string, int>("x", 0))));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program, &p);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 42);
}
