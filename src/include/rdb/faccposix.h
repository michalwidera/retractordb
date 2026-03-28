#pragma once

#include "fainterface.h"

#include <vector>

namespace rdb {

/// @brief Definicja klasy implementującej dostęp do pliku binarnego
///
/// Obiekt posixBinaryFile powinien:
/// - umożliwiać odczyt i zapis danych do pliku binarnego
/// - zapisywać każdą zarejestrowaną wartość do pliku, aby zapewnić trwałość danych
/// - implementować interfejs FileInterface, aby umożliwić integrację z innymi komponentami systemu
/// - być zoptymalizowany pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci
/// - być w stanie obsłużyć duże ilości danych, zapewniając płynne działanie systemu

class posixBinaryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  /**
   * @brief Posix File Descriptor
   */
  int fd;
  int percounter_;

 public:
  posixBinaryFile(const std::string_view fileName, const ssize_t recordSize, int percounter = -1);
  ~posixBinaryFile() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb
