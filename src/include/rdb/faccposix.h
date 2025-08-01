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
template <typename T>
class posixBinaryFileAccessor : public FileAccessorInterface<T> {
  std::string filename;
  const std::size_t size;
  /**
   * @brief Posix File Descriptor
   */
  int fd;

 public:
  ~posixBinaryFileAccessor();

  explicit posixBinaryFileAccessor(const std::string &fileName, const size_t size);

  ssize_t read(T *ptrData, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;
  auto name() const -> const std::string & override;
  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCPOSIX_H_