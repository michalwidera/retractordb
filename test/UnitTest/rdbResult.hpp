#pragma once

#include <gtest/gtest.h>

#include <source_location>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "rdb/error.hpp"

/// @file
/// @brief Pomocnicy testow dla rdb::Result (rdb/error.hpp).
///
/// Rdzen silnika nie rzuca - bledy wracaja wartoscia. Testy buduja sie Z wyjatkami, wiec
/// rdbtest::ok() zamienia nieoczekiwany blad na wyjatek testu z trescia i kategoria bledu:
/// gtest raportuje go jako porazke z komunikatem, zamiast golego bad_expected_access.
/// Oczekiwany blad sprawdzaja EXPECT_RDB_ERROR / ASSERT_RDB_ERROR - kategoria plus (opcjonalnie)
/// fragment komunikatu, czyli to, co przed zmiana sprawdzaly EXPECT_THROW i ThrowsMessage.
namespace rdbtest {

/// Wartosc wyniku albo std::runtime_error z kategoria, trescia i miejscem wywolania.
template <typename T>
T ok(rdb::Result<T> result, const std::source_location where = std::source_location::current()) {
  if (!result) {
    throw std::runtime_error(std::string(where.file_name()) + ":" + std::to_string(where.line()) + ": unexpected " +
                             std::string(rdb::errcName(result.error().code())) + ": " + result.error().message());
  }
  if constexpr (!std::is_void_v<T>) return std::move(*result);
}

/// Wynik jest bledem kategorii `code`, a jego tresc zawiera `fragment` (pusty = bez sprawdzania).
template <typename T>
::testing::AssertionResult isError(const rdb::Result<T> &result, const rdb::Errc code, const std::string &fragment = {}) {
  if (result) return ::testing::AssertionFailure() << "expected " << rdb::errcName(code) << ", got a value";
  if (result.error().code() != code) {
    return ::testing::AssertionFailure() << "expected " << rdb::errcName(code) << ", got "
                                         << rdb::errcName(result.error().code()) << ": " << result.error().message();
  }
  if (!fragment.empty() && result.error().message().find(fragment) == std::string::npos) {
    return ::testing::AssertionFailure() << "message '" << result.error().message() << "' does not contain '" << fragment << "'";
  }
  return ::testing::AssertionSuccess();
}

/// Wynik niesie wartosc; przy bledzie komunikat testu podaje kategorie i tresc.
template <typename T>
::testing::AssertionResult isOk(const rdb::Result<T> &result) {
  if (result) return ::testing::AssertionSuccess();
  return ::testing::AssertionFailure() << rdb::errcName(result.error().code()) << ": " << result.error().message();
}

}  // namespace rdbtest

#define EXPECT_RDB_OK(...)          EXPECT_TRUE(::rdbtest::isOk(__VA_ARGS__))
#define ASSERT_RDB_OK(...)          ASSERT_TRUE(::rdbtest::isOk(__VA_ARGS__))
#define EXPECT_RDB_ERROR(expr, ...) EXPECT_TRUE(::rdbtest::isError((expr), __VA_ARGS__))
#define ASSERT_RDB_ERROR(expr, ...) ASSERT_TRUE(::rdbtest::isError((expr), __VA_ARGS__))
