#pragma once

// Faza 2 refaktoru przeniosla ten naglowek z src/include (katalogu DZIELONEGO) tutaj, do
// prywatnych naglowkow serwera. Powod jest jeden i jest to caly sens fazy 2: plik definiuje
// `inline std::atomic<bool> fatalErrorRaised`, czyli stan CALEGO PROCESU, a stal w katalogu,
// z ktorego bierze naglowki takze warstwa magazynu - ta sama, ktora laduje notatnik. Po fazie 1
// `src/rdb` nie siega juz po FatalError ani razu, wiec nic tam tego nie potrzebowalo; zostawala
// tylko mozliwosc, ze ktos siegnie. Teraz nie ma jej z czego wziac.
//
// Zostaje wylacznie jako mechanizm WYJSCIA PROCESU xretractor: daemonFatalExit jest handlerem
// rdb::fatal() serwera (RDB_ASSERT w rdzeniu, hak RDB_FAULT_FATAL_IN_SLOT w dataModel.cpp), a
// FatalError - jedyne zywe wywolanie to hak RDB_FAULT_FATAL_IN_ADHOC (executorsmAdHoc.cpp) - jest
// jego nakladka z formatowaniem. Zatrzask czytaja executorsm::cleanup() i launcher.cpp. Jego
// wlasciwy zasieg to proces serwera i tam wlasnie teraz mieszka.

#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>

/// Podniesiona przez FatalError tuz przed std::exit. Handlery zarejestrowane przez
/// std::atexit czytaja ja, zeby odroznic zakonczenie KRYTYCZNE od zwyklego wyjscia:
/// std::exit uruchamia te same handlery w obu przypadkach, a rozstrzygniecie po kodzie
/// wyjscia jest tam niedostepne. Uzywa jej executorsm::cleanup() -- usluga systemd, ktora
/// zginela na bledzie krytycznym, ma wstac BEZ planu, zamiast wstawac w kolko na planie,
/// ktory wlasnie ja zabil.
inline std::atomic<bool> fatalErrorRaised{false};

/// Wyjscie procesu serwera po bledzie krytycznym: zatrzask, log krytyczny z miejscem, flush,
/// "FATAL: <tresc>" na stderr, std::exit(EXIT_FAILURE). To jest takze handler rdb::fatal() dla
/// xretractor - launcher instaluje go pierwsza instrukcja main() (rdb::setFatalHandler), wiec
/// zlamany niezmiennik rdzenia (RDB_ASSERT) konczy serwer ta sama droga co FatalError: z
/// handlerami atexit (executorsm::cleanup sprzata IPC i plik zapytan), a nie przez std::abort.
///
/// Sygnatura jest typem rdb::FatalHandler; noexcept, bo wolaja go takze miejsca noexcept.
[[noreturn]] inline void daemonFatalExit(std::string_view message, const std::source_location &where) noexcept {
  // Zatrzask PRZED logowaniem: handler atexit ma poznac prawde takze wtedy, gdy
  // samo logowanie ponizej sie wywroci.
  fatalErrorRaised.store(true, std::memory_order_release);
  // Ta jednostka buduje sie z wyjatkami (biblioteka retractor, nie rdzen), a logowanie i
  // iostream potrafia rzucic (bad_alloc), zanim sterowanie dojdzie do std::exit. Stad
  // catch-all: wazne jest wyjscie z procesu, nie komunikat.
  try {
    if (auto logger = spdlog::default_logger(); logger) {
      logger->log(spdlog::source_loc{where.file_name(), static_cast<int>(where.line()), where.function_name()},
                  spdlog::level::critical, message);
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
    std::cerr << "\nFATAL: " << message << "\n";
  } catch (...) {  // NOLINT(bugprone-empty-catch) - pusty z rozmysla: rzut stad zabralby std::exit i handlerom atexit
  }
  std::exit(EXIT_FAILURE);
}

// [[noreturn]] replacement for the old FATAL_ERROR macro.
// The struct+CTAD idiom allows std::source_location as a defaulted trailing
// parameter alongside a variadic template - impossible with a plain function.
// Format string is checked at compile time via fmt::format_string<Args...>.
template <typename... Args>
struct FatalError {
  [[noreturn]] FatalError(fmt::format_string<Args...> fmt_str, Args &&...args,
                          std::source_location loc = std::source_location::current()) noexcept {
    std::string msg;
    // fmt::format potrafi rzucic (format_error, bad_alloc); bez tresci wyjscie i tak ma zajsc.
    try {
      msg = fmt::format(fmt_str, std::forward<Args>(args)...);
    } catch (...) {  // NOLINT(bugprone-empty-catch)
    }
    daemonFatalExit(msg, loc);
  }
};

template <typename... Args>
FatalError(fmt::format_string<Args...>, Args &&...) -> FatalError<Args...>;
