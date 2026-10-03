#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace rdb {

/// @brief Single entry in a meta index – a null bit-set pattern and count
///        of consecutive records sharing that pattern.
struct IndexRecord {
  std::vector<bool> nullBitset;                            ///< one bit per descriptor field (true = null)
  size_t recordCount{0};                                   ///< number of consecutive records with this pattern
  bool isGap{false};                                       ///< true if this entry represents a transmission gap
  [[nodiscard]] std::vector<std::byte> serialize() const;  ///< serialize the entry to a vector of bytes
  /// Deserialize an entry; nullopt when @p data is shorter than the entry it describes (corrupt .meta).
  [[nodiscard]] static std::optional<IndexRecord> deserialize(std::span<const std::byte> data);
};

}  // namespace rdb
