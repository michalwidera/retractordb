#pragma once
#include <string>

/* Licznik rotacji archiwow planu z dyrektywa :ROTATION. Wartosc trzyma w pliku wskazanym
   przez ta dyrektywe (nazwa przychodzi w konstruktorze).
   Konstruktor wczytuje numer N biezacej sesji i od razu, przez plik tymczasowy i rename,
   zapisuje N+1 - numer jest zarezerwowany, zanim powstanie jakiekolwiek archiwum. Po
   zakonczeniu sesji archiwa nosza przyrostek .old<N>.
   - brak pliku = rotacja 0 (pierwsze uzycie),
   - plik, ktorego nie da sie odczytac jako nieujemnej liczby = FatalError,
   - nieudana rezerwacja N+1 = FatalError.
   Uzasadnienie przy load() i save() w persistentCounter.cpp.
*/

class PersistentCounter {
 public:
  explicit PersistentCounter(std::string initFilename);
  [[nodiscard]] int getCount() const;

 private:
  int count_{0};
  std::string persistentCounterFilename_;
  void load();
  [[nodiscard]] bool save(int value) const;
};
