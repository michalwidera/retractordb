#ifndef STORAGE_RDB_INCLUDE_FACCPOSIX_H_
#define STORAGE_RDB_INCLUDE_FACCPOSIX_H_

#include "descriptor.h"
#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface via posix calls
 *
 * File Accessor as Posix Permanent - type.
 * This is underlying type that supports access to binary fields. Posix functions are
 *
 * Type: DEFAULT
 */
class posixBinaryFileAccessor : public FileAccessorInterface {
  std::string filename;
  const std::size_t size;
  /**
   * @brief Posix File Descriptor
   */
  int fd;
  int percounter_;

 public:
  ~posixBinaryFileAccessor();

  explicit posixBinaryFileAccessor(const std::string_view fileName, const size_t size, int percounter = -1);

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCPOSIX_H_