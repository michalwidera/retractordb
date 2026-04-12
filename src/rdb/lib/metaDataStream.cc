#include "rdb/metaDataStream.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numeric>
#include <stdexcept>

namespace rdb {

// ── IndexRecord serialization ────────────────────────────────────────
// Format (all fields little-endian, native size_t):
//   [recordCount : size_t]
//   [flags       : uint8_t]   // bit 0 = isGap
//   [bitsetSize  : size_t]
//   [bitset bytes: ceil(bitsetSize/8)]

std::vector<std::byte> metaDataStream::IndexRecord::serialize() const {
  const size_t bitsetSize = nullBitset.size();
  const size_t byteCount  = (bitsetSize + 7) / 8;

  const size_t totalSize = sizeof(size_t) + sizeof(uint8_t) + sizeof(size_t) + byteCount;
  std::vector<std::byte> buf(totalSize, std::byte{0});
  std::byte *ptr = buf.data();

  std::memcpy(ptr, &recordCount, sizeof(recordCount));
  ptr += sizeof(recordCount);

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
  currentEntry_.nullBitset.resize(descriptorRef_->size(), false);
  currentEntry_.recordCount = 0;
  currentEntry_.isGap       = false;
}

size_t metaDataStream::entrySize() const {
  const size_t bitsetBytes = (descriptorRef_->size() + 7) / 8;
  return sizeof(size_t) + sizeof(uint8_t) + sizeof(size_t) + bitsetBytes;
}

static constexpr size_t kHeaderSize = sizeof(int64_t) + sizeof(int32_t) + sizeof(int32_t);

void metaDataStream::flushCurrentEntry() {
  if (currentEntry_.recordCount > 0) {
    appendEntry(currentEntry_);
    committedRecordCount_ += currentEntry_.recordCount;
    currentEntry_.recordCount = 0;
  }
}

void metaDataStream::saveHeader() {
  if (metaFilePath_.empty()) return;

  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) return;

  int64_t creationTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(creationTime_.time_since_epoch()).count();
  int32_t rNum           = rInterval_.numerator();
  int32_t rDen           = rInterval_.denominator();
  out.write(reinterpret_cast<const char *>(&creationTimeNs), sizeof(creationTimeNs));
  out.write(reinterpret_cast<const char *>(&rNum), sizeof(rNum));
  out.write(reinterpret_cast<const char *>(&rDen), sizeof(rDen));
}

void metaDataStream::appendEntry(const IndexRecord &entry) {
  if (metaFilePath_.empty()) return;

  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::app);
  if (!out.is_open()) return;

  auto buf = entry.serialize();
  out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

void metaDataStream::rewriteFile(const std::vector<IndexRecord> &entries) {
  if (metaFilePath_.empty()) return;

  const std::string tmpPath = metaFilePath_ + ".tmp";
  std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) return;

  int64_t creationTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(creationTime_.time_since_epoch()).count();
  int32_t rNum           = rInterval_.numerator();
  int32_t rDen           = rInterval_.denominator();
  out.write(reinterpret_cast<const char *>(&creationTimeNs), sizeof(creationTimeNs));
  out.write(reinterpret_cast<const char *>(&rNum), sizeof(rNum));
  out.write(reinterpret_cast<const char *>(&rDen), sizeof(rDen));

  for (const auto &rec : entries) {
    auto buf = rec.serialize();
    out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
  }

  out.close();
  std::filesystem::rename(tmpPath, metaFilePath_);
}

std::vector<metaDataStream::IndexRecord> metaDataStream::readCommittedEntries() const {
  std::vector<IndexRecord> result;
  if (metaFilePath_.empty()) return result;

  std::ifstream in(metaFilePath_, std::ios::binary);
  if (!in.is_open()) return result;

  in.seekg(0, std::ios::end);
  const auto fileSize = in.tellg();
  if (static_cast<size_t>(fileSize) <= kHeaderSize) return result;

  in.seekg(static_cast<std::streamoff>(kHeaderSize), std::ios::beg);

  const auto remainingSize = static_cast<size_t>(fileSize) - kHeaderSize;
  std::vector<std::byte> fileData(remainingSize);
  in.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(remainingSize));

  std::span<const std::byte> remaining(fileData);
  const size_t eSize = entrySize();

  while (remaining.size() >= eSize) {
    auto rec = IndexRecord::deserialize(remaining.subspan(0, eSize));
    result.push_back(std::move(rec));
    remaining = remaining.subspan(eSize);
  }

  return result;
}

void metaDataStream::loadIndex() {
  std::ifstream in(metaFilePath_, std::ios::binary);
  if (!in.is_open()) return;

  in.seekg(0, std::ios::end);
  const auto fileSize = in.tellg();
  if (fileSize <= 0) return;
  in.seekg(0, std::ios::beg);

  if (static_cast<size_t>(fileSize) < kHeaderSize) return;

  int64_t creationTimeNs = 0;
  int32_t rNum = 0, rDen = 1;
  in.read(reinterpret_cast<char *>(&creationTimeNs), sizeof(creationTimeNs));
  in.read(reinterpret_cast<char *>(&rNum), sizeof(rNum));
  in.read(reinterpret_cast<char *>(&rDen), sizeof(rDen));

  creationTime_ = std::chrono::system_clock::time_point(std::chrono::nanoseconds(creationTimeNs));
  if (rDen != 0) rInterval_ = boost::rational<int>(rNum, rDen);

  in.close();

  // Read all committed entries from file
  auto allEntries = readCommittedEntries();

  // Restore currentEntry_ from the last non-gap entry for RLE continuation
  if (!allEntries.empty() && !allEntries.back().isGap) {
    currentEntry_ = allEntries.back();
    allEntries.pop_back();

    // Rewrite file without the last entry (it's now in currentEntry_)
    rewriteFile(allEntries);
  }

  // Compute committedRecordCount_ from what remains on disk
  committedRecordCount_ = 0;
  for (const auto &rec : allEntries) {
    committedRecordCount_ += rec.recordCount;
  }
}

std::pair<size_t, size_t> metaDataStream::locateRecord(size_t recordIndex) const {
  // First check committed entries on disk
  if (recordIndex < committedRecordCount_) {
    auto entries      = readCommittedEntries();
    size_t cumulative = 0;
    for (size_t i = 0; i < entries.size(); ++i) {
      if (entries[i].isGap) continue;
      if (recordIndex < cumulative + entries[i].recordCount) {
        return {i, recordIndex - cumulative};
      }
      cumulative += entries[i].recordCount;
    }
  }

  // Check the currentEntry_
  if (recordIndex < committedRecordCount_ + currentEntry_.recordCount) {
    return {std::numeric_limits<size_t>::max(), recordIndex - committedRecordCount_};
  }

  throw std::out_of_range("recordIndex out of range in metaDataStream::locateRecord");
}

// ── Construction / destruction ───────────────────────────────────────

metaDataStream::metaDataStream(const Descriptor &descriptor, const std::string &metaFilePath, boost::rational<int> rInterval)
    : descriptorRef_(std::make_shared<Descriptor>(descriptor)),
      metaFilePath_(metaFilePath),
      rInterval_(rInterval),
      creationTime_(std::chrono::system_clock::now()) {
  createNullBitsetTemplate();
  loadIndex();  // may overwrite creationTime_ & rInterval_ from file header

  // Persist header even when there are no entries yet,
  // so that the file exists on disk for external tools / crash recovery.
  if (!metaFilePath_.empty() && !std::filesystem::exists(metaFilePath_)) {
    saveHeader();
  }
}

metaDataStream::~metaDataStream() { flushCurrentEntry(); }

// ── Core update interface ────────────────────────────────────────────

void metaDataStream::onRecordAppended(std::vector<bool> &nullBitsetParam) {
  if (currentEntry_.recordCount > 0 && currentEntry_.nullBitset == nullBitsetParam) {
    currentEntry_.recordCount++;
  } else {
    flushCurrentEntry();  // appends old currentEntry_ to file
    currentEntry_.nullBitset  = nullBitsetParam;
    currentEntry_.recordCount = 1;
    currentEntry_.isGap       = false;
  }
}

void metaDataStream::onRecordModified(size_t recordIndex, std::vector<bool> &nullBitsetParam) {
  auto [segIdx, offset] = locateRecord(recordIndex);

  const bool inCurrentEntry = (segIdx == std::numeric_limits<size_t>::max());

  if (inCurrentEntry) {
    // Modify within currentEntry_ (in memory only)
    if (currentEntry_.nullBitset == nullBitsetParam) return;

    // Split currentEntry_ into up to 3 parts: commit prefix to file, keep tail in currentEntry_
    std::vector<IndexRecord> toCommit;

    if (offset > 0) {
      IndexRecord before;
      before.nullBitset  = currentEntry_.nullBitset;
      before.recordCount = offset;
      toCommit.push_back(std::move(before));
    }

    {
      IndexRecord modified;
      modified.nullBitset  = nullBitsetParam;
      modified.recordCount = 1;
      toCommit.push_back(std::move(modified));
    }

    size_t tailCount = currentEntry_.recordCount - offset - 1;

    // Commit all parts except the last one to file; the last stays as currentEntry_
    IndexRecord newCurrent;
    if (tailCount > 0) {
      newCurrent.nullBitset  = currentEntry_.nullBitset;
      newCurrent.recordCount = tailCount;
    } else {
      newCurrent = toCommit.back();
      toCommit.pop_back();
    }

    for (const auto &entry : toCommit) {
      appendEntry(entry);
      committedRecordCount_ += entry.recordCount;
    }

    currentEntry_ = newCurrent;
    return;
  }

  // Modify a committed entry on disk — read, split, rewrite
  auto allEntries = readCommittedEntries();

  auto &seg = allEntries[segIdx];
  if (seg.nullBitset == nullBitsetParam) return;

  std::vector<IndexRecord> replacement;

  if (offset > 0) {
    IndexRecord before;
    before.nullBitset  = seg.nullBitset;
    before.recordCount = offset;
    replacement.push_back(std::move(before));
  }

  {
    IndexRecord modified;
    modified.nullBitset  = nullBitsetParam;
    modified.recordCount = 1;
    replacement.push_back(std::move(modified));
  }

  if (offset + 1 < seg.recordCount) {
    IndexRecord after;
    after.nullBitset  = seg.nullBitset;
    after.recordCount = seg.recordCount - offset - 1;
    replacement.push_back(std::move(after));
  }

  allEntries.erase(allEntries.begin() + static_cast<std::ptrdiff_t>(segIdx));
  allEntries.insert(allEntries.begin() + static_cast<std::ptrdiff_t>(segIdx), replacement.begin(), replacement.end());

  rewriteFile(allEntries);

  // Recompute committedRecordCount_
  committedRecordCount_ = 0;
  for (const auto &rec : allEntries) {
    committedRecordCount_ += rec.recordCount;
  }
}

// ── Query interface ──────────────────────────────────────────────────

std::vector<bool> metaDataStream::getNullBitset(size_t recordIndex) const {
  auto [segIdx, offset] = locateRecord(recordIndex);
  if (segIdx == std::numeric_limits<size_t>::max()) {
    return currentEntry_.nullBitset;
  }
  auto entries = readCommittedEntries();
  return entries[segIdx].nullBitset;
}

bool metaDataStream::isFieldNull(size_t recordIndex, size_t fieldIndex) const {
  auto bitset = getNullBitset(recordIndex);
  if (fieldIndex >= bitset.size()) throw std::out_of_range("fieldIndex out of range");
  return bitset[fieldIndex];
}

size_t metaDataStream::totalRecords() const { return committedRecordCount_ + currentEntry_.recordCount; }

std::vector<metaDataStream::IndexRecord> metaDataStream::entries() const { return readCommittedEntries(); }

// ── Transmission-gap interface ───────────────────────────────────────

void metaDataStream::onTransmissionGap() {
  flushCurrentEntry();

  IndexRecord gapMarker;
  gapMarker.nullBitset.resize(descriptorRef_->size(), false);
  gapMarker.recordCount = 0;
  gapMarker.isGap       = true;
  appendEntry(gapMarker);

  createNullBitsetTemplate();
}

bool metaDataStream::isGapBefore(size_t recordIndex) const {
  if (recordIndex == 0) return false;

  auto allEntries   = readCommittedEntries();
  size_t cumulative = 0;
  for (size_t i = 0; i < allEntries.size(); ++i) {
    if (allEntries[i].isGap) {
      if (cumulative == recordIndex) return true;
      continue;
    }
    cumulative += allEntries[i].recordCount;
  }
  return false;
}

// ── Time calculation interface ───────────────────────────────────────

std::chrono::system_clock::time_point metaDataStream::calculateRecordTimestamp(size_t recordIndex) const {
  // time = creationTime_ + recordIndex * rInterval_ (in seconds)
  int64_t num   = static_cast<int64_t>(recordIndex) * rInterval_.numerator();
  int64_t den   = rInterval_.denominator();
  auto offsetNs = std::chrono::nanoseconds(num * 1000000000LL / den);
  return creationTime_ + offsetNs;
}

std::chrono::system_clock::time_point metaDataStream::getCreationTime() const { return creationTime_; }

boost::rational<int> metaDataStream::getSamplingInterval() const { return rInterval_; }

// ──  ─────────────────────────────────────────────────────────

const Descriptor &metaDataStream::descriptor() const { return *descriptorRef_; }

}  // namespace rdb
