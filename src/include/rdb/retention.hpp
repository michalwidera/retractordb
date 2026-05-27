#pragma once

#include <cstddef>  // size_t
#include <utility>  // std::pair
namespace rdb {
/**
 * @brief Object that implements storage interface for group files
 *
 * Type: DEFAULT (NEW)
 */

using segments_t = size_t;  // silos_count
using capacity_t = size_t;  // silos_size
struct retention_t {
  segments_t segments;
  capacity_t capacity;
  [[nodiscard]] bool noRetention() const { return segments == 0 && capacity == 0; };
  retention_t &operator=(const std::pair<segments_t, capacity_t> &p) {
    segments = p.first;
    capacity = p.second;
    return *this;
  };
};

}  // namespace rdb
