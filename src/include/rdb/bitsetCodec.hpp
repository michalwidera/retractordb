#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "error.hpp"

namespace rdb {

constexpr size_t kBitsPerByte = 8;

/// @brief Number of bytes needed to pack @p bitCount bits (ceil(bitCount / 8)).
constexpr size_t packedByteCount(size_t bitCount) { return (bitCount + (kBitsPerByte - 1)) / kBitsPerByte; }

/// @brief Pack @p bits into @p dest, one bit per position (LSB-first within each byte).
/// @note dest must be exactly packedByteCount(bits.size()) bytes, zero-initialized by the caller -
///       this only sets the 1-bits, matching how both IndexRecord and ShadowOverride serialize
///       their bitset into a pre-zeroed tail of a larger, fixed-size record buffer.
/// @return Errc::Config when dest size differs from packedByteCount(bits.size()).
[[nodiscard]] Result<> packBits(const std::vector<bool> &bits, std::span<std::byte> dest);

/// @brief Unpack @p bitCount bits from @p src (src must be at least packedByteCount(bitCount) bytes).
/// @return Errc::Config when @p src is too short for @p bitCount.
[[nodiscard]] Result<std::vector<bool>> unpackBits(std::span<const std::byte> src, size_t bitCount);

}  // namespace rdb
