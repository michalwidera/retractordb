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

std::optional<IndexRecord> IndexRecord::deserialize(std::span<const std::byte> data) {
  const std::byte *ptr = data.data();
  const std::byte *end = ptr + data.size();

  // Wpis krotszy niz to, co sam deklaruje, to uszkodzony plik .meta, nie blad w kodzie - wraca
  // jako brak wartosci, a decyzje (ostrzezenie, obciecie indeksu) podejmuje wolajacy.
  auto read = [&]<typename T>(T &out) -> bool {
    if (ptr + sizeof(T) > end) return false;
    std::memcpy(&out, ptr, sizeof(T));
    ptr += sizeof(T);
    return true;
  };

  IndexRecord rec;
  uint8_t gapFlag = 0;
  if (!read(gapFlag)) return std::nullopt;
  rec.isGap = (gapFlag != 0);
  if (!read(rec.recordCount)) return std::nullopt;

  size_t bitsetSize = 0;
  if (!read(bitsetSize)) return std::nullopt;
  const size_t byteCount = packedByteCount(bitsetSize);
  if (byteCount > static_cast<size_t>(end - ptr)) return std::nullopt;

  rec.nullBitset = unpackBits(std::span<const std::byte>(ptr, byteCount), bitsetSize);

  return rec;
}

}  // namespace rdb
