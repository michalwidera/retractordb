#pragma once

#include <vector>

#include "descriptor.h"
#include "fainterface.h"

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
/// - interpretować dane zgodnie z dostarczonym opisem (Descriptor), umożliwiając odczyt danych o różnych typach i strukturach.
/// - uniemożliwiać zapis danych do pliku, zapewniając, że obiekt jest tylko do odczytu.
/// - obsługiwać sytuację, gdy osiągnięty zostanie koniec pliku (EOF), z opcją pętli do początku pliku, jeśli jest włączona.
/// - implementować interfejs FileInterface, aby umożliwić integrację z innymi komponentami systemu.
/// - być zoptymalizowany pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych.
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci.
/// - po przeczytaniu ostatnichd danych, jeśli opcja loopToBeginningIfEOF jest włączona, powinien automatycznie wrócić do początku pliku i kontynuować odczyt danych od początku.
/// - w przypadku osiągnięcia końca pliku, jeśli opcja loopToBeginningIfEOF jest wyłączona, powinien zwrócić dane z ustawionymi wartościami null i zerami, a następnie zakończyć odczyt danych.
/// - w przypadku pojawienia się wartości null w danych, powinien odpowiednio ustawić metadane null dla tych wartości, aby umożliwić ich prawidłową obsługę przez inne komponenty systemu.
/// - w przypadku błędu odczytu danych z pliku, powinien odpowiednio ustawić metadane null dla wszystkich pól, aby wskazać, że dane są nieprawidłowe lub niedostępne.

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
  explicit binaryDeviceRO(const std::string_view fileName, const ssize_t recordSize, const rdb::Descriptor &descriptor,
                          bool loopToBeginningIfEOF);
  ~binaryDeviceRO() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override {
    return EXIT_FAILURE;
  };

  auto name() -> std::string & override;
  size_t count() override;

  const std::vector<bool> &lastNullBitset() const;
};
}  // namespace rdb
