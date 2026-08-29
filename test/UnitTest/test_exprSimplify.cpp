#include <list>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "rdb/payload.hpp"
#include "retractor/lib/expressionEvaluator.hpp"
#include "retractor/lib/exprSimplify.hpp"

// ctest -R '^ut-exprSimplify' -V

namespace {

/// Schemat testowy: pole 0 INTEGER, 1 FLOAT, 2 STRING, 3 BYTE. Pole 9 celowo poza mapą —
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

/// Sprawdza, że uproszczenie NIE zmienia wyniku — na programie policzonym oboma wersjami
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
  // Dzielenie przez zero daje w wykonaniu NULL — nie ma literału, którym dałoby się
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

TEST(exprSimplify, reassociates_mixed_plus_minus) {
  // x + 5 - 2 == x + 3
  const std::list<token> original{pushId(0), token(PUSH_VAL, 5), token(ADD), token(PUSH_VAL, 2), token(SUBTRACT)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<int>(std::next(program.begin())->getVT()), 3);
  EXPECT_EQ(program.back().getCommandID(), ADD);
  expectSameResult(original, program, 7);
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
  // 10 - x - 3 == 7 - x
  const std::list<token> original{token(PUSH_VAL, 10), pushId(0), token(SUBTRACT), token(PUSH_VAL, 3), token(SUBTRACT)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 3u);
  EXPECT_EQ(std::get<int>(program.front().getVT()), 7);
  EXPECT_EQ(program.back().getCommandID(), SUBTRACT);
  expectSameResult(original, program, 7);
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
  // 'a' + pole + 'b' NIE zwija się do 'ab' + pole — konkatenacja nie jest przemienna.
  const std::list<token> original{pushString("a"), pushId(2), token(ADD), pushString("b"), token(ADD)};
  std::list<token> program = original;

  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
  EXPECT_EQ(dump(program), dump(original));
}

TEST(exprSimplify, keeps_float_expression_untouched) {
  // Dla float reasocjacja zmienia liczbę zaokrągleń — reguła musi odmówić.
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
  // NULL * 0 daje NULL, a nie 0 — pochłanianie złamałoby logikę trójwartościową.
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
  // PUSH_STREAM należy do algebry strumieni — nie znamy jego arytmetyki stosu,
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

//
// ─── D: powtórzony czynnik jako potęga ──────────────────────────────────────────
//

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
  // Przekręcenie int też ma wyjść tak samo — na tym stoi ścieżka dokładna w power().
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

// Nieznany typ podwyrażenia — odmowa uproszczenia jest zawsze bezpieczna.
TEST(exprSimplify, does_not_fold_repeated_factor_of_unknown_type) {
  std::list<token> program{pushId(9), pushId(9), token(MULTIPLY)};
  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
}

// Różne pola nie są powtórzonym czynnikiem.
TEST(exprSimplify, does_not_fold_distinct_factors) {
  std::list<token> program{pushId(0), pushId(3), token(MULTIPLY)};
  EXPECT_EQ(simplifyExpression(program, testFieldType), 0u);
}

// Stałe należą do reguły A: `2*2` ma się zwinąć do 4, a nie do `2^2`.
TEST(exprSimplify, constant_square_folds_to_value_not_to_power) {
  std::list<token> program{token(PUSH_VAL, 2), token(PUSH_VAL, 2), token(MULTIPLY)};

  EXPECT_EQ(simplifyExpression(program, testFieldType), 1u);
  ASSERT_EQ(program.size(), 1u);
  EXPECT_EQ(std::get<int>(program.front().getVT()), 4);
}
