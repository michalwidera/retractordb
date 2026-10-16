#pragma once

#include <string>

namespace rdb {

/// Funkcja currentStackTrace powinna:
/// - zwracać ślad stosu bieżącego wątku jako tekst, a przy porażce napis "(stack trace unavailable)".
/// - nie przepuszczać żadnego wyjątku do wołającego.
///
/// @note Wylacznie na sciezke diagnostyczna przed rdb::fatal(). Implementacja (src/rdb/lib/stackTrace.cc) jest
///       wyspa z wyjatkami: naglowki Boost.Stacktrace nie kompiluja sie w rdzeniu z -fno-exceptions.
[[nodiscard]] std::string currentStackTrace() noexcept;

}  // namespace rdb
