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
/// - zwracać przez count() liczbę rekordów lub inną implementacyjnie zdefiniowaną miarę postępu zgodną z semantyką danej klasy pochodnej.
///
/// Podstawowe operacje objęte kontraktem polimorficznym:
/// 1. `read(data, nullBitset, position)` odczytuje rekord z magazynu i wypełnia `nullBitset`, jeśli implementacja wspiera metadane null.
/// 2. `write(data, nullBitset, std::numeric_limits<size_t>::max())` dopisuje rekord na końcu magazynu.
/// 3. `write(data, nullBitset, position)` zapisuje rekord pod wskazaną pozycją zgodnie z semantyką implementacji.
///
/// @note FileInterface nie narzuca sposobu przechowywania rekordu ani obowiązkowej obsługi nullBitset; definiuje jedynie wspólną postać wywołań.

struct FileInterface {
  /// @brief Null-aware write: stores data and associated null bitset in the storage.
  /// @param ptrData    pointer to data bytes; nullptr with position=0 triggers purge
  /// @param nullBitset one bool per descriptor field, true = null
  /// @param position   byte position (std::numeric_limits<size_t>::max() = append)
  /// @return status of operation - 0/EXIT_SUCCESS success
  virtual ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) = 0;

  /// @brief Null-aware read: retrieves data and fills nullBitset for the record.
  /// @param ptrData    pointer to destination buffer
  /// @param nullBitset output: one bool per descriptor field, true = null
  /// @param position   byte position
  /// @return status of operation - 0/EXIT_SUCCESS success
  virtual ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) = 0;

  /// @brief Convenience wrapper: Updates or appends data without null tracking.
  /// @param ptrData  pointer to data bytes; nullptr with position=0 triggers purge
  /// @param position byte position (max = append)
  /// @return status of operation - 0/EXIT_SUCCESS success
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) {
    std::vector<bool> ignored;
    return write(ptrData, ignored, position);
  }

  /// @brief Convenience wrapper: Reads data without null tracking.
  /// @param ptrData  pointer to destination buffer
  /// @param position byte position
  /// @return status of operation - 0/EXIT_SUCCESS success
  ssize_t read(uint8_t *ptrData, const size_t position) {
    std::vector<bool> ignored;
    return read(ptrData, ignored, position);
  }

  // following: https://stackoverflow.com/questions/51615363/how-to-write-c-getters-and-setters
  virtual auto name() -> std::string & = 0;

  /// @brief data count in storage
  /// @return number of records in storage
  virtual size_t count() = 0;

  virtual ~FileInterface() = default;
};
}  // namespace rdb
