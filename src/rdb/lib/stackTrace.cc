#include "rdb/stackTrace.hpp"

// Wyspa z wyjatkami (rdb_exception_island w src/rdb/lib/CMakeLists.txt). Boost.Stacktrace
// zglasza bledy wylacznie rzutem i ma `throw`/`try` w funkcjach inline swoich naglowkow -
// zaplecze domyslne (frame_unwind.ipp -> addr_base.hpp) od Boosta, ktory instaluje Conan na
// macOS. W rdzeniu z -fno-exceptions to blad kompilacji, wiec cale uzycie Boost.Stacktrace
// mieszka tutaj, a reszta rdzenia dostaje gotowy napis.

// Wybor zaplecza boost::stacktrace. addr2line rozwiazuje adresy najdokladniej
// (nazwa pliku i numer linii), ale jest osobnym programem z binutils, ktorego na
// czesci systemow nie ma w ogole - na macOS odpowiednikiem jest atos, o innym
// interfejsie. Bez tego makra Boost bierze zaplecze domyslne (backtrace + dladdr):
// slad jest krotszy o numery linii, ale POWSTAJE, zamiast czekac 60 s na program,
// ktorego nie ma. Obecnosc addr2line sprawdza find_program w
// cmake/PlatformChecks.cmake, wiec decyzja zapada raz, przy konfiguracji.
#include "platformConfig.h"

#if RDB_HAS_ADDR2LINE
#define BOOST_STACKTRACE_USE_ADDR2LINE
#endif

// Zaplecze domyslne (rozwijanie stosu + dladdr) stoi na _Unwind_Backtrace, a Boost
// odmawia jego uzycia, dopoki nie zobaczy _GNU_SOURCE. To jest warunek o GLIBC, nie
// o dostepnosci samej funkcji: na Linuksie _GNU_SOURCE definiuje za nas libstdc++,
// wiec byl spelniony przypadkiem i nikt go nie zauwazyl. Poza glibc
// _Unwind_Backtrace pochodzi z libunwind i jest deklarowane bezwarunkowo, wiec nie
// ma tu czego sprawdzac - i tyle Boostowi mowimy. Bez tego jedyny plik w drzewie
// uzywajacy boost::stacktrace nie kompiluje sie wcale.
#if !defined(_GNU_SOURCE) && !defined(BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#endif

#include <boost/stacktrace.hpp>

#include <sstream>
#include <string>

namespace rdb {

std::string currentStackTrace() noexcept {
  // Wyjatek nie ma prawa opuscic wyspy: wolajacy (payload.cc) jest skompilowany bez wyjatkow,
  // a zaraz potem i tak konczy proces przez rdb::fatal(). Brak sladu to tylko ubozsza diagnoza.
  try {
    std::ostringstream text;
    text << boost::stacktrace::stacktrace();
    return text.str();
  } catch (...) {  // NOLINT(bugprone-empty-catch)
    return "(stack trace unavailable)";
  }
}

}  // namespace rdb
