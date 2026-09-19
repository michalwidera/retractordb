#pragma once

// Podmiana wywolania systemowego na wersje testowa - to samo, co robi
// `-Wl,--wrap=<symbol>`, ale dostepne takze tam, gdzie konsolidator tej opcji nie ma.
//
// Trzy testy w tym katalogu (test_faccposix, test_faccposixshd, test_dumpManager)
// wstrzykuja EINTR i krotkie zapisy do kodu, ktory sam wola pread/write/lseek.
// Jedyna droga do tego jest przejecie symbolu miedzy kodem badanym a libc.
//
// Na GNU ld i lld robi to konsolidator: `--wrap=pread` zamienia KAZDE odwolanie
// do `pread` na `__wrap_pread`, a `__real_pread` wskazuje na oryginal. Wtedy ten
// naglowek nie generuje niczego - wystarcza to, co test juz ma.
//
// ld64 Apple'a nie ma `--wrap` i nie ma odpowiednika ("-alias" nadaje symbolowi
// DRUGA nazwe, nie przekierowuje istniejacych odwolan). Odpowiednikiem jest tam
// klasyczna podmiana na etapie konsolidacji statycznej: definiujemy symbol o nazwie
// systemowej we wlasnym pliku obiektowym. Konsolidator rozwiazuje odwolania
// najpierw z plikow obiektowych i archiwow, a dopiero potem z bibliotek dzielonych,
// wiec kod badany - zlinkowany z archiwum - trafia na NASZA definicje, a nie na te
// z libSystem. Do oryginalu wracamy przez dlsym(RTLD_NEXT, ...), czyli "ten sam
// symbol w nastepnym obrazie", co jest dokladnie tym, czym jest __real_.
//
// Uzycie (raz na symbol, obok definicji __wrap_<symbol> w tescie):
//   RDB_WRAP_SYSCALL(ssize_t, pread, (int fd, void *buf, size_t n, off_t off), (fd, buf, n, off))
// Nawiasy wokol listy parametrow i listy argumentow sa obowiazkowe: dzieki nim
// przecinki w srodku nie rozbijaja wywolania makra na wiecej argumentow.

#include "platformConfig.h"

#if RDB_HAS_LD_WRAP

// Konsolidator zalatwia calosc; test deklaruje __real_<symbol> i definiuje
// __wrap_<symbol> tak jak dotad.
#define RDB_WRAP_SYSCALL(returnType, name, parameters, arguments) static_assert(true, "")

#else

#include <dlfcn.h>

#define RDB_WRAP_SYSCALL(returnType, name, parameters, arguments)                                   \
  extern "C" returnType __wrap_##name parameters;                                                   \
  extern "C" returnType __real_##name parameters {                                                  \
    using RdbNextFunction = returnType(*) parameters;                                               \
    /* Rozwiazywane raz: dlsym przy kazdym wywolaniu zmienialoby koszt sciezki, */                  \
    /* ktora te testy mierza jako "zwykly odczyt" obok wstrzyknietego EINTR.    */                  \
    /* Zwykly wskaznik, nie static z inicjalizacja przy pierwszym uzyciu:      */                   \
    /* straznik takiej inicjalizacji jest rekurencyjnie niebezpieczny, gdyby    */                  \
    /* dlsym samo siegnelo po ten symbol. Wyscig dwoch watkow jest tu nieszkodliwy - */             \
    /* obydwa zapisza te sama wartosc.                                          */                  \
    static RdbNextFunction rdbNext = nullptr;                                                       \
    if (rdbNext == nullptr) rdbNext = reinterpret_cast<RdbNextFunction>(::dlsym(RTLD_NEXT, #name)); \
    return rdbNext arguments;                                                                       \
  }                                                                                                 \
  extern "C" returnType name parameters { return __wrap_##name arguments; }                         \
  static_assert(true, "")

#endif
