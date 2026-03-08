#include "rdb/metaDataStream.h"
#include <memory>

namespace rdb {

std::vector<std::byte> metaDataStream::IndexRecord::serialize() const {
  std::vector<std::byte> serializedData;
  const size_t bitsetSize = nullBitset.size();
  const size_t byteCount  = (bitsetSize + 7) / 8;

  std::vector<std::byte> buf(sizeof(size_t) + sizeof(size_t) + byteCount, std::byte{0});
  std::byte *ptr = buf.data();

  std::memcpy(ptr, &recordCount, sizeof(recordCount));
  ptr += sizeof(recordCount);

  std::memcpy(ptr, &bitsetSize, sizeof(bitsetSize));
  ptr += sizeof(bitsetSize);

  for (size_t i = 0; i < bitsetSize; ++i)
    if (nullBitset[i]) ptr[i / 8] |= static_cast<std::byte>(1u << (i % 8));

  return buf;
}

metaDataStream::IndexRecord metaDataStream::IndexRecord::deserialize(std::span<const std::byte> data) {
  const std::byte *ptr = data.data();
  const std::byte *end = ptr + data.size();

  auto read = [&]<typename T>(T &out) {
    if (ptr + sizeof(T) > end) throw std::runtime_error("Buffer underrun while deserializing");
    std::memcpy(&out, ptr, sizeof(T));
    ptr += sizeof(T);
  };

  metaDataStream::IndexRecord rec;

  read(rec.recordCount);

  size_t bitsetSize = 0;
  read(bitsetSize);

  const size_t byteCount = (bitsetSize + 7) / 8;
  if (ptr + byteCount > end) throw std::runtime_error("Buffer underrun in bitset data");

  rec.nullBitset.resize(bitsetSize);
  for (size_t i = 0; i < bitsetSize; ++i)
    rec.nullBitset[i] = (std::to_integer<uint8_t>(ptr[i / 8]) >> (i % 8)) & 1u;

  ptr += byteCount;
  return rec;
}

metaDataStream::metaDataStream(const Descriptor &descriptor, const std::string &metaFilePath)  //
    : descriptorRef_(std::make_shared<Descriptor>(descriptor)),
      metaFilePath_(metaFilePath) {
  createNullBitsetTemplate();

  indexFile_.rdbuf()->pubsetbuf(nullptr, 0);

  indexFile_.open(metaFilePath_, std::ios::app);  // Open for appending (creates the file if it doesn't exist)
};

metaDataStream::~metaDataStream() {
  if (indexFile_.is_open()) {
    indexFile_.write(reinterpret_cast<const char *>(entry.serialize().data()), entry.serialize().size());
    indexFile_.close();
  }
}

void metaDataStream::createNullBitsetTemplate() {
  entry.nullBitset.resize(descriptorRef_->size());
  std::fill(entry.nullBitset.begin(), entry.nullBitset.end(), false);
}

void metaDataStream::onRecordModified(size_t recordIndex, std::vector<bool> &nullBitsetParam) {
  // TODO: Implement logic to update the meta index when a record is modified.
  // Create a redo log entry for the modification and update the meta index accordingly.
};

void metaDataStream::onRecordAppended(std::vector<bool> &nullBitsetParam) {
  if (entry.nullBitset == nullBitsetParam) {
    entry.recordCount++;
  } else {
    auto bitstream = entry.serialize();
    indexFile_.write(reinterpret_cast<const char *>(bitstream.data()), bitstream.size());
    entry.recordCount = 1;
    entry.nullBitset  = nullBitsetParam;
  }
};

}  // namespace rdb
