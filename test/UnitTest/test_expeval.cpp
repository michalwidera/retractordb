#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <boost/rational.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "rdb/payload.hpp"
#include "rdbResult.hpp"
#include "retractor/lib/checkedArith.hpp"
#include "retractor/lib/expressionEvaluator.hpp"

// ctest -R '^ut-expeval' -V

TEST(xExpressionEval, add_int_int) {
  // covers: expressionevaluator.expression-evaluator.15~1
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
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

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
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

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
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::DOUBLE);

  EXPECT_TRUE(std::get<double>(result) == 3.1);
}

TEST(xExpressionEval, add_string_int) {
  // covers: expressionevaluator.expression-evaluator.3~1, expressionevaluator.expression-evaluator.4~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, rdb::descFldVT("data1"));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::STRING);

  EXPECT_TRUE(std::get<std::string>(result) == "1data1");
}

TEST(xExpressionEval, add_rational_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, boost::rational<int>(1, 2));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::RATIONAL);

  EXPECT_TRUE(std::get<boost::rational<int>>(result) == boost::rational<int>(3, 2));
}

TEST(xExpressionEval, add_rational_string) {
  // covers: expressionevaluator.expression-evaluator.5~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT("test"));
  program.emplace_back(PUSH_VAL, boost::rational<int>(1, 2));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::STRING);

  EXPECT_TRUE(std::get<std::string>(result) == "test1/2");
}

TEST(xExpressionEval, sub_int_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(SUBTRACT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::INTEGER);

  EXPECT_TRUE(std::get<int>(result) == 1);
}

TEST(xExpressionEval, sub_int_dobule) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2.1);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(SUBTRACT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::DOUBLE);

  EXPECT_TRUE(std::get<double>(result) == 1.1);
}

TEST(xExpressionEval, mul_int_dobule) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2.1);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(MULTIPLY);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

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
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  // std::cerr << std::get<double>(result) << std::endl;
  EXPECT_TRUE(std::get<double>(result) == 0.8);  // 2 / 2.5 = 0.8
}

TEST(xExpressionEval, neg_dobule) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2.1);
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == -2.1);
}

TEST(xExpressionEval, neg_byte_as_xor_ff) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, static_cast<uint8_t>(0x0f));
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::BYTE);
  EXPECT_TRUE(std::get<uint8_t>(result) == 0xf0);
}

TEST(xExpressionEval, neg_rational_preserves_type) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, boost::rational<int>(3, 2));
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::RATIONAL);
  EXPECT_EQ(std::get<boost::rational<int>>(result), boost::rational<int>(-3, 2));
}

TEST(xExpressionEval, call_floor_function_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.25);
  program.emplace_back(CALL, std::string("floor"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == 1);
}

TEST(xExpressionEval, call_floor_function_rational) {
  // covers: expressionevaluator.expression-evaluator.13~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, boost::rational<int>(3 / 2));
  program.emplace_back(CALL, std::string("floor"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::RATIONAL);
  EXPECT_TRUE(std::get<boost::rational<int>>(result) == 1);
}

TEST(xExpressionEval, call_ceil_function_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.25);
  program.emplace_back(CALL, std::string("ceil"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == 2);
}

TEST(xExpressionEval, long_program) {
  // covers: expressionevaluator.expression-evaluator.1~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.25);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);
  program.emplace_back(MULTIPLY);
  program.emplace_back(CALL, std::string("ceil"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(result.index() == rdb::DOUBLE);
  EXPECT_TRUE(std::get<double>(result) == 4);  // ceil ( (1 + 2) * 1.25 ) == ceil( 3.75 ) == 4
}

TEST(xExpressionEval, add_null_int_propagates_null) {
  // covers: expressionevaluator.expression-evaluator.4~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

TEST(xExpressionEval, and_false_null_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(AND);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, or_true_null_is_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(OR);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, and_true_null_is_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(AND);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

TEST(xExpressionEval, not_true_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(NOT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, not_null_is_null) {
  // covers: expressionevaluator.expression-evaluator.11~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(NOT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

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
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// NULL jest pochlaniajacy takze wtedy, gdy pojawi sie po stronie dzielnej.
TEST(xExpressionEval, divide_null_by_zero_yields_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, std::monostate{});
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// Zero w LICZNIKU to zwykla dana - wynik istnieje i musi pozostac wartoscia, nie NULL-em.
// Kontrola odroznia "pochlanianie braku wyniku" od "pochlaniania wszystkiego, co zawiera zero".
TEST(xExpressionEval, zero_divided_by_value_stays_a_value) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  auto result = rdbtest::ok(test.eval(program));
  ASSERT_FALSE(std::holds_alternative<std::monostate>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// Regula jest GLOBALNA, nie calkowitoliczbowa: `operator/` rozstrzyga zerowy dzielnik PRZED
// rozgalezieniem po typach, wiec float i double nie daja inf, a rational nie rzuca wyjatkiem.
// To jest ten przypadek, ktory bez testu cicho wrocilby do semantyki IEEE.
TEST(xExpressionEval, divide_by_zero_yields_null_for_every_numeric_type) {
  // covers: expressionevaluator.expression-evaluator.19~1
  const std::vector<rdb::descFldVT> zeros{
      rdb::descFldVT{static_cast<uint8_t>(0)}, rdb::descFldVT{0}, rdb::descFldVT{0U}, rdb::descFldVT{0.0F}, rdb::descFldVT{0.0},
      rdb::descFldVT{boost::rational<int>(0)}};
  for (const auto &zero : zeros) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, 7);
    program.emplace_back(PUSH_VAL, zero);
    program.emplace_back(DIVIDE);

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))))
        << "divisor variant index " << zero.index();
  }
}

// Para dzieli sie po skladowych, wiec zero w KTOREJKOLWIEK skladowej dzielnika robi NULL z calej
// pary - tak jak dla int i rational. Do #267 straznik polykal pary catch-allem, a zero w skladowej
// konczylo proces sygnalem SIGFPE na x86-64. Asercja wykonuje sie tylko wtedy, gdy proces zyje.
TEST(xExpressionEval, divide_intpair_by_zero_component_yields_null) {
  // covers: expressionevaluator.expression-evaluator.10~1
  for (const auto &divisor : {std::pair<int, int>(3, 0), std::pair<int, int>(0, 2)}) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, rdb::descFldVT(std::pair<int, int>(6, 4)));
    program.emplace_back(PUSH_VAL, rdb::descFldVT(divisor));
    program.emplace_back(DIVIDE);

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))))
        << divisor.first << "," << divisor.second;
  }
}

// IDXPAIR dzieli tylko skladowa liczbowa - napisu nikt nie dzieli, wiec o NULL decyduje zero w `second`.
TEST(xExpressionEval, divide_idxpair_by_zero_index_yields_null) {
  // covers: expressionevaluator.expression-evaluator.10~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::pair<std::string, int>("a", 6)));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::pair<std::string, int>("b", 0)));
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, malformed_stack_throws) {
  // covers: expressionevaluator.expression-evaluator.17~1, expressionevaluator.expression-evaluator.18~1
  std::list<token> program;
  program.emplace_back(ADD);

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

TEST(xExpressionEval, cmp_equal_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_equal_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 4);
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_not_equal_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 4);
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_lt_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 2 < 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_lt_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 3 < 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_gt_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 3 > 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_gt_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 2 > 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_le_true_less) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 2 <= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_le_true_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 3 <= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_le_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 3 <= 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_ge_true_greater) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 3 >= 2

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_ge_true_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 3 >= 3

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_ge_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 2 >= 3

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
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, mul_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(MULTIPLY);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, div_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// --- neg ---

TEST(xExpressionEval, neg_int) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 7);
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), -7);
}

TEST(xExpressionEval, neg_null_propagates_null) {
  // covers: expressionevaluator.expression-evaluator.9~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// --- null propagation in comparisons ---

TEST(xExpressionEval, cmp_equal_null_propagates_null) {
  // covers: expressionevaluator.expression-evaluator.8~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, cmp_not_equal_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // 3 != 3

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
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, or_false_false_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(OR);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, and_true_true_is_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(AND);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

// --- error handling ---

TEST(xExpressionEval, call_unsupported_function_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0);
  program.emplace_back(CALL, std::string("unsupported_fn"));

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

TEST(xExpressionEval, empty_program_throws) {
  // covers: expressionevaluator.expression-evaluator.18~1
  std::list<token> program;

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

TEST(xExpressionEval, sub_string_string_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("a")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("b")));
  program.emplace_back(SUBTRACT);

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

TEST(xExpressionEval, mul_string_string_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("a")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("b")));
  program.emplace_back(MULTIPLY);

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

TEST(xExpressionEval, div_string_string_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("a")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("b")));
  program.emplace_back(DIVIDE);

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

// --- toLogicValue: string and pair handling ---

TEST(xExpressionEval, not_nonempty_string_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("hello")));
  program.emplace_back(NOT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, not_empty_string_is_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("")));
  program.emplace_back(NOT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, and_string_int_uses_string_truthiness) {
  // covers: expressionevaluator.expression-evaluator.11~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("text")));
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(AND);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  // "text" is truthy, 1 is truthy → true; logic result over a string is INTEGER
  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, not_intpair_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::pair<int, int>(1, 2)));
  program.emplace_back(NOT);

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

// --- PUSH_ID from payload ---

TEST(xExpressionEval, push_id_reads_value_from_payload) {
  // covers: expressionevaluator.expression-evaluator.2~1, expressionevaluator.expression-evaluator.15~1
  // covers: streaminstance.stream-instance.6~1
  auto desc = rdb::Descriptor("x", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);
  p.setItem(0, 42);

  std::list<token> program;
  program.emplace_back(PUSH_ID, rdb::descFldVT(std::pair<std::string, int>("x", 0)));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program, &p));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 42);
}

TEST(xExpressionEval, push_id_null_payload_throws) {
  // covers: expressionevaluator.expression-evaluator.2~1
  std::list<token> program;
  program.emplace_back(PUSH_ID, rdb::descFldVT(std::pair<std::string, int>("x", 0)));

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program, nullptr), rdb::Errc::Eval);
}

// --- PUSH_ID2 ---

TEST(xExpressionEval, push_id2_reads_value_from_payload) {
  // covers: expressionevaluator.expression-evaluator.15~1
  auto desc = rdb::Descriptor("x", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);
  p.setItem(0, 99);

  std::list<token> program;
  program.emplace_back(PUSH_ID2, rdb::descFldVT(std::string("x[0]")));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program, &p));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 99);
}

TEST(xExpressionEval, push_id2_null_payload_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_ID2, rdb::descFldVT(std::string("x[0]")));

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program, nullptr), rdb::Errc::Eval);
}

TEST(xExpressionEval, push_id2_malformed_identifier_throws) {
  auto desc = rdb::Descriptor("x", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);
  p.setItem(0, 1);

  std::list<token> program;
  program.emplace_back(PUSH_ID2, rdb::descFldVT(std::string("fieldname")));

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program, &p), rdb::Errc::Eval);
}

// --- PUSH_IDX ---

TEST(xExpressionEval, push_idx_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_IDX, 0);

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

// --- stack errors ---

TEST(xExpressionEval, too_many_values_on_stack_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(PUSH_VAL, 2);

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

// --- CALL functions ---

// Nazwy pisane wielką literą to POSTAĆ, KTÓRĄ NAPRAWDĘ PRODUKUJE PARSER: `exitFunction_call`
// wkłada do tokena postać kanoniczną z `rqlFunctions.hpp`, a tam `Sqrt`, `Ceil` i `Floor` stoją
// właśnie tak. Pozostałe testy w tym pliku fabrykują nazwy małymi literami, czyli sprawdzają
// samo złożenie wielkości liter w ewaluatorze, a nie postać, którą dostaje z parsera.
TEST(xExpressionEval, call_function_name_as_grammar_writes_it) {
  // covers: expressionevaluator.expression-evaluator.12~1
  const std::vector<std::pair<std::string, double>> cases{{"Sqrt", 2.0}, {"Ceil", 4.0}, {"Floor", 4.0}};

  for (const auto &[name, expected] : cases) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, 4.0);
    program.emplace_back(CALL, name);

    expressionEvaluator test;
    rdb::descFldVT result = rdbtest::ok(test.eval(program));

    ASSERT_TRUE(std::holds_alternative<double>(result)) << name;
    EXPECT_EQ(std::get<double>(result), expected) << name;
  }
}

// Złożenie wielkości liter nie może zamienić nieznanej funkcji w znaną ani zamazać nazwy
// w komunikacie.
//
// Od 2026-08-30 ta ścieżka jest z RQL NIEOSIĄGALNA: `compiler::checkFunctionCalls()` odrzuca
// nieznaną nazwę na kompilacji, więc plan z nią nie dochodzi do wykonania. Odmowa zostaje jako
// kontrola dla programów tokenów budowanych wprost, tak jak tutaj. `Crc` był w gramatyce do
// tej daty i został usunięty razem z ośmioma innymi nazwami bez implementacji. Błąd wraca
// wartością Errc::Eval (do 2026-10 rzutem std::runtime_error).
TEST(xExpressionEval, call_unknown_function_keeps_author_spelling) {
  // covers: expressionevaluator.expression-evaluator.18~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 4.0);
  program.emplace_back(CALL, std::string("Crc"));

  expressionEvaluator test;
  const auto result = test.eval(program);
  ASSERT_RDB_ERROR(result, rdb::Errc::Eval);
  EXPECT_EQ(result.error().message(), "Unsupported function call: Crc");
}

// Funkcje dopisane 2026-08-30. `Abs` liczy się wprost na wariancie, żeby nie tracić dokładności
// wartości wymiernej na okrążeniu przez double, które robi callFun. `IsZero` i `IsNonZero`
// zwracają INTEGER 0/1 niezależnie od typu argumentu - to jest ich cała wartość użytkowa, bo
// porównania żyją w regule `term_logic` i nie są dostępne wewnątrz wyrażenia w SELECT.
TEST(xExpressionEval, call_abs_preserves_type) {
  const std::vector<std::pair<int, int>> intCases{{-9, 9}, {9, 9}, {0, 0}};
  for (const auto &[input, expected] : intCases) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, input);
    program.emplace_back(CALL, std::string("Abs"));

    expressionEvaluator test;
    rdb::descFldVT result = rdbtest::ok(test.eval(program));
    ASSERT_TRUE(std::holds_alternative<int>(result)) << input;
    EXPECT_EQ(std::get<int>(result), expected) << input;
  }

  // Wartość wymierna ma wyjść dokładnie ta sama, bez przybliżenia przez Rationalize.
  std::list<token> program;
  program.emplace_back(PUSH_VAL, boost::rational<int>(-22, 7));
  program.emplace_back(CALL, std::string("Abs"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));
  ASSERT_TRUE(std::holds_alternative<boost::rational<int>>(result));
  EXPECT_EQ(std::get<boost::rational<int>>(result), boost::rational<int>(22, 7));
}

TEST(xExpressionEval, call_iszero_and_isnonzero_return_integer) {
  // covers: expressionevaluator.expression-evaluator.12~1, expressionevaluator.expression-evaluator.13~1
  const std::vector<std::tuple<int, int, int>> cases{{0, 1, 0}, {5, 0, 1}, {-5, 0, 1}};

  for (const auto &[input, zeroExpected, nonZeroExpected] : cases) {
    for (const auto &[name, expected] :
         std::vector<std::pair<std::string, int>>{{"IsZero", zeroExpected}, {"IsNonZero", nonZeroExpected}}) {
      std::list<token> program;
      program.emplace_back(PUSH_VAL, input);
      program.emplace_back(CALL, name);

      expressionEvaluator test;
      rdb::descFldVT result = rdbtest::ok(test.eval(program));
      ASSERT_TRUE(std::holds_alternative<int>(result)) << name << "(" << input << ")";
      EXPECT_EQ(std::get<int>(result), expected) << name << "(" << input << ")";
    }
  }
}

// Length liczy WARTOŚĆ, nie zadeklarowaną szerokość pola: payload::getItemVT przycina pole
// STRING na pierwszym bajcie zerowym, więc `STRING[8]` z wartością `42` dochodzi do ewaluatora
// jako napis dwuznakowy. Wynik jest INTEGER niezależnie od szerokości pola źródłowego.
TEST(xExpressionEval, call_length_counts_value_not_field_width) {
  // covers: expressionevaluator.expression-evaluator.12~1, expressionevaluator.expression-evaluator.13~1
  const std::vector<std::pair<std::string, int>> cases{{"", 0}, {"42", 2}, {"hello", 5}};

  for (const auto &[input, expected] : cases) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, input);
    program.emplace_back(CALL, std::string("Length"));

    expressionEvaluator test;
    rdb::descFldVT result = rdbtest::ok(test.eval(program));
    ASSERT_TRUE(std::holds_alternative<int>(result)) << "Length('" << input << "')";
    EXPECT_EQ(std::get<int>(result), expected) << "Length('" << input << "')";
  }
}

// Argument nietekstowy jest błędem, a nie cichą konwersją przez to_string - symetrycznie do
// `Abs` i `IsZero`, które odrzucają napis. `Length(k)` nad polem INTEGER jest niemal na pewno
// literówką; kto chce długości zapisu dziesiętnego, pisze `Length(to_string(k))`.
TEST(xExpressionEval, call_length_rejects_numeric_operand) {
  // covers: expressionevaluator.expression-evaluator.13~1
  for (const auto &value : std::vector<rdb::descFldVT>{7, 2.5, boost::rational<int>(22, 7)}) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, value);
    program.emplace_back(CALL, std::string("Length"));

    expressionEvaluator test;
    EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval) << "indeks " << value.index();
  }
}

// NULL pochłania: predykat na braku danych jest brakiem danych, a nie fałszem.
TEST(xExpressionEval, new_functions_propagate_null) {
  // covers: expressionevaluator.expression-evaluator.13~1
  for (const char *name : {"Abs", "IsZero", "IsNonZero", "Length"}) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, rdb::descFldVT{std::monostate{}});
    program.emplace_back(CALL, std::string(name));

    expressionEvaluator test;
    rdb::descFldVT result = rdbtest::ok(test.eval(program));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(result)) << name;
  }
}

TEST(xExpressionEval, call_sqrt_function_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 4.0);
  program.emplace_back(CALL, std::string("sqrt"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 2.0);
}

TEST(xExpressionEval, call_round_function_double) {
  // covers: expressionevaluator.expression-evaluator.13~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.5);
  program.emplace_back(CALL, std::string("round"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 2.0);
}

TEST(xExpressionEval, call_trunc_function_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.9);
  program.emplace_back(CALL, std::string("trunc"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 1.0);
}

TEST(xExpressionEval, call_sin_zero) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0.0);
  program.emplace_back(CALL, std::string("sin"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 0.0);
}

TEST(xExpressionEval, call_cos_zero) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0.0);
  program.emplace_back(CALL, std::string("cos"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 1.0);
}

// Argument RATIONAL jest tu policzony CELOWO, mimo ze kompilator odrzuca te trzy nazwy nad
// RATIONAL (rejects_irrational_functions_over_a_rational_value w test_compiler). Bramka jest
// decyzja o kontrakcie JEZYKA i stoi w expressionShape; ewaluator wykonuje takze programy
// skladane z pominieciem kompilatora, a rachunek przez double jest dla nich poprawny. Gdyby
// ktos przeniosl bramke pietro nizej, ten test zapali sie pierwszy i pokaze, ze zmienil sie
// kontrakt, a nie tylko miejsce sprawdzenia.
TEST(xExpressionEval, real_functions_return_double_for_exact_arguments) {
  // covers: expressionevaluator.expression-evaluator.12~1, expressionevaluator.expression-evaluator.13~1
  for (const auto &[name, expected] :
       std::vector<std::pair<std::string, double>>{{"sin", std::sin(1.0)}, {"cos", std::cos(1.0)}, {"exp", std::exp(1.0)}}) {
    for (const auto &argument : {rdb::descFldVT{1}, rdb::descFldVT{boost::rational<int>(1)}}) {
      std::list<token> program;
      program.emplace_back(PUSH_VAL, argument);
      program.emplace_back(CALL, name);
      expressionEvaluator evaluator;
      const auto result = rdbtest::ok(evaluator.eval(program));
      ASSERT_TRUE(std::holds_alternative<double>(result)) << name;
      EXPECT_NEAR(std::get<double>(result), expected, 1e-12) << name;
    }
  }
}

TEST(xExpressionEval, real_functions_propagate_null_and_reject_text) {
  // covers: expressionevaluator.expression-evaluator.13~1
  for (const char *name : {"sin", "cos", "exp"}) {
    std::list<token> missing;
    missing.emplace_back(PUSH_VAL, rdb::descFldVT{std::monostate{}});
    missing.emplace_back(CALL, std::string(name));
    std::list<token> text;
    text.emplace_back(PUSH_VAL, std::string("text"));
    text.emplace_back(CALL, std::string(name));
    expressionEvaluator evaluator;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(evaluator.eval(missing)))) << name;
    EXPECT_RDB_ERROR(evaluator.eval(text), rdb::Errc::Eval) << name;
  }
}

TEST(xExpressionEval, exp_overflow_returns_null) {
  // covers: expressionevaluator.expression-evaluator.13~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1000.0);
  program.emplace_back(CALL, std::string("exp"));
  expressionEvaluator evaluator;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(evaluator.eval(program))));
}

TEST(xExpressionEval, call_tan_zero) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 0.0);
  program.emplace_back(CALL, std::string("tan"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 0.0);
}

TEST(xExpressionEval, call_log_of_one) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0);
  program.emplace_back(CALL, std::string("log"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 0.0);
}

TEST(xExpressionEval, call_log2_of_one) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0);
  program.emplace_back(CALL, std::string("log2"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 0.0);
}

TEST(xExpressionEval, call_function_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("sqrt"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, call_function_on_string_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("text")));
  program.emplace_back(CALL, std::string("floor"));

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

// --- null propagation in remaining comparisons ---

TEST(xExpressionEval, cmp_lt_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, cmp_gt_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, cmp_le_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, cmp_ge_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, cmp_not_equal_null_propagates_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// --- neg on string ---

TEST(xExpressionEval, neg_string_throws) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("hello")));
  program.emplace_back(NEGATE);

  expressionEvaluator test;
  EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
}

// --- 3VL: null AND null, null OR null ---

TEST(xExpressionEval, and_null_null_is_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(AND);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, or_null_null_is_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(OR);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// --- string comparisons ---

TEST(xExpressionEval, cmp_equal_string_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_equal_string_not_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// --- float arithmetic ---

TEST(xExpressionEval, add_float_float) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0F);
  program.emplace_back(PUSH_VAL, 2.0F);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

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
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<unsigned>(result));
  EXPECT_EQ(std::get<unsigned>(result), 7U);
}

// Arytmetyka UINT poza [0, 2^32) daje NULL, tak jak przepelnienie INTEGER. Do 2026-09-26
// zawijala sie po cichu: `3 - 5` dawalo 4294967294.
TEST(xExpressionEval, uint_sub_below_zero_is_null) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3U);
  program.emplace_back(PUSH_VAL, 5U);
  program.emplace_back(SUBTRACT);

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

TEST(xExpressionEval, uint_add_and_mul_overflow_is_null) {
  // covers: expressionevaluator.expression-evaluator.19~1
  for (const auto op : {ADD, MULTIPLY}) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, 4000000000U);
    program.emplace_back(PUSH_VAL, 400000000U);
    program.emplace_back(op);

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program)))) << GetStringcommand_id(op);
  }
}

TEST(xExpressionEval, uint_arithmetic_at_the_bound_is_exact) {
  // covers: expressionevaluator.expression-evaluator.19~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 65535U);
  program.emplace_back(PUSH_VAL, 65537U);
  program.emplace_back(MULTIPLY);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<unsigned>(result));
  EXPECT_EQ(std::get<unsigned>(result), 4294967295U);
}

TEST(xExpressionEval, isnull_returns_1_for_null) {
  // covers: expressionevaluator.expression-evaluator.12~1, expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("isnull"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, isnull_returns_0_for_non_null) {
  // covers: expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 42);
  program.emplace_back(CALL, std::string("isnull"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// null2zero zamienia wartosc POCHLANIAJACA na zero: dziure w danych, dzielenie przez zero
// albo okno bez ani jednej wartosci. Ogona strumienia nie dotyczy - tam nie ma rekordu,
// wiec nie ma czego zamieniac (patrz query::startupLatency).
TEST(xExpressionEval, null2zero_replaces_null_with_zero) {
  // covers: expressionevaluator.expression-evaluator.12~1, expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("null2zero"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// Wartosc nie-NULL przechodzi BEZ ZMIANY - takze co do typu, zeby zawiniecie pola nie
// przycinalo wyniku okna z RATIONAL do liczby calkowitej.
TEST(xExpressionEval, null2zero_passes_a_value_through_unchanged) {
  // covers: expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, boost::rational<int>(7, 2));
  program.emplace_back(CALL, std::string("null2zero"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<boost::rational<int>>(result));
  EXPECT_EQ(std::get<boost::rational<int>>(result), boost::rational<int>(7, 2));
}

// --- string comparison operators (CMP_NOT_EQUAL, CMP_LT, CMP_GT, CMP_LE, CMP_GE) ---
// Wynik porównania stringów jest INTEGER 1/0 (do 2026-09-26 napis "1"/"0"),
// porównanie leksykograficzne zgodnie z std::string::operator<.

TEST(xExpressionEval, cmp_not_equal_string_not_equal) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "abc" != "xyz"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_not_equal_string_equal) {
  // covers: expressionevaluator.expression-evaluator.8~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_NOT_EQUAL);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "abc" != "abc"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_lt_string_true) {
  // covers: expressionevaluator.expression-evaluator.8~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "abc" < "xyz"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_lt_string_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_LT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "xyz" < "abc"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_gt_string_true) {
  // covers: expressionevaluator.expression-evaluator.8~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "xyz" > "abc"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_gt_string_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_GT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "abc" > "xyz"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_le_string_true_less) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "abc" <= "xyz"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_le_string_true_equal) {
  // covers: expressionevaluator.expression-evaluator.8~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "abc" <= "abc"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_le_string_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_LE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "xyz" <= "abc"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

TEST(xExpressionEval, cmp_ge_string_true_greater) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "xyz" >= "abc"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_ge_string_true_equal) {
  // covers: expressionevaluator.expression-evaluator.8~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "abc" >= "abc"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, cmp_ge_string_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("abc")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("xyz")));
  program.emplace_back(CMP_GE);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "abc" >= "xyz"

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// --- OR z operandem string ---
// toLogicValue(string) = !string.empty(); wynik logiczny nad napisem jest INTEGER 1/0.

TEST(xExpressionEval, or_nonempty_string_false_is_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("text")));
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(OR);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "text" OR 0 → true, typ INTEGER

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, or_empty_string_false_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("")));
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(OR);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "" OR 0 → false, typ INTEGER

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// Porownanie napisow jako operand logiki. Dopoki porownanie oddawalo napis "0", toLogicValue()
// czytalo go jako prawde (napis niepusty): NOT ('a' = 'b') dawalo falsz, a ('a' = 'b') OR 0 - prawde.
TEST(xExpressionEval, not_of_false_string_comparison_is_true) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("a")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("b")));
  program.emplace_back(CMP_EQUAL);
  program.emplace_back(NOT);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 1);
}

TEST(xExpressionEval, or_of_false_string_comparison_is_false) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("a")));
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("b")));
  program.emplace_back(CMP_EQUAL);
  program.emplace_back(PUSH_VAL, 0);
  program.emplace_back(OR);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// --- ADD: string + float i string + double przez normalize/castFldVT ---
// normalize(string, T): string.index()=8 > T.index() → T rzutowane na STRING przez std::to_string.
// Kolejność push: float/double pierwszy, string drugi → na stosie string jest na wierzchu,
// więc po pop: a=string, b=float → normalize(string, float) → konkatenacja.

TEST(xExpressionEval, add_string_float_converts_via_to_string) {
  // covers: expressionevaluator.expression-evaluator.3~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0F);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("unit")));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "unit" + std::to_string(1.0f)

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), std::to_string(1.0F) + "unit");
}

TEST(xExpressionEval, add_string_double_converts_via_to_string) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2.0);
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("val")));
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));  // "val" + std::to_string(2.0)

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), std::to_string(2.0) + "val");
}

// --- to_integer ---

TEST(xExpressionEval, call_to_integer_from_string) {
  // covers: expressionevaluator.expression-evaluator.12~1, expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("42")));
  program.emplace_back(CALL, std::string("to_integer"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 42);
}

TEST(xExpressionEval, call_to_integer_from_double_truncates) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 3.9);
  program.emplace_back(CALL, std::string("to_integer"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 3);
}

TEST(xExpressionEval, call_to_integer_null_propagates_null) {
  // covers: expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("to_integer"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// Wartosc, ktorej INTEGER nie pomiesci, nie ma wyniku: NULL, jak przepelnienie arytmetyki.
// Na x86-64 bylo to INT_MIN (zachowanie nieokreslone), w a0082a34 INT_MAX (nasycenie).
TEST(xExpressionEval, call_to_integer_out_of_range_is_null) {
  const std::vector<double> cases{1e30, -1e30, std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::quiet_NaN()};
  for (const double in : cases) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, in);
    program.emplace_back(CALL, std::string("to_integer"));

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program)))) << in;
  }
}

// Funkcja matematyczna nad INTEGER wraca przez double do INTEGER (callFun). Wynik bez
// odpowiednika w INTEGER - NaN z Sqrt(-4) i log(-1), -inf z log(0) - jest NULL-em, tak samo
// jak niefinitywny wynik `^` i sin/cos/exp. Przedtem x86-64 oddawalo tu INT_MIN.
TEST(xExpressionEval, call_math_over_integer_without_integer_result_is_null) {
  const std::vector<std::pair<std::string, int>> cases{{"Sqrt", -4}, {"log", 0}, {"log", -1}, {"log2", 0}};
  for (const auto &[name, arg] : cases) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, arg);
    program.emplace_back(CALL, name);

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program)))) << name << "(" << arg << ")";
  }
}

// --- to_float ---

TEST(xExpressionEval, call_to_float_from_string) {
  // covers: expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("2.5")));
  program.emplace_back(CALL, std::string("to_float"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<float>(result));
  EXPECT_EQ(std::get<float>(result), 2.5F);
}

TEST(xExpressionEval, call_to_float_from_int) {
  // covers: expressionevaluator.expression-evaluator.12~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 5);
  program.emplace_back(CALL, std::string("to_float"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<float>(result));
  EXPECT_EQ(std::get<float>(result), 5.0F);
}

TEST(xExpressionEval, call_to_float_null_propagates_null) {
  // covers: expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("to_float"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// --- to_double ---

TEST(xExpressionEval, call_to_double_from_string) {
  // covers: expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("1.5")));
  program.emplace_back(CALL, std::string("to_double"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 1.5);
}

TEST(xExpressionEval, call_to_double_from_int) {
  // covers: expressionevaluator.expression-evaluator.12~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 7);
  program.emplace_back(CALL, std::string("to_double"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 7.0);
}

TEST(xExpressionEval, call_to_double_null_propagates_null) {
  // covers: expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("to_double"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// --- to_string ---

TEST(xExpressionEval, call_to_string_from_int) {
  // covers: expressionevaluator.expression-evaluator.12~1, expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 42);
  program.emplace_back(CALL, std::string("to_string"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "42");
}

TEST(xExpressionEval, call_to_string_from_float) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 1.0F);
  program.emplace_back(CALL, std::string("to_string"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), std::to_string(1.0F));
}

TEST(xExpressionEval, call_to_string_identity) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::string("hello")));
  program.emplace_back(CALL, std::string("to_string"));

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<std::string>(result));
  EXPECT_EQ(std::get<std::string>(result), "hello");
}

TEST(xExpressionEval, call_to_string_null_propagates_null) {
  // covers: expressionevaluator.expression-evaluator.14~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, rdb::descFldVT(std::monostate{}));
  program.emplace_back(CALL, std::string("to_string"));

  expressionEvaluator test;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
}

// --- POWER: operator `^` z RQL.g4 (ExpPow) ------------------------------------------------

// Kolejnosc operandow. ONP `PUSH 2; PUSH 3; POWER` ma znaczyc 2^3 = 8, a nie 3^2 = 9.
// Potegowanie jest pierwszym NIEPRZEMIENNYM operatorem w tym pliku, ktory nie ma
// osobnego znaku po obu stronach, wiec pomylka stron przeszlaby przez kazdy inny test.
TEST(xExpressionEval, pow_operand_order) {
  // covers: streaminstance.stream-instance.6~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 3);
  program.emplace_back(POWER);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 8);
}

// Typ wyniku ustala normalize(), tak samo jak dla `*`. Kwadrat pola INTEGER zostaje
// INTEGER-em - inaczej `pole ^ 2` nie bylby zamiennikiem dla `pole * pole`.
TEST(xExpressionEval, pow_int_int_stays_integer) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 7);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(POWER);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  EXPECT_EQ(result.index(), rdb::INTEGER);
  EXPECT_EQ(std::get<int>(result), 49);
}

// Wykladnik ulamkowy promuje wynik do DOUBLE - normalize() bierze wyzszy indeks wariantu.
TEST(xExpressionEval, pow_int_double_promotes_to_double) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, 0.5);
  program.emplace_back(POWER);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_EQ(result.index(), rdb::DOUBLE);
  EXPECT_NEAR(std::get<double>(result), 1.4142135623730951, 1E-12);
}

// Podstawa calkowita obcina wynik, dokladnie jak dzielenie calkowite: 2^-1 = 0.5 -> 0.
TEST(xExpressionEval, pow_negative_exponent_truncates_on_integer_base) {
  // covers: expressionevaluator.expression-evaluator.7~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(PUSH_VAL, -1);
  program.emplace_back(POWER);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 0);
}

// Wynik poza zbiorem wartosci jest wartoscia POCHLANIAJACA, jak dzielenie przez zero.
// Rzut nieskonczonosci na int bylby zreszta zachowaniem niezdefiniowanym.
TEST(xExpressionEval, pow_non_finite_result_gives_null) {
  // covers: expressionevaluator.expression-evaluator.7~1
  for (const auto &operands : {std::pair<rdb::descFldVT, rdb::descFldVT>{0, -1},         // 0^-1 = inf
                               std::pair<rdb::descFldVT, rdb::descFldVT>{-8.0, 0.5}}) {  // (-8)^0.5 = NaN
    std::list<token> program;
    program.emplace_back(PUSH_VAL, operands.first);
    program.emplace_back(PUSH_VAL, operands.second);
    program.emplace_back(POWER);

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
  }
}

TEST(xExpressionEval, pow_null_operand_propagates_null) {
  for (bool nullOnLeft : {true, false}) {
    std::list<token> program;
    program.emplace_back(PUSH_VAL, nullOnLeft ? rdb::descFldVT(std::monostate{}) : rdb::descFldVT(2));
    program.emplace_back(PUSH_VAL, nullOnLeft ? rdb::descFldVT(2) : rdb::descFldVT(std::monostate{}));
    program.emplace_back(POWER);

    expressionEvaluator test;
    EXPECT_TRUE(std::holds_alternative<std::monostate>(rdbtest::ok(test.eval(program))));
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
    EXPECT_RDB_ERROR(test.eval(program), rdb::Errc::Eval);
  }
}

// Sciezka dokladna: dla typow calkowitych i wymiernych `a^k` MA BYC iloczynem `a*a*...*a`,
// razem z przekreceniem i promocja typu. Na tej rownosci stoi regula D w exprSimplify,
// ktora przepisuje `a*a` na `a^2`.
TEST(xExpressionEval, pow_on_exact_types_matches_multiplication) {
  // covers: expressionevaluator.expression-evaluator.7~1
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
    EXPECT_TRUE(rdbtest::ok(test.eval(asPower)) == rdbtest::ok(test.eval(asProduct))) << "wykladnik " << item.exponent;
  }
}

// Przepelnienie arytmetyki wartosci (2026-09-14). INTEGER to int32, RATIONAL to
// boost::rational<int>; wynik spoza zakresu jest NULL, tak jak dzielenie przez zero - nie
// zawinieta liczba. Kazdy przypadek ma pare: wynik dokladnie na granicy i o krok za nia.
namespace {

rdb::descFldVT evalBinary(const rdb::descFldVT &a, const rdb::descFldVT &b, command_id op) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, a);
  program.emplace_back(PUSH_VAL, b);
  program.emplace_back(op);
  expressionEvaluator test;
  return rdbtest::ok(test.eval(program));
}

rdb::Result<rdb::descFldVT> evalUnaryResult(const rdb::descFldVT &a, const token &op) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, a);
  program.push_back(op);
  expressionEvaluator test;
  return test.eval(program);
}

rdb::descFldVT evalUnary(const rdb::descFldVT &a, const token &op) { return rdbtest::ok(evalUnaryResult(a, op)); }

bool isNull(const rdb::descFldVT &value) { return std::holds_alternative<std::monostate>(value); }

constexpr int intMax = std::numeric_limits<int>::max();
constexpr int intMin = std::numeric_limits<int>::min();

}  // namespace

TEST(xExpressionEval, int_overflow_returns_null) {
  // covers: expressionevaluator.expression-evaluator.19~1
  EXPECT_EQ(evalBinary(7, 300000000, MULTIPLY), rdb::descFldVT{2100000000});
  EXPECT_TRUE(isNull(evalBinary(7, 1000000000, MULTIPLY)));
  EXPECT_EQ(evalBinary(-2, 1073741824, MULTIPLY), rdb::descFldVT{intMin});
  EXPECT_TRUE(isNull(evalBinary(2, 1073741824, MULTIPLY)));

  EXPECT_EQ(evalBinary(intMax - 7, 7, ADD), rdb::descFldVT{intMax});
  EXPECT_TRUE(isNull(evalBinary(intMax - 6, 7, ADD)));
  EXPECT_EQ(evalBinary(intMin + 3, 3, SUBTRACT), rdb::descFldVT{intMin});
  EXPECT_TRUE(isNull(evalBinary(intMin + 2, 3, SUBTRACT)));

  EXPECT_EQ(evalBinary(intMin, 1, DIVIDE), rdb::descFldVT{intMin});
  EXPECT_TRUE(isNull(evalBinary(intMin, -1, DIVIDE)));

  EXPECT_EQ(evalUnary(intMin + 1, token(NEGATE)), rdb::descFldVT{intMax});
  EXPECT_TRUE(isNull(evalUnary(intMin, token(NEGATE))));
  EXPECT_EQ(evalUnary(intMin + 1, token(CALL, std::string("Abs"))), rdb::descFldVT{intMax});
  EXPECT_TRUE(isNull(evalUnary(intMin, token(CALL, std::string("Abs")))));

  // `^` nad typem dokladnym jest iloczynem, wiec dziedziczy te sama kontrole.
  EXPECT_EQ(evalBinary(2, 30, POWER), rdb::descFldVT{1073741824});
  EXPECT_TRUE(isNull(evalBinary(2, 31, POWER)));

  // BYTE promuje sie do int, wiec 255*255 nie jest przepelnieniem.
  EXPECT_EQ(evalBinary(uint8_t(255), uint8_t(255), MULTIPLY), rdb::descFldVT{65025});
}

// Zerowy mianownik nie dochodzi do gcd. gcd(0, 0) to zero, a dzielenie przez nie konczy sie inaczej na
// kazdej architekturze: SIGFPE na x86-64, cichy 0 na arm64 i dopiero potem wyjatek z konstruktora.
// Wejscie takie powstawalo z ulamka 0/0 wczytanego z surowych bajtow (payload::getItemVT), ale `div`
// dochodzi tu tez wprost, gdy licznik dzielnika jest zerem.
TEST(xExpressionEval, narrowed_rejects_zero_denominator) {
  EXPECT_FALSE(checkedArith::detail::narrowed(0, 0).has_value());
  EXPECT_FALSE(checkedArith::detail::narrowed(7, 0).has_value());

  // Mianownik niezerowy liczy sie jak dotad - straz nie zabiera poprawnej drogi.
  EXPECT_EQ(checkedArith::detail::narrowed(6, 4), boost::rational<int>(3, 2));
}

TEST(xExpressionEval, rational_overflow_returns_null) {
  // covers: expressionevaluator.expression-evaluator.19~1
  using R = boost::rational<int>;

  EXPECT_EQ(evalBinary(R(9), R(200000000), MULTIPLY), rdb::descFldVT{R(1800000000)});
  EXPECT_TRUE(isNull(evalBinary(R(9), R(1000000000), MULTIPLY)));
  // Skrocenie na krzyz miesci wynik, choc iloczyn licznikow nie miesci sie w int32.
  EXPECT_EQ(evalBinary(R(2000000000, 3), R(3, 2000000000), MULTIPLY), rdb::descFldVT{R(1)});
  EXPECT_TRUE(isNull(evalBinary(R(9, 1000000000), R(1, 7), MULTIPLY)));

  EXPECT_EQ(evalBinary(R(9), R(1000000000), DIVIDE), rdb::descFldVT{R(9, 1000000000)});
  EXPECT_TRUE(isNull(evalBinary(R(9, 1000000000), R(7), DIVIDE)));

  EXPECT_EQ(evalBinary(R(intMax - 9), R(9), ADD), rdb::descFldVT{R(intMax)});
  EXPECT_TRUE(isNull(evalBinary(R(intMax - 8), R(9), ADD)));
  // Wspolny mianownik poza int32, ale wynik po skroceniu sie miesci: 1/65536 + 1/65536 = 1/32768.
  EXPECT_EQ(evalBinary(R(1, 65536), R(1, 65536), ADD), rdb::descFldVT{R(1, 32768)});
  EXPECT_TRUE(isNull(evalBinary(R(1, 65536), R(1, 65537), ADD)));
  EXPECT_EQ(evalBinary(R(intMin + 9), R(9), SUBTRACT), rdb::descFldVT{R(intMin)});
  EXPECT_TRUE(isNull(evalBinary(R(intMin + 8), R(9), SUBTRACT)));
  // Te same granice poza skrotem calkowitym (mianownik rozny od 1) liczy sciezka int64.
  EXPECT_EQ(evalBinary(R(3, 65536), R(1, 65536), SUBTRACT), rdb::descFldVT{R(1, 32768)});
  EXPECT_TRUE(isNull(evalBinary(R(1, 65536), R(1, 65537), SUBTRACT)));
  EXPECT_EQ(evalBinary(R(intMax, 2), R(intMax, 2), ADD), rdb::descFldVT{R(intMax)});

  EXPECT_TRUE(isNull(evalUnary(R(intMin), token(NEGATE))));
  EXPECT_TRUE(isNull(evalUnary(R(intMin), token(CALL, std::string("Abs")))));
  EXPECT_EQ(evalUnary(R(intMin + 1, 3), token(NEGATE)), rdb::descFldVT{R(intMax, 3)});

  // Po przepelnieniu wynik NULL pochlania dalsza arytmetyke, jak kazdy NULL.
  std::list<token> program;
  program.emplace_back(PUSH_VAL, R(9));
  program.emplace_back(PUSH_VAL, R(1000000000));
  program.emplace_back(MULTIPLY);
  program.emplace_back(PUSH_VAL, R(1000000000));
  program.emplace_back(DIVIDE);
  expressionEvaluator test;
  EXPECT_TRUE(isNull(rdbtest::ok(test.eval(program))));
}

// --- FLOAT na granicy 2^24: gdzie dokladnie gubi sie zaokraglenie ---
//
// Regresja it_self_ref_simplify_synth-value (macOS arm64, 2026-09-18): pole FLOAT
// `w[1] + 1 + 2` nad x = 2^24 dalo 16777220 zamiast 16777218, a wpis -shape tego samego
// testu PRZESZEDL, czyli plan pola nie zostal przepisany na `x + 3`. Roznica siedzi wiec
// ponizej kompilatora - w eval() albo w samym FPU. Sa dokladnie trzy mozliwe przyczyny
// i ponizsze testy rozdzielaja je tak, ze ten, ktory padnie, wskazuje winnego:
//
//   (1) tryb zaokraglania nie jest round-to-nearest-even  -> pada float_hardware_rounding_*
//   (2) normalize() podnosi FLOAT + INTEGER na DOUBLE     -> pada float_add_int_stays_float_*
//   (3) lancuch liczy sie w DOUBLE i rzutuje raz na koncu -> pada float_chain_rounds_*
//
// 2^24 = 16777216 jest ostatnia liczba calkowita, po ktorej FLOAT ma ULP = 2. 16777217 stoi
// w polowie miedzy 16777216 a 16777218, a 16777219 w polowie miedzy 16777218 a 16777220 -
// obie polowki rozstrzyga reguly ties-to-even, i to wlasnie ona odroznia wynik poprawny
// (16777218) od wyniku przez DOUBLE (16777220).

// (1) KONTROLA JEZYKA I SPRZETU: bez wariantu, bez eval(). `volatile` zdejmuje zwijanie
// stalych, wiec dodawanie dzieje sie w RUNTIME i pyta o faktyczny tryb zaokraglania.
TEST(xExpressionEval, float_hardware_rounding_is_ties_to_even_at_2p24) {
  volatile float base = 16777216.0F;  // 2^24
  volatile float one  = 1.0F;
  volatile float two  = 2.0F;

  const float afterOne = base + one;
  const float afterTwo = afterOne + two;

  EXPECT_EQ(afterOne, 16777216.0F) << "tryb zaokraglania FPU nie jest round-to-nearest-even";
  EXPECT_EQ(afterTwo, 16777218.0F) << "tryb zaokraglania FPU nie jest round-to-nearest-even";
}

// (2) JEDEN KROK PRZEZ WARIANT: FLOAT + INTEGER ma zostac FLOAT i zaokraglic sie TERAZ.
TEST(xExpressionEval, float_add_int_stays_float_at_2p24) {
  // covers: expressionevaluator.expression-evaluator.5~1
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 16777216.0F);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<float>(result))
      << "normalize() podniosl FLOAT + INTEGER ponad FLOAT; indeks wyniku: " << result.index();
  EXPECT_EQ(std::get<float>(result), 16777216.0F);
}

// (3) CALY LANCUCH Z PROGRAMU POLA: dokladnie to, co liczy it_self_ref_simplify_synth.
TEST(xExpressionEval, float_chain_rounds_at_every_step_at_2p24) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 16777216.0F);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(ADD);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<float>(result)) << "indeks wyniku: " << result.index();
  EXPECT_EQ(std::get<float>(result), 16777218.0F) << "16777220 znaczy DOUBLE w posredniku albo przepisanie na x+3";
}

// (3b) TEN SAM LANCUCH NAD SLOTEM FLOAT REKORDU - `w[1] + 1 + 2` jeden do jednego.
// Odroznia blad arytmetyki od bledu ODCZYTU slotu (gdyby PUSH_ID oddawal DOUBLE).
TEST(xExpressionEval, float_slot_chain_rounds_at_every_step_at_2p24) {
  auto desc = rdb::Descriptor("x", 4, 1, rdb::FLOAT);
  rdb::payload p(desc);
  p.setItemVT(0, rdb::descFldVT{16777216.0F});

  std::list<token> program;
  program.emplace_back(PUSH_ID, rdb::descFldVT(std::pair<std::string, int>("x", 0)));
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(ADD);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program, &p));

  ASSERT_TRUE(std::holds_alternative<float>(result)) << "indeks wyniku: " << result.index();
  EXPECT_EQ(std::get<float>(result), 16777218.0F);
}

// KONTRAST: ten sam lancuch w DOUBLE jest dokladny i konczy na 16777219. Zapis takiej
// wartosci do slotu FLOAT daje 16777220 - czyli dokladnie liczbe z regresji. Test stoi tu
// po to, zeby przy nastepnej awarii bylo widac, ze 16777220 to slad DOUBLE, nie przypadek.
TEST(xExpressionEval, double_chain_at_2p24_is_exact_and_narrows_to_16777220) {
  std::list<token> program;
  program.emplace_back(PUSH_VAL, 16777216.0);
  program.emplace_back(PUSH_VAL, 1);
  program.emplace_back(ADD);
  program.emplace_back(PUSH_VAL, 2);
  program.emplace_back(ADD);

  expressionEvaluator test;
  rdb::descFldVT result = rdbtest::ok(test.eval(program));

  ASSERT_TRUE(std::holds_alternative<double>(result));
  EXPECT_EQ(std::get<double>(result), 16777219.0);
  EXPECT_EQ(static_cast<float>(std::get<double>(result)), 16777220.0F);
}

// INTEGER z UINT (2026-09-27). Ujemny INTEGER nie ma reprezentacji w UINT, ale WYNIK operacji
// moze ja miec: para liczy sie dokladnie, a do UINT zawezany jest dopiero wynik. NULL zostaje
// tylko dla wyniku spoza zakresu. Od da67e5a3 do tej zmiany kazda operacja z ujemnym INTEGER
// dawala NULL, takze `10 + (-2)` i porownanie `-998 < 165`.
TEST(xExpressionEval, uint_with_negative_int_is_exact_when_result_fits) {
  // covers: expressionevaluator.expression-evaluator.6~1
  struct testCase {
    rdb::descFldVT a;
    rdb::descFldVT b;
    command_id op;
    unsigned expected;
  };
  for (const auto &item :
       {testCase{10U, -2, ADD, 8U}, testCase{-2, 10U, ADD, 8U}, testCase{10U, -2, SUBTRACT, 12U}, testCase{-2, 0U, MULTIPLY, 0U},
        testCase{1U, -2, DIVIDE, 0U}, testCase{3000000000U, -1, ADD, 2999999999U}}) {
    const auto result = evalBinary(item.a, item.b, item.op);
    ASSERT_TRUE(std::holds_alternative<unsigned>(result)) << GetStringcommand_id(item.op);
    EXPECT_EQ(std::get<unsigned>(result), item.expected) << GetStringcommand_id(item.op);
  }
}

// Wynik ujemny nie ma reprezentacji w UINT - NULL, a nie iloczyn liczby 4294966298, jak przed
// da67e5a3. NULL na wejsciu i dzielenie przez zero zostaja NULL.
TEST(xExpressionEval, uint_with_negative_int_is_null_when_result_does_not_fit) {
  // covers: expressionevaluator.expression-evaluator.6~1
  struct testCase {
    rdb::descFldVT a;
    rdb::descFldVT b;
    command_id op;
  };
  for (const auto &item :
       {testCase{-998, 165U, MULTIPLY}, testCase{-998, 165U, ADD}, testCase{1U, -2, ADD}, testCase{-2, 1U, SUBTRACT},
        testCase{10U, -2, DIVIDE}, testCase{-5, 0U, DIVIDE}, testCase{rdb::descFldVT{std::monostate{}}, -2, ADD}})
    EXPECT_TRUE(isNull(evalBinary(item.a, item.b, item.op))) << GetStringcommand_id(item.op);
}

// Porownanie jest dokladne i daje UINT 1/0, jak porownanie dwoch UINT. Porownania zyja tylko
// w RULE WHEN, wiec zaden zapisany artefakt nie zmienia typu.
TEST(xExpressionEval, comparison_of_uint_with_negative_int_is_exact) {
  // covers: expressionevaluator.expression-evaluator.6~1
  struct testCase {
    command_id op;
    unsigned expected;
  };
  for (const auto &item : {testCase{CMP_LT, 1U}, testCase{CMP_LE, 1U}, testCase{CMP_GT, 0U}, testCase{CMP_GE, 0U},
                           testCase{CMP_EQUAL, 0U}, testCase{CMP_NOT_EQUAL, 1U}}) {
    const auto result = evalBinary(-998, 165U, item.op);
    ASSERT_TRUE(std::holds_alternative<unsigned>(result)) << GetStringcommand_id(item.op);
    EXPECT_EQ(std::get<unsigned>(result), item.expected) << GetStringcommand_id(item.op);
  }
  const auto reversed = evalBinary(165U, -998, CMP_GT);
  ASSERT_TRUE(std::holds_alternative<unsigned>(reversed));
  EXPECT_EQ(std::get<unsigned>(reversed), 1U);
}

// `^` wedlug regul typow dokladnych: wykladnik nieujemny - iloczyn, ujemny - std::pow z obcieciem.
TEST(xExpressionEval, power_of_uint_with_negative_int_is_exact_when_result_fits) {
  // covers: expressionevaluator.expression-evaluator.6~1
  struct testCase {
    rdb::descFldVT base;
    rdb::descFldVT exponent;
    std::optional<unsigned> expected;
  };
  for (const auto &item : {testCase{-2, 2U, 4U}, testCase{-2, 3U, std::nullopt}, testCase{-2, 30U, 1073741824U},
                           testCase{-2, 32U, std::nullopt},  // 2^32 - poza zakresem UINT
                           testCase{-1, 4000000000U, 1U},    // bez petli na 4e9 krokow
                           testCase{-1, 4000000001U, std::nullopt}, testCase{2U, -1, 0U}, testCase{1U, -5, 1U},
                           testCase{0U, -1, std::nullopt}}) {  // nieskonczonosc
    const auto result = evalBinary(item.base, item.exponent, POWER);
    if (!item.expected.has_value()) {
      EXPECT_TRUE(isNull(result));
      continue;
    }
    ASSERT_TRUE(std::holds_alternative<unsigned>(result));
    EXPECT_EQ(std::get<unsigned>(result), *item.expected);
  }
}

// --- #328: `-a` i `~a` nad BYTE i UINT ---------------------------------------------------
//
// Decyzja wlasciciela: w typach bez liczb ujemnych `-a` jest negacja bitowa (neg()), a `~a`
// daje ten sam wynik. Granice: 0, 1 i wartosc najwieksza typu. NULL przechodzi bez zmian.
TEST(xExpressionEval, minus_and_bitwise_not_agree_on_byte_and_uint) {
  // covers: expressionevaluator.expression-evaluator.9~1, streaminstance.stream-instance.6~1
  for (const uint8_t a : {uint8_t{0}, uint8_t{1}, uint8_t{0x0f}, uint8_t{0xff}}) {
    const rdb::descFldVT expected{static_cast<uint8_t>(~a)};
    EXPECT_EQ(evalUnary(a, token(NEGATE)), expected) << int{a};
    EXPECT_EQ(evalUnary(a, token(BIT_NOT)), expected) << int{a};
  }
  for (const unsigned a : {0U, 1U, 0x0f0f0f0fU, std::numeric_limits<unsigned>::max()}) {
    const rdb::descFldVT expected{~a};
    EXPECT_EQ(evalUnary(a, token(NEGATE)), expected) << a;
    EXPECT_EQ(evalUnary(a, token(BIT_NOT)), expected) << a;
  }
  EXPECT_TRUE(isNull(evalUnary(std::monostate{}, token(NEGATE))));
  EXPECT_TRUE(isNull(evalUnary(std::monostate{}, token(BIT_NOT))));
}

// `~` nad innym typem odrzuca kompilator; ewaluator, do ktorego taki program mimo to dotarl,
// nie zgaduje wartosci, tylko zglasza blad - tak jak `negate` nad napisem.
TEST(xExpressionEval, bitwise_not_outside_byte_and_uint_throws) {
  // covers: expressionevaluator.expression-evaluator.9~1
  for (const rdb::descFldVT &a : {rdb::descFldVT{5}, rdb::descFldVT{boost::rational<int>(3, 2)}, rdb::descFldVT{2.5F},
                                  rdb::descFldVT{2.5}, rdb::descFldVT{std::string("ab")}})
    EXPECT_RDB_ERROR(evalUnaryResult(a, token(BIT_NOT)), rdb::Errc::Eval) << a.index();
}

// Kontrola z drugiej strony: `-a` nad typami ze znakiem zostaje arytmetyczne, a przepelnienie
// (`-INT_MIN`) daje NULL - neg() jest tym samym kodem, co przed #328.
TEST(xExpressionEval, unary_minus_stays_arithmetic_for_signed_types) {
  // covers: expressionevaluator.expression-evaluator.9~1
  EXPECT_EQ(evalUnary(5, token(NEGATE)), rdb::descFldVT{-5});
  EXPECT_EQ(evalUnary(2.5, token(NEGATE)), rdb::descFldVT{-2.5});
  EXPECT_EQ(evalUnary(2.5F, token(NEGATE)), rdb::descFldVT{-2.5F});
  EXPECT_TRUE(isNull(evalUnary(intMin, token(NEGATE))));
  EXPECT_RDB_ERROR(evalUnaryResult(std::string("ab"), token(NEGATE)), rdb::Errc::Eval);
}

TEST(xExpressionEval, call2_accepts_only_the_canonical_to_string_name) {
  // covers: expressionevaluator.expression-evaluator.12~1, streaminstance.stream-instance.6~1
  expressionEvaluator evaluator;
  EXPECT_EQ(rdbtest::ok(evaluator.eval({token(PUSH_VAL, 42), token(CALL2, std::pair<std::string, int>{"to_string", 8})})),
            rdb::descFldVT(std::string("42")));
  EXPECT_EQ(rdbtest::ok(
                evaluator.eval({token(PUSH_VAL, std::monostate{}), token(CALL2, std::pair<std::string, int>{"to_string", 8})})),
            rdb::descFldVT(std::monostate{}));
  for (const char *name : {"TO_STRING", "To_StRiNg", "Sqrt"})
    EXPECT_RDB_ERROR(evaluator.eval({token(PUSH_VAL, 1), token(CALL2, std::pair<std::string, int>{name, 8})}), rdb::Errc::Eval);
}

TEST(xExpressionEval, window_tokens_read_the_selected_group_and_reject_invalid_indices) {
  // covers: expressionevaluator.expression-evaluator.15~1, streaminstance.stream-instance.6~1
  expressionEvaluator evaluator;
  std::vector<windowStats> groups(2);
  groups[0] = {.minValue = -999, .maxValue = 999, .sumValue = 777, .avgValue = 555};
  groups[1] = {.minValue = -3, .maxValue = 7, .sumValue = 9, .avgValue = boost::rational<int>(9, 4)};
  const std::vector<std::pair<command_id, rdb::descFldVT>> cases{
      {WINDOW_MIN, -3}, {WINDOW_MAX, 7}, {WINDOW_SUM, 9}, {WINDOW_AVG, boost::rational<int>(9, 4)}};
  for (const auto &[op, expected] : cases) {
    EXPECT_EQ(rdbtest::ok(evaluator.eval({token(op, 1)}, nullptr, &groups)), expected);
    EXPECT_RDB_ERROR(evaluator.eval({token(op, 1)}), rdb::Errc::Eval);
    for (const int index : {-1, 2})
      EXPECT_RDB_ERROR(evaluator.eval({token(op, index)}, nullptr, &groups), rdb::Errc::Eval);
  }
}

TEST(xExpressionEval, three_valued_truth_tables_cover_both_operand_orders) {
  // covers: expressionevaluator.expression-evaluator.11~1, streaminstance.stream-instance.6~1
  const std::vector<rdb::descFldVT> values{0, 1, std::monostate{}};
  // Wiersze i kolumny: FALSE, TRUE, UNKNOWN; -1 oznacza UNKNOWN.
  constexpr int andTable[3][3]{{0, 0, 0}, {0, 1, -1}, {0, -1, -1}};
  constexpr int orTable[3][3]{{0, 1, -1}, {1, 1, 1}, {-1, 1, -1}};
  expressionEvaluator evaluator;
  for (size_t left = 0; left < 3; ++left)
    for (size_t right = 0; right < 3; ++right)
      for (const auto op : {AND, OR}) {
        const int expected = op == AND ? andTable[left][right] : orTable[left][right];
        const auto result =
            rdbtest::ok(evaluator.eval({token(PUSH_VAL, values[left]), token(PUSH_VAL, values[right]), token(op)}));
        EXPECT_EQ(result, expected < 0 ? rdb::descFldVT(std::monostate{}) : rdb::descFldVT(expected));
      }
}

TEST(xExpressionEval, pair_arithmetic_and_refusals_preserve_the_contract) {
  // covers: expressionevaluator.expression-evaluator.4~1, expressionevaluator.expression-evaluator.8~1
  // covers: expressionevaluator.expression-evaluator.10~1, expressionevaluator.expression-evaluator.11~1
  // covers: expressionevaluator.expression-evaluator.18~1
  using integers = std::pair<int, int>;
  using indexed  = std::pair<std::string, int>;
  expressionEvaluator evaluator;
  for (const auto &[op, expected] :
       std::vector<std::pair<command_id, integers>>{{ADD, {8, 12}}, {SUBTRACT, {4, 4}}, {MULTIPLY, {12, 32}}, {DIVIDE, {3, 2}}})
    EXPECT_EQ(rdbtest::ok(evaluator.eval({token(PUSH_VAL, integers{6, 8}), token(PUSH_VAL, integers{2, 4}), token(op)})),
              rdb::descFldVT(expected));
  for (const auto &[op, expected] : std::vector<std::pair<command_id, indexed>>{
           {ADD, {"leftright", 10}}, {SUBTRACT, {"?? -", 6}}, {MULTIPLY, {"?? *", 16}}, {DIVIDE, {"?? /", 4}}})
    EXPECT_EQ(
        rdbtest::ok(evaluator.eval({token(PUSH_VAL, indexed{"left", 8}), token(PUSH_VAL, indexed{"right", 2}), token(op)})),
        rdb::descFldVT(expected));
  EXPECT_EQ(rdbtest::ok(evaluator.eval({token(PUSH_VAL, integers{6, 8}), token(NEGATE)})), rdb::descFldVT(integers{-6, -8}));
  EXPECT_EQ(rdbtest::ok(evaluator.eval({token(PUSH_VAL, indexed{"left", 8}), token(NEGATE)})),
            rdb::descFldVT(indexed{"-left", -8}));
  for (const auto &value : std::vector<rdb::descFldVT>{integers{1, 2}, indexed{"x", 2}}) {
    for (const auto op : {POWER, CMP_EQUAL, CMP_NOT_EQUAL, CMP_LT, CMP_GT, CMP_LE, CMP_GE, AND, OR})
      EXPECT_RDB_ERROR(evaluator.eval({token(PUSH_VAL, value), token(PUSH_VAL, value), token(op)}), rdb::Errc::Eval);
    EXPECT_RDB_ERROR(evaluator.eval({token(PUSH_VAL, value), token(NOT)}), rdb::Errc::Eval);
  }
}

TEST(xExpressionEval, deep_stack_is_released_after_success_and_error) {
  // covers: expressionevaluator.expression-evaluator.16~1, expressionevaluator.expression-evaluator.18~1
  expressionEvaluator evaluator;
  std::list<token> program;
  for (int i = 0; i < 32; ++i)
    program.emplace_back(PUSH_VAL, std::string(64, 'x'));
  auto invalid = program;
  invalid.emplace_back(PUSH_STREAM, std::string("unsupported"));
  for (int i = 1; i < 32; ++i)
    program.emplace_back(ADD);
  EXPECT_EQ(rdbtest::ok(evaluator.eval(program)), rdb::descFldVT(std::string(2048, 'x')));
  EXPECT_RDB_ERROR(evaluator.eval(invalid), rdb::Errc::Eval);
  EXPECT_EQ(rdbtest::ok(evaluator.eval({token(PUSH_VAL, 7)})), rdb::descFldVT(7));
}

TEST(xExpressionEval, numeric_arithmetic_and_comparisons_cover_all_promotions) {
  // covers: expressionevaluator.expression-evaluator.3~1, expressionevaluator.expression-evaluator.4~1
  // covers: expressionevaluator.expression-evaluator.5~1, expressionevaluator.expression-evaluator.8~1
  // covers: expressionevaluator.expression-evaluator.17~1, streaminstance.stream-instance.6~1
  using value = rdb::descFldVT;
  using ratio = boost::rational<int>;
  const std::vector<value> left{uint8_t{6}, 6, 6U, ratio(6), 6.0F, 6.0};
  const std::vector<value> right{uint8_t{2}, 2, 2U, ratio(2), 2.0F, 2.0};
  // Tabela promocji z kontraktu typow, niezalezna od cast i normalize.
  constexpr int types[6][6]{{0, 1, 2, 3, 4, 5}, {1, 1, 2, 3, 4, 5}, {2, 2, 2, 3, 4, 5},
                            {3, 3, 3, 3, 4, 5}, {4, 4, 4, 4, 4, 5}, {5, 5, 5, 5, 5, 5}};
  const std::vector<std::pair<command_id, std::vector<value>>> cases{{ADD, {uint8_t{8}, 8, 8U, ratio(8), 8.0F, 8.0}},
                                                                     {SUBTRACT, {uint8_t{4}, 4, 4U, ratio(4), 4.0F, 4.0}},
                                                                     {MULTIPLY, {uint8_t{12}, 12, 12U, ratio(12), 12.0F, 12.0}},
                                                                     {DIVIDE, {uint8_t{3}, 3, 3U, ratio(3), 3.0F, 3.0}},
                                                                     {POWER, {uint8_t{36}, 36, 36U, ratio(36), 36.0F, 36.0}},
                                                                     {CMP_EQUAL, {uint8_t{0}, 0, 0U, ratio(0), 0.0F, 0.0}},
                                                                     {CMP_NOT_EQUAL, {uint8_t{1}, 1, 1U, ratio(1), 1.0F, 1.0}},
                                                                     {CMP_LT, {uint8_t{0}, 0, 0U, ratio(0), 0.0F, 0.0}},
                                                                     {CMP_GT, {uint8_t{1}, 1, 1U, ratio(1), 1.0F, 1.0}},
                                                                     {CMP_LE, {uint8_t{0}, 0, 0U, ratio(0), 0.0F, 0.0}},
                                                                     {CMP_GE, {uint8_t{1}, 1, 1U, ratio(1), 1.0F, 1.0}}};
  for (size_t a = 0; a < 6; ++a)
    for (size_t b = 0; b < 6; ++b)
      for (const auto &[op, expected] : cases) {
        SCOPED_TRACE(std::to_string(a) + "," + std::to_string(b) + "," + std::to_string(op));
        const int type =
            types[a][b] == 0 && (op == ADD || op == SUBTRACT || op == MULTIPLY || op == DIVIDE || op == POWER) ? 1 : types[a][b];
        EXPECT_EQ(evalBinary(left[a], right[b], op), expected[type]);
      }
}

TEST(xExpressionEval, exact_power_zero_exponent_is_one_of_the_product_type) {
  // covers: expressionevaluator.expression-evaluator.7~1
  using ratio = boost::rational<int>;
  // BYTE^0 to INTEGER 1, jak BYTE^k dla k >= 1 i jak deklaruje kompilator (arithmeticValueType).
  for (const auto &[base, one] : std::vector<std::pair<rdb::descFldVT, rdb::descFldVT>>{
           {uint8_t{7}, 1}, {uint8_t{0}, 1}, {7, 1}, {7U, 1U}, {ratio(2, 3), ratio(1)}}) {
    const auto zero = std::visit(
        [](const auto &v) -> rdb::descFldVT {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, ratio>)
            return T(0);
          else
            return std::monostate{};
        },
        base);
    EXPECT_EQ(evalBinary(base, zero, POWER), one);
  }
}

// Regresja #521: `-(b^e)` nad polami BYTE przy e = 0 dawalo 254 - NEGATE na BYTE odwraca bity,
// a `b^0` zostawal BYTE, choc pole ma typ INTEGER. Wartosc oczekiwana: -(b^e) = -(liczba b do potegi e).
TEST(xExpressionEval, negated_byte_power_with_zero_exponent_is_minus_one) {
  // covers: expressionevaluator.expression-evaluator.7~1
  expressionEvaluator evaluator;
  for (const auto &[b, e, expected] : {std::tuple{3, 0, -1}, std::tuple{0, 0, -1}, std::tuple{3, 1, -3}, std::tuple{3, 2, -9}}) {
    SCOPED_TRACE(std::to_string(b) + "^" + std::to_string(e));
    EXPECT_EQ(rdbtest::ok(evaluator.eval({token(PUSH_VAL, static_cast<uint8_t>(b)), token(PUSH_VAL, static_cast<uint8_t>(e)),
                                          token(POWER), token(NEGATE)})),
              rdb::descFldVT(expected));
  }
}

TEST(xExpressionEval, logic_result_uses_the_first_non_null_operand_type) {
  // covers: expressionevaluator.expression-evaluator.11~1
  using value = rdb::descFldVT;
  using ratio = boost::rational<int>;
  const std::vector<value> truthy{uint8_t{2}, 2, 2U, ratio(2), 2.0F, 2.0, std::string("x")};
  const std::vector<value> ones{uint8_t{1}, 1, 1U, ratio(1), 1.0F, 1.0, 1};
  const std::vector<value> zeros{uint8_t{0}, 0, 0U, ratio(0), 0.0F, 0.0, 0};
  for (size_t i = 0; i < truthy.size(); ++i) {
    EXPECT_EQ(evalBinary(truthy[i], 1.0, OR), ones[i]);
    EXPECT_EQ(evalBinary(std::monostate{}, truthy[i], OR), ones[i]);
    EXPECT_EQ(evalBinary(truthy[i], 0.0, AND), zeros[i]);
    EXPECT_EQ(evalUnary(truthy[i], token(NOT)), zeros[i]);
  }
}

TEST(xExpressionEval, scalar_math_functions_preserve_numeric_types_and_reject_pairs) {
  // covers: expressionevaluator.expression-evaluator.12~1, expressionevaluator.expression-evaluator.13~1
  using value = rdb::descFldVT;
  using ratio = boost::rational<int>;
  const std::vector<value> zeros{uint8_t{0}, 0, 0U, ratio(0), 0.0F, 0.0};
  const std::vector<value> ones{uint8_t{1}, 1, 1U, ratio(1), 1.0F, 1.0};
  for (const char *name : {"Floor", "Ceil", "Sqrt", "Round", "Tan", "Log", "Log2", "Trunc", "Abs"}) {
    for (size_t i = 0; i < zeros.size(); ++i) {
      const auto &input = std::string_view(name).starts_with("Log") ? ones[i] : zeros[i];
      EXPECT_EQ(evalUnary(input, token(CALL, std::string(name))), zeros[i]) << name;
    }
    EXPECT_TRUE(isNull(evalUnary(std::monostate{}, token(CALL, std::string(name))))) << name;
    for (const auto &input : std::vector<value>{std::pair{1, 2}, std::pair<std::string, int>{"x", 2}})
      EXPECT_RDB_ERROR(evalUnaryResult(input, token(CALL, std::string(name))), rdb::Errc::Eval) << name;
  }
}

TEST(xExpressionEval, push_var_reads_the_variable_table_by_index) {
  // covers: expressionevaluator.expression-evaluator.20~1
  // Zmienna VAR (#531): wartosc taktu z tablicy wolajacego, takze NULL, ktory propaguje sie dalej.
  const auto var = [](int index) { return token(PUSH_VAR, std::pair<std::string, int>{"V", index}); };
  const std::vector<rdb::descFldVT> variables{7, 2.5F, std::monostate{}};
  expressionEvaluator evaluator;

  EXPECT_EQ(rdbtest::ok(evaluator.eval({var(0), token(PUSH_VAL, 3), token(MULTIPLY)}, nullptr, nullptr, &variables)),
            rdb::descFldVT{21});
  EXPECT_EQ(rdbtest::ok(evaluator.eval({var(1)}, nullptr, nullptr, &variables)), rdb::descFldVT{2.5F});
  // `?X * 0` przy NULL to NULL, nie zero.
  EXPECT_TRUE(std::holds_alternative<std::monostate>(
      rdbtest::ok(evaluator.eval({var(2), token(PUSH_VAL, 0), token(MULTIPLY)}, nullptr, nullptr, &variables))));

  // Brak tablicy (zwijanie stalych liczy bez niej) i indeks spoza niej to blad programu.
  EXPECT_RDB_ERROR(evaluator.eval({var(0)}), rdb::Errc::Eval, "variable values not supplied");
  EXPECT_RDB_ERROR(evaluator.eval({var(3)}, nullptr, nullptr, &variables), rdb::Errc::Eval, "out of range");
  EXPECT_RDB_ERROR(evaluator.eval({var(-1)}, nullptr, nullptr, &variables), rdb::Errc::Eval, "out of range");
}
