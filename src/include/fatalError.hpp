#pragma once

#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <source_location>

/// Podniesiona przez FatalError tuz przed std::exit. Handlery zarejestrowane przez
/// std::atexit czytaja ja, zeby odroznic zakonczenie KRYTYCZNE od zwyklego wyjscia:
/// std::exit uruchamia te same handlery w obu przypadkach, a rozstrzygniecie po kodzie
/// wyjscia jest tam niedostepne. Uzywa jej executorsm::cleanup() -- usluga systemd, ktora
/// zginela na bledzie krytycznym, ma wstac BEZ planu, zamiast wstawac w kolko na planie,
/// ktory wlasnie ja zabil.
inline std::atomic<bool> fatalErrorRaised{false};

// [[noreturn]] replacement for the old FATAL_ERROR macro.
// The struct+CTAD idiom allows std::source_location as a defaulted trailing
// parameter alongside a variadic template — impossible with a plain function.
// Format string is checked at compile time via fmt::format_string<Args...>.
template <typename... Args>
struct FatalError {
  [[noreturn]] FatalError(fmt::format_string<Args...> fmt_str, Args &&...args,
                          std::source_location loc = std::source_location::current()) {
    // Zatrzask PRZED logowaniem: handler atexit ma poznac prawde takze wtedy, gdy
    // samo logowanie ponizej sie wywroci.
    fatalErrorRaised.store(true, std::memory_order_release);
    auto msg = fmt::format(fmt_str, std::forward<Args>(args)...);
    if (auto logger = spdlog::default_logger(); logger) {
      logger->log(spdlog::source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()},
                  spdlog::level::critical, msg);
      // FLUSH, nie shutdown. std::exit ponizej uruchamia funkcje zarejestrowane przez
      // std::atexit, a te loguja — executorsm::cleanup() zaczyna od SPDLOG_WARN. Po
      // spdlog::shutdown() rejestr jest pusty i default_logger_raw() zwraca nullptr, wiec
      // makro SPDLOG_* wolalo should_log() na wskazniku zerowym: KAZDY blad krytyczny
      // konczyl sie SIGSEGV w atexit, tuz po wypisaniu wlasciwego komunikatu. Proces
      // zwracal 139 zamiast EXIT_FAILURE, komunikat ginal za sladem crashu, a IPC
      // (RetractorShmemMap, RetractorQueryQueue) zostawal nieposprzatany, bo cleanup()
      // ginal przed swoimi wywolaniami remove().
      //
      // Flush wystarcza do trwalosci: wszystkie sinki tego projektu sa SYNCHRONICZNE
      // (basic_file_sink_mt, stderr_sink_mt — patrz uxSysTermTools.cpp::logger), nie ma
      // ani jednego loggera asynchronicznego, ktory wymagalby drenowania kolejki.
      // Rejestr zamyka sie sam przy destrukcji statykow, juz PO handlerach atexit:
      // spdlog::registry::instance() powstaje przy konfiguracji logowania, czyli wczesniej
      // niz std::atexit(cleanup), a kolejnosc sprzatania jest odwrotna do rejestracji.
      logger->flush();
    }
    std::cerr << "\nFATAL: " << msg << "\n";
    std::exit(EXIT_FAILURE);
  }
};

template <typename... Args>
FatalError(fmt::format_string<Args...>, Args &&...) -> FatalError<Args...>;
