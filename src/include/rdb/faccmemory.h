#pragma once

#include "fainterface.h"

namespace rdb {

/// @brief Definicja klasy implementującej dostęp danych przechowywanych w pamięci
///
/// Obiekt memoryFileAccessor powinien:
/// - umożliwiać odczyt i zapis danych w pamięci
/// - zapewniać trwałość danych w pamięci podczas działania programu
/// - implementować interfejs FileAccessorInterface, aby umożliwić integrację z innymi komponentami systemu
/// - być zoptymalizowany pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci
/// - umożliwiać kontrolę ilości przechowywanych danych za pomocą mechanizmu retencji, który usuwa najstarsze dane po przekroczeniu określonego limitu

struct memoryFileAccessor : public FileAccessorInterface {
  std::string filename_;
  const ssize_t recordSize_;
  const size_t retentionSize_;  // Retention size for the records, if set to no_retention, no limit is applied
  int removed_count_ = 0;     // Count of removed records, used for retention management
  enum { no_retention = 0 };  // Default retention size if not specified
 public:
  memoryFileAccessor(const std::string_view fileName, const ssize_t recordSize, std::pair<std::string, size_t> retentionSize)
      : filename_(std::string(fileName)),  //
        recordSize_(recordSize),           //
        retentionSize_(retentionSize.second) {};

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() -> std::string & override;
  size_t count() override;

  memoryFileAccessor()                                            = delete;
  memoryFileAccessor(const memoryFileAccessor &)                  = delete;
  const memoryFileAccessor &operator=(const memoryFileAccessor &) = delete;
};

}  // namespace rdb
