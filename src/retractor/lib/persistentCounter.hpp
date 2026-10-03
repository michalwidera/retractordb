#pragma once
#include <memory>
#include <string>

#include "rdb/error.hpp"

/* Licznik rotacji archiwow planu z dyrektywa :ROTATION. Wartosc trzyma w pliku wskazanym
   przez ta dyrektywe (nazwa przychodzi w konstruktorze).
   Konstruktor wczytuje numer N biezacej sesji i od razu, przez plik tymczasowy i rename,
   zapisuje N+1 - numer jest zarezerwowany, zanim powstanie jakiekolwiek archiwum. Po
   zakonczeniu sesji archiwa nosza przyrostek .old<N>.
   - brak pliku = rotacja 0 (pierwsze uzycie),
   - plik, ktorego nie da sie odczytac jako nieujemnej liczby = Errc::Config,
   - nieudana rezerwacja N+1 = Errc::IO.
   Obie odmowy zwraca create() - konstruktor jest prywatny, wiec licznik bez zarezerwowanego
   numeru nie istnieje.
   Uzasadnienie przy load() i save() w persistentCounter.cpp.
*/

class PersistentCounter {
 public:
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
