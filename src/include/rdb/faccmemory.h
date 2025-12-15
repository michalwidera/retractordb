#ifndef STORAGE_RDB_INCLUDE_FACCMEMORY_H_
#define STORAGE_RDB_INCLUDE_FACCMEMORY_H_

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface in memory
 *
 * Type: MEMORY
 */
struct memoryFileAccessor : public FileAccessorInterface {
  std::string filename;
  const std::size_t size;
  const int retention_size;   // Retention size for the records, if set to no_retention, no limit is applied
  int removed_count = 0;      // Count of removed records, used for retention management
  enum { no_retention = 0 };  // Default retention size if not specified
 public:
  explicit memoryFileAccessor(const std::string_view fileName, const size_t size, std::pair<std::string, size_t> retention_size)
      : filename(std::string(fileName)),  //
        size(size),                       //
        retention_size(retention_size.second){};

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() const -> const std::string & override;
  auto name() -> std::string & override;
  size_t count() override;

  memoryFileAccessor()                                            = delete;
  memoryFileAccessor(const memoryFileAccessor &)                  = delete;
  const memoryFileAccessor &operator=(const memoryFileAccessor &) = delete;
};

}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCMEMORY_H_