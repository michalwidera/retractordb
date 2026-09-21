#pragma once

#include <stdexcept>
#include <string>

namespace rdb {

/// @brief Wyjatki silnika - baza taksonomii wprowadzanej przez faze 1 refaktoru.
///
/// Do fazy 1 kazdy blad krytyczny konczyl sie przez FatalError, czyli
/// std::exit(EXIT_FAILURE) (fatalError.hpp). To zalozenie jest nie do utrzymania w
/// procesie, ktory silnika tylko UZYWA: rozszerzenie CPythona, ktore konczy proces,
/// zabija jadro notatnika razem z cala sesja uzytkownika. Miejsca konwertowane
/// kolejnymi plastrami fazy 1 rzucaja typami z tego naglowka.
///
/// Pelna taksonomia (blad skladni RQL, blad kompilacji planu, blad wejscia-wyjscia)
/// nalezy do fazy 5; tutaj jest tylko to, co realnie ma dzis kto rzucic.
///
/// @note Zadna z tych klas NIE MOZE dziedziczyc po antlr4::RecognitionException.
/// Generowany przez ANTLR kod parsera lapie wylacznie ten typ, wiec rzut wyprowadzony
/// z niego zostalby polkniety przez odzyskiwanie po bledzie zamiast opuscic parser -
/// pulapka opisana przy abortParse() w RQLParser.cpp.

/// @brief Baza wszystkich wyjatkow silnika. Odpowiednik `RetractorDBError` po stronie Pythona.
class Error : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// @brief Plik .desc jest pusty albo nie daje sie sparsowac.
///
/// Rzucane przez loadDescriptorFile(); parser deskryptora (DESCParser.cc) zglasza blad
/// skladni statusem, a na wyjatek zamienia go dopiero warstwa wczytujaca plik, bo to ona
/// zna nazwe pliku i to ona jest granica biblioteki.
class CorruptDescriptor : public Error {
 public:
  using Error::Error;
};

/// @brief Magazynu nie da sie skonfigurowac tak, jak zazadal wolajacy.
///
/// Pusty identyfikator zapytania albo nazwa pliku, nieznany typ magazynu, katalog z
/// dyrektywy :STORAGE ktory nie istnieje albo nie jest katalogiem, deskryptor bez pola
/// REF przy pustym :STORAGE. Wspolna cecha: wejscie jest zle, silnik jest caly. To jest
/// ten rodzaj bledu, ktory osadzajacy proces chce ZLAPAC i pokazac uzytkownikowi.
class ConfigError : public Error {
 public:
  using Error::Error;
};

/// @brief Zlamany niezmiennik silnika - blad w kodzie, nie w danych wejsciowych.
///
/// Odpowiednik dawnego asercyjnego FatalError: payload nie podpiety, licznik rekordow
/// rozjechany z accessorem, pozycja poza zakresem juz po sprawdzeniu zakresu. Zadnego z
/// nich nie da sie wywolac poprawnym uzyciem API, wiec zaden nie nalezy do umowy z
/// wolajacym - ale w procesie osadzajacym nadal lepiej go RZUCIC niz zabic nim jadro
/// notatnika. Wyjatek daje slad stosu i zywy proces do obejrzenia; std::exit daje
/// wpis w dzienniku i nic wiecej.
///
/// @note Lapanie tego typu w celu kontynuowania pracy jest bledem. On mowi, ze stan
/// jest juz nieprawidlowy - nadaje sie do zaraportowania i zakonczenia operacji.
class LogicError : public Error {
 public:
  using Error::Error;
};

/// @brief Operacja wejscia-wyjscia nie powiodla sie.
///
/// Nieudane otwarcie pliku magazynu albo cienia, odczyt spod pozycji, ktora accessor
/// odrzucil, nieudany zapis deskryptora. Wspolna cecha: i wejscie, i silnik sa w porzadku
/// - zawiodl system plikow, uprawnienia albo miejsce na dysku.
///
/// @note Komunikat ma niesc strerror(errno), a nie sam kod powrotu. Poprzednie wersje tych
/// miejsc wypisywaly wartosc `fd`, ktora po nieudanym ::open jest zawsze -1 i nie mowi nic
/// poza tym, ze sie nie udalo.
class IOError : public Error {
 public:
  using Error::Error;
};

}  // namespace rdb
