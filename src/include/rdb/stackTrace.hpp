#pragma once

#include <string>

namespace rdb {

/// Funkcja currentStackTrace powinna:
/// - [stacktrace.current-stack-trace.1~1] zwracać ślad stosu bieżącego wątku jako tekst, a przy porażce napis "(stack trace unavailable)".
/// - [stacktrace.current-stack-trace.2~1] nie przepuszczać żadnego wyjątku do wołającego.
///   Sprawdzenie kompilacji: test/UnitTest/test_stackTrace.cpp - static_assert(noexcept(rdb::currentStackTrace())).
///
/// @note Wylacznie na sciezke diagnostyczna przed rdb::fatal(). Implementacja (src/rdb/lib/stackTrace.cc) jest
///       wyspa z wyjatkami: naglowki Boost.Stacktrace nie kompiluja sie w rdzeniu z -fno-exceptions.
[[nodiscard]] std::string currentStackTrace() noexcept;

}  // namespace rdb
