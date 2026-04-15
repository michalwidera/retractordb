#pragma once

#include "descriptor.hpp"
#include "fainterface.hpp"

#include <vector>

namespace rdb {

/// @brief Definicja klasy implementującej dostęp danych przechowywanych w pamięci
///
/// Obiekt memoryFile powinien:
/// - umożliwiać odczyt i zapis danych w pamięci
/// - zapewniać trwałość danych w pamięci podczas działania programu
/// - implementować interfejs FileInterface, aby umożliwić integrację z innymi komponentami systemu
/// - być zoptymalizowany pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci
/// - umożliwiać kontrolę ilości przechowywanych danych za pomocą mechanizmu retencji, który usuwa najstarsze dane po przekroczeniu określonego limitu

struct memoryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  const size_t retentionSize_;  // Retention size for the records, if set to no_retention, no limit is applied
  int removed_count_ = 0;       // Count of removed records, used for retention management
  enum { no_retention = 0 };    // Default retention size if not specified
 public:
  memoryFile(const std::string_view fileName, const Descriptor &descriptor, std::pair<std::string, size_t> retentionSize)
      : filename_(std::string(fileName)),  //
        recordSize_(descriptor.getSizeInBytes()),           //
        retentionSize_(retentionSize.second) {};

  using FileInterface::write;
  using FileInterface::read;
  ssize_t write(const uint8_t *ptrData, const size_t position, const std::vector<bool> &nullBitset) override;
  ssize_t read(uint8_t *ptrData, const size_t position, std::vector<bool> &nullBitset) override;

  auto name() -> std::string & override;
  size_t count() override;

 private:
  ssize_t readRaw(uint8_t *ptrData, const size_t position);
  ssize_t writeRaw(const uint8_t *ptrData, const size_t position);

  memoryFile()                                    = delete;
  memoryFile(const memoryFile &)                  = delete;
  const memoryFile &operator=(const memoryFile &) = delete;
};

}  // namespace rdb
