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
///   Test: `PersistentCounterTest.read_missing_file_is_missing`, `PersistentCounterTest.read_valid_file_is_value_and_reserves_nothing`, `PersistentCounterTest.read_empty_file_is_invalid`, `PersistentCounterTest.read_directory_is_unreadable_not_invalid`, `PersistentCounterTest.existence_failure_is_unreadable_instead_of_missing`, `PersistentCounterTest.read_retries_interrupted_and_short_reads_without_reserving`.
/// - traktować brak pliku jako numer rotacji 0 wyłącznie wtedy, gdy brak został potwierdzony, a nie gdy samo sprawdzenie istnienia się nie powiodło.
///   Test: `PersistentCounterTest.starts_at_zero_when_no_file`, `PersistentCounterTest.existence_failure_is_unreadable_instead_of_missing`.
/// - w create wczytywać numer N bieżącej sesji i przed zwróceniem obiektu zapisywać N+1, tak aby numer był zarezerwowany przed powstaniem pierwszego archiwum.
///   Test: `PersistentCounterTest.construction_reserves_next_value`, `PersistentCounterTest.reservation_orders_sync_rename_and_directory_sync`.
/// - zwracać z create Errc::IO dla pliku Unreadable lub Invalid oraz dla nieudanej rezerwacji N+1, nie zastępując numeru wartością zgadniętą.
///   Test: `PersistentCounterTest.empty_file_is_not_rotation_zero`, `PersistentCounterTest.unreadable_file_exits_with_eio`, `PersistentCounterTest.failed_reservation_returns_io_and_keeps_previous_value`, `PersistentCounterTest.failed_write_sync_close_or_rename_keeps_old_bytes_and_removes_temporary`.
/// - zapisywać nową wartość przez plik tymczasowy w tym samym katalogu, z fsync pliku przed atomowym rename i fsync katalogu po nim, tak aby każdy odczyt widział starą albo nową wartość, a nigdy plik pusty.
///   Test: `PersistentCounterTest.reservation_orders_sync_rename_and_directory_sync`, `PersistentCounterTest.failed_write_sync_close_or_rename_keeps_old_bytes_and_removes_temporary`. Kontrola strukturalna: plik tymczasowy jest w katalogu celu, a podmianę wykonuje filesystem::rename; test nie symuluje utraty zasilania. Kolejność z rename i awaria rename są pomijane, gdy sonda renameInterposed() wykaże, że biblioteka standardowa omija podmianę rename (Darwin); na Linuksie taki wynik sondy jest błędem testu.
/// - po nieudanym zapisie lub rename usuwać plik tymczasowy, a porażkę fsync katalogu po udanym rename zgłaszać w logu bez odmowy.
///   Test: `PersistentCounterTest.failed_write_sync_close_or_rename_keeps_old_bytes_and_removes_temporary`, `PersistentCounterTest.directory_sync_failure_logs_but_keeps_the_completed_reservation`.
/// - przez getCount zwracać numer bieżącej sesji N.
///   Test: `PersistentCounterTest.second_instance_loads_saved_value`, `PersistentCounterTest.getCount_is_idempotent_before_destruction`.
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
