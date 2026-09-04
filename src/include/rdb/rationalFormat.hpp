#pragma once

#include <format>

#include <fmt/format.h>
#include <boost/rational.hpp>

/// Liczba wymierna wypisywana jako "licznik/mianownik" — dokładnie tak, jak boost::operator<<,
/// z mianownikiem także wtedy, gdy równy 1. Ani Boost 1.91, ani fmt nie dostarczają tego same,
/// a projekt formatuje przez oba silniki: std::print/std::format oraz fmt (spdlog, FatalError).
/// Stąd dwie bliźniacze specjalizacje. Żadna nie obsługuje specyfikatora formatu — dopuszczalne
/// jest wyłącznie puste "{}".
///
/// Uwaga przy podnoszeniu Boosta: gdyby biblioteka dodała własny std::formatter dla rational,
/// poniższa specjalizacja przestanie się kompilować i trzeba ją wtedy usunąć.
template <typename IntType>
struct std::formatter<boost::rational<IntType>, char> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
  auto format(const boost::rational<IntType> &value, std::format_context &ctx) const {
    return std::format_to(ctx.out(), "{}/{}", value.numerator(), value.denominator());
  }
};

template <typename IntType>
struct fmt::formatter<boost::rational<IntType>, char> {
  constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }
  auto format(const boost::rational<IntType> &value, fmt::format_context &ctx) const {
    return fmt::format_to(ctx.out(), "{}/{}", value.numerator(), value.denominator());
  }
};
