#ifndef STORAGE_RDB_INCLUDE_FACCBINDEV_H_
#define STORAGE_RDB_INCLUDE_FACCBINDEV_H_

#include "descriptor.h"
#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements READ ONLY data source interface via posix calls
 *
 * Type: BINARY
 */

class binaryDeviceAccessorRO : public FileAccessorInterface {
  std::string filename_;
  const std::size_t recSize_;
  /**
   * @brief Posix File Descriptor
   */
  int fd_;

  size_t cnt_ = 0;

  bool loopToBeginningIfEOF_ = true;

 public:
  explicit binaryDeviceAccessorRO(const std::string_view fileName, const size_t recSize, bool loopToBeginningIfEOF);
  ~binaryDeviceAccessorRO() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override {
    return EXIT_FAILURE;
  };

  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCBINDEV_H_