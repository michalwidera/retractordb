#pragma once

#include <fstream>

#include "descriptor.h"
#include "fainterface.h"
#include "payload.h"

namespace rdb {

/// @brief Definicja klasy implementującej dostęp do pliku binarnego
///
/// Obiekt textSourceRO powinien:
/// - umożliwiać odczyt danych z pliku tekstowego.
/// - interpretować dane zgodnie z dostarczonym opisem (Descriptor), umożliwiając odczyt danych o różnych typach i strukturach.
/// - uniemożliwiać zapis danych do pliku, zapewniając, że obiekt jest tylko do odczytu.
/// - obsługiwać sytuację, gdy osiągnięty zostanie koniec pliku (EOF), z opcją pętli do początku pliku, jeśli jest włączona.
/// - implementować interfejs FileInterface, aby umożliwić integrację z innymi komponentami systemu.
/// - być zoptymalizowany pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych.
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci.
/// - po przeczytaniu ostatnichd danych, jeśli opcja loopToBeginningIfEOF jest włączona, powinien automatycznie wrócić do początku pliku i kontynuować odczyt danych od początku.
class textSourceRO : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;

  Descriptor descriptor_;

  std::unique_ptr<rdb::payload> payload_;

  std::fstream myFile_;

  size_t readCount_ = 0;

  bool loopToBeginningIfEOF_ = true;

 public:
  textSourceRO(const std::string_view fileName,    //
                       const ssize_t recordSize,           //
                       const rdb::Descriptor &descriptor,  //
                       bool loopToBeginningIfEOF);

  ~textSourceRO() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override {
    return EXIT_FAILURE;
  }

  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb
