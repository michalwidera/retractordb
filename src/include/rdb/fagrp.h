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

typedef size_t segments_t;  // silos_count
typedef size_t capacity_t;  // silos_size
struct retention_t {
  segments_t segments;
  capacity_t capacity;
  bool noRetention() const { return segments == 0 && capacity == 0; };
  retention_t &operator=(const std::pair<segments_t, capacity_t> &p) {
    segments = p.first;
    capacity = p.second;
    return *this;
  };
};

template <typename T>
class groupFileAccessor : public FileAccessorInterface<T> {
  const std::string filename;
  const std::size_t recSize;

  retention_t retention{0, 0};

  std::unique_ptr<posixBinaryFileAccessor<T>> accessor;

  size_t writeCount     = 0;
  size_t currentSegment = 0;

 public:
  ~groupFileAccessor();

  explicit groupFileAccessor(const std::string &fileName, const size_t recSize, const retention_t &retention);

  ssize_t read(T *ptrData, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;
  std::string fileName() override;
  size_t count() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FAGRP_H_