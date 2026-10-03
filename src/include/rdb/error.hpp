#pragma once

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
/// Domyslny build kompiluje go z -fno-exceptions (opcja RDB_NO_EXCEPTIONS), wiec kazde `throw`,
/// `try` albo `catch` w rdzeniu jest bledem kompilacji, a nie kwestia przegladu kodu. Uzasadnienie
/// i lista luk, ktore ta zmiana zamyka, sa w docs/embedded-realtime-gaps.md.
///
/// Trzy drogi zglaszania bledu:
///
///  1. **Result<T>** (std::expected<T, Error>) - dla wszystkiego, co moze sie nie udac przy
///     poprawnym silniku: zle wejscie (plan, .desc, argumenty API), system plikow, wyrazenie,
///     ktorego nie da sie policzyc dla tych danych. Wolajacy MUSI sprawdzic wynik - typ jest
///     [[nodiscard]]. Na sciezce bez bledu nie ma alokacji ani rozwijania stosu: to jedna flaga.
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

/// Rodzina "zle wejscie": Config i jej dwie pochodne. Odpowiednik `catch (const ConfigError &)`.
[[nodiscard]] constexpr bool isConfigFamily(Errc code) noexcept {
  return code == Errc::Config || code == Errc::Syntax || code == Errc::Compile;
}

/// Nazwa kategorii do komunikatow i logow.
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
class Error {
 public:
  Error(Errc code, std::string message) noexcept : code_(code), message_(std::move(message)) {}

  [[nodiscard]] Errc code() const noexcept { return code_; }
  [[nodiscard]] const std::string &message() const noexcept { return message_; }
  /// To samo co message().c_str() - nazwa jak w std::exception, zeby komunikaty w logach i
  /// testach nie zmienily brzmienia.
  [[nodiscard]] const char *what() const noexcept { return message_.c_str(); }

 private:
  Errc code_;
  std::string message_;
};

/// Wynik operacji, ktora moze sie nie udac. Result<> (czyli Result<void>) dla operacji bez wartosci.
template <typename T = void>
using Result = std::expected<T, Error>;

/// Zbudowanie bledu do `return`: `return rdb::fail(Errc::Config, "...")`.
[[nodiscard]] inline std::unexpected<Error> fail(Errc code, std::string message) {
  return std::unexpected<Error>(std::in_place, code, std::move(message));
}

/// Przekazanie istniejacego bledu wyzej, z innym typem wartosci.
[[nodiscard]] inline std::unexpected<Error> fail(Error error) noexcept { return std::unexpected<Error>(std::move(error)); }

/// Handler zlamanego niezmiennika. MUSI nie wracac; gdyby wrocil, fatal() i tak wola std::abort().
using FatalHandler = void (*)(std::string_view message, const std::source_location &where) noexcept;

/// Zlamany niezmiennik silnika: zapis przyczyny i zatrzymanie procesu w znanym stanie.
/// Nie odwija stosu i nie wraca. Domyslny handler loguje (spdlog, stderr) i wola std::abort().
[[noreturn]] void fatal(std::string_view message, std::source_location where = std::source_location::current()) noexcept;

/// Instaluje handler fatal() i zwraca poprzedni. nullptr przywraca domyslny. Demon instaluje go
/// raz, przed startem watkow (launcher.cpp); host osadzajacy moze zainstalowac wlasny, np. zeby
/// zrzucic stan przed zakonczeniem.
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
#define RDB_ASSERT(cond, ...)                   \
  do {                                          \
    if (!(cond)) [[unlikely]] {                 \
      ::rdb::fatal(::fmt::format(__VA_ARGS__)); \
    }                                           \
  } while (false)
