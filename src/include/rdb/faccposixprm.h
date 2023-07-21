#ifndef STORAGE_RDB_INCLUDE_FACCPRMPOSIX_H_
#define STORAGE_RDB_INCLUDE_FACCPRMPOSIX_H_

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface via posix calls
 *
 * File Accessor as Posix Permanent - type.
 * This is underlying type that supports access to binary fields. Posix functions are
 * used as interface. Difference between faccposix is that file descriptor remains open during entire live of object :Rdb user
 * does not need to use this object directly. This is development area.
 */
template <typename T>
class posixPrmBinaryFileAccessor : public FileAccessorInterface<T> {
  std::string fileNameStr;

  /**
   * @brief Posix File Descriptor
   */
  int fd;

 public:
  ~posixPrmBinaryFileAccessor();

  explicit posixPrmBinaryFileAccessor(std::string fileName);

  int read(T *ptrData, const size_t size, const size_t position) override;
  int write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
  std::string fileName() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCPRMPOSIX_H_