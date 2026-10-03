#include <gtest/gtest.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include "config.h"

// AddressSanitizer w testach jednostkowych: bez kontroli przepelnienia kontenera.
//
// Kontrola "container overflow" dziala na adnotacjach, ktore libc++ zaklada na pojemnosc
// std::vector - ale tylko w kodzie instrumentowanym, i jest wiarygodna tylko wtedy, gdy
// instrumentowany jest CALY kod dotykajacy tego samego wektora. Testy linkuja GoogleTest z Conana,
// zbudowany bez sanitizerow. Funkcje inline wektora konsolidator skleja w jedna kopie na symbol, a
// od RDB_NO_EXCEPTIONS rdzen (-fno-exceptions) i gtest (z wyjatkami) maja inne znaczniki ABI libc++
// ([abi:..n..] wobec [abi:..e..]), wiec czesc kopii przychodzi z instrumentowanego rdzenia, a
// czesc z gtest. Wzrost TestSuite::test_indices_ w gtest szedl wtedy przez kopie z rdzenia, ktora
// stawiala adnotacje, a nastepny push_back gtest juz jej nie przesuwal: kolejny wzrost czytal
// wlasne elementy jako zatrute. Na macOS (--sanitize) ut_planSource i ut_qTree padaly na tym
// jeszcze przed main(), w rejestracji testow - "heap-buffer-overflow", bo granica adnotacji wypada
// w polowie granulki. Ten sam uklad (rdzen bez wyjatkow z ASan, biblioteka bez ASan) odtwarza to
// na Linuksie z libc++; z rdzeniem budowanym z wyjatkami alarmu nie ma.
//
// Dokumentacja ASan zaleca wtedy wlasnie detect_container_overflow=0. Pozostale kontrole ASan
// (przepelnienie sterty i stosu, uzycie po zwolnieniu) dzialaja dalej. Valgrind - straz pamieci na
// Linuksie - przepelnienia kontenera tez nie widzi, wiec obie platformy pilnuja tego samego.
// ASAN_OPTIONS z otoczenia nadal wygrywa: ASAN_OPTIONS=detect_container_overflow=1 wlacza kontrole.
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define RDB_UNIT_TEST_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define RDB_UNIT_TEST_ASAN 1
#endif

#ifdef RDB_UNIT_TEST_ASAN
extern "C" __attribute__((visibility("default"), used, no_sanitize("address", "undefined"))) const char *
__asan_default_options() {  // NOLINT(bugprone-reserved-identifier,readability-identifier-naming)
  return "detect_container_overflow=0";
}
#endif

int main(int argc, char *argv[]) {  // NOLINT(bugprone-exception-escape)
  auto filelog = spdlog::basic_logger_mt("log", std::string(argv[0]) + ".log");
  spdlog::set_default_logger(filelog);
  constexpr auto common_log_pattern = "%C%m%d %T.%e %^%s:%# [%L] %v%$";
  spdlog::set_pattern(common_log_pattern);
  spdlog::flush_on(spdlog::level::trace);
  SPDLOG_INFO("{}", config_line);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
