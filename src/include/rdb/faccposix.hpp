#pragma once

#include "descriptor.hpp"
#include "fainterface.hpp"

#include <memory>
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
/// - przed zakończeniem życia obiektu, dane powinny być bezpiecznie zapisane w pliku, a zasoby systemowe powinny być zwolnione
/// - po ponownym utworzeniu obiektu, powinien odtworzyć stan z pliku, jeśli plik już istnieje, aby zapewnić ciągłość danych między uruchomieniami programu

class posixBinaryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  /**
   * @brief Posix File Descriptor
   */
  int fd;
  int percounter_;

 public:
  posixBinaryFile(const std::string_view fileName, const Descriptor &descriptor, int percounter = -1);
  ~posixBinaryFile() override;

  using FileInterface::write;
  using FileInterface::read;
  ssize_t write(const uint8_t *ptrData, const size_t position, const std::vector<bool> &nullBitset) override;
  ssize_t read(uint8_t *ptrData, const size_t position, std::vector<bool> &nullBitset) override;

  auto name() -> std::string & override;
  size_t count() override;

 private:
  ssize_t readRaw(uint8_t *ptrData, const size_t position);
  ssize_t writeRaw(const uint8_t *ptrData, const size_t position);
};
}  // namespace rdb
