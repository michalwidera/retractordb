#pragma once

#include <spdlog/spdlog.h>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <source_location>
#include <string>

/// Podniesiona przez FatalError tuz przed std::exit. Handlery zarejestrowane przez
/// std::atexit czytaja ja, zeby odroznic zakonczenie KRYTYCZNE od zwyklego wyjscia:
/// std::exit uruchamia te same handlery w obu przypadkach, a rozstrzygniecie po kodzie
/// wyjscia jest tam niedostepne. Uzywa jej executorsm::cleanup() -- usluga systemd, ktora
/// zginela na bledzie krytycznym, ma wstac BEZ planu, zamiast wstawac w kolko na planie,
/// ktory wlasnie ja zabil.
inline std::atomic<bool> fatalErrorRaised{false};

namespace rdb {

/// Klasa bledu na granicy silnika - kategorie K1-K5 z analizy w #463. K6 (ostrzezenia)
/// zostaje lista komunikatow, K7 nie jest kategoria do pisania. Statusy lokalne
/// (ReadStatus, WriteStatus, ClaimStatus, lockFile::Result) zostaja w swoich miejscach.
enum class ErrorClass {
  Internal,     // K1: naruszony niezmiennik silnika - FatalError
  Plan,         // K2: plan odrzucony przed startem
  Request,      // K3: odrzucone zadanie ad hoc / IPC
  Environment,  // K4: srodowisko (plik, dysk, pamiec dzielona) - EnvironmentError
  Data          // K5: wadliwa wartosc w danych - NULL i ostrzezenie
};

/// Odmowa na granicy planu: pusty message oznacza sukces, err zachowuje kod systemowy K4.
struct Refusal {
  ErrorClass cls{ErrorClass::Plan};
  std::string message;
  int err{0};
};

/// K1 albo K4 zgloszony w zakresie ThrowOnFatalScope zamiast zakonczenia procesu (#476).
/// Celowo NIE dziedziczy po std::exception: rdzen ma handlery `catch (const std::exception &)`
/// (foldConstants w exprSimplify.cpp otacza nim ewaluator), ktore polknelyby blad krytyczny
/// i liczyly dalej tam, gdzie serwis konczy proces. Lapie go tylko granica, ktora zalozyla zakres.
struct EngineFatal {
  ErrorClass cls;
  std::string message;
  std::source_location where;
};

namespace detail {

/// Glebokosc zakresow ThrowOnFatalScope w biezacym watku.
inline thread_local int throwOnFatalDepth = 0;

/// Czy FatalError / EnvironmentError rzuca teraz EngineFatal: tylko w zakresie ThrowOnFatalScope
/// i nigdy w trakcie zwijania stosu - drugi wyjatek oznaczalby std::terminate.
inline bool throwOnFatalActive() noexcept { return throwOnFatalDepth > 0 && std::uncaught_exceptions() == 0; }

/// Wspolna akcja konczaca proces dla FatalError (K1) i EnvironmentError (K4). Klasa
/// decyduje tylko o kodzie wyjscia i o zatrzasku fatalErrorRaised; reszta jest wspolna.
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

/// Wybor akcji dla FatalError / EnvironmentError: rzut EngineFatal, gdy throwOnFatalActive(),
/// w przeciwnym razie zakonczenie procesu. Rzut bez zatrzasku fatalErrorRaised - to stan procesu
/// uslugi. Wywolanie z funkcji noexcept (payload::operator=(payload &&)) konczy sie w trybie rzucaj
/// std::terminate - znane miejsca: #463, kontrola bugprone-exception-escape.
template <ErrorClass Cls, typename... Args>
[[noreturn]] void raise(std::source_location loc, fmt::format_string<Args...> fmt_str, Args &&...args) {
  if (throwOnFatalActive()) {
    std::string msg;
    try {
      msg = fmt::format(fmt_str, std::forward<Args>(args)...);
    } catch (...) {  // NOLINT(bugprone-empty-catch) - jak w terminateProcess: wazna jest klasa, nie komunikat
    }
    if (auto logger = spdlog::default_logger(); logger)
      logger->log(spdlog::source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()},
                  spdlog::level::critical, "{}{}", Cls == ErrorClass::Internal ? "" : "environment: ", msg);
    throw EngineFatal{Cls, std::move(msg), loc};
  }
  terminateProcess<Cls, Args...>(loc, fmt_str, std::forward<Args>(args)...);
}

}  // namespace detail

/// Zakres trybu rzucaj w biezacym watku: FatalError i EnvironmentError rzucaja EngineFatal
/// zamiast konczyc proces. Zaklada go tylko granica biblioteki osadzonej (fasada Engine, #476)
/// na czas jednego wywolania; xretractor (serwis i standalone) nigdy, wiec tam obie akcje
/// koncza proces jak dotad. Watki IPC i wykonawcze zakresu nie dziedzicza.
class ThrowOnFatalScope {
 public:
  ThrowOnFatalScope() noexcept { ++detail::throwOnFatalDepth; }
  ~ThrowOnFatalScope() { --detail::throwOnFatalDepth; }
  ThrowOnFatalScope(const ThrowOnFatalScope &)            = delete;
  ThrowOnFatalScope &operator=(const ThrowOnFatalScope &) = delete;
};

// [[noreturn]] replacement for the old FATAL_ERROR macro.
// The struct+CTAD idiom allows std::source_location as a defaulted trailing
// parameter alongside a variadic template - impossible with a plain function.
// Format string is checked at compile time via fmt::format_string<Args...>.
template <typename... Args>
struct FatalError {
  [[noreturn]] FatalError(fmt::format_string<Args...> fmt_str, Args &&...args,
                          std::source_location loc = std::source_location::current()) {
    detail::raise<ErrorClass::Internal, Args...>(loc, fmt_str, std::forward<Args>(args)...);
  }
};

template <typename... Args>
FatalError(fmt::format_string<Args...>, Args &&...) -> FatalError<Args...>;

/// K4: srodowisko odmowilo (plik, dysk, pamiec dzielona). Konczy proces kodem EIO i nie
/// podnosi zatrzasku fatalErrorRaised, wiec usluga wstaje po nim z tym samym planem.
template <typename... Args>
struct EnvironmentError {
  [[noreturn]] EnvironmentError(fmt::format_string<Args...> fmt_str, Args &&...args,
                                std::source_location loc = std::source_location::current()) {
    detail::raise<ErrorClass::Environment, Args...>(loc, fmt_str, std::forward<Args>(args)...);
  }
};

template <typename... Args>
EnvironmentError(fmt::format_string<Args...>, Args &&...) -> EnvironmentError<Args...>;

}  // namespace rdb

// 235 miejsc wywolan pisze FatalError bez przestrzeni nazw; CTAD dziala przez
// using-declaration, bo przewodniki dedukcji naleza do szablonu, nie do nazwy.
using rdb::FatalError;
