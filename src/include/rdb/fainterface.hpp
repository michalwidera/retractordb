#pragma once

#include <sys/types.h>  // sszie_t
#include <cstdint>      // uint8_t
#include <limits>       // numeric_limits
#include <string>
#include <vector>

namespace rdb {

/// @brief Abstrakcyjny interfejs operacji wejścia/wyjścia dla magazynów używanych przez storage.
///
/// Klasa dziedzicząca po FileInterface powinna:
/// - udostępniać wspólny kontrakt odczytu, zapisu i raportowania liczby rekordów niezależnie od rodzaju magazynu,
/// - obsługiwać dane binarne o rozmiarze określanym poza tym interfejsem, zwykle przez Descriptor klasy pochodnej,
/// - przyjmować nullBitset jako kanał przekazywania informacji o wartościach null dla pojedynczego rekordu, jeśli dana implementacja takie informacje wspiera,
/// - umożliwiać dopisywanie danych przez przekazanie pozycji równej std::numeric_limits<size_t>::max(),
/// - interpretować pozostałe wartości position zgodnie z kontraktem danej implementacji; dla większości magazynów losowego dostępu oznacza to pozycję w bajtach,
/// - móc ograniczać dopuszczalne wartości position w przypadku źródeł sekwencyjnych, które nie wspierają losowego dostępu,
/// - zwracać przez count() liczbę rekordów lub inną implementacyjnie zdefiniowaną miarę postępu zgodną z semantyką danej klasy pochodnej,
/// - raportować przez hasShadow(), czy magazyn utrzymuje plik cienia danych (.shadow) dla operacji update;
///   na tej podstawie storage dobiera wariant indeksu metadanych null (storageShadow zamiast metaData),
///   niezależnie od tego, czy magazyn w ogóle posiada indeks metadanych.
///
/// Podstawowe operacje objęte kontraktem polimorficznym:
/// 1. `read(data, nullBitset, position)` odczytuje rekord z magazynu i wypełnia `nullBitset`, jeśli implementacja wspiera metadane null.
/// 2. `write(data, nullBitset, std::numeric_limits<size_t>::max())` dopisuje rekord na końcu magazynu.
/// 3. `write(data, nullBitset, position)` zapisuje rekord pod wskazaną pozycją zgodnie z semantyką implementacji.
///
/// @note FileInterface nie narzuca sposobu przechowywania rekordu ani obowiązkowej obsługi nullBitset; definiuje jedynie wspólną postać wywołań.

struct FileInterface {
  /// @brief Null-aware write: stores data and associated null bitset in the storage.
  ///
  /// Kontrakt statusu read() i write() należy do interfejsu, nie do implementacji: 0 to
  /// powodzenie, każda inna wartość to dodatni kod z rodziny errno, który opisuje przyczynę.
  /// Kody o ustalonym znaczeniu:
  /// - EBADF   - akcesor bez otwartego nośnika (niepusty initializationError()),
  /// - ERANGE  - pod tą pozycją nie ma rekordu: poza końcem magazynu albo usunięty przez retencję,
  /// - EINVAL  - pozycja albo rozmiar rekordu poza kontraktem implementacji (np. pozycja różna
  ///             od 0 w źródle sekwencyjnym),
  /// - ENOTSUP - zapis do źródła tylko do odczytu,
  /// - EINTR   - wyczerpany limit ponowień wywołania systemowego przerwanego sygnałem,
  /// - EIO     - rekord urwany w połowie albo awaria strumienia bez dokładniejszej przyczyny.
  /// Każdy inny kod to errno nieudanego wywołania systemowego, przekazane bez zmian.
  ///
  /// Kod errno, a nie nazwany stan, bo ścieżki posixowe i tak niosą errno, a wołający sprawdzają
  /// wyłącznie różność od zera i wstawiają kod do komunikatu. Nazwany stan wymagałby mapowania
  /// przy każdym wywołaniu systemowym i gubiłby szczegół (ENOSPC, EROFS). Z tego wyboru wynikają
  /// dwie reguły:
  /// - nigdy EXIT_FAILURE: to 1, czyli także EPERM, więc komunikat `result=1` był niejednoznaczny,
  /// - `return errno` tylko bezpośrednio po nieudanym wywołaniu systemowym, a errno zapamiętane
  ///   przed zapisem do logu. Inaczej wraca wartość po cudzym wywołaniu - dla zamkniętego
  ///   deskryptora bywało to 0, czyli meldunek powodzenia (E-03, #270).
  ///
  /// @param ptrData    pointer to data bytes; nullptr with position=0 triggers purge
  /// @param nullBitset one bool per descriptor field, true = null
  /// @param position   byte position (std::numeric_limits<size_t>::max() = append)
  /// @return 0 przy powodzeniu, w przeciwnym razie dodatni kod z rodziny errno (patrz wyżej)
  virtual ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, size_t position) = 0;

  /// @brief Null-aware read: retrieves data and fills nullBitset for the record.
  /// @param ptrData    pointer to destination buffer
  /// @param nullBitset output: one bool per descriptor field, true = null
  /// @param position   byte position
  /// @return 0 przy powodzeniu, w przeciwnym razie dodatni kod z rodziny errno, jak w write().
  virtual ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, size_t position) = 0;

  /// @brief Convenience wrapper: Updates or appends data without null tracking.
  /// @param ptrData  pointer to data bytes; nullptr with position=0 triggers purge
  /// @param position byte position (max = append)
  /// @return 0 przy powodzeniu, w przeciwnym razie dodatni kod z rodziny errno.
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) {
    std::vector<bool> ignored;
    return write(ptrData, ignored, position);
  }

  /// @brief Convenience wrapper: Reads data without null tracking.
  /// @param ptrData  pointer to destination buffer
  /// @param position byte position
  /// @return 0 przy powodzeniu, w przeciwnym razie dodatni kod z rodziny errno.
  ssize_t read(uint8_t *ptrData, const size_t position) {
    std::vector<bool> ignored;
    return read(ptrData, ignored, position);
  }

  // following: https://stackoverflow.com/questions/51615363/how-to-write-c-getters-and-setters
  virtual auto name() -> std::string & = 0;

  /// @brief Liczba rekordów w magazynie.
  ///
  /// Kontrakt awarii należy do interfejsu, nie do implementacji: count() nie ma wartości
  /// oznaczającej błąd. Magazyn, którego nośnika jeszcze nie ma - plik nieutworzony albo
  /// usunięty przez purge - jest magazynem pustym i zwraca 0. Każda inna awaria odczytu
  /// rozmiaru rzuca IOError.
  ///
  /// Trzeciej odpowiedzi nie ma, bo wywołujący czyta tę liczbę wyłącznie jako rozmiar.
  /// `return -1` z wariantu posixowego docierało do storage::recordsCount_ jako SIZE_MAX,
  /// a `return 0` po błędzie innym niż ENOENT znaczyło "magazyn pusty", więc storage::write
  /// dopisywał od indeksu 0 po istniejących danych. Obie odpowiedzi są gorsze od zatrzymania.
  ///
  /// @return liczba rekordów, albo inna miara postępu właściwa implementacji (patrz opis klasy);
  ///         nigdy wartość sygnalizująca błąd
  virtual size_t count() = 0;

  /// @brief Powod, dla ktorego konstruktor nie otworzyl magazynu; pusty napis = akcesor gotowy.
  ///
  /// Import planu odczytuje go przed pierwszym count(), read() albo write() i odmawia statusem
  /// zamiast FatalError. Akcesor z niepustym bledem nie nadaje sie do niczego poza zniszczeniem.
  [[nodiscard]] virtual const std::string &initializationError() const {
    static const std::string none;
    return none;
  }

  /// @brief Whether this storage keeps a data shadow file (.shadow) for update operations.
  /// @return true when updates go to a shadow file (see posixBinaryFileWithShadow)
  [[nodiscard]] virtual bool hasShadow() const { return false; }

  /// @brief Czy sekwencyjne źródło wyczerpało wejście i nie ma już czego czytać.
  ///
  /// Dotyczy wyłącznie źródeł deklarowanych czytanych bez zawijania (ONESHOT): po końcu wejścia
  /// zwracają one rekordy all-null, a bez tego pytania nie da się ich odróżnić od danych. Magazyny
  /// zapisywalne nigdy się nie wyczerpują, stąd domyślne false.
  [[nodiscard]] virtual bool exhausted() const { return false; }

  virtual ~FileInterface() = default;
};
}  // namespace rdb
