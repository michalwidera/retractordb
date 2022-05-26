#ifndef STORAGE_RDB_INCLUDE_FACCPRMPOSIX_H_
#define STORAGE_RDB_INCLUDE_FACCPRMPOSIX_H_

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface via posix calls
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

  posixPrmBinaryFileAccessor(std::string fileName);

  int Read(T *ptrData, const size_t size, const size_t position) override;
  int Write(
      const T *ptrData, const size_t size,
      const size_t position = std::numeric_limits<size_t>::max()) override;
  std::string FileName() override;

  posixPrmBinaryFileAccessor() = delete;
  posixPrmBinaryFileAccessor(const posixPrmBinaryFileAccessor &) = delete;
  posixPrmBinaryFileAccessor &operator=(const posixPrmBinaryFileAccessor &) =
      delete;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCPRMPOSIX_H_