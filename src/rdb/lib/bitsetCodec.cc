#include "rdb/bitsetCodec.hpp"

#include <cstdint>

namespace rdb {

void packBits(const std::vector<bool> &bits, std::span<std::byte> dest) {
  for (size_t i = 0; i < bits.size(); ++i)
    if (bits[i]) dest[i / kBitsPerByte] |= static_cast<std::byte>(1U << (i % kBitsPerByte));
}

std::vector<bool> unpackBits(std::span<const std::byte> src, size_t bitCount) {
  std::vector<bool> bits(bitCount);
  for (size_t i = 0; i < bitCount; ++i)
    bits[i] = ((std::to_integer<uint8_t>(src[i / kBitsPerByte]) >> (i % kBitsPerByte)) & 1U) != 0;
  return bits;
}

}  // namespace rdb
