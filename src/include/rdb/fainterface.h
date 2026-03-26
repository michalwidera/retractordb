#pragma once

#include <sys/types.h>  // sszie_t
#include <cstdint>      // uint8_t
#include <limits>       // numeric_limits
#include <string>

namespace rdb {

/// @brief To jest klasa abstrakcyjna, która definiuje interfejs do operacji na różnych typach magazynów danych (np. pliki,
/// obiekty, blob) używanych jako storageAccessor w klasie storageAccessor.
///
/// obiekt klasy dziedziczącej po klasie FileAccessorInterface powinien:
/// - zapewniać jednolity interfejs do operacji na różnych typach magazynów danych (np. pliki, obiekty, blob) używanych jako
/// storageAccessor w klasie storageAccessor.
/// - umożliwiać odczyt danych z magazynu na podstawie pozycji (w bajtach) i rozmiaru danych określonego przez descriptor klasy
/// pochodnej.
/// - w przyadku źródła danych sekwencyjnych (np. sterowników urządzeń), które nie obsługują odczytu z określonej pozycji dane
/// odczytywane są jedynie z pozycji 0.
/// - umożliwiać dodawanie danych na końcu magazynu, traktując określoną wartość pozycji jako sygnał do operacji append.
/// - zapewniać informacje o liczbie rekordów w magazynie, co jest istotne dla zarządzania danymi w storageAccessor.
/// - w przypadku źródła danych sekwencyjnych (np. sterowników urządzeń), które nie obsługują odczytu z określonej pozycji,
/// metoda count() może zwracać liczbę odczytów wykonanych na tym źródle danych.
///
/// Oto trzy podstawowe operacje, które powinny być obsługiwane przez implementację tego interfejsu:
/// 1. read ::= read(data, pozycja) :== odczyt danych z magazynu na podstawie pozycji (w bajtach) i rozmiaru danych określonego
/// przez descriptor klasy pochodnej.
/// 2. append :== write(data, pozycja == maksymalna wartość size_t) :== dodawanie danych na końcu magazynu, traktując określoną
/// wartość pozycji jako sygnał do operacji append.
/// 3. update :== write(data, pozycja) :== aktualizacja danych w magazynie na podstawie pozycji (w bajtach) i rozmiaru danych
/// określonego przez descriptor klasy pochodnej.
///
/// @note pozycja wyrażona jest w bajtach, a rozmiar danych jest określany przez descriptor klasy pochodnej i nie jest częścią
/// tego interfejsu.

struct FileAccessorInterface {
  /// @brief Reads from storage amount of bytes into memory pointed by ptrData from position in storage
  /// @param ptrData pointer to data in memory where data will be fetched from storage
  /// @param position position from the beginning of file [unit: Bytes]
  /// @return status of operation - 0/EXIT_SUCCESS success
  virtual ssize_t read(uint8_t *ptrData, const size_t position) = 0;

  /// @brief Updates or appends data in the storage
  /// @param ptrData pointer to table of bytes in memory that will be updated in storage
  /// @param position position from the beginning of file [unit: Bytes]. If max possible value - works as append.
  /// @return status of operation - 0/EXIT_SUCCESS success
  virtual ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) = 0;

  // following: https://stackoverflow.com/questions/51615363/how-to-write-c-getters-and-setters
  virtual auto name() -> std::string & = 0;

  /// @brief data count in storage
  /// @return number of records in storage
  virtual size_t count() = 0;

  virtual ~FileAccessorInterface() = default;
};
}  // namespace rdb
