#ifndef STORAGE_RDB_INCLUDE_FACCBINDEV_H_
#define STORAGE_RDB_INCLUDE_FACCBINDEV_H_

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface via posix calls
 */
template <typename T>
class binaryDeviceAccessor : public FileAccessorInterface<T> {
  std::string fileNameStr;

  /**
   * @brief Posix File Descriptor
   */
  int fd;

 public:
  ~binaryDeviceAccessor();

  binaryDeviceAccessor(std::string fileName);

  int read(T *ptrData, const size_t size, const size_t position) override;
  int write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
  std::string fileName() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCBINDEV_H_