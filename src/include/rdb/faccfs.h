#ifndef STORAGE_RDB_INCLUDE_FACCFS_H_
#define STORAGE_RDB_INCLUDE_FACCFS_H_

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface via fstream
 *
 * File Accessor as File system - type. This is underlying type that supports access to binary fields.
 * std::fstream is used as interface. :Rdb user does not need to use this object directly
 *
 * Type: GENERIC
 */
struct genericBinaryFileAccessor : public FileAccessorInterface {
  std::string filename;
  const std::size_t size;
  int percounter_;

 public:
  explicit genericBinaryFileAccessor(const std::string_view fileName, const size_t size, int percounter = -1);
  ~genericBinaryFileAccessor();

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() -> std::string & override;
  size_t count() override;

  genericBinaryFileAccessor()                                                   = delete;
  genericBinaryFileAccessor(const genericBinaryFileAccessor &)                  = delete;
  const genericBinaryFileAccessor &operator=(const genericBinaryFileAccessor &) = delete;
};

}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCFS_H_