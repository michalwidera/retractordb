#include "rdb/indexRecord.hpp"

#include <cstdint>
#include <cstring>
#include <format>
#include <span>

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
    return true;
  };

  write(static_cast<uint8_t>(isGap ? 1 : 0));
  write(recordCount);
  write(bitsetSize);
  RDB_ASSERT(packBits(nullBitset, std::span<std::byte>(ptr, byteCount)).has_value(), "invalid serialization buffer");

  return buf;
}

Result<IndexRecord> IndexRecord::deserialize(std::span<const std::byte> data) {
  const std::byte *ptr = data.data();
  const std::byte *end = ptr + data.size();

  auto read = [&]<typename T>(T &out) -> bool {
    if (static_cast<size_t>(end - ptr) < sizeof(T)) return false;
    std::memcpy(&out, ptr, sizeof(T));
    ptr += sizeof(T);
    return true;
  };

  IndexRecord rec;
  uint8_t gapFlag = 0;
  if (!read(gapFlag)) return fail(Errc::IO, "Buffer underrun while deserializing", EIO);
  rec.isGap = (gapFlag != 0);
  if (!read(rec.recordCount)) return fail(Errc::IO, "Buffer underrun while deserializing", EIO);

  size_t bitsetSize = 0;
  if (!read(bitsetSize)) return fail(Errc::IO, "Buffer underrun while deserializing", EIO);
  // Dlugosc z pliku porownujemy z ramka PRZED zaokragleniem: packedByteCount zawija sie dla
  // bitsetSize bliskiego SIZE_MAX, a ptr + byteCount wychodziloby poza obiekt (#421).
  const auto available = static_cast<size_t>(end - ptr);
  if (bitsetSize > available * kBitsPerByte)
    return fail(Errc::IO, std::format("Bitset size {} exceeds remaining buffer ({} bytes)", bitsetSize, available), EIO);
  const size_t byteCount = packedByteCount(bitsetSize);

  auto bits = unpackBits(std::span<const std::byte>(ptr, byteCount), bitsetSize);
  if (!bits) return std::unexpected(bits.error());
  rec.nullBitset = std::move(*bits);

  return rec;
}

}  // namespace rdb
