#ifndef STORAGE_RDB_INCLUDE_FACCFS_H_
#define STORAGE_RDB_INCLUDE_FACCFS_H_

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface via fstream
 *
 * File Accessor as File system - type. This is underlying type that supports access to binary fields.
 * std::fstream is used as interface. :Rdb user does not need to use this object directly
 */
template <typename T>
struct genericBinaryFileAccessor : public FileAccessorInterface<T> {
  std::string fileNameStr;

 public:
  explicit genericBinaryFileAccessor(const std::string fileName);

  ssize_t read(T *ptrData, const size_t size, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
  std::string fileName() override;

  genericBinaryFileAccessor() = delete;
  genericBinaryFileAccessor(const genericBinaryFileAccessor &) = delete;
  const genericBinaryFileAccessor &operator=(const genericBinaryFileAccessor &) = delete;
};

}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCFS_H_