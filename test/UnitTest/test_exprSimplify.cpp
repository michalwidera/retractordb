#include <cmath>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <boost/rational.hpp>

#include "rdb/payload.hpp"
#include "retractor/lib/expressionEvaluator.hpp"
#include "retractor/lib/exprSimplify.hpp"

// ctest -R '^ut-exprSimplify' -V

namespace {

/// Schemat testowy: pole 0 INTEGER, 1 FLOAT, 2 STRING, 3 BYTE. Pole 9 celowo poza mapą -
/// reprezentuje odwołanie o nieznanym typie (np. do strumienia, którego nie ma w planie).
std::optional<rdb::descFld> testFieldType(const std::string &, int index) {
  static const std::map<int, rdb::descFld> schema{{0, rdb::INTEGER}, {1, rdb::FLOAT}, {2, rdb::STRING}, {3, rdb::BYTE}};
  auto found = schema.find(index);
  if (found == schema.end()) return std::nullopt;
  return found->second;
}

token pushId(int index) { return token(PUSH_ID, std::pair<std::string, int>{"A", index}); }

token pushString(const std::string &text) { return token(PUSH_VAL, rdb::descFldVT(text)); }

std::string dump(const std::list<token> &program) {
  std::ostringstream out;
  for (const auto &tk : program)
    out << tk << ";";
  return out.str();
}

/// Sprawdza, że uproszczenie NIE zmienia wyniku - na programie policzonym oboma wersjami
/// nad tym samym payloadem. To jest właściwe kryterium poprawności reguł; kształt programu
/// jest tylko środkiem.
void expectSameResult(const std::list<token> &original, const std::list<token> &simplified, int fieldValue) {
  auto descriptor = rdb::Descriptor("x", 4, 1, rdb::INTEGER);
  rdb::payload data(descriptor);
  data.setItem(0, fieldValue);

  expressionEvaluator evaluator;
  EXPECT_TRUE(evaluator.eval(original, &data) == evaluator.eval(simplified, &data))
      << dump(original) << " != " << dump(simplified) << " dla x=" << fieldValue;
}

/// Schemat z jednym polem UINT pod indeksem 0 - payload expectSameUintResult ma to samo pole.
std::optional<rdb::descFld> uintFieldType(const std::string &, int index) {
  if (index == 0) return rdb::UINT;
  return std::nullopt;
}

/// expectSameResult nad polem UINT: jedyny typ dokładny wyżej niż literał INTEGER, w którym
/// ujemna stała nie ma reprezentacji.
void expectSameUintResult(const std::list<token> &original, const std::list<token> &simplified, unsigned fieldValue) {
  auto descriptor = rdb::Descriptor("u", static_cast<int>(sizeof(unsigned)), 1, rdb::UINT);
  rdb::payload data(descriptor);
  data.setItem(0, fieldValue);

  expressionEvaluator evaluator;
  EXPECT_TRUE(evaluator.eval(original, &data) == evaluator.eval(simplified, &data))
      << dump(original) << " != " << dump(simplified) << " dla u=" << fieldValue;
}

}  // namespace

//
// ─── A: zwijanie stałych ────────────────────────────────────────────────────────
//

TEST(exprSimplify, folds_constant_arithmetic) {
  std::list<token> program{token(PUSH_VAL, 1), token(PUSH_VAL, 1), token(ADD)};

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 1u);
  EXPECT_EQ(program.front().getCommandID(), PUSH_VAL);
  EXPECT_EQ(std::get<int>(program.front().getVT()), 2);
}

TEST(exprSimplify, concatenates_string_literals) {
  std::list<token> program{pushString("a"), pushString("b"), token(ADD)};

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 1u);
  EXPECT_EQ(std::get<std::string>(program.front().getVT()), "ab");
}

TEST(exprSimplify, folds_function_call_on_constant) {
  std::list<token> program{token(PUSH_VAL, 4), token(CALL, rdb::descFldVT(std::string("sqrt")))};

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 1u);
  EXPECT_EQ(std::get<int>(program.front().getVT()), 2);
}

TEST(exprSimplify, leaves_expression_the_evaluator_cannot_compute) {
  // '-' nie jest zdefiniowane dla łańcuchów: błąd ma zostać zgłoszony w wykonaniu,
  // a nie zamieniony przez kompilator na cokolwiek innego.
  const std::list<token> original{pushString("a"), pushString("b"), token(SUBTRACT)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

TEST(exprSimplify, leaves_division_by_constant_zero) {
  // Dzielenie przez zero daje w wykonaniu NULL - nie ma literału, którym dałoby się
  // ten wynik wstawić z powrotem do programu.
  const std::list<token> original{token(PUSH_VAL, 1), token(PUSH_VAL, 0), token(DIVIDE)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

//
// ─── B: reasocjacja ogona stałych ───────────────────────────────────────────────
//

TEST(exprSimplify, reassociates_addition_tail) {
  const std::list<token> original{pushId(0), token(PUSH_VAL, 1), token(ADD), token(PUSH_VAL, 1), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<int>(std::next(program.begin())->getVT()), 2);
  EXPECT_EQ(program.back().getCommandID(), ADD);
  expectSameResult(original, program, 7);
}

TEST(exprSimplify, reassociates_whole_chain_in_one_pass) {
  const std::list<token> original{pushId(0),  token(PUSH_VAL, 1), token(ADD), token(PUSH_VAL, 1),
                                  token(ADD), token(PUSH_VAL, 1), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 2u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<int>(std::next(program.begin())->getVT()), 3);
  expectSameResult(original, program, 7);
}

TEST(exprSimplify, keeps_mixed_plus_minus_with_intermediate_overflow) {
  const std::list<token> original{pushId(0), token(PUSH_VAL, 5), token(ADD), token(PUSH_VAL, 2), token(SUBTRACT)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
  for (int x : {7, std::numeric_limits<int>::max() - 4, std::numeric_limits<int>::max()})
    expectSameResult(original, program, x);
}

TEST(exprSimplify, reassociates_subtraction_chain) {
  // x - 1 - 2 == x - 3
  const std::list<token> original{pushId(0), token(PUSH_VAL, 1), token(SUBTRACT), token(PUSH_VAL, 2), token(SUBTRACT)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<int>(std::next(program.begin())->getVT()), 3);
  EXPECT_EQ(program.back().getCommandID(), SUBTRACT);
  expectSameResult(original, program, 7);
}

TEST(exprSimplify, reassociates_multiplication) {
  const std::list<token> original{pushId(0), token(PUSH_VAL, 2), token(MULTIPLY), token(PUSH_VAL, 3), token(MULTIPLY)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<int>(std::next(program.begin())->getVT()), 6);
  expectSameResult(original, program, 7);
}

TEST(exprSimplify, reassociates_with_constant_on_the_left) {
  // 10 - x + 3 == 13 - x; określoność wyniku gwarantuje określoność 10-x.
  const std::list<token> original{token(PUSH_VAL, 10), pushId(0), token(SUBTRACT), token(PUSH_VAL, 3), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<int>(program.front().getVT()), 13);
  EXPECT_EQ(program.back().getCommandID(), SUBTRACT);
  for (int x : {7, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()})
    expectSameResult(original, program, x);
}

TEST(exprSimplify, keeps_constant_on_the_left_with_intermediate_overflow) {
  // 10 - x - 3 == 7 - x matematycznie, ale dla x = INT_MIN + 7 forma krokowa przepełnia się
  // w 10 - x (NULL), a forma 7 - x daje INT_MAX. Strażnik reguły B odmawia (#327).
  const std::list<token> original{token(PUSH_VAL, 10), pushId(0), token(SUBTRACT), token(PUSH_VAL, 3), token(SUBTRACT)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
  for (int x : {7, std::numeric_limits<int>::min() + 7, std::numeric_limits<int>::max()})
    expectSameResult(original, program, x);
}

TEST(exprSimplify, concatenates_string_tail) {
  // pole STRING + 'a' + 'b' == pole + 'ab'
  const std::list<token> original{pushId(2), pushString("a"), token(ADD), pushString("b"), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<std::string>(std::next(program.begin())->getVT()), "ab");
}

TEST(exprSimplify, keeps_string_constants_apart_when_they_surround_the_field) {
  // 'a' + pole + 'b' NIE zwija się do 'ab' + pole - konkatenacja nie jest przemienna.
  const std::list<token> original{pushString("a"), pushId(2), token(ADD), pushString("b"), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

TEST(exprSimplify, keeps_float_expression_untouched) {
  // Dla float reasocjacja zmienia liczbę zaokrągleń - reguła musi odmówić.
  const std::list<token> original{pushId(1), token(PUSH_VAL, 1), token(ADD), token(PUSH_VAL, 1), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

TEST(exprSimplify, keeps_expression_of_unknown_type_untouched) {
  const std::list<token> original{pushId(9), token(PUSH_VAL, 1), token(ADD), token(PUSH_VAL, 1), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

TEST(exprSimplify, keeps_uint_tail_whose_folded_constant_is_negative) {
  // Zwinięta stała -2 nie ma reprezentacji w UINT. Do 7471948b `u+3-5` zwijało się do `u+(-2)`,
  // które do 2026-09-27 dawało NULL dla każdego u - przy ON NULL, przy ablacji u-2.
  const std::list<std::list<token>> originals{
      {pushId(0), token(PUSH_VAL, 3), token(ADD), token(PUSH_VAL, 5), token(SUBTRACT)},  // u+3-5
      {token(PUSH_VAL, 3), pushId(0), token(ADD), token(PUSH_VAL, 5), token(SUBTRACT)},  // 3+u-5
      {pushId(0), token(PUSH_VAL, 3), token(SUBTRACT), token(PUSH_VAL, 5), token(ADD)},  // (u-3)+5
      {pushId(0), token(PUSH_VAL, 3), token(ADD), token(PUSH_VAL, 5), token(SUBTRACT),   //
       token(PUSH_VAL, 7), token(ADD)}};                                                 // (u+3-5)+7
  for (const auto &original : originals) {
    std::list<token> program = original;

    EXPECT_EQ(simplifyExpression(program, uintFieldType), 0u) << dump(original);
    EXPECT_EQ(dump(program), dump(original));
  }
}

TEST(exprSimplify, keeps_uint_tail_with_intermediate_overflow_despite_nonnegative_fold) {
  const std::list<token> original{pushId(0), token(PUSH_VAL, 5), token(ADD), token(PUSH_VAL, 3), token(SUBTRACT)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, uintFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
  for (unsigned u : {0U, 1U, 2U, 10U, std::numeric_limits<unsigned>::max() - 4, std::numeric_limits<unsigned>::max()})
    expectSameUintResult(original, program, u);
}

TEST(exprSimplify, reassociates_safe_uint_addition_tail) {
  const std::list<token> original{pushId(0), token(PUSH_VAL, 3), token(ADD), token(PUSH_VAL, 5), token(ADD)};
  auto program = original;
  EXPECT_EQ(simplifyExpression(program, uintFieldType), 1u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<int>(std::next(program.begin())->getVT()), 8);
  for (unsigned u : {0U, 10U, std::numeric_limits<unsigned>::max() - 7, std::numeric_limits<unsigned>::max()})
    expectSameUintResult(original, program, u);
}

TEST(exprSimplify, preserves_null_in_issue327_counterexamples) {
  struct example {
    rdb::descFld type;
    int width;
    rdb::descFldVT value;
    std::list<token> program;
  };
  const std::vector<example> examples{
      {rdb::UINT, 4, 1U, {pushId(0), token(PUSH_VAL, -2), token(MULTIPLY), token(PUSH_VAL, -3), token(MULTIPLY)}},
      {rdb::INTEGER,
       4,
       std::numeric_limits<int>::max(),
       {pushId(0), token(PUSH_VAL, 1), token(ADD), token(PUSH_VAL, 1), token(SUBTRACT)}},
      {rdb::INTEGER,
       4,
       std::numeric_limits<int>::min(),
       {pushId(0), token(PUSH_VAL, -1), token(MULTIPLY), token(PUSH_VAL, -1), token(MULTIPLY)}},
      {rdb::UINT,
       4,
       std::numeric_limits<unsigned>::max() - 4,
       {pushId(0), token(PUSH_VAL, 5), token(ADD), token(PUSH_VAL, 3), token(SUBTRACT)}},
      {rdb::RATIONAL,
       8,
       boost::rational<int>(1, std::numeric_limits<int>::max()),
       {pushId(0), token(PUSH_VAL, 1), token(ADD), token(PUSH_VAL, 1), token(SUBTRACT)}},
      {rdb::BYTE,
       1,
       uint8_t{128},
       {pushId(0), pushId(0), token(ADD), token(PUSH_VAL, 8388608), token(MULTIPLY), token(PUSH_VAL, -1), token(MULTIPLY)}}};
  expressionEvaluator evaluator;
  for (const auto &example : examples) {
    SCOPED_TRACE(dump(example.program));
    auto descriptor = rdb::Descriptor("E", example.width, 1, example.type);
    rdb::payload data(descriptor);
    data.setItemVT(0, example.value);
    ASSERT_EQ(data.getItemVT(0), std::optional{example.value});
    auto simplified = example.program;
    EXPECT_EQ(simplifyExpression(simplified, [&](const std::string &, int) { return std::optional{example.type}; }), 0u);
    EXPECT_EQ(evaluator.eval(example.program, &data), rdb::descFldVT(std::monostate{}));
    EXPECT_EQ(evaluator.eval(simplified, &data), rdb::descFldVT(std::monostate{}));
  }
}

TEST(exprSimplify, constant_tail_matrix_preserves_values_and_null_at_boundaries) {
  const std::vector<int> constants{std::numeric_limits<int>::min(), -65536, -3, -2, -1, 0, 1, 2, 3, 65536,
                                   std::numeric_limits<int>::max()};
  const std::vector<command_id> operators{ADD, SUBTRACT, MULTIPLY};
  expressionEvaluator evaluator;
  std::size_t rewritten = 0;
  for (auto type : {rdb::BYTE, rdb::INTEGER, rdb::UINT}) {
    auto descriptor = rdb::Descriptor("E", type == rdb::BYTE ? 1 : 4, 1, type);
    rdb::payload data(descriptor);
    std::vector<rdb::descFldVT> values{std::monostate{}};
    if (type == rdb::BYTE) {
      for (int x : {0, 1, 127, 128, 254, 255})
        values.emplace_back(static_cast<uint8_t>(x));
    } else if (type == rdb::INTEGER) {
      for (int x : constants)
        values.emplace_back(x);
      values.emplace_back(std::numeric_limits<int>::min() + 1);
      values.emplace_back(std::numeric_limits<int>::max() - 1);
    } else {
      for (unsigned x : {0U, 1U, 2U, 3U, 65536U, 2147483647U, 2147483648U, 4294967291U, 4294967294U, 4294967295U})
        values.emplace_back(x);
    }
    for (int c1 : constants)
      for (int c2 : constants)
        for (auto op1 : operators)
          for (auto op2 : operators)
            for (bool left : {false, true}) {
              const auto field = pushId(0);
              const std::list<token> original{left ? token(PUSH_VAL, c1) : field, left ? field : token(PUSH_VAL, c1), token(op1),
                                              token(PUSH_VAL, c2), token(op2)};
              auto simplified = original;
              rewritten += simplifyExpression(simplified, [type](const std::string &, int) { return std::optional{type}; }) > 0;
              for (const auto &value : values) {
                data.setItemVT(0, std::holds_alternative<std::monostate>(value) ? std::nullopt : std::optional{value});
                ASSERT_EQ(data.getItemVT(0),
                          std::holds_alternative<std::monostate>(value) ? std::nullopt : std::optional{value});
                ASSERT_EQ(evaluator.eval(original, &data), evaluator.eval(simplified, &data))
                    << dump(original) << " type=" << type << " E=" << token(PUSH_VAL, value).getStr_();
              }
            }
  }
  EXPECT_GT(rewritten, 0u);
}

//
// ─── C: elementy neutralne ──────────────────────────────────────────────────────
//

TEST(exprSimplify, drops_neutral_operands) {
  struct testCase {
    command_id op;
    int constant;
  };
  for (const auto &item : {testCase{ADD, 0}, testCase{SUBTRACT, 0}, testCase{MULTIPLY, 1}, testCase{DIVIDE, 1}}) {
    const std::list<token> original{pushId(0), token(PUSH_VAL, item.constant), token(item.op)};
    std::list<token> program = original;

    EXPECT_EQ(simplifyExpression(program, testFieldType), 1u) << dump(original);
    ASSERT_EQ(program.size(), 1u) << dump(program);
    EXPECT_EQ(program.front().getCommandID(), PUSH_ID);
    expectSameResult(original, program, 7);
  }
}

TEST(exprSimplify, drops_neutral_operand_written_on_the_left) {
  const std::list<token> original{token(PUSH_VAL, 0), pushId(0), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 1u);
  EXPECT_EQ(program.front().getCommandID(), PUSH_ID);
  expectSameResult(original, program, 7);
}

TEST(exprSimplify, keeps_multiplication_by_zero) {
  // NULL * 0 daje NULL, a nie 0 - pochłanianie złamałoby logikę trójwartościową.
  const std::list<token> original{pushId(0), token(PUSH_VAL, 0), token(MULTIPLY)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

TEST(exprSimplify, keeps_neutral_operand_of_a_wider_type) {
  // Pole BYTE + literał INTEGER: usunięcie ADD skasowałoby promocję do int i dalsza
  // arytmetyka zaczęłaby zawijać modulo 256.
  const std::list<token> original{pushId(3), token(PUSH_VAL, 0), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

//
// ─── Bramki ogólne ──────────────────────────────────────────────────────────────
//

TEST(exprSimplify, refuses_program_with_token_outside_the_evaluator) {
  // PUSH_STREAM należy do algebry strumieni - nie znamy jego arytmetyki stosu,
  // więc program zostaje nietknięty w całości, razem ze zwijalnymi stałymi.
  const std::list<token> original{token(PUSH_STREAM, rdb::descFldVT(std::string("A"))), token(PUSH_VAL, 1), token(PUSH_VAL, 1),
                                  token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

TEST(exprSimplify, refuses_malformed_program) {
  const std::list<token> original{token(PUSH_VAL, 1), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

TEST(exprSimplify, keeps_expression_without_constants_untouched) {
  const std::list<token> original{pushId(0), pushId(0), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

// Stałe należą do reguły A niezależnie od `aggressive_expr_optimization`: `2*2` ma się
// zwinąć do 4, a nie do `2^2`.
TEST(exprSimplify, constant_square_folds_to_value_not_to_power) {
  std::list<token> program{token(PUSH_VAL, 2), token(PUSH_VAL, 2), token(MULTIPLY)};

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 1u);
  EXPECT_EQ(std::get<int>(program.front().getVT()), 4);
}

// Potęga o niecałkowitym wykładniku nad RATIONAL liczy się w double i wraca na RATIONAL przez
// Rationalize. Konwergent ponad `int` przepełniał tam `boost::rational<int>`: `3^(3/2)` dawało
// -685059943/143682433, czyli -4.768 zamiast 5.196 (#309).
TEST(exprSimplify, folds_rational_power_without_overflow) {
  std::list<token> program{token(PUSH_VAL, rdb::descFldVT(boost::rational<int>(3, 1))),
                           token(PUSH_VAL, rdb::descFldVT(boost::rational<int>(3, 2))), token(POWER)};

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 1u);
  const double folded = boost::rational_cast<double>(std::get<boost::rational<int>>(program.front().getVT()));
  const double exact  = std::pow(3.0, 1.5);
  EXPECT_GT(folded, 0.0);
  EXPECT_LE(std::abs(folded - exact) / exact, 1e-9) << folded;
}

//
// ─── D: powtórzony czynnik jako potęga ──────────────────────────────────────────
//
// Cała reguła stoi za `aggressive_expr_optimization`, domyślnie wyłączonym - powód jest
// w exprSimplify.hpp (korpus H9). Przy wyłączonym przełączniku sprawdzamy to, co ma być
// wtedy prawdą: program zostaje nietknięty.
//

#if aggressive_expr_optimization

TEST(exprSimplify, folds_squared_factor_into_power) {
  // x * x == x ^ 2
  const std::list<token> original{pushId(0), pushId(0), token(MULTIPLY)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(program.front().getCommandID(), PUSH_ID);
  EXPECT_EQ(std::get<int>(std::next(program.begin())->getVT()), 2);
  EXPECT_EQ(program.back().getCommandID(), POWER);
  expectSameResult(original, program, 7);
  expectSameResult(original, program, -3);
  // Przekręcenie int też ma wyjść tak samo - na tym stoi ścieżka dokładna w power().
  expectSameResult(original, program, 100000);
}

TEST(exprSimplify, folds_multiplication_chain_into_power) {
  // x * x * x * x == x ^ 4, w jednym przebiegu
  const std::list<token> original{pushId(0), pushId(0), token(MULTIPLY), pushId(0), token(MULTIPLY), pushId(0), token(MULTIPLY)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 3u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<int>(std::next(program.begin())->getVT()), 4);
  EXPECT_EQ(program.back().getCommandID(), POWER);
  expectSameResult(original, program, 3);
  expectSameResult(original, program, -2);
}

// Powtórzonym czynnikiem może być całe podwyrażenie, nie tylko pole.
TEST(exprSimplify, folds_repeated_subexpression) {
  // (x+1) * (x+1) == (x+1) ^ 2
  const std::list<token> original{pushId(0),          token(PUSH_VAL, 1), token(ADD),     pushId(0),
                                  token(PUSH_VAL, 1), token(ADD),         token(MULTIPLY)};
  std::list<token> program = original;

  EXPECT_GE(simplifyExpression(program, testFieldType), 1u);
  EXPECT_EQ(program.back().getCommandID(), POWER);
  expectSameResult(original, program, 7);
}

// FLOAT i DOUBLE zostają nietknięte: `x*x` to jedno mnożenie IEEE, a `x^2` idzie przez
// std::pow, który nie ma gwarancji poprawnego zaokrąglenia.
TEST(exprSimplify, does_not_fold_repeated_factor_for_inexact_types) {
  std::list<token> program{pushId(1), pushId(1), token(MULTIPLY)};
  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(std::list<token>{pushId(1), pushId(1), token(MULTIPLY)}));
}

// Nieznany typ podwyrażenia - odmowa uproszczenia jest zawsze bezpieczna.
TEST(exprSimplify, does_not_fold_repeated_factor_of_unknown_type) {
  std::list<token> program{pushId(9), pushId(9), token(MULTIPLY)};
  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
}

// Różne pola nie są powtórzonym czynnikiem.
TEST(exprSimplify, does_not_fold_distinct_factors) {
  std::list<token> program{pushId(0), pushId(3), token(MULTIPLY)};
  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
}

#else

TEST(exprSimplify, keeps_repeated_factor_when_aggressive_rewrites_are_off) {
  const std::list<token> original{pushId(0), pushId(0), token(MULTIPLY)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

#endif
