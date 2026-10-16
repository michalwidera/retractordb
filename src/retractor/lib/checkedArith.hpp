#pragma once

#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>

#include <boost/rational.hpp>

/// Arytmetyka WARTOSCI pol z wykrywaniem przepelnienia: INTEGER (int32), UINT (uint32) i RATIONAL
/// (boost::rational<int>). nullopt oznacza, ze wyniku nie da sie zapisac w typie, a wolajacy
/// zamienia go na NULL - tak jak dzielenie przez zero w expressionEvaluator.
///
/// boost::rational 1.91 niczego tu nie sprawdza: `9/1 * 1000000000/1` daje po cichu 410065408/1.
/// Ta sama klasa liczy tez os czasu (CRSMath, SOperations.hpp), ale tamtej sciezki ten plik nie
/// dotyczy - sluzy ewaluatorowi wyrazen i reduktorom w streamInstance, a wariant int takze
/// zwijaniu indeksu generatora w kompilatorze, gdzie nullopt oznacza blad planu, nie NULL.
///
/// Dzielnik rozny od zera jest warunkiem wstepnym `div`; zero obsluguje wolajacy.
namespace checkedArith {

/// Funkcja add powinna:
/// - dla int zwracać sumę albo nullopt, gdy nie mieści się w int32.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - dla unsigned zwracać sumę albo nullopt, gdy przekracza zakres [0, 2^32).
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - dla boost::rational<int> zwracać sumę w postaci skróconej albo nullopt, gdy skrócony licznik lub mianownik nie mieści się w int, a dla dwóch ułamków całkowitych (mianownik 1) liczyć ją przez sumę int32.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
///
/// @note UINT do 2026-09-26 zawijal sie po cichu. Skladniki RATIONAL sa znormalizowane (mianownik dodatni,
///       |licznik| <= 2^31), wiec iloczyny krzyzowe mieszcza sie w 2^62, a ich suma w int64. Skrot przez int32
///       to typowa suma reduktora nad polami INTEGER promowanymi do RATIONAL.
inline std::optional<int> add(int a, int b) {
  int result = 0;
  if (__builtin_add_overflow(a, b, &result)) return std::nullopt;
  return result;
}

/// Funkcja sub powinna:
/// - dla int zwracać różnicę albo nullopt, gdy nie mieści się w int32.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - dla unsigned zwracać różnicę albo nullopt, gdy jest ujemna.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - dla boost::rational<int> zwracać różnicę w postaci skróconej albo nullopt, gdy skrócony licznik lub mianownik nie mieści się w int, a dla dwóch ułamków całkowitych liczyć ją przez różnicę int32.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
///
/// @note UINT do 2026-09-26 zawijal sie po cichu (`3 - 5` dawalo 4294967294).
inline std::optional<int> sub(int a, int b) {
  int result = 0;
  if (__builtin_sub_overflow(a, b, &result)) return std::nullopt;
  return result;
}

/// Funkcja mul powinna:
/// - dla int zwracać iloczyn albo nullopt, gdy nie mieści się w int32.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - dla unsigned zwracać iloczyn albo nullopt, gdy przekracza zakres [0, 2^32).
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - dla boost::rational<int> zwracać iloczyn w postaci skróconej albo nullopt, gdy skrócony licznik lub mianownik nie mieści się w int, a dla dwóch ułamków całkowitych liczyć go przez iloczyn int32.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
inline std::optional<int> mul(int a, int b) {
  int result = 0;
  if (__builtin_mul_overflow(a, b, &result)) return std::nullopt;
  return result;
}

/// Funkcja div powinna:
/// - dla int zwracać iloraz obcięty w stronę zera, a nullopt wyłącznie dla INT_MIN / -1, przy niezerowym dzielniku zapewnionym przez wołającego.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - dla boost::rational<int> zwracać iloraz w postaci skróconej albo nullopt, gdy dzielnik jest zerem albo skrócony licznik lub mianownik nie mieści się w int.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
///
/// @note Jedyny iloraz int32 spoza zakresu to INT_MIN / -1. Iloraz UINT miesci sie zawsze, wiec wariantu
///       unsigned nie ma.
inline std::optional<int> div(int a, int b) {
  if (a == std::numeric_limits<int>::min() && b == -1) return std::nullopt;
  return a / b;
}

/// Funkcja neg powinna:
/// - dla int zwracać -a albo nullopt dla INT_MIN.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - dla boost::rational<int> zwracać -a albo nullopt, gdy licznik jest równy INT_MIN.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
inline std::optional<int> neg(int a) {
  if (a == std::numeric_limits<int>::min()) return std::nullopt;
  return -a;
}

inline std::optional<unsigned> add(unsigned a, unsigned b) {
  unsigned result = 0;
  if (__builtin_add_overflow(a, b, &result)) return std::nullopt;
  return result;
}

inline std::optional<unsigned> sub(unsigned a, unsigned b) {
  unsigned result = 0;
  if (__builtin_sub_overflow(a, b, &result)) return std::nullopt;
  return result;
}

inline std::optional<unsigned> mul(unsigned a, unsigned b) {
  unsigned result = 0;
  if (__builtin_mul_overflow(a, b, &result)) return std::nullopt;
  return result;
}

namespace detail {

/// Funkcja narrowed powinna:
/// - dla zerowego mianownika zwracać nullopt.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - skracać ułamek int64 przez gcd i przenosić znak do licznika, tak by mianownik był dodatni.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
/// - zwracać skrócony ułamek jako boost::rational<int>, a nullopt, gdy skrócony licznik lub mianownik nie mieści się w int.
///   Test: `RequirementContracts.checked_arithmetic_bounds_and_reduced_rationals`.
///
/// @note Dopiero postac skrocona rozstrzyga, czy wartosc NAPRAWDE nie miesci sie w rational<int>, wiec nie
///       ma falszywych alarmow (`1/65536 + 1/65536` przechodzi przez mianownik 2^32 i daje 1/32768).
inline std::optional<boost::rational<int>> narrowed(std::int64_t numerator, std::int64_t denominator) {
  // Zerowy mianownik nie ma postaci skroconej i nie moze dojsc do gcd: gcd(0, 0) to zero, a dzielenie
  // ponizej dzieli wtedy przez zero - SIGFPE na x86-64, cichy 0 na arm64, czyli to samo wejscie i dwa
  // rozne konce procesu. Powstawalo z ulamka 0/0 wczytanego z surowych bajtow; payload takiego juz nie
  // wypuszcza, wiec to straz drugiego rzedu - i jedyna, ktora obowiazuje tez wolajacego `div`.
  if (denominator == 0) return std::nullopt;
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

inline std::optional<boost::rational<int>> add(boost::rational<int> a, boost::rational<int> b) {
  if (a.denominator() == 1 && b.denominator() == 1) {
    const auto sum = add(a.numerator(), b.numerator());
    return sum.has_value() ? std::optional<boost::rational<int>>{*sum} : std::nullopt;
  }
  return detail::narrowed((std::int64_t{a.numerator()} * b.denominator()) + (std::int64_t{b.numerator()} * a.denominator()),
                          std::int64_t{a.denominator()} * b.denominator());
}

inline std::optional<boost::rational<int>> sub(boost::rational<int> a, boost::rational<int> b) {
  if (a.denominator() == 1 && b.denominator() == 1) {
    const auto difference = sub(a.numerator(), b.numerator());
    return difference.has_value() ? std::optional<boost::rational<int>>{*difference} : std::nullopt;
  }
  return detail::narrowed((std::int64_t{a.numerator()} * b.denominator()) - (std::int64_t{b.numerator()} * a.denominator()),
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
