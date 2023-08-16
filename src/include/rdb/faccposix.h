#ifndef STORAGE_RDB_INCLUDE_FACCPOSIX_H_
#define STORAGE_RDB_INCLUDE_FACCPOSIX_H_

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface via posix calls
 *
 * File Accessor as Posix - type. This is underlying type that supports access to binary fields.
 * Posix functions are used as interface. :Rdb user does not need to use this object directly
 */
template <typename T>
class posixBinaryFileAccessor : public FileAccessorInterface<T> {
  std::string fileNameStr;

 public:
  explicit posixBinaryFileAccessor(const std::string fileName);

  ssize_t read(T *ptrData, const size_t size, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
  std::string fileName() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCPOSIX_H_