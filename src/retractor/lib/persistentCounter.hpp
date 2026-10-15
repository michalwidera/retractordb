#pragma once
#include <cstdint>
#include <memory>
#include <string>

#include "rdb/error.hpp"

/* Licznik rotacji archiwow planu z dyrektywa :ROTATION. Wartosc trzyma w pliku wskazanym
   przez ta dyrektywe (nazwa przychodzi w konstruktorze).
   Konstruktor wczytuje numer N biezacej sesji i od razu, przez plik tymczasowy i rename,
   zapisuje N+1 - numer jest zarezerwowany, zanim powstanie jakiekolwiek archiwum. Po
   zakonczeniu sesji archiwa nosza przyrostek .old<N>.
   - brak pliku = rotacja 0 (pierwsze uzycie),
   - plik, ktorego nie da sie odczytac albo ktorego tresc nie jest nieujemna liczba = Errc::IO,
   - nieudana rezerwacja N+1 = Errc::IO.
   Wadliwa tresc odrzuca jako blad planu (K2) wczesniej sprawdzenie checkRotationCounter() - tutaj,
   po punkcie bez powrotu, znaczy juz zmiane pliku po tym sprawdzeniu.
   Uzasadnienie przy load() i save() w persistentCounter.cpp.
*/

/// Obiekt klasy PersistentCounter powinien:
/// - przez read rozróżniać brak pliku (Missing), wartość (Value), plik, którego nie da się sprawdzić, otworzyć lub odczytać (Unreadable) oraz treść, która po obcięciu końcowych białych znaków nie jest w całości nieujemną liczbą całkowitą (Invalid), podając w detail przyczynę albo rozmiar i początek treści.
/// - traktować brak pliku jako numer rotacji 0 wyłącznie wtedy, gdy brak został potwierdzony, a nie gdy samo sprawdzenie istnienia się nie powiodło.
/// - w create wczytywać numer N bieżącej sesji i przed zwróceniem obiektu zapisywać N+1, tak aby numer był zarezerwowany przed powstaniem pierwszego archiwum.
/// - zwracać z create Errc::IO dla pliku Unreadable lub Invalid oraz dla nieudanej rezerwacji N+1, nie zastępując numeru wartością zgadniętą.
/// - zapisywać nową wartość przez plik tymczasowy w tym samym katalogu, z fsync pliku przed atomowym rename i fsync katalogu po nim, tak aby każdy odczyt widział starą albo nową wartość, a nigdy plik pusty.
/// - po nieudanym zapisie lub rename usuwać plik tymczasowy, a porażkę fsync katalogu po udanym rename zgłaszać w logu bez odmowy.
/// - przez getCount zwracać numer bieżącej sesji N.
class PersistentCounter {
 public:
  /// Odczyt pliku licznika bez rezerwacji numeru. Rozdziela przyczyne: brak pliku jest stanem
  /// poprawnym, nieudane sprawdzenie, otwarcie lub odczyt to srodowisko (K4), a odczytana w
  /// calosci tresc, ktora nie jest nieujemna liczba, to wadliwy stan wejsciowy planu (K2).
  /// @requirements-exempt Reading: Wynik odczytu licznika: stan, wartosc i diagnostyka.
  struct Reading {
    enum class State : std::uint8_t { Missing, Value, Unreadable, Invalid };
    State state{State::Missing};
    int value{0};
    std::string detail;  ///< Unreadable: operacja i przyczyna; Invalid: rozmiar i poczatek tresci
  };
  [[nodiscard]] static Reading read(const std::string &filename);

  /// Wczytuje numer biezacej sesji i rezerwuje nastepny. Odmowa jak w opisie wyzej.
  [[nodiscard]] static rdb::Result<std::unique_ptr<PersistentCounter>> create(std::string initFilename);
  [[nodiscard]] int getCount() const;

 private:
  explicit PersistentCounter(std::string initFilename);
  int count_{0};
  std::string persistentCounterFilename_;
  [[nodiscard]] rdb::Result<> load();
  [[nodiscard]] bool save(int value) const;
};
