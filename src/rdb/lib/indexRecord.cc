#include "rdb/indexRecord.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>

#include "rdb/bitsetCodec.hpp"

namespace rdb {

std::vector<std::byte> IndexRecord::serialize() const {
  const size_t bitsetSize = nullBitset.size();
  const size_t byteCount  = packedByteCount(bitsetSize);
  std::vector<std::byte> buf(sizeof(uint8_t) + sizeof(size_t) + sizeof(size_t) + byteCount, std::byte{0});
  std::byte *ptr = buf.data();

  auto write = [&]<typename T>(const T &val) {
    std::memcpy(ptr, &val, sizeof(T));
    ptr += sizeof(T);
  };

  write(static_cast<uint8_t>(isGap ? 1 : 0));
  write(recordCount);
  write(bitsetSize);
  packBits(nullBitset, std::span<std::byte>(ptr, byteCount));

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
  const size_t byteCount = packedByteCount(bitsetSize);
  if (ptr + byteCount > end) throw std::runtime_error("Buffer underrun in bitset data");

  rec.nullBitset = unpackBits(std::span<const std::byte>(ptr, byteCount), bitsetSize);

  return rec;
}

}  // namespace rdb
