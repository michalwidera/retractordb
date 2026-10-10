#pragma once
#include <string>

/* Licznik rotacji archiwow planu z dyrektywa :ROTATION. Wartosc trzyma w pliku wskazanym
   przez ta dyrektywe (nazwa przychodzi w konstruktorze).
   Konstruktor wczytuje numer N biezacej sesji i od razu, przez plik tymczasowy i rename,
   zapisuje N+1 - numer jest zarezerwowany, zanim powstanie jakiekolwiek archiwum. Po
   zakonczeniu sesji archiwa nosza przyrostek .old<N>.
   - brak pliku = rotacja 0 (pierwsze uzycie),
   - plik, ktorego nie da sie odczytac albo ktorego tresc nie jest nieujemna liczba = EnvironmentError,
   - nieudana rezerwacja N+1 = EnvironmentError.
   Wadliwa tresc odrzuca jako blad planu (K2) wczesniej sprawdzenie checkRotationCounter() - tutaj,
   po punkcie bez powrotu, znaczy juz zmiane pliku po tym sprawdzeniu.
   Uzasadnienie przy load() i save() w persistentCounter.cpp.
*/

class PersistentCounter {
 public:
  /// Odczyt pliku licznika bez rezerwacji numeru. Rozdziela przyczyne: brak pliku jest stanem
  /// poprawnym, nieudane sprawdzenie, otwarcie lub odczyt to srodowisko (K4), a odczytana w
  /// calosci tresc, ktora nie jest nieujemna liczba, to wadliwy stan wejsciowy planu (K2).
  struct Reading {
    enum class State { Missing, Value, Unreadable, Invalid };
    State state{State::Missing};
    int value{0};
    std::string detail;  ///< Unreadable: operacja i przyczyna; Invalid: rozmiar i poczatek tresci
  };
  [[nodiscard]] static Reading read(const std::string &filename);

  explicit PersistentCounter(std::string initFilename);
  [[nodiscard]] int getCount() const;

 private:
  int count_{0};
  std::string persistentCounterFilename_;
  void load();
  [[nodiscard]] bool save(int value) const;
};
