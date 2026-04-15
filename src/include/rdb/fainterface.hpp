#pragma once

#include <sys/types.h>  // sszie_t
#include <cstdint>      // uint8_t
#include <limits>       // numeric_limits
#include <string>
#include <vector>

namespace rdb {

/// @brief To jest klasa abstrakcyjna, która definiuje interfejs do operacji na różnych typach magazynów danych (np. pliki,
/// obiekty, blob) używanych jako storage w klasie storage.
///
/// obiekt klasy dziedziczącej po klasie FileInterface powinien zapewniać interfejs:
/// - jednolity, do operacji na różnych typach magazynów danych (np. pliki, obiekty, blob) używanych jako storage w klasie storage.
/// - umożliwiający odczyt danych z magazynu na podstawie pozycji (w bajtach) i rozmiaru danych określonego przez descriptor klasy pochodnej.
/// - który, w przypadku źródła danych sekwencyjnych (np. sterowników urządzeń), które nie obsługują odczytu z określonej pozycji dane odczytywane są jedynie z pozycji 0.
/// - umożliwiający dodawanie danych na końcu magazynu, traktując określoną wartość pozycji jako sygnał do operacji append.
/// - dostarczający informacje o liczbie rekordów w magazynie, co jest istotne dla zarządzania danymi w storage.
/// - w przypadku źródła danych sekwencyjnych (np. sterowników urządzeń), które nie obsługują odczytu z określonej pozycji, metoda count() może zwracać liczbę odczytów wykonanych na tym źródle danych.
///
/// Podstawowe operacje obsługiwane przez implementację tego interfejsu (podstawowy kontrakt polimorficzny wymaga nullBitset):
/// 1. read   ::= read(data, nullBitset, pozycja) :== odczyt danych z magazynu na podstawie pozycji (w bajtach); wypełnia nullBitset informacją o wartościach null dla każdego pola.
/// 2. append :== write(data, nullBitset, max_size_t) :== dodawanie danych na końcu magazynu z przekazaniem wektora wartości null.
/// 3. update :== write(data, nullBitset, pozycja) :== aktualizacja danych w magazynie na podstawie pozycji z przekazaniem wektora wartości null.
///
/// @note pozycja wyrażona jest w bajtach, a rozmiar danych jest określany przez descriptor klasy pochodnej i nie jest częścią tego interfejsu.

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
