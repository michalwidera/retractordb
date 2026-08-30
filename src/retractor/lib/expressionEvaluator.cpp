#include "expressionEvaluator.hpp"

#include <spdlog/spdlog.h>
#include <boost/container/small_vector.hpp>

#include <algorithm>   // std::ranges::transform
#include <cctype>      // std::tolower
#include <cmath>       // sqrt, std::fabs
#include <cstdlib>     // std::abs
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <optional>
#include <regex>
#include <stack>
#include <stdexcept>
#include <string>
#include <typeinfo>  // operator typeid
#include <variant>
#include "fatalError.hpp"

#include "rdb/convertTypes.hpp"
#include "rdb/probe.hpp"

static cast<rdb::descFldVT> castFldVT;

expressionEvaluator::expressionEvaluator(/* args */) = default;

using pairVar = std::pair<rdb::descFldVT, rdb::descFldVT>;

/// Nazwa funkcji złożona do małych liter. Nazwy pochodzą z gramatyki, więc ASCII wystarcza,
/// a wynik mieści się w SSO — dopasowanie nazwy nie alokuje.
static std::string lowercased(std::string text) {
  std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

bool isNullValue(const rdb::descFldVT &value) { return std::holds_alternative<std::monostate>(value); }

std::optional<bool> toLogicValue(const rdb::descFldVT &value) {
  return std::visit(
      Overload{[](std::monostate) -> std::optional<bool> { return std::nullopt; },
               [](uint8_t a) -> std::optional<bool> { return a != 0; }, [](int a) -> std::optional<bool> { return a != 0; },
               [](unsigned a) -> std::optional<bool> { return a != 0; },
               [](double a) -> std::optional<bool> { return a != 0.0; },
               [](float a) -> std::optional<bool> { return a != 0.0F; },
               [](boost::rational<int> a) -> std::optional<bool> { return a != boost::rational<int>(0); },
               [](const std::string &a) -> std::optional<bool> { return !a.empty(); },
               [](std::pair<int, int>) -> std::optional<bool> {
                 throw std::runtime_error("toLogicValue: INTPAIR type not supported in logical context");
               },
               [](const std::pair<std::string, int> &) -> std::optional<bool> {
                 throw std::runtime_error("toLogicValue: IDXPAIR type not supported in logical context");
               }},
      value);
}

rdb::descFldVT logicResultAsType(bool value, const rdb::descFldVT &typeRef) {
  return std::visit(
      Overload{[value](uint8_t) -> rdb::descFldVT { return static_cast<uint8_t>(value ? 1 : 0); },
               [value](int) -> rdb::descFldVT { return value ? 1 : 0; },
               [value](unsigned) -> rdb::descFldVT { return value ? 1U : 0U; },
               [value](double) -> rdb::descFldVT { return value ? 1.0 : 0.0; },
               [value](float) -> rdb::descFldVT { return value ? 1.0F : 0.0F; },
               [value](boost::rational<int>) -> rdb::descFldVT { return boost::rational<int>(value ? 1 : 0); },
               [](std::monostate) -> rdb::descFldVT { return std::monostate{}; },
               [value](const std::string &) -> rdb::descFldVT { return value ? std::string("1") : std::string("0"); },
               [](std::pair<int, int>) -> rdb::descFldVT {
                 throw std::runtime_error("logicResultAsType: INTPAIR type not supported");
               },
               [](const std::pair<std::string, int> &) -> rdb::descFldVT {
                 throw std::runtime_error("logicResultAsType: IDXPAIR type not supported");
               }},

      typeRef);
}

rdb::descFldVT logicResultTypeRef(const rdb::descFldVT &a, const rdb::descFldVT &b) {
  if (!isNullValue(a)) return a;
  if (!isNullValue(b)) return b;
  return a;
}

pairVar normalize(const rdb::descFldVT &a, const rdb::descFldVT &b) {
  if (a.index() == b.index()) return {a, b};

  pairVar retVal;
  if (a.index() > b.index()) {
    return {a, castFldVT(b, static_cast<rdb::descFld>(a.index()))};
  }
  return {castFldVT(a, static_cast<rdb::descFld>(b.index())), b};
}

rdb::descFldVT operator+(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },       //
                 [&retVal](uint8_t a, uint8_t b) { retVal = a + b; },                            //
                 [&retVal](int a, int b) { retVal = a + b; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = a + b; },                          //
                 [&retVal](const std::string &a, const std::string &b) { retVal = a + b; },      //
                 [&retVal](double a, double b) { retVal = a + b; },                              //
                 [&retVal](float a, float b) { retVal = a + b; },                                //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a + b; },  //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first + b.first, a.second + b.second);
                 },
                 [&retVal](const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) {
                   retVal = std::make_pair(a.first + b.first, a.second + b.second);
                 },
                 [&retVal](auto a, auto b) { retVal = a + b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT operator-(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = a - b; },                       //
                 [&retVal](int a, int b) { retVal = a - b; },                               //
                 [&retVal](unsigned a, unsigned b) { retVal = a - b; },                     //
                 [](const std::string &, const std::string &) {
                   throw std::runtime_error("Operator '-' not defined for string operands");
                 },                                                                              //
                 [&retVal](double a, double b) { retVal = a - b; },                              //
                 [&retVal](float a, float b) { retVal = a - b; },                                //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a - b; },  //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first - b.first, a.second - b.second);
                 },
                 [&retVal](const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) {
                   retVal = std::make_pair(/* TODO? define str-str */ "?? -", a.second - b.second);
                 },
                 [&retVal](auto a, auto b) { retVal = a - b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT operator*(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = a * b; },                       //
                 [&retVal](int a, int b) { retVal = a * b; },                               //
                 [&retVal](unsigned a, unsigned b) { retVal = a * b; },                     //
                 [](const std::string &, const std::string &) {
                   throw std::runtime_error("Operator '*' not defined for string operands");
                 },                                                                              //
                 [&retVal](double a, double b) { retVal = a * b; },                              //
                 [&retVal](float a, float b) { retVal = a * b; },                                //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a * b; },  //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first * b.first, a.second * b.second);
                 },
                 [&retVal](const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) {
                   retVal = std::make_pair(/* TODO? define str*str */ "?? *", a.second * b.second);
                 },
                 [&retVal](auto a, auto b) { retVal = a * b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT operator/(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  const bool divisorIsZero =
      std::visit(Overload{[](uint8_t v) { return v == 0; }, [](int v) { return v == 0; }, [](unsigned v) { return v == 0U; },
                          [](double v) { return v == 0.0; }, [](float v) { return v == 0.0F; },
                          [](boost::rational<int> v) { return v == boost::rational<int>(0); },
                          [](std::monostate) { return false; }, [](const auto &) { return false; }},
                 b);

  if (divisorIsZero) {
    // Dzielenie przez zero nie ma wyniku w zbiorze wartości, więc jest wartością POCHŁANIAJĄCĄ —
    // tym samym, czym dane oczekiwane a nieobecne. Strumień oddaje NULL i pracuje dalej.
    //
    // Wcześniej leciał tu std::domain_error, czyli pojedyncza próbka o zerowym mianowniku
    // przerywała przetwarzanie całego zapytania. To była niezgodność z semantyką NULL: silnik
    // ma pochłaniać brak wyniku, a nie zatrzymywać strumień. Rozróżnienie względem pozostałych
    // wyjątków w tym pliku: tamte sygnalizują błędy TYPÓW i programu (np. '/' na łańcuchach),
    // które nie zależą od danych i muszą pozostać błędami.
    return std::monostate{};
  }

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = a / b; },                       //
                 [&retVal](int a, int b) { retVal = a / b; },                               //
                 [&retVal](unsigned a, unsigned b) { retVal = a / b; },                     //
                 [](const std::string &, const std::string &) {
                   throw std::runtime_error("Operator '/' not defined for string operands");
                 },                                                                              //
                 [&retVal](double a, double b) { retVal = a / b; },                              //
                 [&retVal](float a, float b) { retVal = a / b; },                                //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a / b; },  //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first / b.first, a.second / b.second);
                 },  //
                 [&retVal](const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) {
                   retVal = std::make_pair(/* TODO? define str/str */ "?? /", a.second / b.second);
                 },                                             //
                 [&retVal](auto a, auto b) { retVal = a / b; }  //
             },
             a, b);

  return retVal;
}

/// Potegowanie `b ^ a` — operator ExpPow z RQL.g4.
///
/// Typ wyniku ustala normalize(), tak samo jak dla `+`, `-`, `*` i `/`: wygrywa wyzszy
/// indeks wariantu. Dzieki temu `pole ^ 2` na polu INTEGER zostaje INTEGER-em (dokladny
/// odpowiednik dotychczasowego `pole * pole`), a `pole ^ 0.5` promuje sie do FLOAT.
///
/// Rachunek idzie przez double — tak samo jak callFun() dla Sqrt/Log/Sin — bo std::pow nie
/// ma przeciazenia dla boost::rational, a wykladnik ulamkowy i tak wyprowadza poza ciala
/// calkowite. Rzut z powrotem na typ znormalizowany robi ten sam castFldVT, ktorego uzywa
/// reszta pliku.
///
/// Wynik nieskonczony albo NaN (`0 ^ -1`, `(-8) ^ 0.5`) daje NULL. Jest to ta sama decyzja,
/// co przy dzieleniu przez zero kilkadziesiat linii wyzej: brak wyniku w zbiorze wartosci
/// jest wartoscia POCHLANIAJACA, a nie bledem zatrzymujacym strumien. Rzutowanie takiego
/// double na int byloby zreszta zachowaniem niezdefiniowanym.
///
/// Operand tekstowy jest bledem, jak dla `*`, `-` i `/`. normalize() promuje wtedy druga
namespace {

/// Typy o arytmetyce DOKLADNEJ — te same, ktore isExact() wyroznia w exprSimplify.
bool hasExactArithmetic(rdb::descFld type) {
  return type == rdb::BYTE || type == rdb::INTEGER || type == rdb::UINT || type == rdb::RATIONAL;
}

/// Wykladnik jako nieujemna liczba calkowita — o ile nia jest.
std::optional<int> integralExponent(const rdb::descFldVT &value) {
  return std::visit(
      Overload{[](uint8_t v) -> std::optional<int> { return v; },
               [](int v) -> std::optional<int> { return v >= 0 ? std::optional<int>{v} : std::nullopt; },
               [](unsigned v) -> std::optional<int> {
                 return v <= static_cast<unsigned>(std::numeric_limits<int>::max()) ? std::optional<int>{static_cast<int>(v)}
                                                                                    : std::nullopt;
               },
               [](boost::rational<int> v) -> std::optional<int> {
                 return (v.denominator() == 1 && v.numerator() >= 0) ? std::optional<int>{v.numerator()} : std::nullopt;
               },
               [](const auto &) -> std::optional<int> { return std::nullopt; }},
      value);
}

/// Potega typu dokladnego, liczona TYM SAMYM operator*, ktorego uzywa MULTIPLY.
///
/// To nie jest optymalizacja, tylko warunek poprawnosci przepisania `a*a` -> `a^2`
/// (regula D w exprSimplify): gdyby `^` szlo tu przez std::pow, przepisanie zmienialoby
/// wynik wszedzie tam, gdzie mnozenie sie przekreca albo promuje typ. Przy tej definicji
/// `a^k` JEST iloczynem `a*a*...*a` — z zawinieciem, promocja BYTE do int i dokladna
/// arytmetyka wymierna wlacznie.
///
/// Potegowanie przez kwadraty wolno tu zastosowac, bo mnozenie w tych typach jest laczne
/// (takze modulo 2^n), wiec grupowanie nie zmienia wyniku. Chroni to przed `a^1000000000`
/// w petli na kazdym interwale.
///
/// Wynik NIE jest rzutowany z powrotem na typ podstawy — typ ma byc dokladnie ten, ktory
/// dalby zapisany wprost iloczyn (dla BYTE jest to INTEGER, bo `uint8_t * uint8_t`
/// promuje sie do int).
rdb::descFldVT exactPower(const rdb::descFldVT &base, int exponent) {
  rdb::descFldVT result = castFldVT(rdb::descFldVT{1}, static_cast<rdb::descFld>(base.index()));
  rdb::descFldVT factor = base;
  for (int rest = exponent; rest > 0; rest >>= 1) {
    if ((rest & 1) != 0) result = result * factor;
    if (rest > 1) factor = factor * factor;
  }
  return result;
}

}  // namespace

/// strone do STRING, wiec wystarczy sprawdzic typ znormalizowany. To samo zdanie zalatwia
/// INTPAIR i IDXPAIR, ktore nie sa wartosciami wyrazen.
rdb::descFldVT power(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [base, exponent] = normalize(aParam, bParam);

  const auto resultType = static_cast<rdb::descFld>(base.index());
  if (resultType > rdb::DOUBLE) throw std::runtime_error("Operator '^' not defined for non-numeric operands");

  // Typ dokladny + calkowity nieujemny wykladnik: liczymy iloczynem, nie std::pow. Patrz
  // exactPower() — od tego zalezy, czy `a*a` wolno przepisac na `a^2`.
  if (hasExactArithmetic(resultType))
    if (const auto steps = integralExponent(exponent)) return exactPower(base, *steps);

  const double result =
      std::pow(std::get<double>(castFldVT(base, rdb::DOUBLE)), std::get<double>(castFldVT(exponent, rdb::DOUBLE)));
  if (!std::isfinite(result)) return std::monostate{};

  return castFldVT(rdb::descFldVT{result}, resultType);
}

rdb::descFldVT is_eq(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a == b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a == b) ? 1 : 0; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = (a == b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a == b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a == b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a == b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a == b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                           //
                 [](std::pair<int, int>, std::pair<int, int>) { throw std::runtime_error("is_eq: INTPAIR not supported"); },  //
                 [](const std::pair<std::string, int> &, const std::pair<std::string, int> &) {
                   throw std::runtime_error("is_eq: IDXPAIR not supported");
                 },                                                                                                //
                 [](const auto &, const auto &) { throw std::runtime_error("is_eq: unsupported operand types"); }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_neq(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a != b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a != b) ? 1 : 0; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = (a != b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a != b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a != b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a != b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a != b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                            //
                 [](std::pair<int, int>, std::pair<int, int>) { throw std::runtime_error("is_neq: INTPAIR not supported"); },  //
                 [](const std::pair<std::string, int> &, const std::pair<std::string, int> &) {
                   throw std::runtime_error("is_neq: IDXPAIR not supported");
                 },                                                                                                 //
                 [](const auto &, const auto &) { throw std::runtime_error("is_neq: unsupported operand types"); }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_lt(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                 //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a < b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a < b) ? 1 : 0; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = (a < b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a < b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a < b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a < b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a < b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                           //
                 [](std::pair<int, int>, std::pair<int, int>) { throw std::runtime_error("is_lt: INTPAIR not supported"); },  //
                 [](const std::pair<std::string, int> &, const std::pair<std::string, int> &) {
                   throw std::runtime_error("is_lt: IDXPAIR not supported");
                 },                                                                                                //
                 [](const auto &, const auto &) { throw std::runtime_error("is_lt: unsupported operand types"); }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_gt(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                 //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a > b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a > b) ? 1 : 0; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = (a > b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a > b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a > b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a > b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a > b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                           //
                 [](std::pair<int, int>, std::pair<int, int>) { throw std::runtime_error("is_gt: INTPAIR not supported"); },  //
                 [](const std::pair<std::string, int> &, const std::pair<std::string, int> &) {
                   throw std::runtime_error("is_gt: IDXPAIR not supported");
                 },                                                                                                //
                 [](const auto &, const auto &) { throw std::runtime_error("is_gt: unsupported operand types"); }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_le(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a <= b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a <= b) ? 1 : 0; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = (a <= b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a <= b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a <= b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a <= b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a <= b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                           //
                 [](std::pair<int, int>, std::pair<int, int>) { throw std::runtime_error("is_le: INTPAIR not supported"); },  //
                 [](const std::pair<std::string, int> &, const std::pair<std::string, int> &) {
                   throw std::runtime_error("is_le: IDXPAIR not supported");
                 },                                                                                                //
                 [](const auto &, const auto &) { throw std::runtime_error("is_le: unsupported operand types"); }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_ge(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  if (typeid(a) != typeid(b)) FatalError("expressionEvaluator: operand types do not match after normalization");

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a >= b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a >= b) ? 1 : 0; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = (a >= b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a >= b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a >= b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a >= b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a >= b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                           //
                 [](std::pair<int, int>, std::pair<int, int>) { throw std::runtime_error("is_ge: INTPAIR not supported"); },  //
                 [](const std::pair<std::string, int> &, const std::pair<std::string, int> &) {
                   throw std::runtime_error("is_ge: IDXPAIR not supported");
                 },                                                                                                //
                 [](const auto &, const auto &) { throw std::runtime_error("is_ge: unsupported operand types"); }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_logic_or(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  auto a = toLogicValue(aParam);
  auto b = toLogicValue(bParam);

  if ((a.has_value() && *a) || (b.has_value() && *b)) {
    return logicResultAsType(true, logicResultTypeRef(aParam, bParam));
  }

  if (a.has_value() && b.has_value()) {
    return logicResultAsType(false, logicResultTypeRef(aParam, bParam));
  }

  return std::monostate{};
}

rdb::descFldVT is_logic_and(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  auto a = toLogicValue(aParam);
  auto b = toLogicValue(bParam);

  if ((a.has_value() && !*a) || (b.has_value() && !*b)) {
    return logicResultAsType(false, logicResultTypeRef(aParam, bParam));
  }

  if (a.has_value() && b.has_value()) {
    return logicResultAsType(true, logicResultTypeRef(aParam, bParam));
  }

  return std::monostate{};
}

rdb::descFldVT neg(const rdb::descFldVT &inVar) {
  rdb::descFldVT retVal;
  if (isNullValue(inVar)) return std::monostate{};

  std::visit(Overload{
                 [&retVal](std::monostate) { retVal = std::monostate{}; },                            //
                 [&retVal](uint8_t a) { retVal = static_cast<uint8_t>(~a); },                         // xor ?
                 [&retVal](int a) { retVal = -a; },                                                   //
                 [&retVal](unsigned a) { retVal = (~a); },                                            // xor ?
                 [&retVal](boost::rational<int> a) { retVal = -a; },                                  //
                 [&retVal](float a) { retVal = -a; },                                                 //
                 [&retVal](double a) { retVal = -a; },                                                //
                 [&retVal](std::pair<int, int> a) { retVal = std::make_pair(-a.first, -a.second); },  //
                 [&retVal](const std::pair<std::string, int> &a) { retVal = std::make_pair("-" + a.first, -a.second); },     //
                 [](const std::string &) { throw std::runtime_error("Operator 'negate' not defined for string operands"); }  //
             },
             inVar);

  return retVal;
}

rdb::descFldVT logic_not(const rdb::descFldVT &inVar) {
  auto value = toLogicValue(inVar);
  if (!value.has_value()) return std::monostate{};
  return logicResultAsType(!(*value), inVar);
}

rdb::descFldVT isnull(const rdb::descFldVT &inVar) { return isNullValue(inVar) ? 1 : 0; }

/// Wartosc bezwzgledna, napisana wprost na wariancie, a NIE przez callFun.
///
/// callFun przepuszcza argument przez double i z powrotem (castFldVT), a droga powrotna dla
/// RATIONAL idzie przez Rationalize z tolerancja 1e-6 — czyli gubi dokladna wartosc wymierna.
/// Dla `Abs` ta strata bylaby czysto zbedna: wartosc bezwzgledna nie zmienia ani typu, ani
/// mianownika. Wzorzec wziety z neg() powyzej, ktore jest tym samym rodzajem operacji.
rdb::descFldVT absolute(const rdb::descFldVT &inVar) {
  if (isNullValue(inVar)) return std::monostate{};

  rdb::descFldVT retVal;
  std::visit(Overload{[&retVal](std::monostate) { retVal = std::monostate{}; },  //
                      [&retVal](uint8_t a) { retVal = a; },                      // bez znaku — tozsamosc
                      [&retVal](int a) { retVal = std::abs(a); },                //
                      [&retVal](unsigned a) { retVal = a; },                     // bez znaku — tozsamosc
                      [&retVal](boost::rational<int> a) { retVal = (a < boost::rational<int>(0)) ? -a : a; },  //
                      [&retVal](float a) { retVal = std::fabs(a); },                                           //
                      [&retVal](double a) { retVal = std::fabs(a); },                                          //
                      [](std::pair<int, int>) { throw std::runtime_error("Function 'Abs' not defined for INTPAIR operands"); },
                      [](const std::pair<std::string, int> &) {
                        throw std::runtime_error("Function 'Abs' not defined for IDXPAIR operands");
                      },
                      [](const std::string &) { throw std::runtime_error("Function 'Abs' not defined for string operands"); }},
             inVar);

  return retVal;
}

/// IsZero / IsNonZero — predykat liczbowy zwracajacy 0 albo 1 jako INTEGER.
///
/// Wynik jest INTEGER, a nie typem argumentu (inaczej niz w logic_not), bo na tym polega cala
/// ich wartosc uzytkowa: porownania (`>=`, `<`, ...) zyja w regule `term_logic` i nie sa dostepne
/// wewnatrz wyrazenia w SELECT. Te dwie funkcje sa jedynym sposobem wniesienia predykatu do
/// wyrazenia jako wartosci 0/1 — stad ich przydatnosc obok RULE.
///
/// Dla stringa predykat nie ma sensu i jest bledem, a nie cicha konwersja: toLogicValue()
/// uznaje kazdy niepusty napis za prawde, co dla nazwy `IsZero` byloby mylace.
rdb::descFldVT isZeroValue(const rdb::descFldVT &inVar, bool wantZero) {
  if (isNullValue(inVar)) return std::monostate{};
  if (std::holds_alternative<std::string>(inVar))
    throw std::runtime_error("Functions 'IsZero'/'IsNonZero' are not defined for string operands");

  const auto value = toLogicValue(inVar);
  if (!value.has_value()) return std::monostate{};
  const bool isZero = !(*value);
  return (isZero == wantZero) ? 1 : 0;
}

/// Length — liczba bajtow wartosci tekstowej, jako INTEGER.
///
/// Liczona jest WARTOSC, nie zadeklarowana szerokosc pola: payload::getItemVT przycina pole
/// STRING na pierwszym bajcie zerowym, wiec `STRING[8]` z wartoscia `42` dochodzi tu jako
/// dwuznakowy napis. Zadeklarowanej szerokosci ewaluator nie widzi i widziec nie moze.
///
/// Argument nietekstowy jest bledem, a nie cicha konwersja przez to_string — symetrycznie do
/// Abs i IsZero, ktore odrzucaja napis. `Length(k)` nad polem INTEGER jest niemal na pewno
/// literowka, a nie prosba o dlugosc zapisu dziesietnego; kto chce tej drugiej rzeczy, pisze
/// `Length(to_string(k))` i mowi to wprost.
rdb::descFldVT stringLength(const rdb::descFldVT &inVar) {
  if (isNullValue(inVar)) return std::monostate{};

  const auto *text = std::get_if<std::string>(&inVar);
  if (text == nullptr) throw std::runtime_error("Function 'Length' is defined for string operands only");

  return static_cast<int>(text->length());
}

rdb::descFldVT callFun(rdb::descFldVT &inVar, const std::function<double(double)> &fnName) {
  if (isNullValue(inVar)) return std::monostate{};
  auto backResultType = inVar.index();
  if (backResultType >= rdb::BYTE && backResultType <= rdb::DOUBLE) {
    rdb::descFldVT floValue{fnName(std::get<double>(castFldVT(inVar, rdb::DOUBLE)))};
    return castFldVT(floValue, (rdb::descFld)backResultType);
  }
  throw std::runtime_error("callFun: unsupported type - math functions require numeric operand");
}

rdb::descFldVT expressionEvaluator::eval(const std::list<token> &program, rdb::payload *payload) {
  // Kontener stosu: small_vector z inline-storage zamiast domyślnej std::deque.
  // std::deque alokuje mapę + blok już przy konstrukcji pustego stosu (~2 alok.),
  // a eval() jest wołane raz na pole/regułę co interwał (gorąca ścieżka K1 —
  // 52.8% alokacji processRows). Inline 16 pokrywa typowe głębokości wyrażeń RPN
  // bez sterty; głębsze spilują na stertę (bezpieczny fallback). API std::stack
  // bez zmian — ciało eval() nietknięte. Patrz speed_improvement (run_alloc.sh).
  std::stack<rdb::descFldVT, boost::container::small_vector<rdb::descFldVT, 16>> rStack;
  rdb::descFldVT a;
  rdb::descFldVT b;

  auto popOrThrow = [&rStack](const char *opName) -> rdb::descFldVT {
    if (rStack.empty()) {
      throw std::runtime_error(std::string("Invalid expression: missing operand for ") + opName);
    }
    auto v = rStack.top();
    rStack.pop();
    return v;
  };

  rdb::probe::onEval(program.size());

  // S1: token przez referencje (byl przez wartosc -> kopia tokena per iteracja, a token
  // trzyma descFldVT, ktory moze zawierac std::string). getStr_() liczone LENIWIE — tylko
  // CALL/CALL2/PUSH_ID2 go potrzebuja, a wczesniej budowal sie string dla kazdego tokena
  // (arytmetyka, PUSH_VAL, PUSH_ID) tylko po to, by go wyrzucic.
  for (const auto &tk : program) {
    switch (tk.getCommandID()) {
      case ADD:
      case SUBTRACT:
      case MULTIPLY:
      case DIVIDE:
      case POWER:
      case CMP_EQUAL:
      case CMP_NOT_EQUAL:
      case CMP_LT:
      case CMP_GT:
      case CMP_LE:
      case CMP_GE:
      case OR:
      case AND:
        a = popOrThrow("binary operator");
      case CALL:
      case CALL2:
      case NEGATE:
      case NOT:
        b = popOrThrow("unary operator");
        break;
      default:
        break;
    }
    switch (tk.getCommandID()) {
      case PUSH_VAL:
        rStack.push(tk.getVT());
        break;
      case ADD:
        rStack.push(b + a);
        break;
      case SUBTRACT:
        rStack.push(b - a);
        break;
      case MULTIPLY:
        rStack.push(a * b);
        break;
      case DIVIDE:
        rStack.push(b / a);
        break;
      case POWER:
        rStack.push(power(b, a));
        break;
      case NEGATE:
        rStack.push(neg(b));
        break;
      case NOT:
        rStack.push(logic_not(b));
        break;
      case CMP_EQUAL: {
        rStack.push(is_eq(b, a));
      }; break;
      case CMP_NOT_EQUAL:
        rStack.push(is_neq(b, a));
        break;
      case CMP_LT:
        rStack.push(is_lt(b, a));
        break;
      case CMP_GT:
        rStack.push(is_gt(b, a));
        break;
      case CMP_LE:
        rStack.push(is_le(b, a));
        break;
      case CMP_GE:
        rStack.push(is_ge(b, a));
        break;
      case OR:
        rStack.push(is_logic_or(b, a));
        break;
      case AND:
        rStack.push(is_logic_and(b, a));
        break;
      case CALL: {
        // Parser zapisuje do tokena postać KANONICZNĄ z rqlFunctions.hpp ('Sqrt', 'to_integer'),
        // więc dopasowanie po złożeniu wielkości liter jest tu nadmiarowe — i zostaje właśnie
        // dlatego, że jest tanie, a chroni przed rozjazdem, gdyby ktoś dopisał do tabeli nazwę
        // o innej pisowni niż gałąź poniżej.
        const auto original = tk.getStr_();
        const auto tkStr    = lowercased(original);
        // https://learnmoderncpp.com/2020/06/01/strings-as-switch-case-labels/ (?)
        if (tkStr == "floor")
          rStack.push(callFun(b, floor));
        else if (tkStr == "ceil")
          rStack.push(callFun(b, ceil));
        else if (tkStr == "sqrt")
          rStack.push(callFun(b, sqrt));
        else if (tkStr == "round")
          rStack.push(callFun(b, round));
        else if (tkStr == "sin")
          rStack.push(callFun(b, sin));
        else if (tkStr == "cos")
          rStack.push(callFun(b, cos));
        else if (tkStr == "tan")
          rStack.push(callFun(b, tan));
        else if (tkStr == "log")
          rStack.push(callFun(b, log));
        else if (tkStr == "log2")
          rStack.push(callFun(b, log2));
        else if (tkStr == "trunc")
          rStack.push(callFun(b, trunc));
        else if (tkStr == "isnull")
          rStack.push(isnull(b));
        else if (tkStr == "abs")
          rStack.push(absolute(b));
        else if (tkStr == "iszero")
          rStack.push(isZeroValue(b, true));
        else if (tkStr == "isnonzero")
          rStack.push(isZeroValue(b, false));
        else if (tkStr == "length")
          rStack.push(stringLength(b));
        else if (tkStr == "to_integer")
          rStack.push(isNullValue(b) ? rdb::descFldVT{std::monostate{}} : castFldVT(b, rdb::INTEGER));
        else if (tkStr == "to_float")
          rStack.push(isNullValue(b) ? rdb::descFldVT{std::monostate{}} : castFldVT(b, rdb::FLOAT));
        else if (tkStr == "to_double")
          rStack.push(isNullValue(b) ? rdb::descFldVT{std::monostate{}} : castFldVT(b, rdb::DOUBLE));
        else if (tkStr == "to_string")
          rStack.push(isNullValue(b) ? rdb::descFldVT{std::monostate{}} : castFldVT(b, rdb::STRING));
        else
          // Nieosiągalne z RQL od 2026-08-30: compiler::checkFunctionCalls() odrzuca nieznaną
          // nazwę przez `Check result:`, więc plan z taką nazwą nie dochodzi do wykonania.
          // Rzut zostaje jako kontrola dla ścieżek omijających kompilator (testy jednostkowe
          // budujące program tokenów wprost) — tak samo jak FatalError przy nierozwiązanym
          // węźle planu.
          throw std::runtime_error(std::string("Unsupported function call: ") + original);
      } break;
      case CALL2: {
        const auto tkStr = tk.getStr_();
        if (tkStr == "to_string")
          rStack.push(isNullValue(b) ? rdb::descFldVT{std::monostate{}} : castFldVT(b, rdb::STRING));
        else
          throw std::runtime_error(std::string("Unsupported 2-arg function call: ") + tkStr);
      } break;
      case PUSH_ID: {
        if (payload == nullptr) throw std::runtime_error("PUSH_ID: payload is null");
        auto instancePosition = get<std::pair<std::string, int>>(tk.getVT());
        // P1-E1: odczyt wprost do wariantu (getItemVT) — bez posrednika std::any
        // i any_to_variant_cast. Parytet z getItem potwierdzony w test_payload.
        auto valueOpt = payload->getItemVT(instancePosition.second);
        if (!valueOpt.has_value()) {
          rStack.emplace(std::monostate{});
          break;
        }
        rStack.push(std::move(*valueOpt));
      } break;
      case PUSH_IDX:
        SPDLOG_ERROR("There should not appear PUSH_IDX here.");
        throw std::runtime_error("PUSH_IDX should be translated to other PUSH_ before eval");
        break;
      case PUSH_ID2: {
        if (payload == nullptr) throw std::runtime_error("PUSH_ID2: payload is null");
        const auto tkStr = tk.getStr_();
        std::regex r(R"((\w*)\[(\d*)\])");
        std::smatch what;
        std::regex_search(tkStr, what, r);  // something[1]
        if (what.size() != 3)
          throw std::runtime_error("PUSH_ID2: malformed identifier '" + tkStr + "', expected format: name[index]");
        // const std::string schema(what[1]);
        const std::string sOffset1(what[2]);
        const int offset1(atoi(sOffset1.c_str()));

        // P1-E1: odczyt wprost do wariantu (getItemVT) — patrz PUSH_ID wyzej.
        auto valueOpt = payload->getItemVT(offset1);
        if (!valueOpt.has_value()) {
          rStack.emplace(std::monostate{});
          break;
        }

        rStack.push(std::move(*valueOpt));
      } break;
      default:
        throw std::runtime_error("Unsupported token in expressionEvaluator");
    }
  };

  if (rStack.empty()) {
    throw std::runtime_error("Invalid expression: empty evaluation stack");
  }
  if (rStack.size() != 1) {
    throw std::runtime_error("Invalid expression: too many values on evaluation stack");
  }

  return rStack.top();
}  // end fn
