#include "rdb/indexRecord.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace rdb {

namespace {
constexpr size_t kBitsPerByte = 8;
}  // namespace

std::vector<std::byte> IndexRecord::serialize() const {
  const size_t bitsetSize = nullBitset.size();
  const size_t byteCount  = (bitsetSize + (kBitsPerByte - 1)) / kBitsPerByte;
  std::vector<std::byte> buf(sizeof(uint8_t) + sizeof(size_t) + sizeof(size_t) + byteCount, std::byte{0});
  std::byte *ptr = buf.data();

  auto write = [&]<typename T>(const T &val) {
    std::memcpy(ptr, &val, sizeof(T));
    ptr += sizeof(T);
  };

  write(static_cast<uint8_t>(isGap ? 1 : 0));
  write(recordCount);
  write(bitsetSize);
  for (size_t i = 0; i < bitsetSize; ++i)
    if (nullBitset[i]) ptr[i / kBitsPerByte] |= static_cast<std::byte>(1U << (i % kBitsPerByte));

  return buf;
}

IndexRecord IndexRecord::deserialize(std::span<const std::byte> data) {
  const std::byte *ptr = data.data();
  const std::byte *end = ptr + data.size();

  auto read = [&]<typename T>(T &out) {
    if (ptr + sizeof(T) > end) throw std::runtime_error("Buffer underrun while deserializing");
    std::memcpy(&out, ptr, sizeof(T));
    ptr += sizeof(T);
  };

  IndexRecord rec;
  uint8_t gapFlag = 0;
  read(gapFlag);
  rec.isGap = (gapFlag != 0);
  read(rec.recordCount);

  size_t bitsetSize = 0;
  read(bitsetSize);
  const size_t byteCount = (bitsetSize + (kBitsPerByte - 1)) / kBitsPerByte;
  if (ptr + byteCount > end) throw std::runtime_error("Buffer underrun in bitset data");

  rec.nullBitset.resize(bitsetSize);
  for (size_t i = 0; i < bitsetSize; ++i)
    rec.nullBitset[i] = ((std::to_integer<uint8_t>(ptr[i / kBitsPerByte]) >> (i % kBitsPerByte)) & 1U) != 0;

  return rec;
}

}  // namespace rdb
