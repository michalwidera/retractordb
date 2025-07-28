#ifndef STORAGE_RDB_INCLUDE_FACCMEMORY_H_
#define STORAGE_RDB_INCLUDE_FACCMEMORY_H_

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface in memory
 *
 * Type: MEMORY
 */
template <typename T>
struct memoryFileAccessor : public FileAccessorInterface<T> {
  std::string filename;
  const std::size_t size;

 public:
  explicit memoryFileAccessor(const std::string &fileName, const size_t size);

  ssize_t read(T *ptrData, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() const -> const std::string & override;
  auto name() -> std::string & override;
  size_t count() override;

  memoryFileAccessor()                                            = delete;
  memoryFileAccessor(const memoryFileAccessor &)                  = delete;
  const memoryFileAccessor &operator=(const memoryFileAccessor &) = delete;
};

}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCMEMORY_H_