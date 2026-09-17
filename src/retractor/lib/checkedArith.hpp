#pragma once

#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>

#include <boost/rational.hpp>

/// Arytmetyka WARTOSCI pol z wykrywaniem przepelnienia: INTEGER (int32) i RATIONAL
/// (boost::rational<int>). nullopt oznacza, ze wyniku nie da sie zapisac w typie, a wolajacy
/// zamienia go na NULL - tak jak dzielenie przez zero w expressionEvaluator.
///
/// boost::rational 1.91 niczego tu nie sprawdza: `9/1 * 1000000000/1` daje po cichu 410065408/1.
/// Ta sama klasa liczy tez os czasu (CRSMath, SOperations.hpp), ale tamtej sciezki ten plik nie
/// dotyczy - sluzy wylacznie ewaluatorowi wyrazen i reduktorom w streamInstance.
///
/// Dzielnik rozny od zera jest warunkiem wstepnym `div`; zero obsluguje wolajacy.
namespace checkedArith {

inline std::optional<int> add(int a, int b) {
  int result = 0;
  if (__builtin_add_overflow(a, b, &result)) return std::nullopt;
  return result;
}

inline std::optional<int> sub(int a, int b) {
  int result = 0;
  if (__builtin_sub_overflow(a, b, &result)) return std::nullopt;
  return result;
}

inline std::optional<int> mul(int a, int b) {
  int result = 0;
  if (__builtin_mul_overflow(a, b, &result)) return std::nullopt;
  return result;
}

/// Jedyny iloraz int32 spoza zakresu to INT_MIN / -1.
inline std::optional<int> div(int a, int b) {
  if (a == std::numeric_limits<int>::min() && b == -1) return std::nullopt;
  return a / b;
}

inline std::optional<int> neg(int a) {
  if (a == std::numeric_limits<int>::min()) return std::nullopt;
  return -a;
}

namespace detail {

/// Ulamek policzony na int64 sprowadzony do najnizszych terminow. Dopiero wtedy wiadomo, czy
/// wartosc NAPRAWDE nie miesci sie w rational<int> - skrocona postac jest jedyna, wiec nie ma
/// falszywych alarmow (`1/65536 + 1/65536` przechodzi przez mianownik 2^32 i daje 1/32768).
inline std::optional<boost::rational<int>> narrowed(std::int64_t numerator, std::int64_t denominator) {
  const auto divisor = std::gcd(numerator, denominator);
  numerator /= divisor;
  denominator /= divisor;
  if (denominator < 0) {
    numerator   = -numerator;
    denominator = -denominator;
  }
  if (numerator < std::numeric_limits<int>::min() || numerator > std::numeric_limits<int>::max() ||
      denominator > std::numeric_limits<int>::max())
    return std::nullopt;
  return boost::rational<int>(static_cast<int>(numerator), static_cast<int>(denominator));
}

}  // namespace detail

// Skladniki sa znormalizowane (mianownik dodatni, |licznik| <= 2^31), wiec kazdy iloczyn ponizej
// miesci sie w 2^62, a suma dwoch takich w int64.

// Ulamki calkowite (mianownik 1) ida skrotem przez int32 bez gcd - to typowa suma reduktora
// nad polami INTEGER promowanymi do RATIONAL.

inline std::optional<boost::rational<int>> add(boost::rational<int> a, boost::rational<int> b) {
  if (a.denominator() == 1 && b.denominator() == 1) {
    const auto sum = add(a.numerator(), b.numerator());
    return sum.has_value() ? std::optional<boost::rational<int>>{*sum} : std::nullopt;
  }
  return detail::narrowed(std::int64_t{a.numerator()} * b.denominator() + std::int64_t{b.numerator()} * a.denominator(),
                          std::int64_t{a.denominator()} * b.denominator());
}

inline std::optional<boost::rational<int>> sub(boost::rational<int> a, boost::rational<int> b) {
  if (a.denominator() == 1 && b.denominator() == 1) {
    const auto difference = sub(a.numerator(), b.numerator());
    return difference.has_value() ? std::optional<boost::rational<int>>{*difference} : std::nullopt;
  }
  return detail::narrowed(std::int64_t{a.numerator()} * b.denominator() - std::int64_t{b.numerator()} * a.denominator(),
                          std::int64_t{a.denominator()} * b.denominator());
}

inline std::optional<boost::rational<int>> mul(boost::rational<int> a, boost::rational<int> b) {
  if (a.denominator() == 1 && b.denominator() == 1) {
    const auto product = mul(a.numerator(), b.numerator());
    return product.has_value() ? std::optional<boost::rational<int>>{*product} : std::nullopt;
  }
  return detail::narrowed(std::int64_t{a.numerator()} * b.numerator(), std::int64_t{a.denominator()} * b.denominator());
}

inline std::optional<boost::rational<int>> div(boost::rational<int> a, boost::rational<int> b) {
  return detail::narrowed(std::int64_t{a.numerator()} * b.denominator(), std::int64_t{a.denominator()} * b.numerator());
}

inline std::optional<boost::rational<int>> neg(boost::rational<int> a) {
  if (a.numerator() == std::numeric_limits<int>::min()) return std::nullopt;
  return -a;
}

}  // namespace checkedArith
