#pragma once

#include <spdlog/spdlog.h>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>

#include "rdb/error.hpp"

/// Podniesiona przez FatalError tuz przed std::exit. Handlery zarejestrowane przez
/// std::atexit czytaja ja, zeby odroznic zakonczenie KRYTYCZNE od zwyklego wyjscia:
/// std::exit uruchamia te same handlery w obu przypadkach, a rozstrzygniecie po kodzie
/// wyjscia jest tam niedostepne. Uzywa jej executorsm::cleanup() -- usluga systemd, ktora
/// zginela na bledzie krytycznym, ma wstac BEZ planu, zamiast wstawac w kolko na planie,
/// ktory wlasnie ja zabil.
inline std::atomic<bool> fatalErrorRaised{false};

namespace rdb {

namespace detail {

/// Funkcja terminateProcess powinna:
/// - kończyć proces wyłącznie dla klas Internal (K1) i Environment (K4), co sprawdza się w czasie kompilacji.
/// - dla klasy Internal ustawiać zatrzask fatalErrorRaised przed logowaniem, a dla Environment pozostawiać go bez zmian.
/// - logować sformatowany komunikat na poziomie critical z miejscem wywołania (dla Environment z przedrostkiem "environment: "), opróżniać logger bez jego zamykania i wypisywać komunikat na stderr z przedrostkiem "FATAL: " albo "FATAL (environment): ".
/// - nigdy nie zgłaszać wyjątku: błąd formatowania lub logowania pomijać i mimo to zakończyć proces.
///   Test: `FatalErrorTest.FormattingFailureStillRunsAtexit`.
/// - kończyć proces przez std::exit z kodem EXIT_FAILURE dla Internal i EIO dla Environment, tak aby wykonały się handlery atexit.
///   Test: `FatalErrorTest.FormattingFailureStillRunsAtexit`.
///
/// @note Wolana takze z noexcept-owych operatorow przenoszacych. Flush zamiast spdlog::shutdown: handlery atexit
///       loguja (executorsm::cleanup), a po shutdown makra SPDLOG_* konczyly sie SIGSEGV. Blad srodowiska nie
///       obciaza planu, wiec usluga wstaje po nim z tym samym planem.
template <ErrorClass Cls, typename... Args>
[[noreturn]] void terminateProcess(std::source_location loc, fmt::format_string<Args...> fmt_str, Args &&...args) noexcept {
  static_assert(Cls == ErrorClass::Internal || Cls == ErrorClass::Environment,
                "proces koncza tylko klasy Internal (K1) i Environment (K4)");
  // Zatrzask PRZED logowaniem: handler atexit ma poznac prawde takze wtedy, gdy
  // samo logowanie ponizej sie wywroci. Tylko K1 - blad srodowiska nie obciaza planu,
  // wiec usluga ma po nim wstac z tym samym planem.
  if constexpr (Cls == ErrorClass::Internal) fatalErrorRaised.store(true, std::memory_order_release);
  // Akcja jest wolana m.in. z noexcept-owych operatorow przenoszacych, wiec NIE WOLNO jej
  // rzucic - a fmt::format potrafi (format_error, bad_alloc), zanim sterowanie dojdzie do
  // std::exit. Stad catch-all: wazne jest wyjscie z procesu, nie komunikat.
  try {
    auto msg = fmt::format(fmt_str, std::forward<Args>(args)...);
    // Klasa w komunikacie, takze w Release: skrypt i operator odrozniaja awarie srodowiska od
    // bledu silnika bez zagladania w kod wyjscia. Przedrostek K4 zaczyna sie od "FATAL", wiec
    // dotychczasowe wyszukiwanie po nim lapie obie klasy.
    constexpr const char *stderrPrefix = Cls == ErrorClass::Internal ? "\nFATAL: " : "\nFATAL (environment): ";
    constexpr const char *logPrefix    = Cls == ErrorClass::Internal ? "" : "environment: ";
    if (auto logger = spdlog::default_logger(); logger) {
      logger->log(spdlog::source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()},
                  spdlog::level::critical, "{}{}", logPrefix, msg);
      // FLUSH, nie shutdown. std::exit ponizej uruchamia funkcje zarejestrowane przez
      // std::atexit, a te loguja - executorsm::cleanup() zaczyna od SPDLOG_WARN. Po
      // spdlog::shutdown() rejestr jest pusty i default_logger_raw() zwraca nullptr, wiec
      // makro SPDLOG_* wolalo should_log() na wskazniku zerowym: KAZDY blad krytyczny
      // konczyl sie SIGSEGV w atexit, tuz po wypisaniu wlasciwego komunikatu. Proces
      // zwracal 139 zamiast EXIT_FAILURE, komunikat ginal za sladem crashu, a IPC
      // (segment odpowiedzi, RetractorQueryQueue) zostawal nieposprzatany, bo cleanup()
      // ginal przed swoimi wywolaniami remove().
      //
      // Flush wystarcza do trwalosci: wszystkie sinki tego projektu sa SYNCHRONICZNE
      // (basic_file_sink_mt, stderr_sink_mt - patrz uxSysTermTools.cpp::logger), nie ma
      // ani jednego loggera asynchronicznego, ktory wymagalby drenowania kolejki.
      // Rejestr zamyka sie sam przy destrukcji statykow, juz PO handlerach atexit:
      // spdlog::registry::instance() powstaje przy konfiguracji logowania, czyli wczesniej
      // niz std::atexit(cleanup), a kolejnosc sprzatania jest odwrotna do rejestracji.
      logger->flush();
    }
    std::cerr << stderrPrefix << msg << "\n";
  } catch (...) {  // NOLINT(bugprone-empty-catch) - pusty z rozmysla: rzut stad zabralby std::exit i handlerom atexit
  }
  std::exit(Cls == ErrorClass::Internal ? EXIT_FAILURE : EIO);
}

}  // namespace detail

// [[noreturn]] replacement for the old FATAL_ERROR macro.
// The struct+CTAD idiom allows std::source_location as a defaulted trailing
// parameter alongside a variadic template - impossible with a plain function.
// Format string is checked at compile time via fmt::format_string<Args...>.
/// @requirements-exempt FatalError: Adapter CTAD terminateProcess dla kategorii Internal, bez wlasnego stanu i logiki.
template <typename... Args>
struct FatalError {
  [[noreturn]] FatalError(fmt::format_string<Args...> fmt_str, Args &&...args,
                          std::source_location loc = std::source_location::current()) {
    detail::terminateProcess<ErrorClass::Internal, Args...>(loc, fmt_str, std::forward<Args>(args)...);
  }
};

template <typename... Args>
FatalError(fmt::format_string<Args...>, Args &&...) -> FatalError<Args...>;

/// K4: srodowisko odmowilo (plik, dysk, pamiec dzielona). Konczy proces kodem EIO i nie
/// podnosi zatrzasku fatalErrorRaised, wiec usluga wstaje po nim z tym samym planem.
/// @requirements-exempt EnvironmentError: Adapter CTAD terminateProcess dla kategorii Environment, bez wlasnego stanu i logiki.
template <typename... Args>
struct EnvironmentError {
  [[noreturn]] EnvironmentError(fmt::format_string<Args...> fmt_str, Args &&...args,
                                std::source_location loc = std::source_location::current()) {
    detail::terminateProcess<ErrorClass::Environment, Args...>(loc, fmt_str, std::forward<Args>(args)...);
  }
};

template <typename... Args>
EnvironmentError(fmt::format_string<Args...>, Args &&...) -> EnvironmentError<Args...>;

}  // namespace rdb

// 235 miejsc wywolan pisze FatalError bez przestrzeni nazw; CTAD dziala przez
// using-declaration, bo przewodniki dedukcji naleza do szablonu, nie do nazwy.
using rdb::FatalError;

/// Funkcja daemonFatalExit powinna:
/// - kończyć proces usługi jak FatalError klasy Internal (terminateProcess), z komunikatem i miejscem przekazanym przez fatal(), zachowując std::exit i handlery atexit.
///
/// @note Handler fatal() demona; rdzen z Result wola fatal bezposrednio.
[[noreturn]] inline void daemonFatalExit(std::string_view message, const std::source_location &where) noexcept {
  rdb::detail::terminateProcess<rdb::ErrorClass::Internal>(where, "{}", message);
}
