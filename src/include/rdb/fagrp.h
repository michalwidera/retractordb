#ifndef STORAGE_RDB_INCLUDE_FAGRP_H_
#define STORAGE_RDB_INCLUDE_FAGRP_H_

#include <memory>
#include <utility>
#include <vector>

#include "faccposix.h"
#include "fainterface.h"

namespace rdb {
/**
 * @brief Object that implements storage interface for group files
 *
 * Type: DEFAULT (NEW)
 */
template <typename T>
class groupFileAccessor : public FileAccessorInterface<T> {
  const std::string filename;
  const std::size_t size;

  std::pair<int, int> retention = {0, 0};  // segments , capacity

  std::vector<std::unique_ptr<posixBinaryFileAccessor<T>>> vec;

  size_t writeCount = 0;

 public:
  ~groupFileAccessor();

  explicit groupFileAccessor(const std::string &fileName, const size_t size, const std::pair<int, int> &retention);

  ssize_t read(T *ptrData, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;
  std::string fileName() override;
  size_t count() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FAGRP_H_