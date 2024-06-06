#ifndef STORAGE_RDB_INCLUDE_FACCBINDEV_H_
#define STORAGE_RDB_INCLUDE_FACCBINDEV_H_

#include "descriptor.h"
#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements data source interface via posix calls
 *
 * Type: BINARY
 */
template <typename T>
class binaryDeviceAccessor : public FileAccessorInterface<T> {
  const std::string filename;
  const std::size_t size;
  /**
   * @brief Posix File Descriptor
   */
  int fd;

 public:
  ~binaryDeviceAccessor();

  explicit binaryDeviceAccessor(const std::string fileName, const size_t size);

  ssize_t read(T *ptrData, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;
  std::string fileName() override;
  size_t count() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCBINDEV_H_