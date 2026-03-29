#pragma once

#include <memory>
#include <vector>

#include "faccposixshd.h"
#include "retention.h"

namespace rdb {
class groupFile : public FileInterface {
  std::string filename_;
  std::string currentFilename_;
  const ssize_t recordSize_;

  retention_t retention_{0, 0};

  std::vector<std::unique_ptr<posixBinaryFileWithShadow>> vec_;

  size_t writeCount_     = 0;
  size_t currentSegment_ = 0;

  size_t removedSegments_ = 0;

  int percounter_;

 public:
  groupFile(const std::string_view fileName, const ssize_t recordSize, const retention_t &retention, int percounter);
  ~groupFile() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb
