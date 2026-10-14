#include <gtest/gtest.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include "config.h"
#include "fatalError.hpp"

// Adnotacje pojemnosci std::vector w libc++ (macOS) wymagaja instrumentacji calego
// kodu dotykajacego wektora. GoogleTest z Conana nie ma sanitizerow, wiec polaczenie
// jego funkcji inline z instrumentowanymi kopiami moze dac falszywy alarm ASan
// juz podczas rejestracji testow, przed main(). libstdc++ (GCC na Linuksie) stawia
// te adnotacje tylko z -D_GLIBCXX_SANITIZE_VECTOR, ktorego build nie ustawia, wiec
// tam opcja niczego nie zmienia. Wylaczamy tylko te kontrole; pozostale kontrole
// pamieci ASan nadal dzialaja. ASAN_OPTIONS z otoczenia ma pierwszenstwo, np.
// detect_container_overflow=1 przywraca kontrole.
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
  rdb::setFatalHandler(&daemonFatalExit);
  auto filelog = spdlog::basic_logger_mt("log", std::string(argv[0]) + ".log");
  spdlog::set_default_logger(filelog);
  constexpr auto common_log_pattern = "%C%m%d %T.%e %^%s:%# [%L] %v%$";
  spdlog::set_pattern(common_log_pattern);
  spdlog::flush_on(spdlog::level::trace);
  SPDLOG_INFO("{}", config_line);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
