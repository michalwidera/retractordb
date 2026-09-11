// Kontrakt typu wyniku wyrazenia — tabela regul i jej konfrontacja z ewaluatorem.
//
// Ten plik odpowiada za jedno zdanie z expressionShape.hpp: **analizator odtwarza to, co robi
// expressionEvaluator**. Wiekszosc przypadkow nie sprawdza wiec analizatora wobec wartosci
// wpisanej recznie do testu, tylko wobec `expressionEvaluator::eval()` puszczonego na tym
// SAMYM programie — bo to ewaluator, a nie ten test, jest miara poprawnosci.
//
// ctest -R '^ut_expressionShape' -V

#include <cstdint>

#include <list>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>
#include <boost/rational.hpp>

#include "rdb/payload.hpp"
#include "retractor/lib/expressionEvaluator.hpp"
#include "retractor/lib/expressionShape.hpp"
#include "retractor/lib/exprSimplify.hpp"  // kToStringDefaultWidth
#include "rqlFunctions.hpp"

namespace {

using ratio = boost::rational<int>;

/// Sloty plaskie testowego rekordu — po jednym polu na typ liczbowy, plus napis i tablica.
enum slot : int { sByte = 0, sInt, sUint, sRational, sFloat, sDouble, sText, sArray0, sArray1, sArray2, sNull };

/// Rekord, na ktorym liczy ewaluator. Wartosci dobrane tak, zeby dzielenie bylo dokladne
/// i niezerowe w kazdym typie: 6 / 3 = 2 wychodzi tak samo w BYTE, INTEGER, UINT i RATIONAL.
rdb::Descriptor testDescriptor() {
  rdb::Descriptor desc("b", static_cast<int>(sizeof(uint8_t)), 1, rdb::BYTE);
  desc += rdb::Descriptor("i", static_cast<int>(sizeof(int)), 1, rdb::INTEGER);
  desc += rdb::Descriptor("u", static_cast<int>(sizeof(unsigned)), 1, rdb::UINT);
  desc += rdb::Descriptor("r", static_cast<int>(sizeof(ratio)), 1, rdb::RATIONAL);
  desc += rdb::Descriptor("f", static_cast<int>(sizeof(float)), 1, rdb::FLOAT);
  desc += rdb::Descriptor("d", static_cast<int>(sizeof(double)), 1, rdb::DOUBLE);
  desc += rdb::Descriptor("t", static_cast<int>(sizeof(uint8_t)), 8, rdb::STRING);
  desc += rdb::Descriptor("a", static_cast<int>(sizeof(int)), 3, rdb::INTEGER);
  desc += rdb::Descriptor("n", static_cast<int>(sizeof(int)), 1, rdb::INTEGER);
  return desc;
}

rdb::payload testPayload(int value) {
  rdb::payload record(testDescriptor());
  record.setItemVT(sByte, rdb::descFldVT{static_cast<uint8_t>(value)});
  record.setItemVT(sInt, rdb::descFldVT{value});
  record.setItemVT(sUint, rdb::descFldVT{static_cast<unsigned>(value)});
  record.setItemVT(sRational, rdb::descFldVT{ratio(value)});
  record.setItemVT(sFloat, rdb::descFldVT{static_cast<float>(value)});
  record.setItemVT(sDouble, rdb::descFldVT{static_cast<double>(value)});
  record.setItemVT(sText, rdb::descFldVT{std::string("abc")});
  record.setItemVT(sArray0, rdb::descFldVT{value});
  record.setItemVT(sArray1, rdb::descFldVT{value});
  record.setItemVT(sArray2, rdb::descFldVT{value});
  record.setItemVT(sNull, std::nullopt);  // pole o typie INTEGER, ktorego wartosc jest NULL
  return record;
}

/// Ksztalt pola dokladnie taka regula, jaka stosuje compiler::inferFieldShapes():
/// wpis wieloslotowy wchodzi do odczytu SLOTEM (`rarray = 1`), `STRING[N]` zostaje jednym
/// slotem o szerokosci N.
exprFieldShapeFn shapes() {
  return [](const std::string &, int flatIndex) -> std::optional<exprShape> {
    const rdb::Descriptor desc = testDescriptor();
    const auto position        = desc.flatIndexToDescriptorPosition(flatIndex);
    if (!position.has_value()) return std::nullopt;
    const auto &field = desc[position->first];
    const int arity   = (field.rtype == rdb::STRING) ? field.rarray : 1;
    return exprShape{.rtype = field.rtype, .rlen = field.rlen, .rarray = arity};
  };
}

token readField(int flatIndex) { return token(PUSH_ID, std::make_pair(std::string("src"), flatIndex)); }

int slotOf(rdb::descFld type) {
  switch (type) {
    case rdb::BYTE:
      return sByte;
    case rdb::INTEGER:
      return sInt;
    case rdb::UINT:
      return sUint;
    case rdb::RATIONAL:
      return sRational;
    case rdb::FLOAT:
      return sFloat;
    default:
      return sDouble;
  }
}

std::string typeName(rdb::descFld type) { return std::string(rdb::GetStringdescFld(type)); }

exprShapeResult analyse(const std::list<token> &program) { return inferExpressionShape(program, shapes(), {}); }

/// Typ, jaki NAPRAWDE oddaje ewaluator dla tego programu na tym rekordzie.
rdb::descFld evaluatedType(const std::list<token> &program, rdb::payload &record) {
  expressionEvaluator evaluator;
  return static_cast<rdb::descFld>(evaluator.eval(program, &record).index());
}

const std::vector<rdb::descFld> &numericTypes() {
  static const std::vector<rdb::descFld> types{rdb::BYTE, rdb::INTEGER, rdb::UINT, rdb::RATIONAL, rdb::FLOAT, rdb::DOUBLE};
  return types;
}

}  // namespace

// --- czyste odczyty -------------------------------------------------------------------

TEST(xExpressionShape, pure_field_read_keeps_type_and_length) {
  const rdb::Descriptor desc = testDescriptor();
  for (const auto type : numericTypes()) {
    const auto result = analyse({readField(slotOf(type))});
    ASSERT_TRUE(result.resolved()) << typeName(type);
    EXPECT_EQ(result.shape.rtype, type) << typeName(type);
    EXPECT_EQ(result.shape.rlen, fieldLengthOfType(type)) << typeName(type);
    EXPECT_EQ(result.shape.rarray, 1) << typeName(type);
  }
}

TEST(xExpressionShape, pure_string_read_keeps_declared_width) {
  const auto result = analyse({readField(sText)});
  ASSERT_TRUE(result.resolved());
  EXPECT_EQ(result.shape.rtype, rdb::STRING);
  EXPECT_EQ(result.shape.rlen * result.shape.rarray, 8);
}

// Odczyt JEDNEGO slotu tablicy liczbowej daje jedna liczbe, wiec krotnosc spada do jednego.
TEST(xExpressionShape, numeric_array_element_has_arity_one) {
  for (const int flat : {sArray0, sArray1, sArray2}) {
    const auto result = analyse({readField(flat)});
    ASSERT_TRUE(result.resolved()) << flat;
    EXPECT_EQ(result.shape.rtype, rdb::INTEGER);
    EXPECT_EQ(result.shape.rarray, 1) << flat;
  }
}

// --- macierz operatorow dwuargumentowych -----------------------------------------------

// Pelna macierz 6x6 dla `+`, `-`, `*`, `/` i `^`, kazde pole skonfrontowane z ewaluatorem.
// Jest to jedyny test, ktory ustala regule promocji BYTE: `bajt + bajt` daje INTEGER, bo
// `uint8_t + uint8_t` promuje sie w C++ do `int`.
TEST(xExpressionShape, binary_arithmetic_matrix_matches_the_evaluator) {
  auto record = testPayload(6);
  auto right  = testPayload(3);

  for (const auto leftType : numericTypes()) {
    for (const auto rightType : numericTypes()) {
      for (const auto op : {ADD, SUBTRACT, MULTIPLY, DIVIDE, POWER}) {
        // Wykladnik bierzemy z tego samego rekordu; wartosc 6 jest nieujemna i calkowita,
        // wiec kazdy typ dokladny idzie sciezka exactPower, a FLOAT/DOUBLE przez std::pow.
        std::list<token> program{readField(slotOf(leftType)), readField(slotOf(rightType)), token(op)};

        const auto inferred = analyse(program);
        const auto label    = typeName(leftType) + " " + std::string(GetStringcommand_id(op)) + " " + typeName(rightType);
        ASSERT_TRUE(inferred.resolved()) << label;
        EXPECT_EQ(inferred.shape.rtype, arithmeticValueType(leftType, rightType)) << label;
        EXPECT_EQ(inferred.shape.rlen, fieldLengthOfType(inferred.shape.rtype)) << label;

        // Ewaluator liczy na WARTOSCIACH z dwoch rekordow: lewy operand rowny 6, prawy 3.
        // Dzieki temu dzielenie jest niezerowe i dokladne w kazdym z szesciu typow, a potega
        // ma nieujemny calkowity wykladnik — czyli idzie sciezka exactPower dla typow
        // dokladnych i przez std::pow dla FLOAT/DOUBLE.
        const std::list<token> literals{token(PUSH_VAL, *record.getItemVT(slotOf(leftType))),
                                        token(PUSH_VAL, *right.getItemVT(slotOf(rightType))), token(op)};
        expressionEvaluator evaluator;
        const auto actual = static_cast<rdb::descFld>(evaluator.eval(literals).index());

        EXPECT_EQ(inferred.shape.rtype, actual) << label << " (ewaluator)";
      }
    }
  }
}

// `bajt ^ 0` jest jedynym miejscem, w ktorym typ wyniku zalezy od WARTOSCI wykladnika:
// exactPower() startuje od jedynki w typie podstawy i przy zerowym wykladniku nie wykonuje
// ani jednego mnozenia, wiec nie ma promocji. Kontrakt statyczny podaje INTEGER — czyli
// odpowiedz dla kazdego wykladnika >= 1 — a wartosc 1 zapisuje sie do pola INTEGER bez straty.
TEST(xExpressionShape, byte_power_zero_is_the_documented_value_dependent_case) {
  auto record = testPayload(0);

  std::list<token> program{readField(sByte), readField(sByte), token(POWER)};
  EXPECT_EQ(analyse(program).shape.rtype, rdb::INTEGER);

  // 0 ^ 0 w typie dokladnym: wynik jest jedynka w typie PODSTAWY, czyli BYTE.
  EXPECT_EQ(evaluatedType(program, record), rdb::BYTE);

  // Kazdy wykladnik dodatni daje juz INTEGER, zgodnie z kontraktem statycznym.
  auto positive = testPayload(2);
  EXPECT_EQ(evaluatedType(program, positive), rdb::INTEGER);
}

// --- operatory jednoargumentowe --------------------------------------------------------

// `neg()` i `logic_not()` ZACHOWUJA typ argumentu — promocji tu nie ma, inaczej niz przy
// operatorach dwuargumentowych. Dla BYTE `neg` liczy `~a` i oddaje `uint8_t`.
TEST(xExpressionShape, unary_operators_keep_the_argument_type) {
  auto record = testPayload(6);
  for (const auto type : numericTypes()) {
    for (const auto op : {NEGATE, NOT}) {
      std::list<token> program{readField(slotOf(type)), token(op)};
      const auto inferred = analyse(program);
      const auto label    = std::string(GetStringcommand_id(op)) + " " + typeName(type);
      ASSERT_TRUE(inferred.resolved()) << label;
      EXPECT_EQ(inferred.shape.rtype, type) << label;
      EXPECT_EQ(inferred.shape.rtype, evaluatedType(program, record)) << label << " (ewaluator)";
    }
  }
}

// --- porownania i logika ---------------------------------------------------------------

// Porownanie NIE promuje BYTE: `is_eq` zapisuje wprost `uint8_t(1)`. Operatory te zyja
// w regule `term_logic`, czyli w warunku RULE — deskryptora nie opisuja, ale analizator
// musi je znac, bo przez ten sam program chodzi upraszczanie wyrazen.
TEST(xExpressionShape, comparison_result_is_the_normalized_operand_type) {
  auto record = testPayload(6);
  for (const auto type : numericTypes()) {
    std::list<token> program{readField(slotOf(type)), readField(slotOf(type)), token(CMP_LT)};
    const auto inferred = analyse(program);
    ASSERT_TRUE(inferred.resolved()) << typeName(type);
    EXPECT_EQ(inferred.shape.rtype, normalizedOperandType(type, type)) << typeName(type);
    EXPECT_EQ(inferred.shape.rtype, evaluatedType(program, record)) << typeName(type) << " (ewaluator)";
  }
}

// --- funkcje ---------------------------------------------------------------------------

// Kazda funkcja z kRqlFunctions ma tu swoj wiersz. Tabela jest zamknieta: test pilnuje, ze
// zadna nazwa z rqlFunctions.hpp nie zostala pominieta, wiec dopisanie funkcji bez decyzji
// o jej typie wyniku zapala ten test.
TEST(xExpressionShape, every_rql_function_has_a_declared_result_type) {
  auto record = testPayload(4);

  struct functionCase {
    std::string name;
    int argumentSlot;
    rdb::descFld expected;
  };

  const std::vector<functionCase> cases{
      // Funkcje matematyczne licza w double i wracaja rzutem na typ ARGUMENTU (callFun).
      {"Sqrt", sDouble, rdb::DOUBLE},
      {"Sqrt", sInt, rdb::INTEGER},
      {"Ceil", sDouble, rdb::DOUBLE},
      {"Ceil", sFloat, rdb::FLOAT},
      {"Floor", sDouble, rdb::DOUBLE},
      {"round", sDouble, rdb::DOUBLE},
      {"trunc", sDouble, rdb::DOUBLE},
      {"sin", sDouble, rdb::DOUBLE},
      {"cos", sDouble, rdb::DOUBLE},
      {"tan", sDouble, rdb::DOUBLE},
      {"log", sDouble, rdb::DOUBLE},
      {"log2", sDouble, rdb::DOUBLE},
      // Abs liczy wprost na wariancie, ale typu takze nie zmienia.
      {"Abs", sDouble, rdb::DOUBLE},
      {"Abs", sInt, rdb::INTEGER},
      {"Abs", sRational, rdb::RATIONAL},
      // Predykaty i dlugosc napisu sa zawsze INTEGER.
      {"isnull", sDouble, rdb::INTEGER},
      {"IsZero", sDouble, rdb::INTEGER},
      {"IsNonZero", sDouble, rdb::INTEGER},
      {"Length", sText, rdb::INTEGER},
      // null2zero przepuszcza wartosc nie-NULL bez zmiany typu.
      {"null2zero", sDouble, rdb::DOUBLE},
      {"null2zero", sByte, rdb::BYTE},
      // Konwersje jawne wyznaczaja typ swojego wyniku.
      {"to_integer", sDouble, rdb::INTEGER},
      {"to_float", sInt, rdb::FLOAT},
      {"to_double", sInt, rdb::DOUBLE},
      {"to_string", sInt, rdb::STRING},
  };

  std::set<std::string> covered;
  for (const auto &item : cases) {
    covered.insert(item.name);
    std::list<token> program{readField(item.argumentSlot), token(CALL, item.name)};
    const auto inferred = analyse(program);
    const auto label    = item.name + "(" + std::to_string(item.argumentSlot) + ")";
    ASSERT_TRUE(inferred.resolved()) << label;
    EXPECT_EQ(inferred.shape.rtype, item.expected) << label;
    EXPECT_EQ(inferred.shape.rtype, evaluatedType(program, record)) << label << " (ewaluator)";
  }

  for (const auto &fn : rdb::kRqlFunctions)
    EXPECT_TRUE(covered.contains(std::string(fn.canonical))) << "brak wiersza kontraktu dla " << fn.canonical;
}

TEST(xExpressionShape, to_string_width_comes_from_the_declaration) {
  const auto declared = analyse({readField(sInt), token(CALL2, std::make_pair(std::string("to_string"), 16))});
  ASSERT_TRUE(declared.resolved());
  EXPECT_EQ(declared.shape.rtype, rdb::STRING);
  EXPECT_EQ(declared.shape.rlen * declared.shape.rarray, 16);

  const auto fallback = analyse({readField(sInt), token(CALL, std::string("to_string"))});
  ASSERT_TRUE(fallback.resolved());
  EXPECT_EQ(fallback.shape.rlen * fallback.shape.rarray, kToStringDefaultWidth);
}

TEST(xExpressionShape, string_concatenation_sums_widths_and_numbers_add_nothing) {
  // `to_string(i:16) + '_test'` — 16 + 5.
  std::list<token> program{readField(sInt), token(CALL2, std::make_pair(std::string("to_string"), 16)),
                           token(PUSH_VAL, std::string("_test")), token(ADD)};
  const auto inferred = analyse(program);
  ASSERT_TRUE(inferred.resolved());
  EXPECT_EQ(inferred.shape.rtype, rdb::STRING);
  EXPECT_EQ(inferred.shape.rlen * inferred.shape.rarray, 21);

  // Liczba w konkatenacji nie wnosi szerokosci — regula zachowana z inferStringWidth().
  const std::list<token> withNumber{token(PUSH_VAL, std::string("ab")), readField(sInt), token(ADD)};
  const auto mixed = analyse(withNumber);
  ASSERT_TRUE(mixed.resolved());
  EXPECT_EQ(mixed.shape.rtype, rdb::STRING);
  EXPECT_EQ(mixed.shape.rlen * mixed.shape.rarray, 2);
}

// --- konwersje zagniezdzone -------------------------------------------------------------

// Sedno granicy 2 z pozycji 16: konwersja wyznacza typ takze WEWNATRZ wiekszego wyrazenia,
// a nie tylko wtedy, gdy stoi na koncu programu.
TEST(xExpressionShape, explicit_conversion_decides_inside_a_larger_expression) {
  auto record = testPayload(2);

  // to_float('2.5') * 2 -> FLOAT
  std::list<token> toFloat{token(PUSH_VAL, std::string("2.5")), token(CALL, std::string("to_float")), token(PUSH_VAL, 2),
                           token(MULTIPLY)};
  EXPECT_EQ(analyse(toFloat).shape.rtype, rdb::FLOAT);
  expressionEvaluator evaluator;
  EXPECT_EQ(static_cast<rdb::descFld>(evaluator.eval(toFloat).index()), rdb::FLOAT);

  // to_integer(d) + 1 -> INTEGER, mimo ze argument byl DOUBLE
  std::list<token> toInteger{readField(sDouble), token(CALL, std::string("to_integer")), token(PUSH_VAL, 1), token(ADD)};
  EXPECT_EQ(analyse(toInteger).shape.rtype, rdb::INTEGER);
  EXPECT_EQ(evaluatedType(toInteger, record), rdb::INTEGER);

  // Konwersja w konwersji: to_double(to_integer(d)) -> DOUBLE
  std::list<token> nested{readField(sDouble), token(CALL, std::string("to_integer")), token(CALL, std::string("to_double"))};
  EXPECT_EQ(analyse(nested).shape.rtype, rdb::DOUBLE);
  EXPECT_EQ(evaluatedType(nested, record), rdb::DOUBLE);
}

// --- null2zero i NULL --------------------------------------------------------------------

// NULL nie jest typem statycznym: pole ma typ, a brak wartosci jest BITEM w nullBitset.
// Analizator nigdy nie oddaje NULLTYPE — takze dla programu, ktory na tym rekordzie policzy
// sie na NULL.
TEST(xExpressionShape, null_value_does_not_change_the_static_type) {
  auto record = testPayload(6);

  std::list<token> program{readField(sNull), readField(sDouble), token(ADD)};
  const auto inferred = analyse(program);
  ASSERT_TRUE(inferred.resolved());
  EXPECT_EQ(inferred.shape.rtype, rdb::DOUBLE);

  // Ewaluator oddaje na tym rekordzie NULL — i to jest zgodne: pole zostaje DOUBLE,
  // a jego wartosc jest nieobecna.
  EXPECT_EQ(evaluatedType(program, record), rdb::NULLTYPE);
}

// `null2zero` jest jedyna funkcja, ktorej typ WYNIKU zalezy w wykonaniu od tego, czy
// argument byl NULL: wartosc nie-NULL przechodzi bez zmiany, a NULL zastepuje calkowite zero.
// Kontrakt statyczny podaje typ ARGUMENTU. Rozjazd nie dociera do artefaktu, bo
// payload::setItemVT() rzutuje zapisywana wartosc na typ pola — zero jest dokladnie
// reprezentowalne w kazdym typie liczbowym, wiec `null2zero(d)` daje w polu DOUBLE 0.0.
TEST(xExpressionShape, null2zero_static_type_is_the_argument_type) {
  std::list<token> program{readField(sDouble), token(CALL, std::string("null2zero"))};
  EXPECT_EQ(analyse(program).shape.rtype, rdb::DOUBLE);

  auto present = testPayload(7);
  EXPECT_EQ(evaluatedType(program, present), rdb::DOUBLE);

  // Argument NULL: ewaluator klada calkowite zero, a pole pozostaje DOUBLE.
  std::list<token> overNull{readField(sNull), token(CALL, std::string("null2zero"))};
  EXPECT_EQ(analyse(overNull).shape.rtype, rdb::INTEGER);  // pole `n` jest zadeklarowane INTEGER
  auto missing = testPayload(7);
  EXPECT_EQ(evaluatedType(overNull, missing), rdb::INTEGER);

  // Zapis do pola o innym typie nie gubi wartosci: zero przechodzi przez rzut bez straty.
  rdb::payload target(rdb::Descriptor("out", static_cast<int>(sizeof(double)), 1, rdb::DOUBLE));
  expressionEvaluator evaluator;
  target.setItemVT(0, evaluator.eval(overNull, &missing));
  ASSERT_TRUE(target.getItemVT(0).has_value());
  EXPECT_DOUBLE_EQ(std::get<double>(*target.getItemVT(0)), 0.0);
}

// --- agregaty okienne --------------------------------------------------------------------

TEST(xExpressionShape, window_aggregate_shape_comes_from_its_group) {
  const auto windows = [](int groupIndex) -> std::optional<exprShape> {
    if (groupIndex != 0) return std::nullopt;
    return numericShape(rdb::RATIONAL);
  };

  // Samo okno.
  std::list<token> bare{token(WINDOW_AVG, 0)};
  const auto alone = inferExpressionShape(bare, shapes(), windows);
  ASSERT_TRUE(alone.resolved());
  EXPECT_EQ(alone.shape.rtype, rdb::RATIONAL);

  // `to_integer(AVG(x:10)) + 1` — granica 3 z pozycji 16.
  std::list<token> casted{token(WINDOW_AVG, 0), token(CALL, std::string("to_integer")), token(PUSH_VAL, 1), token(ADD)};
  const auto result = inferExpressionShape(casted, shapes(), windows);
  ASSERT_TRUE(result.resolved());
  EXPECT_EQ(result.shape.rtype, rdb::INTEGER);

  // Bez tablicy grup token okna jest nierozstrzygalny, a nie zgadywany.
  EXPECT_EQ(inferExpressionShape(bare, shapes(), {}).status, exprShapeStatus::unknown);
  EXPECT_EQ(inferExpressionShape({token(WINDOW_AVG, 7)}, shapes(), windows).status, exprShapeStatus::unknown);
}

// --- stany kontrolowane -------------------------------------------------------------------

TEST(xExpressionShape, unresolved_program_reports_unknown) {
  // Postac przedrozwiazaniowa odwolania do pola.
  EXPECT_EQ(analyse({token(PUSH_ID3, std::string("x"))}).status, exprShapeStatus::unknown);
  // Token spoza zestawu ewaluatora.
  EXPECT_EQ(analyse({token(PUSH_STREAM, std::string("s"))}).status, exprShapeStatus::unknown);
  // Odwolanie do slotu, ktorego zrodlo nie ma.
  EXPECT_EQ(analyse({readField(99)}).status, exprShapeStatus::unknown);
  // Pusty program.
  EXPECT_EQ(analyse({}).status, exprShapeStatus::unknown);
}

// Program bez wartosci daje `illTyped`, a nie zgadniety typ. Kompilator traktuje to tak samo
// jak `unknown` — pole zostaje przy sentinelu — zeby blad polecial w WYKONANIU, dokladnie
// tam, gdzie lecial dotad.
TEST(xExpressionShape, ill_typed_program_reports_ill_typed) {
  // `'abc' * 2` — ewaluator rzuca `Operator '*' not defined for string operands`.
  std::list<token> product{readField(sText), token(PUSH_VAL, 2), token(MULTIPLY)};
  EXPECT_EQ(analyse(product).status, exprShapeStatus::illTyped);
  auto record = testPayload(2);
  expressionEvaluator evaluator;
  EXPECT_THROW((void)evaluator.eval(product, &record), std::exception);

  // Brakujacy operand i wartosc nadmiarowa.
  EXPECT_EQ(analyse({token(ADD)}).status, exprShapeStatus::illTyped);
  EXPECT_EQ(analyse({token(PUSH_VAL, 1), token(PUSH_VAL, 2)}).status, exprShapeStatus::illTyped);
}

// --- niezaleznosc od upraszczania wyrazen ------------------------------------------------

// Deskryptor nie moze zalezec od `RDB_OPT_SIMPLIFY_EXPRESSIONS`. Kompilator zapewnia to
// PORZADKIEM — inferFieldShapes() stoi przed simplifyFieldExpressions() — ale sam niezmiennik
// jest mocniejszy i to on jest tu sprawdzany: `simplifyExpression()` nie zmienia KSZTALTU
// wyrazenia, wiec ponowna analiza uproszczonego programu daje te sama odpowiedz.
//
// To samo zdanie odpowiada za idempotencje ponownej kompilacji zywego planu: drugi przebieg
// widzi programy juz uproszczone.
TEST(xExpressionShape, simplification_does_not_change_the_inferred_shape) {
  const auto typeOfField = [](const std::string &, int flatIndex) -> std::optional<rdb::descFld> {
    const auto shape = shapes()(std::string{}, flatIndex);
    if (!shape.has_value()) return std::nullopt;
    return shape->rtype;
  };

  const std::vector<std::pair<std::string, std::list<token>>> programs{
      // Zwijanie stalych (regula A).
      {"1+1", {token(PUSH_VAL, 1), token(PUSH_VAL, 1), token(ADD)}},
      // Reasocjacja ogona stalych (regula B).
      {"i+2+3", {readField(sInt), token(PUSH_VAL, 2), token(ADD), token(PUSH_VAL, 3), token(ADD)}},
      // Element neutralny (regula C).
      {"i*1", {readField(sInt), token(PUSH_VAL, 1), token(MULTIPLY)}},
      {"i+0", {readField(sInt), token(PUSH_VAL, 0), token(ADD)}},
      // Element neutralny nad BYTE: stala jest INTEGER-em, wiec regula C ODMAWIA przepisania,
      // bo usuniecie operatora skasowaloby promocje. Ksztalt zostaje INTEGER w obie strony.
      {"b*1", {readField(sByte), token(PUSH_VAL, 1), token(MULTIPLY)}},
      {"b+0", {readField(sByte), token(PUSH_VAL, 0), token(ADD)}},
      // Konwersja pod arytmetyka — program, ktory regula „ostatniego tokenu" gubila.
      {"to_double(i)+0", {readField(sInt), token(CALL, std::string("to_double")), token(PUSH_VAL, 0), token(ADD)}},
      // Powtorzony czynnik: przy aggressive_expr_optimization=ON przechodzi w `^2`, przy OFF
      // zostaje iloczynem. Ksztalt ma byc ten sam w obu budowach.
      {"i*i", {readField(sInt), readField(sInt), token(MULTIPLY)}},
      {"b*b", {readField(sByte), readField(sByte), token(MULTIPLY)}},
      // Napis zlozony z odczytu pola: zwijanie stalych go nie dotyka, bo nie jest stala.
      {"to_string(i:16)+'_x'",
       {readField(sInt), token(CALL2, std::make_pair(std::string("to_string"), 16)), token(PUSH_VAL, std::string("_x")),
        token(ADD)}},
  };

  for (const auto &[label, original] : programs) {
    const auto before = analyse(original);
    ASSERT_TRUE(before.resolved()) << label;

    std::list<token> simplified = original;
    simplifyExpression(simplified, typeOfField);

    const auto after = analyse(simplified);
    ASSERT_TRUE(after.resolved()) << label << " (po uproszczeniu)";
    EXPECT_TRUE(before.shape.sameAs(after.shape))
        << label << ": " << typeName(before.shape.rtype) << "/" << before.shape.rlen << "/" << before.shape.rarray << " -> "
        << typeName(after.shape.rtype) << "/" << after.shape.rlen << "/" << after.shape.rarray;
  }
}

// JEDYNY ksztalt programu, dla ktorego powyzszy niezmiennik NIE zachodzi — i powod, dla
// ktorego compiler::inferFieldShapes() stoi PRZED simplifyFieldExpressions(), a nie po nim.
//
// `to_string(expr : N)` niesie N jako DEKLARACJE szerokosci pola, a nie jako wartosc na stosie.
// Gdy caly argument jest stala, zwijanie (regula A) zastepuje program literalem tekstowym —
// i razem z programem znika deklaracja. Analiza uproszczonego programu widzi juz tylko dlugosc
// samego napisu.
//
// Kolejnosc przebiegow zamyka to przy kompilacji planu z pliku. Zostaje jeden przypadek,
// w ktorym program dociera do analizy juz uproszczony: PONOWNA kompilacja zywego planu
// (executorsm::getAdHoc). Pole `SELECT to_string(42:16)` zwezaloby sie wtedy z 16 na 2.
// Zachowanie jest STARSZE od tej analizy — compiler::inferStringFieldTypes() liczyl szerokosc
// dokladnie tak samo — i dotyczy wylacznie szerokosci napisu, nie typu liczbowego. Test pinuje
// je jawnie, zeby rozjazd nie przeszedl milczkiem, i nie udaje, ze jest zamierzony.
TEST(xExpressionShape, constant_to_string_loses_its_declared_width_when_folded) {
  const auto typeOfField = [](const std::string &, int) -> std::optional<rdb::descFld> { return rdb::INTEGER; };

  std::list<token> program{token(PUSH_VAL, 1), token(PUSH_VAL, 1), token(ADD),
                           token(CALL2, std::make_pair(std::string("to_string"), 16))};

  const auto before = analyse(program);
  ASSERT_TRUE(before.resolved());
  EXPECT_EQ(before.shape.rtype, rdb::STRING);
  EXPECT_EQ(before.shape.rlen * before.shape.rarray, 16);

  ASSERT_GT(simplifyExpression(program, typeOfField), 0u);

  const auto after = analyse(program);
  ASSERT_TRUE(after.resolved());
  EXPECT_EQ(after.shape.rtype, rdb::STRING);
  EXPECT_EQ(after.shape.rlen * after.shape.rarray, 1);  // dlugosc napisu "2"
}
