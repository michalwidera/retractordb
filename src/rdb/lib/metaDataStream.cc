#include "rdb/metaDataStream.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <memory>
#include <numeric>
#include <stdexcept>

namespace rdb {

// ── IndexRecord serialization ────────────────────────────────────────
// Format (all fields little-endian, native size_t):
//   [recordCount : size_t]
//   [timestamp   : int64_t]
//   [flags       : uint8_t]   // bit 0 = isGap
//   [bitsetSize  : size_t]
//   [bitset bytes: ceil(bitsetSize/8)]

std::vector<std::byte> metaDataStream::IndexRecord::serialize() const {
  const size_t bitsetSize = nullBitset.size();
  const size_t byteCount  = (bitsetSize + 7) / 8;

  const size_t totalSize = sizeof(size_t) + sizeof(int64_t) + sizeof(uint8_t) + sizeof(size_t) + byteCount;
  std::vector<std::byte> buf(totalSize, std::byte{0});
  std::byte *ptr = buf.data();

  std::memcpy(ptr, &recordCount, sizeof(recordCount));
  ptr += sizeof(recordCount);

  std::memcpy(ptr, &timestamp, sizeof(timestamp));
  ptr += sizeof(timestamp);

  uint8_t flags = isGap ? 1u : 0u;
  std::memcpy(ptr, &flags, sizeof(flags));
  ptr += sizeof(flags);

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
  read(rec.timestamp);

  uint8_t flags = 0;
  read(flags);
  rec.isGap = (flags & 1u) != 0;

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

// ── Private helpers ──────────────────────────────────────────────────

void metaDataStream::createNullBitsetTemplate() {
  currentEntry_.nullBitset.resize(descriptorRef_->size());
  std::fill(currentEntry_.nullBitset.begin(), currentEntry_.nullBitset.end(), false);
  currentEntry_.recordCount = 0;
  currentEntry_.timestamp   = 0;
  currentEntry_.isGap       = false;
}

void metaDataStream::flushCurrentEntry() {
  if (currentEntry_.recordCount > 0) {
    index_.push_back(currentEntry_);
  }
}

void metaDataStream::loadIndex() {
  std::ifstream in(metaFilePath_, std::ios::binary);
  if (!in.is_open()) return;  // no existing file – start fresh

  in.seekg(0, std::ios::end);
  const auto fileSize = in.tellg();
  if (fileSize <= 0) return;
  in.seekg(0, std::ios::beg);

  std::vector<std::byte> fileData(static_cast<size_t>(fileSize));
  in.read(reinterpret_cast<char *>(fileData.data()), fileSize);

  std::span<const std::byte> remaining(fileData);
  const size_t bitsetBytes = (descriptorRef_->size() + 7) / 8;
  const size_t entrySize   = sizeof(size_t) + sizeof(int64_t) + sizeof(uint8_t) + sizeof(size_t) + bitsetBytes;

  while (remaining.size() >= entrySize) {
    auto rec = IndexRecord::deserialize(remaining.subspan(0, entrySize));
    if (rec.recordCount > 0 || rec.isGap) {
      index_.push_back(std::move(rec));
    }
    remaining = remaining.subspan(entrySize);
  }

  // Restore currentEntry_ from the last non-gap entry for RLE continuation
  if (!index_.empty() && !index_.back().isGap) {
    currentEntry_ = index_.back();
    index_.pop_back();
  }
}

void metaDataStream::saveIndex() {
  // Rewrite the entire file with the current in-memory state
  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) return;

  for (const auto &rec : index_) {
    auto buf = rec.serialize();
    out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
  }

  if (currentEntry_.recordCount > 0) {
    auto buf = currentEntry_.serialize();
    out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
  }
}

std::pair<size_t, size_t> metaDataStream::locateRecord(size_t recordIndex) const {
  size_t cumulative = 0;

  for (size_t i = 0; i < index_.size(); ++i) {
    if (index_[i].isGap) continue;  // gap markers have recordCount == 0
    if (recordIndex < cumulative + index_[i].recordCount) {
      return {i, recordIndex - cumulative};
    }
    cumulative += index_[i].recordCount;
  }

  // Check the currentEntry_
  if (recordIndex < cumulative + currentEntry_.recordCount) {
    return {index_.size(), recordIndex - cumulative};  // sentinel: index_.size() means currentEntry_
  }

  throw std::out_of_range("recordIndex out of range in metaDataStream::locateRecord");
}

// ── Construction / destruction ───────────────────────────────────────

static int64_t nowEpochMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

metaDataStream::metaDataStream(const Descriptor &descriptor, const std::string &metaFilePath)
    : descriptorRef_(std::make_shared<Descriptor>(descriptor)),
      metaFilePath_(metaFilePath) {
  createNullBitsetTemplate();
  loadIndex();

  // Ensure the meta file exists on disk (even if empty) so that
  // external tools / tests can detect its presence.  When the process
  // is killed before the destructor runs, the file still exists.
  if (!std::filesystem::exists(metaFilePath_)) {
    std::ofstream touch(metaFilePath_, std::ios::binary);
  }
}

metaDataStream::~metaDataStream() { saveIndex(); }

// ── Core update interface ────────────────────────────────────────────

void metaDataStream::onRecordAppended(std::vector<bool> &nullBitsetParam) {
  if (currentEntry_.recordCount > 0 && currentEntry_.nullBitset == nullBitsetParam) {
    currentEntry_.recordCount++;
  } else {
    flushCurrentEntry();
    currentEntry_.nullBitset  = nullBitsetParam;
    currentEntry_.recordCount = 1;
    currentEntry_.timestamp   = nowEpochMs();
    currentEntry_.isGap       = false;
  }
}

void metaDataStream::onRecordModified(size_t recordIndex, std::vector<bool> &nullBitsetParam) {
  auto [segIdx, offset] = locateRecord(recordIndex);

  // Determine which segment we are modifying
  auto &seg = (segIdx < index_.size()) ? index_[segIdx] : currentEntry_;

  // If the pattern hasn't changed, nothing to do
  if (seg.nullBitset == nullBitsetParam) return;

  // Split the RLE segment into up to 3 parts: [before | modified | after]
  std::vector<IndexRecord> replacement;

  // 1) Records before the modified one (same pattern as original)
  if (offset > 0) {
    IndexRecord before;
    before.nullBitset  = seg.nullBitset;
    before.recordCount = offset;
    before.timestamp   = seg.timestamp;
    replacement.push_back(std::move(before));
  }

  // 2) The modified record itself (new pattern)
  {
    IndexRecord modified;
    modified.nullBitset  = nullBitsetParam;
    modified.recordCount = 1;
    modified.timestamp   = nowEpochMs();
    replacement.push_back(std::move(modified));
  }

  // 3) Records after the modified one (same pattern as original)
  if (offset + 1 < seg.recordCount) {
    IndexRecord after;
    after.nullBitset  = seg.nullBitset;
    after.recordCount = seg.recordCount - offset - 1;
    after.timestamp   = seg.timestamp;
    replacement.push_back(std::move(after));
  }

  // Replace the segment
  if (segIdx < index_.size()) {
    index_.erase(index_.begin() + static_cast<std::ptrdiff_t>(segIdx));
    index_.insert(index_.begin() + static_cast<std::ptrdiff_t>(segIdx), replacement.begin(), replacement.end());
  } else {
    // Modified segment was currentEntry_
    currentEntry_ = replacement.back();
    replacement.pop_back();
    index_.insert(index_.end(), replacement.begin(), replacement.end());
  }
}

// ── Query interface ──────────────────────────────────────────────────

std::vector<bool> metaDataStream::getNullBitset(size_t recordIndex) const {
  auto [segIdx, offset] = locateRecord(recordIndex);
  const auto &seg       = (segIdx < index_.size()) ? index_[segIdx] : currentEntry_;
  return seg.nullBitset;
}

bool metaDataStream::isFieldNull(size_t recordIndex, size_t fieldIndex) const {
  auto bitset = getNullBitset(recordIndex);
  if (fieldIndex >= bitset.size()) throw std::out_of_range("fieldIndex out of range");
  return bitset[fieldIndex];
}

size_t metaDataStream::totalRecords() const {
  size_t total = currentEntry_.recordCount;
  for (const auto &rec : index_) {
    total += rec.recordCount;
  }
  return total;
}

const std::vector<metaDataStream::IndexRecord> &metaDataStream::entries() const { return index_; }

// ── Transmission-gap interface ───────────────────────────────────────

void metaDataStream::onTransmissionGap() {
  flushCurrentEntry();

  IndexRecord gapMarker;
  gapMarker.nullBitset.resize(descriptorRef_->size(), false);
  gapMarker.recordCount = 0;
  gapMarker.timestamp   = nowEpochMs();
  gapMarker.isGap       = true;
  index_.push_back(std::move(gapMarker));

  createNullBitsetTemplate();
}

bool metaDataStream::isGapBefore(size_t recordIndex) const {
  if (recordIndex == 0) return false;

  size_t cumulative = 0;
  for (size_t i = 0; i < index_.size(); ++i) {
    if (index_[i].isGap) {
      if (cumulative == recordIndex) return true;
      continue;
    }
    cumulative += index_[i].recordCount;
  }
  return false;
}

// ── Timestamp interface ──────────────────────────────────────────────

int64_t metaDataStream::getRecordTimestamp(size_t recordIndex) const {
  auto [segIdx, offset] = locateRecord(recordIndex);
  const auto &seg       = (segIdx < index_.size()) ? index_[segIdx] : currentEntry_;
  return seg.timestamp;
}

// ── Accessor ─────────────────────────────────────────────────────────

const Descriptor &metaDataStream::descriptor() const { return *descriptorRef_; }

}  // namespace rdb
