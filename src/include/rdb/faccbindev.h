#pragma once

#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements READ ONLY data source interface via posix calls
 *
 * Type: BINARY
 */

class binaryDeviceRO : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  /**
   * @brief Posix File Descriptor
   */
  int fd_;

  size_t cnt_ = 0;

  bool loopToBeginningIfEOF_ = true;

 public:
  explicit binaryDeviceRO(const std::string_view fileName, const ssize_t recordSize, bool loopToBeginningIfEOF);
  ~binaryDeviceRO() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override {
    return EXIT_FAILURE;
  };

  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb
