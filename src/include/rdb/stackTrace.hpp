#pragma once

#include <string>

namespace rdb {

/// Slad stosu biezacego watku jako tekst - wylacznie na sciezke diagnostyczna przed rdb::fatal().
///
/// Implementacja (src/rdb/lib/stackTrace.cc) jest wyspa z wyjatkami: naglowki Boost.Stacktrace
/// maja `throw` i `try` w funkcjach inline (zaplecze domyslne, addr_base.hpp w nowszych Boostach),
/// wiec nie kompiluja sie w rdzeniu z -fno-exceptions. Tu nic nie wychodzi wyjatkiem - przy
/// porazce wynik mowi, ze sladu nie ma.
[[nodiscard]] std::string currentStackTrace() noexcept;

}  // namespace rdb
