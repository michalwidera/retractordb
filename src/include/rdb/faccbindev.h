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
  std::string filename;
  const std::size_t recSize;
  /**
   * @brief Posix File Descriptor
   */
  int fd;

  size_t cnt = 0;

 public:
  ~binaryDeviceAccessor();

  explicit binaryDeviceAccessor(const std::string fileName, const size_t recSize);

  ssize_t read(T *ptrData, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() const -> const std::string & override;
  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCBINDEV_H_