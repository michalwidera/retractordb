#include "rdb/bitsetCodec.hpp"

#include <cstdint>

namespace rdb {

Result<> packBits(const std::vector<bool> &bits, std::span<std::byte> dest) {
  if (dest.size() != packedByteCount(bits.size())) return fail(Errc::Config, "packBits: destination size mismatch");
  for (size_t i = 0; i < bits.size(); ++i)
    if (bits[i]) dest[i / kBitsPerByte] |= static_cast<std::byte>(1U << (i % kBitsPerByte));
  return {};
}

Result<std::vector<bool>> unpackBits(std::span<const std::byte> src, size_t bitCount) {
  // Bez packedByteCount(bitCount): ono zawija sie dla bitCount bliskiego SIZE_MAX (#421).
  if (bitCount > src.size() * kBitsPerByte) return fail(Errc::Config, "unpackBits: bit count exceeds source size");
  std::vector<bool> bits(bitCount);
  for (size_t i = 0; i < bitCount; ++i)
    bits[i] = ((std::to_integer<uint8_t>(src[i / kBitsPerByte]) >> (i % kBitsPerByte)) & 1U) != 0;
  return bits;
}

}  // namespace rdb
