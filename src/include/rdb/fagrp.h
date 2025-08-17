#ifndef STORAGE_RDB_INCLUDE_FAGRP_H_
#define STORAGE_RDB_INCLUDE_FAGRP_H_

#include <memory>
#include <utility>
#include <vector>

#include "faccposix.h"
#include "fainterface.h"
#include "retention.h"

namespace rdb {

class groupFileAccessor : public FileAccessorInterface {
  std::string filename;
  std::string currentFilename;
  const std::size_t recSize;

  retention_t retention{0, 0};

  std::vector<std::unique_ptr<posixBinaryFileAccessor>> vec;

  size_t writeCount     = 0;
  size_t currentSegment = 0;

  size_t removedSegments = 0;

 public:
  ~groupFileAccessor();

  explicit groupFileAccessor(const std::string_view fileName, const size_t recSize, const retention_t &retention);

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;
  auto name() const -> const std::string & override;
  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FAGRP_H_