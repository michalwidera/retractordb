#pragma once

#include <vector>

#include "descriptor.hpp"
#include "fainterface.hpp"

namespace rdb {
/**
 * @brief Object that implements READ ONLY data source interface via posix calls
 *
 * Type: BINARY
 */

/// @brief Definicja klasy implementującej dostęp do pliku binarnego
///
/// Obiekt binaryDeviceRO powinien:
/// - umożliwiać odczyt danych z pliku binarnego.
/// - wyznaczać długość paczki danych zgodnie z dostarczonym opisem (Descriptor).
/// - uniemożliwiać zapis danych do pliku, zapewniając, że obiekt jest tylko do odczytu; metoda `write(data, position, nullBitset)` zawsze zwraca `EXIT_FAILURE`.
/// - zapewnić obsługę wartości null w poleceniach odczytu danych.
/// - poprawny odczyt ustawia wektor null na same wartości `false`
/// - w przypadku błędu, braku odczytu lub rozpoznania końca pliku/EOF (jeśli loopToBeginningIfEOF wyłączone) ustawiać wektor null na same wartości `true`.
/// - obsługiwać jedynie sekwencyjny odczyt danych, gdzie jedyna dopuszczalna pozycja odczytu to 0 a dane są odczytywane w kolejności występowania w pliku.
/// - implementować interfejs FileInterface, aby umożliwić integrację z innymi komponentami systemu.
/// - być zoptymalizowany pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych.
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci.
/// - po przeczytaniu ostatnich danych, jeśli opcja loopToBeginningIfEOF jest włączona, powinien automatycznie wrócić do początku pliku i kontynuować odczyt danych od początku.
/// - w przypadku osiągnięcia końca pliku, jeśli opcja loopToBeginningIfEOF jest wyłączona, powinien zwrócić wartości oznaczone jako null a dane wypełnione zerami lub innym formatem failover
class binaryDeviceRO : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  Descriptor descriptor_;
  /**
   * @brief Posix File Descriptor
   */
  int fd_;

  size_t cnt_ = 0;

  bool loopToBeginningIfEOF_ = true;
  std::vector<bool> lastNullBitset_;

 public:
  explicit binaryDeviceRO(const std::string_view fileName,    //
                          const rdb::Descriptor &descriptor,  //
                          bool loopToBeginningIfEOF);
  ~binaryDeviceRO() override;

  using FileInterface::write;
  using FileInterface::read;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) override {
    return EXIT_FAILURE;
  };

  auto name() -> std::string & override;
  size_t count() override;

  const std::vector<bool> &lastNullBitset() const;
};
}  // namespace rdb
