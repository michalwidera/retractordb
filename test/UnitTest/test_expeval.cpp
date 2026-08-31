#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <boost/rational.hpp>

#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

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
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);

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
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);

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
  program.emplace_back(PUSH_VAL, 1.1);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);

  EXPECT_TRUE(std::get<double>(result) == 3.1);
}

TEST(xExpressionEval, add_string_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, rdb::descFldVT("data1"));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::STRING);

  EXPECT_TRUE(std::get<std::string>(result) == "1data1");
}

TEST(xExpressionEval, add_rational_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, boost::rational<int>(1, 2));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::RATIONAL);

  EXPECT_TRUE(std::get<boost::rational<int>>(result) == boost::rational<int>(3, 2));
}

TEST(xExpressionEval, add_rational_string) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT("test"));
  program.emplace_back(PUSH_VAL, boost::rational<int>(1, 2));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::STRING);

  EXPECT_TRUE(std::get<std::string>(result) == "test1/2");
}

TEST(xExpressionEval, sub_int_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(SUBTRACT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::INTEGER);

  EXPECT_TRUE(std::get<int>(result) == 1);
}

TEST(xExpressionEval, sub_int_dobule) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2.1);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(SUBTRACT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);

  EXPECT_TRUE(std::get<double>(result) == 1.1);
}

TEST(xExpressionEval, mul_int_dobule) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2.1);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(MULTIPLY);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  // std::cerr << std::get<double>(result) << std::endl;
  EXPECT_TRUE(std::get<double>(result) == 4.2);
}

TEST(xExpressionEval, div_int_dobule) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 2.5);
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  // std::cerr << std::get<double>(result) << std::endl;
  EXPECT_TRUE(std::get<double>(result) == 0.8);  // 2 / 2.5 = 0.8
}

TEST(xExpressionEval, neg_dobule) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2.1);
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == -2.1);
}

TEST(xExpressionEval, neg_byte_as_xor_ff) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, static_cast<uint8_t>(0x0f));
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::BYTE);
  EXPECT_TRUE(std::get<uint8_t>(result) == 0xf0);
}

TEST(xExpressionEval, neg_rational_preserves_type) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, boost::rational<int>(3, 2));
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::RATIONAL);
  EXPECT_EQ(std::get<boost::rational<int>>(result), boost::rational<int>(-3, 2));
}

TEST(xExpressionEval, call_floor_function_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.25);
  program.emplace_back(CALL, std::string("floor"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == 1);
}

TEST(xExpressionEval, call_floor_function_rational) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, boost::rational<int>(3 / 2));
  program.emplace_back(CALL, std::string("floor"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::RATIONAL);
  EXPECT_TRUE(std::get<boost::rational<int>>(result) == 1);
}

TEST(xExpressionEval, call_ceil_function_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.25);
  program.emplace_back(CALL, std::string("ceil"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == 2);
}

TEST(xExpressionEval, long_program) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.25);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);
  program.emplace_back(MULTIPLY);
  program.emplace_back(CALL, std::string("ceil"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == 4);  // ceil ( (1 + 2) * 1.25 ) == ceil( 3.75 ) == 4
}

TEST(xExpressionEval, add_null_int_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

TEST(xExpressionEval, and_false_null_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(AND);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, or_true_null_is_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(OR);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, and_true_null_is_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(AND);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

TEST(xExpressionEval, not_true_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(NOT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, not_null_is_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(NOT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

// Dzielenie przez zero nie ma wyniku w zbiorze wartosci, wiec jest wartoscia POCHLANIAJACA,
// a nie bledem: strumien oddaje NULL i pracuje dalej. Wczesniej lecial std::domain_error, czyli
// jedna probka o zerowym mianowniku przerywala przetwarzanie calego zapytania.
TEST(xExpressionEval, divide_by_zero_yields_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// NULL jest pochlaniajacy takze wtedy, gdy pojawi sie po stronie dzielnej.
TEST(xExpressionEval, divide_null_by_zero_yields_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, std::monostate{});
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// Zero w LICZNIKU to zwykla dana — wynik istnieje i musi pozostac wartoscia, nie NULL-em.
// Kontrola odroznia "pochlanianie braku wyniku" od "pochlaniania wszystkiego, co zawiera zero".
TEST(xExpressionEval, zero_divided_by_value_stays_a_value) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  auto result = test.eval(program);
  ASSERT_FALSE(std::holds_alternative<std::monostate>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// Regula jest GLOBALNA, nie calkowitoliczbowa: `operator/` rozstrzyga zerowy dzielnik PRZED
// rozgalezieniem po typach, wiec float i double nie daja inf, a rational nie rzuca wyjatkiem.
// To jest ten przypadek, ktory bez testu cicho wrocilby do semantyki IEEE.
TEST(xExpressionEval, divide_by_zero_yields_null_for_every_numeric_type) {
  const std::vector<rdb::descFldVT> zeros{
      rdb::descFldVT{static_cast<uint8_t>(0)}, rdb::descFldVT{0}, rdb::descFldVT{0U}, rdb::descFldVT{0.0F}, rdb::descFldVT{0.0},
      rdb::descFldVT{boost::rational<int>(0)}};
  for (const auto &zero : zeros) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, 7);
    program.emplace_back(PUSH_VAL, zero);
    program.emplace_back(DIVIDE);

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program))) << "divisor variant index " << zero.index();
  }
}

TEST(xExpressionEval, malformed_stack_throws) {
  std::list<token> program;
  program.emplace_back(ADD);

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

TEST(xExpressionEval, cmp_equal_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_equal_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 4);
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_not_equal_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 4);
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_lt_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 2 < 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_lt_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 < 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_gt_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 > 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_gt_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 2 > 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_le_true_less) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 2 <= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_le_true_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 <= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_le_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 <= 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_ge_true_greater) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 >= 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_ge_true_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 >= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_ge_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 2 >= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// --- null propagation in arithmetic ---

TEST(xExpressionEval, sub_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(SUBTRACT);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, mul_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(MULTIPLY);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, div_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- neg ---

TEST(xExpressionEval, neg_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 7);
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), -7);
}

TEST(xExpressionEval, neg_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- null propagation in comparisons ---

TEST(xExpressionEval, cmp_equal_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, cmp_not_equal_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // 3 != 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// --- three-value logic ---

TEST(xExpressionEval, or_false_null_is_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(OR);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, or_false_false_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(OR);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, and_true_true_is_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(AND);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

// --- error handling ---

TEST(xExpressionEval, call_unsupported_function_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0);
  program.emplace_back(CALL, std::string("unsupported_fn"));

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
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("a")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("b")));
  program.emplace_back(SUBTRACT);

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

TEST(xExpressionEval, mul_string_string_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("a")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("b")));
  program.emplace_back(MULTIPLY);

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

TEST(xExpressionEval, div_string_string_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("a")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("b")));
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

// --- toLogicValue: string and pair handling ---

TEST(xExpressionEval, not_nonempty_string_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("hello")));
  program.emplace_back(NOT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "0");
}

TEST(xExpressionEval, not_empty_string_is_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("")));
  program.emplace_back(NOT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, and_string_int_uses_string_truthiness) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("text")));
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(AND);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  // "text" is truthy, 1 is truthy → true; result type matches first operand (string)
  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, not_intpair_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::pair<int, int>(1, 2)));
  program.emplace_back(NOT);

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

// --- PUSH_ID from payload ---

TEST(xExpressionEval, push_id_reads_value_from_payload) {
  auto desc = rdb::Descriptor("x", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);
  p.setItem(0, 42);

  std::list<token> program;
  program.emplace_back(PUSH_ID, rdb::descFldVT(std::pair<std::string, int>("x", 0)));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program, &p);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 42);
}

TEST(xExpressionEval, push_id_null_payload_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_ID, rdb::descFldVT(std::pair<std::string, int>("x", 0)));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program, nullptr), std::runtime_error);
}

// --- PUSH_ID2 ---

TEST(xExpressionEval, push_id2_reads_value_from_payload) {
  auto desc = rdb::Descriptor("x", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);
  p.setItem(0, 99);

  std::list<token> program;
  program.emplace_back(PUSH_ID2, rdb::descFldVT(std::string("x[0]")));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program, &p);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 99);
}

TEST(xExpressionEval, push_id2_null_payload_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_ID2, rdb::descFldVT(std::string("x[0]")));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program, nullptr), std::runtime_error);
}

TEST(xExpressionEval, push_id2_malformed_identifier_throws) {
  auto desc = rdb::Descriptor("x", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);
  p.setItem(0, 1);

  std::list<token> program;
  program.emplace_back(PUSH_ID2, rdb::descFldVT(std::string("fieldname")));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program, &p), std::runtime_error);
}

// --- PUSH_IDX ---

TEST(xExpressionEval, push_idx_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_IDX, 0);

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

// --- stack errors ---

TEST(xExpressionEval, too_many_values_on_stack_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, 2);

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

// --- CALL functions ---

// Nazwy pisane wielką literą to POSTAĆ, KTÓRĄ NAPRAWDĘ PRODUKUJE PARSER: `exitFunction_call`
// wkłada do tokena postać kanoniczną z `rqlFunctions.hpp`, a tam `Sqrt`, `Ceil` i `Floor` stoją
// właśnie tak. Pozostałe testy w tym pliku fabrykują nazwy małymi literami, czyli sprawdzają
// samo złożenie wielkości liter w ewaluatorze, a nie postać, którą dostaje z parsera.
TEST(xExpressionEval, call_function_name_as_grammar_writes_it) {
  const std::vector<std::pair<std::string, double>> cases{{"Sqrt", 2.0}, {"Ceil", 4.0}, {"Floor", 4.0}};

  for (const auto &[name, expected] : cases) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, 4.0);
    program.emplace_back(CALL, name);

    expressionEvaluator test;
    rdb::descFldVT result = test.eval(program);

    ASSERT_TRUE(std::holds_alternative<double>(result)) << name;
    EXPECT_EQ(std::get<double>(result), expected) << name;
  }
}

// Złożenie wielkości liter nie może zamienić nieznanej funkcji w znaną ani zamazać nazwy
// w komunikacie.
//
// Od 2026-08-30 ta ścieżka jest z RQL NIEOSIĄGALNA: `compiler::checkFunctionCalls()` odrzuca
// nieznaną nazwę na kompilacji, więc plan z nią nie dochodzi do wykonania. Rzut zostaje jako
// kontrola dla programów tokenów budowanych wprost, tak jak tutaj. `Crc` był w gramatyce do
// tej daty i został usunięty razem z ośmioma innymi nazwami bez implementacji.
TEST(xExpressionEval, call_unknown_function_keeps_author_spelling) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 4.0);
  program.emplace_back(CALL, std::string("Crc"));

  expressionEvaluator test;
  EXPECT_THROW(
      {
        try {
          test.eval(program);
        } catch (const std::runtime_error &error) {
          EXPECT_STREQ(error.what(), "Unsupported function call: Crc");
          throw;
        }
      },
      std::runtime_error);
}

// Funkcje dopisane 2026-08-30. `Abs` liczy się wprost na wariancie, żeby nie tracić dokładności
// wartości wymiernej na okrążeniu przez double, które robi callFun. `IsZero` i `IsNonZero`
// zwracają INTEGER 0/1 niezależnie od typu argumentu — to jest ich cała wartość użytkowa, bo
// porównania żyją w regule `term_logic` i nie są dostępne wewnątrz wyrażenia w SELECT.
TEST(xExpressionEval, call_abs_preserves_type) {
  const std::vector<std::pair<int, int>> intCases{{-9, 9}, {9, 9}, {0, 0}};
  for (const auto &[input, expected] : intCases) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, input);
    program.emplace_back(CALL, std::string("Abs"));

    expressionEvaluator test;
    rdb::descFldVT result = test.eval(program);
    ASSERT_TRUE(std::holds_alternative<int>(result)) << input;
    EXPECT_EQ(std::get<int>(result), expected) << input;
  }

  // Wartość wymierna ma wyjść dokładnie ta sama, bez przybliżenia przez Rationalize.
  std::list<token> program;
  program.emplace_back(PUSH_VAL, boost::rational<int>(-22, 7));
  program.emplace_back(CALL, std::string("Abs"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);
  ASSERT_TRUE(std::holds_alternative<boost::rational<int>>(result));
  EXPECT_EQ(std::get<boost::rational<int>>(result), boost::rational<int>(22, 7));
}

TEST(xExpressionEval, call_iszero_and_isnonzero_return_integer) {
  const std::vector<std::tuple<int, int, int>> cases{{0, 1, 0}, {5, 0, 1}, {-5, 0, 1}};

  for (const auto &[input, zeroExpected, nonZeroExpected] : cases) {
    for (const auto &[name, expected] :
         std::vector<std::pair<std::string, int>>{{"IsZero", zeroExpected}, {"IsNonZero", nonZeroExpected}}) {
      std::list<token> program;
      program.emplace_back(PUSH_VAL, input);
      program.emplace_back(CALL, name);

      expressionEvaluator test;
      rdb::descFldVT result = test.eval(program);
      ASSERT_TRUE(std::holds_alternative<int>(result)) << name << "(" << input << ")";
      EXPECT_EQ(std::get<int>(result), expected) << name << "(" << input << ")";
    }
  }
}

// Length liczy WARTOŚĆ, nie zadeklarowaną szerokość pola: payload::getItemVT przycina pole
// STRING na pierwszym bajcie zerowym, więc `STRING[8]` z wartością `42` dochodzi do ewaluatora
// jako napis dwuznakowy. Wynik jest INTEGER niezależnie od szerokości pola źródłowego.
TEST(xExpressionEval, call_length_counts_value_not_field_width) {
  const std::vector<std::pair<std::string, int>> cases{{"", 0}, {"42", 2}, {"hello", 5}};

  for (const auto &[input, expected] : cases) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, input);
    program.emplace_back(CALL, std::string("Length"));

    expressionEvaluator test;
    rdb::descFldVT result = test.eval(program);
    ASSERT_TRUE(std::holds_alternative<int>(result)) << "Length('" << input << "')";
    EXPECT_EQ(std::get<int>(result), expected) << "Length('" << input << "')";
  }
}

// Argument nietekstowy jest błędem, a nie cichą konwersją przez to_string — symetrycznie do
// `Abs` i `IsZero`, które odrzucają napis. `Length(k)` nad polem INTEGER jest niemal na pewno
// literówką; kto chce długości zapisu dziesiętnego, pisze `Length(to_string(k))`.
TEST(xExpressionEval, call_length_rejects_numeric_operand) {
  for (const auto &value : std::vector<rdb::descFldVT>{7, 2.5, boost::rational<int>(22, 7)}) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, value);
    program.emplace_back(CALL, std::string("Length"));

    expressionEvaluator test;
    EXPECT_THROW({ auto discarded = test.eval(program); }, std::runtime_error) << "indeks " << value.index();
  }
}

// NULL pochłania: predykat na braku danych jest brakiem danych, a nie fałszem.
TEST(xExpressionEval, new_functions_propagate_null) {
  for (const char *name : {"Abs", "IsZero", "IsNonZero", "Length"}) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, rdb::descFldVT{std::monostate{}});
    program.emplace_back(CALL, std::string(name));

    expressionEvaluator test;
    rdb::descFldVT result = test.eval(program);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(result)) << name;
  }
}

TEST(xExpressionEval, call_sqrt_function_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 4.0);
  program.emplace_back(CALL, std::string("sqrt"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 2.0);
}

TEST(xExpressionEval, call_round_function_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.5);
  program.emplace_back(CALL, std::string("round"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 2.0);
}

TEST(xExpressionEval, call_trunc_function_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.9);
  program.emplace_back(CALL, std::string("trunc"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 1.0);
}

TEST(xExpressionEval, call_sin_zero) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0.0);
  program.emplace_back(CALL, std::string("sin"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 0.0);
}

TEST(xExpressionEval, call_cos_zero) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0.0);
  program.emplace_back(CALL, std::string("cos"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 1.0);
}

TEST(xExpressionEval, call_tan_zero) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0.0);
  program.emplace_back(CALL, std::string("tan"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 0.0);
}

TEST(xExpressionEval, call_log_of_one) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0);
  program.emplace_back(CALL, std::string("log"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 0.0);
}

TEST(xExpressionEval, call_log2_of_one) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0);
  program.emplace_back(CALL, std::string("log2"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 0.0);
}

TEST(xExpressionEval, call_function_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("sqrt"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, call_function_on_string_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("text")));
  program.emplace_back(CALL, std::string("floor"));

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

// --- null propagation in remaining comparisons ---

TEST(xExpressionEval, cmp_lt_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, cmp_gt_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, cmp_le_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, cmp_ge_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, cmp_not_equal_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- neg on string ---

TEST(xExpressionEval, neg_string_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("hello")));
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  EXPECT_THROW(test.eval(program), std::runtime_error);
}

// --- 3VL: null AND null, null OR null ---

TEST(xExpressionEval, and_null_null_is_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(AND);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

TEST(xExpressionEval, or_null_null_is_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(OR);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- string comparisons ---

TEST(xExpressionEval, cmp_equal_string_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, cmp_equal_string_not_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "0");
}

// --- float arithmetic ---

TEST(xExpressionEval, add_float_float) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0F);
  program.emplace_back(PUSH_VAL, 2.0F);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<float>(result));
  EXPECT_EQ(std::get<float>(result), 3.0F);
}

// --- unsigned arithmetic ---

TEST(xExpressionEval, add_uint_uint) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3U);
  program.emplace_back(PUSH_VAL, 4U);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<unsigned>(result));
  EXPECT_EQ(std::get<unsigned>(result), 7U);
}

TEST(xExpressionEval, isnull_returns_1_for_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("isnull"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, isnull_returns_0_for_non_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 42);
  program.emplace_back(CALL, std::string("isnull"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// null2zero zamienia wartosc POCHLANIAJACA na zero: dziure w danych, dzielenie przez zero
// albo okno bez ani jednej wartosci. Ogona strumienia nie dotyczy — tam nie ma rekordu,
// wiec nie ma czego zamieniac (patrz query::startupLatency).
TEST(xExpressionEval, null2zero_replaces_null_with_zero) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("null2zero"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// Wartosc nie-NULL przechodzi BEZ ZMIANY — takze co do typu, zeby zawiniecie pola nie
// przycinalo wyniku okna z RATIONAL do liczby calkowitej.
TEST(xExpressionEval, null2zero_passes_a_value_through_unchanged) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, boost::rational<int>(7, 2));
  program.emplace_back(CALL, std::string("null2zero"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<boost::rational<int>>(result));
  EXPECT_EQ(std::get<boost::rational<int>>(result), boost::rational<int>(7, 2));
}

// --- string comparison operators (CMP_NOT_EQUAL, CMP_LT, CMP_GT, CMP_LE, CMP_GE) ---
// Wynik porównania stringów jest zawsze typu std::string ("1" lub "0"),
// porównanie leksykograficzne zgodnie z std::string::operator<.

TEST(xExpressionEval, cmp_not_equal_string_not_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "abc" != "xyz"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, cmp_not_equal_string_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "abc" != "abc"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "0");
}

TEST(xExpressionEval, cmp_lt_string_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "abc" < "xyz"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, cmp_lt_string_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "xyz" < "abc"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "0");
}

TEST(xExpressionEval, cmp_gt_string_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "xyz" > "abc"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, cmp_gt_string_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "abc" > "xyz"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "0");
}

TEST(xExpressionEval, cmp_le_string_true_less) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "abc" <= "xyz"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, cmp_le_string_true_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "abc" <= "abc"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, cmp_le_string_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "xyz" <= "abc"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "0");
}

TEST(xExpressionEval, cmp_ge_string_true_greater) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "xyz" >= "abc"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, cmp_ge_string_true_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "abc" >= "abc"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, cmp_ge_string_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "abc" >= "xyz"

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "0");
}

// --- OR z operandem string ---
// toLogicValue(string) = !string.empty(); typ wyniku pochodzi od pierwszego niezerowego operandu.

TEST(xExpressionEval, or_nonempty_string_false_is_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("text")));
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(OR);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "text" OR 0 → true, typ string

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "1");
}

TEST(xExpressionEval, or_empty_string_false_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("")));
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(OR);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "" OR 0 → false, typ string

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "0");
}

// --- ADD: string + float i string + double przez normalize/castFldVT ---
// normalize(string, T): string.index()=8 > T.index() → T rzutowane na STRING przez std::to_string.
// Kolejność push: float/double pierwszy, string drugi → na stosie string jest na wierzchu,
// więc po pop: a=string, b=float → normalize(string, float) → konkatenacja.

TEST(xExpressionEval, add_string_float_converts_via_to_string) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0F);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("unit")));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "unit" + std::to_string(1.0f)

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), std::to_string(1.0F) + "unit");
}

TEST(xExpressionEval, add_string_double_converts_via_to_string) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2.0);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("val")));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);  // "val" + std::to_string(2.0)

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), std::to_string(2.0) + "val");
}

// --- to_integer ---

TEST(xExpressionEval, call_to_integer_from_string) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("42")));
  program.emplace_back(CALL, std::string("to_integer"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 42);
}

TEST(xExpressionEval, call_to_integer_from_double_truncates) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3.9);
  program.emplace_back(CALL, std::string("to_integer"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 3);
}

TEST(xExpressionEval, call_to_integer_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("to_integer"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- to_float ---

TEST(xExpressionEval, call_to_float_from_string) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("2.5")));
  program.emplace_back(CALL, std::string("to_float"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<float>(result));
  EXPECT_EQ(std::get<float>(result), 2.5F);
}

TEST(xExpressionEval, call_to_float_from_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(CALL, std::string("to_float"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<float>(result));
  EXPECT_EQ(std::get<float>(result), 5.0F);
}

TEST(xExpressionEval, call_to_float_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("to_float"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- to_double ---

TEST(xExpressionEval, call_to_double_from_string) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("1.5")));
  program.emplace_back(CALL, std::string("to_double"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 1.5);
}

TEST(xExpressionEval, call_to_double_from_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 7);
  program.emplace_back(CALL, std::string("to_double"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 7.0);
}

TEST(xExpressionEval, call_to_double_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("to_double"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- to_string ---

TEST(xExpressionEval, call_to_string_from_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 42);
  program.emplace_back(CALL, std::string("to_string"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "42");
}

TEST(xExpressionEval, call_to_string_from_float) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0F);
  program.emplace_back(CALL, std::string("to_string"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), std::to_string(1.0F));
}

TEST(xExpressionEval, call_to_string_identity) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("hello")));
  program.emplace_back(CALL, std::string("to_string"));

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "hello");
}

TEST(xExpressionEval, call_to_string_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("to_string"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
}

// --- POWER: operator `^` z RQL.g4 (ExpPow) ------------------------------------------------

// Kolejnosc operandow. ONP `PUSH 2; PUSH 3; POWER` ma znaczyc 2^3 = 8, a nie 3^2 = 9.
// Potegowanie jest pierwszym NIEPRZEMIENNYM operatorem w tym pliku, ktory nie ma
// osobnego znaku po obu stronach, wiec pomylka stron przeszlaby przez kazdy inny test.
TEST(xExpressionEval, pow_operand_order) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(POWER);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 8);
}

// Typ wyniku ustala normalize(), tak samo jak dla `*`. Kwadrat pola INTEGER zostaje
// INTEGER-em — inaczej `pole ^ 2` nie bylby zamiennikiem dla `pole * pole`.
TEST(xExpressionEval, pow_int_int_stays_integer) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 7);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(POWER);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  EXPECT_EQ(result.index(), rdb::INTEGER);
  EXPECT_EQ(std::get<int>(result), 49);
}

// Wykladnik ulamkowy promuje wynik do DOUBLE — normalize() bierze wyzszy indeks wariantu.
TEST(xExpressionEval, pow_int_double_promotes_to_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 0.5);
  program.emplace_back(POWER);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_EQ(result.index(), rdb::DOUBLE);
  EXPECT_NEAR(std::get<double>(result), 1.4142135623730951, 1E-12);
}

// Podstawa calkowita obcina wynik, dokladnie jak dzielenie calkowite: 2^-1 = 0.5 -> 0.
TEST(xExpressionEval, pow_negative_exponent_truncates_on_integer_base) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, -1);
  program.emplace_back(POWER);

  expressionEvaluator test;
  rdb::descFldVT result = test.eval(program);

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// Wynik poza zbiorem wartosci jest wartoscia POCHLANIAJACA, jak dzielenie przez zero.
// Rzut nieskonczonosci na int bylby zreszta zachowaniem niezdefiniowanym.
TEST(xExpressionEval, pow_non_finite_result_gives_null) {
  for (const auto &operands : {std::pair<rdb::descFldVT, rdb::descFldVT>{0, -1},         // 0^-1 = inf
                               std::pair<rdb::descFldVT, rdb::descFldVT>{-8.0, 0.5}}) {  // (-8)^0.5 = NaN
    std::list<token> program;
    program.emplace_back(PUSH_VAL, operands.first);
    program.emplace_back(PUSH_VAL, operands.second);
    program.emplace_back(POWER);

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
  }
}

TEST(xExpressionEval, pow_null_operand_propagates_null) {
  for (bool nullOnLeft : {true, false}) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, nullOnLeft ? rdb::descFldVT(std::monostate{}) : rdb::descFldVT(2));
    program.emplace_back(PUSH_VAL, nullOnLeft ? rdb::descFldVT(2) : rdb::descFldVT(std::monostate{}));
    program.emplace_back(POWER);

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(test.eval(program)));
  }
}

// Operand tekstowy jest bledem, tak samo jak dla `*`, `-` i `/`. normalize() promuje
// druga strone do STRING, wiec lapane sa oba ustawienia operandow.
TEST(xExpressionEval, pow_string_operand_throws) {
  for (bool stringOnLeft : {true, false}) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, stringOnLeft ? rdb::descFldVT(std::string("abc")) : rdb::descFldVT(2));
    program.emplace_back(PUSH_VAL, stringOnLeft ? rdb::descFldVT(2) : rdb::descFldVT(std::string("abc")));
    program.emplace_back(POWER);

    expressionEvaluator test;
    EXPECT_THROW(test.eval(program), std::runtime_error);
  }
}

// Sciezka dokladna: dla typow calkowitych i wymiernych `a^k` MA BYC iloczynem `a*a*...*a`,
// razem z przekreceniem i promocja typu. Na tej rownosci stoi regula D w exprSimplify,
// ktora przepisuje `a*a` na `a^2`.
TEST(xExpressionEval, pow_on_exact_types_matches_multiplication) {
  struct testCase {
    rdb::descFldVT value;
    int exponent;
  };

  for (const auto &item : {testCase{100000, 2},                         // przekrecenie int
                           testCase{-3, 3},                             // ujemna podstawa
                           testCase{rdb::descFldVT{uint8_t(200)}, 2},   // promocja BYTE do int
                           testCase{7U, 3},                             // unsigned
                           testCase{boost::rational<int>(2, 3), 3}}) {  // arytmetyka wymierna
    std::list<token> asPower;
    asPower.emplace_back(PUSH_VAL, item.value);
    asPower.emplace_back(PUSH_VAL, item.exponent);
    asPower.emplace_back(POWER);

    std::list<token> asProduct;
    asProduct.emplace_back(PUSH_VAL, item.value);
    for (int step = 1; step < item.exponent; ++step) {
      asProduct.emplace_back(PUSH_VAL, item.value);
      asProduct.emplace_back(MULTIPLY);
    }

    expressionEvaluator test;
    EXPECT_TRUE(test.eval(asPower) == test.eval(asProduct)) << "wykladnik " << item.exponent;
  }
}
