#ifndef STORAGE_RDB_INCLUDE_FACCBINDEV_H_
#define STORAGE_RDB_INCLUDE_FACCBINDEV_H_

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements data source interface via posix calls
 */
template <typename T>
class binaryDeviceAccessor : public FileAccessorInterface<T> {
  const std::string fileNameStr;

  /**
   * @brief Posix File Descriptor
   */
  int fd;

 public:
  ~binaryDeviceAccessor();

  explicit binaryDeviceAccessor(const std::string fileName);

  ssize_t read(T *ptrData, const size_t size, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
  std::string fileName() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCBINDEV_H_