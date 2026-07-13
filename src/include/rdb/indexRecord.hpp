#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace rdb {

/// @brief Single entry in a meta index – a null bit-set pattern and count
///        of consecutive records sharing that pattern.
struct IndexRecord {
  std::vector<bool> nullBitset;                                     ///< one bit per descriptor field (true = null)
  size_t recordCount{0};                                            ///< number of consecutive records with this pattern
  bool isGap{false};                                                ///< true if this entry represents a transmission gap
  [[nodiscard]] std::vector<std::byte> serialize() const;           ///< serialize the entry to a vector of bytes
  static IndexRecord deserialize(std::span<const std::byte> data);  ///< deserialize an entry from a vector of bytes
};

}  // namespace rdb
