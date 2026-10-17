#pragma once

#include <cerrno>
#include <cstdint>
#include <expected>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>

/// @file
/// @brief Bledy silnika jako WARTOSCI - bez wyjatkow C++.
///
/// Rdzen silnika (biblioteki rdb, retractorcore i rdbembed) nie rzuca i nie lapie wyjatkow.
/// Domyslny build kompiluje go dodatkowo z -fno-exceptions (kontrola RDB_NOEXCEPT_CHECK), wiec kazde
/// `throw`, `try` albo `catch` w rdzeniu jest bledem kompilacji, a nie kwestia przegladu kodu. Uzasadnienie
/// jest w historii feature/jupyter-integration (1d014279).
///
/// Trzy drogi zglaszania bledu:
///
///  1. **Result<T>** (std::expected<T, Error>) - dla wszystkiego, co moze sie nie udac przy
///     poprawnym silniku: zle wejscie (plan, .desc, argumenty API), system plikow, wyrazenie,
///     ktorego nie da sie policzyc dla tych danych. Wolajacy MUSI sprawdzic wynik.
///     Na sciezce bez bledu nie ma alokacji ani rozwijania stosu: to jedna flaga.
///
///  2. **RDB_ASSERT / rdb::fatal()** - dla zlamanego niezmiennika WEWNATRZ silnika, czyli bledu w
///     kodzie, ktorego zaden poprawny wolajacy nie wywola (indeks pola juz sprawdzony przez
///     kompilator, payload niepodpiety po udanej budowie modelu). Proces konczy sie w znanym
///     stanie: handler loguje przyczyne i miejsce, a potem domyslnie std::abort(). Demon instaluje
///     wlasny handler (fatalError.hpp), ktory - jak dawny FatalError - podnosi zatrzask
///     fatalErrorRaised i wychodzi przez std::exit(EXIT_FAILURE), zeby sprzatanie IPC sie wykonalo.
///
///  3. **Granica hosta** - wiazanie Pythona (src/python/module.cpp) zamienia Error na wyjatek
///     PYTHONA tej samej klasy co dotad (ConfigError, InternalError, ...). To jedyne miejsce, w
///     ktorym blad silnika staje sie wyjatkiem, i nie jest to wyjatek C++ przechodzacy przez rdzen.
///
/// Zasada podzialu miedzy 1 i 2: wszystko, co da sie wywolac przez publiczne API albo trescia
/// planu, jest sprawdzane NA GRANICY i wraca jako Result. RDB_ASSERT zostaje dla tego, co granica
/// juz wykluczyla - tam dalsza praca na zepsutym stanie bylaby gorsza niz zatrzymanie.

namespace rdb {

/// Klasy odmowy zachowane z aktualnej granicy silnika (K1-K5).
enum class ErrorClass : std::uint8_t { Internal, Plan, Request, Environment, Data };
/// @requirements-exempt Refusal: Nosnik kategorii, komunikatu i errno odmowy.
struct Refusal {
  ErrorClass cls{ErrorClass::Plan};
  std::string message;
  int err{0};
};

/// @brief Kategoria bledu. Ten sam podzial co dawna hierarchia wyjatkow rdb::Error, tylko jako
/// wartosc - wiazanie Pythona mapuje kazda kategorie na te sama klase wyjatku co przedtem.
enum class Errc : std::uint8_t {
  Config,             ///< wejscie jest zle, silnik jest caly (dawny ConfigError)
  CorruptDescriptor,  ///< plik .desc jest pusty albo nie daje sie sparsowac (dawny CorruptDescriptor)
  Syntax,             ///< tekst RQL nie parsuje sie (dawny embed::SyntaxError, rodzina Config)
  Compile,            ///< plan parsuje sie, ale nie przechodzi kompilacji (dawny embed::CompileError, rodzina Config)
  Eval,               ///< wyrazenie nie daje sie policzyc dla tych operandow (dawny std::runtime_error ewaluatora)
  IO,                 ///< zawiodl system plikow, uprawnienia albo miejsce (dawny IOError)
  Logic               ///< zlamany niezmiennik wykryty w miejscu, ktore umie go zwrocic (dawny LogicError)
};

/// Funkcja errcName powinna:
/// - [error.errc-name.1~1] zwracać nazwę kategorii do komunikatów i logów: ConfigError, CorruptDescriptor, SyntaxError, CompileError, EvalError, IOError albo LogicError, a dla wartości spoza wyliczenia "Error".
[[nodiscard]] constexpr std::string_view errcName(Errc code) noexcept {
  switch (code) {
    case Errc::Config:
      return "ConfigError";
    case Errc::CorruptDescriptor:
      return "CorruptDescriptor";
    case Errc::Syntax:
      return "SyntaxError";
    case Errc::Compile:
      return "CompileError";
    case Errc::Eval:
      return "EvalError";
    case Errc::IO:
      return "IOError";
    case Errc::Logic:
      return "LogicError";
  }
  return "Error";
}

/// @brief Blad: kategoria plus komunikat dla czlowieka.
///
/// Komunikat alokuje dopiero wtedy, gdy blad naprawde powstaje - sciezka bez bledu nie buduje
/// zadnego napisu. Typ jest przenoszony, nie kopiowany, przez cala droge do granicy.
/// @requirements-exempt Error: Nosnik kodu, komunikatu i errno bledu; metody udostepniaja zapisane dane.
class Error {
 public:
  Error(Errc code, std::string message, int err = 0) noexcept : code_(code), message_(std::move(message)), err_(err) {}

  [[nodiscard]] Errc code() const noexcept { return code_; }
  [[nodiscard]] int err() const noexcept { return err_; }
  [[nodiscard]] const std::string &message() const noexcept { return message_; }
  /// To samo co message().c_str() - nazwa jak w std::exception, zeby komunikaty w logach i
  /// testach nie zmienily brzmienia.
  [[nodiscard]] const char *what() const noexcept { return message_.c_str(); }

 private:
  Errc code_;
  std::string message_;
  int err_{0};
};

/// Wynik operacji, ktora moze sie nie udac. Result<> (czyli Result<void>) dla operacji bez wartosci.
template <typename T = void>
using Result = std::expected<T, Error>;

/// Funkcja fail powinna:
/// - [error.fail.1~1] budować błąd do `return` jako std::unexpected<Error> z podaną kategorią, komunikatem i errno (domyślnie 0), także z komunikatu w postaci literału.
/// - [error.fail.2~1] dla Refusal zachowywać komunikat i errno, a kategorię wyznaczać z klasy odmowy: Environment jako IO, Internal jako Logic, pozostałe jako Compile.
/// - [error.fail.3~1] dla istniejącego Error przekazywać go wyżej bez zmian, z innym typem wartości.
///
/// @note Warianty z kategoria sa zimne i niewklejane (#511): budowa bledu, takze std::string z literalu, nie
///       wnosi kodu do funkcji z goraca sciezka, ktora inaczej przestaje sie miescic w progu inline.
[[nodiscard, gnu::cold, gnu::noinline]] inline std::unexpected<Error> fail(Errc code, std::string message, int err = 0) {
  return std::unexpected<Error>(std::in_place, code, std::move(message), err);
}

[[nodiscard, gnu::cold, gnu::noinline]] inline std::unexpected<Error> fail(Errc code, const char *message, int err = 0) {
  return std::unexpected<Error>(std::in_place, code, std::string(message), err);
}

[[nodiscard]] inline std::unexpected<Error> fail(Refusal refusal) {
  Errc code = Errc::Compile;
  if (refusal.cls == ErrorClass::Environment)
    code = Errc::IO;
  else if (refusal.cls == ErrorClass::Internal)
    code = Errc::Logic;
  return fail(code, std::move(refusal.message), refusal.err);
}

[[nodiscard]] inline std::unexpected<Error> fail(Error error) noexcept { return std::unexpected<Error>(std::move(error)); }

/// Handler zlamanego niezmiennika. MUSI nie wracac; gdyby wrocil, fatal() i tak wola std::abort().
using FatalHandler = void (*)(std::string_view message, const std::source_location &where) noexcept;

/// Funkcja fatal powinna:
/// - [error.fatal.1~1] przy złamanym niezmienniku silnika wywoływać zainstalowany handler z komunikatem i miejscem wywołania, a bez handlera - domyślny.
/// - [error.fatal.2~1] w domyślnym handlerze, przy nierzucającym loggerze, logować przyczynę na poziomie critical z miejscem wywołania, opróżniać logger, wypisywać "FATAL: <komunikat>" na stderr i kończyć proces przez std::abort().
///   Ograniczenie weryfikacji: domyslny handler jest noexcept, ale nie przechwytuje wyjatku z logowania; test nie zapewnia diagnostyki przy rzucajacym loggerze.
/// - [error.fatal.3~1] nigdy nie wracać i nie odwijać stosu: gdy handler wróci, kończyć proces jak domyślny handler.
[[noreturn]] void fatal(std::string_view message, std::source_location where = std::source_location::current()) noexcept;

/// Funkcja setFatalHandler powinna:
/// - [error.set-fatal-handler.1~1] atomowo instalować handler fatal() i zwracać poprzedni, a dla nullptr przywracać handler domyślny.
///   Nietestowalne: atomowosc i kolejnosc acq_rel potwierdza przeglad fatalHandlerSlot().exchange w error.cc; test sprawdza wymiane, nie wszystkie przeploty.
///
/// @note Demon instaluje go raz, przed startem watkow (launcher.cpp); host osadzajacy moze zainstalowac
///       wlasny, np. zeby zrzucic stan przed zakonczeniem.
FatalHandler setFatalHandler(FatalHandler handler) noexcept;

}  // namespace rdb

#define RDB_ERROR_CONCAT_IMPL_(a, b) a##b
#define RDB_ERROR_CONCAT_(a, b)      RDB_ERROR_CONCAT_IMPL_(a, b)

/// Przekazuje blad wyrazenia typu Result<...> wyzej i konczy biezaca funkcje (ktora tez zwraca
/// Result). Przy sukcesie wartosc wyniku jest pomijana: `RDB_TRY(storage->write());`
#define RDB_TRY(...)                                                     \
  do {                                                                   \
    if (auto &&rdbTryResult_ = (__VA_ARGS__); !rdbTryResult_) {          \
      [[unlikely]] return ::rdb::fail(std::move(rdbTryResult_).error()); \
    }                                                                    \
  } while (false)

#define RDB_TRY_ASSIGN_IMPL_(tmp, lhs, ...)     \
  auto tmp = (__VA_ARGS__);                     \
  if (!tmp) [[unlikely]]                        \
    return ::rdb::fail(std::move(tmp).error()); \
  lhs = std::move(*tmp)

/// Wartosc wyniku albo powrot z bledem: `RDB_TRY_ASSIGN(auto paths, StoragePaths::make(...));`
/// Rozwija sie do kilku instrukcji - nie uzywac jako jedynej instrukcji w if/for bez klamer.
#define RDB_TRY_ASSIGN(lhs, ...) RDB_TRY_ASSIGN_IMPL_(RDB_ERROR_CONCAT_(rdbTryAssign_, __LINE__), lhs, __VA_ARGS__)

/// Zlamany niezmiennik => rdb::fatal(). Argumenty po warunku to format fmt; komunikat budowany
/// jest WYLACZNIE po niespelnieniu warunku. Sprawdzenie zostaje w buildzie Release: kosztuje jedna
/// galaz, a dalsza praca na zepsutym stanie (zapis poza payload, odczyt cudzego rekordu) jest
/// gorsza niz zatrzymanie.
///
/// Formatowanie i wywolanie fatal() siedza w zimnej lambdzie, ktorej kompilator nie wkleja: inaczej
/// kazdy RDB_ASSERT wnosil kod fmt::format do funkcji z goraca sciezka, ta przestawala sie miescic
/// w progu inline i wolanie jej kosztowalo wiecej niz samo sprawdzenie (#511). Miejsce zgloszenia
/// to wiersz RDB_ASSERT, nie lambda.
#define RDB_ASSERT(cond, ...)                                                                                         \
  do {                                                                                                                \
    const bool rdbAssertCondition_ = static_cast<bool>(cond);                                                         \
    if (!rdbAssertCondition_) [[unlikely]] {                                                                          \
      const auto rdbAssertWhere_ = ::std::source_location::current();                                                 \
      [&] [[noreturn, gnu::cold, gnu::noinline]] () { ::rdb::fatal(::fmt::format(__VA_ARGS__), rdbAssertWhere_); }(); \
    }                                                                                                                 \
  } while (false)
