#pragma once

#include <memory>
#include <utility>

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

}  // namespace rdb
