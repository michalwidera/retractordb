#ifndef STORAGE_RDB_INCLUDE_FACCTXTSRC_H_
#define STORAGE_RDB_INCLUDE_FACCTXTSRC_H_

#include "descriptor.h"
#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface via posix calls
 */
template <typename T>
class textSrouceAccessor : public FileAccessorInterface<T> {
  std::string fileNameStr;

  /**
   * @brief Posix File Descriptor
   */
  int fd;

  Descriptor descriptor;  // filled by fctrl method

 public:
  ~textSrouceAccessor();

  textSrouceAccessor(std::string fileName);

  int read(T *ptrData, const size_t size, const size_t position) override;
  int write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
  int fctrl(void *ptrData, const size_t size) override;
  std::string fileName() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCTXTSRC_H_