#include "rdb/metaDataStream.hpp"

#include <spdlog/spdlog.h>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace rdb {

// ── IndexRecord serialization ────────────────────────────────────────

std::vector<std::byte> metaDataStream::IndexRecord::serialize() const {
  const size_t bitsetSize = nullBitset.size();
  const size_t byteCount  = (bitsetSize + 7) / 8;
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

  IndexRecord rec;
  uint8_t gapFlag = 0;
  read(gapFlag);
  rec.isGap = (gapFlag != 0);
  read(rec.recordCount);

  size_t bitsetSize = 0;
  read(bitsetSize);
  const size_t byteCount = (bitsetSize + 7) / 8;
  if (ptr + byteCount > end) throw std::runtime_error("Buffer underrun in bitset data");

  rec.nullBitset.resize(bitsetSize);
  for (size_t i = 0; i < bitsetSize; ++i)
    rec.nullBitset[i] = (std::to_integer<uint8_t>(ptr[i / 8]) >> (i % 8)) & 1u;

  return rec;
}

// ── File I/O helpers ─────────────────────────────────────────────────

namespace {

constexpr size_t kHeaderSize = sizeof(int64_t);

void writeHeader(std::ostream &out, std::chrono::system_clock::time_point t) {
  int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
  out.write(reinterpret_cast<const char *>(&ns), sizeof(ns));
}

void writeEntry(std::ostream &out, const metaDataStream::IndexRecord &entry) {
  auto buf = entry.serialize();
  out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

std::vector<metaDataStream::IndexRecord> splitSegment(const metaDataStream::IndexRecord &seg, size_t offset,
                                                      const std::vector<bool> &newBitset) {
  std::vector<metaDataStream::IndexRecord> result;
  if (offset > 0) result.push_back({seg.nullBitset, offset, false});
  result.push_back({newBitset, 1, false});
  if (offset + 1 < seg.recordCount) result.push_back({seg.nullBitset, seg.recordCount - offset - 1, false});
  return result;
}

size_t sumNonGapRecords(const std::vector<metaDataStream::IndexRecord> &entries) {
  size_t total = 0;
  for (const auto &e : entries)
    if (!e.isGap) total += e.recordCount;
  return total;
}

}  // namespace

// ── Private helpers ──────────────────────────────────────────────────

void metaDataStream::createNullBitsetTemplate() {
  currentEntry_.nullBitset.resize(descriptorRef_->size(), false);
  currentEntry_.recordCount = 0;
}

size_t metaDataStream::entrySize() const {
  return sizeof(uint8_t) + sizeof(size_t) + sizeof(size_t) + (descriptorRef_->size() + 7) / 8;
}

void metaDataStream::flushCurrentEntry() {
  if (currentEntry_.recordCount > 0) {
    if (tailDirty_) {
      overwriteLastEntry(currentEntry_);
      tailDirty_ = false;
    } else {
      appendEntry(currentEntry_);
    }
    committedRecordCount_ += currentEntry_.recordCount;
    pendingCommittedCount_    = currentEntry_.recordCount;
    currentEntry_.recordCount = 0;
  }
}

void metaDataStream::overwriteLastEntry(const IndexRecord &entry) {
  if (metaFilePath_.empty()) return;
  std::fstream f(metaFilePath_, std::ios::binary | std::ios::in | std::ios::out);
  if (!f.is_open()) return;
  f.seekp(0, std::ios::end);
  if (f.tellp() < static_cast<std::streamoff>(kHeaderSize + entrySize())) return;
  f.seekp(-static_cast<std::streamoff>(entrySize()), std::ios::end);
  auto buf = entry.serialize();
  f.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

void metaDataStream::saveHeader() {
  if (metaFilePath_.empty()) return;
  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::trunc);
  if (out.is_open()) writeHeader(out, creationTime_);
}

void metaDataStream::appendEntry(const IndexRecord &entry) {
  if (metaFilePath_.empty()) return;
  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::app);
  if (out.is_open()) writeEntry(out, entry);
}

void metaDataStream::rewriteFile(const std::vector<IndexRecord> &entries) {
  if (metaFilePath_.empty()) return;
  const std::string tmpPath = metaFilePath_ + ".tmp";
  std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) return;
  writeHeader(out, creationTime_);
  for (const auto &rec : entries)
    writeEntry(out, rec);
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
  if (fileSize <= 0 || static_cast<size_t>(fileSize) <= kHeaderSize) return result;

  const size_t eSize       = entrySize();
  const size_t payloadSize = static_cast<size_t>(fileSize) - kHeaderSize;
  if (payloadSize % eSize != 0)
    SPDLOG_WARN("metaDataStream: unexpected payload alignment (payloadSize={}, entrySize={})", payloadSize, eSize);

  in.seekg(static_cast<std::streamoff>(kHeaderSize), std::ios::beg);
  std::vector<std::byte> fileData(payloadSize);
  in.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(payloadSize));

  std::span<const std::byte> remaining(fileData);
  while (remaining.size() >= eSize) {
    result.push_back(IndexRecord::deserialize(remaining.subspan(0, eSize)));
    remaining = remaining.subspan(eSize);
  }

  return result;
}

void metaDataStream::loadIndex() {
  std::ifstream in(metaFilePath_, std::ios::binary);
  if (!in.is_open()) return;

  in.seekg(0, std::ios::end);
  const auto fileSize = in.tellg();
  if (fileSize <= 0 || static_cast<size_t>(fileSize) < kHeaderSize) return;
  in.seekg(0, std::ios::beg);

  int64_t creationTimeNs = 0;
  in.read(reinterpret_cast<char *>(&creationTimeNs), sizeof(creationTimeNs));
  in.close();

  creationTime_ = std::chrono::system_clock::time_point(std::chrono::nanoseconds(creationTimeNs));

  auto allEntries = readCommittedEntries();

  if (!allEntries.empty() && !allEntries.back().isGap) {
    currentEntry_ = allEntries.back();
    allEntries.pop_back();
    rewriteFile(allEntries);
  }

  committedRecordCount_ = sumNonGapRecords(allEntries);
}

std::pair<size_t, size_t> metaDataStream::locateRecord(size_t recordIndex) const {
  if (recordIndex < committedRecordCount_) {
    auto entries      = readCommittedEntries();
    size_t cumulative = 0;
    for (size_t i = 0; i < entries.size(); ++i) {
      if (entries[i].isGap) continue;
      if (recordIndex < cumulative + entries[i].recordCount) return {i, recordIndex - cumulative};
      cumulative += entries[i].recordCount;
    }
    throw std::logic_error("metaDataStream: committedRecordCount_ inconsistent with on-disk entries");
  }

  if (recordIndex < committedRecordCount_ + currentEntry_.recordCount)
    return {std::numeric_limits<size_t>::max(), recordIndex - committedRecordCount_};

  throw std::out_of_range("recordIndex out of range in metaDataStream::locateRecord");
}

// ── Construction / destruction ───────────────────────────────────────

metaDataStream::metaDataStream(const Descriptor &descriptor, const std::string &metaFilePath)
    : descriptorRef_(std::make_shared<Descriptor>(descriptor)),
      metaFilePath_(metaFilePath),
      creationTime_(std::chrono::system_clock::now()) {
  createNullBitsetTemplate();
  loadIndex();
  if (!metaFilePath_.empty() && !std::filesystem::exists(metaFilePath_)) saveHeader();
}

metaDataStream::~metaDataStream() { flushCurrentEntry(); }

// ── Core update interface ────────────────────────────────────────────

void metaDataStream::onRecordAppended(const std::vector<bool> &nullBitsetParam) {
  if (currentEntry_.nullBitset == nullBitsetParam && (currentEntry_.recordCount > 0 || pendingCommittedCount_ > 0)) {
    if (currentEntry_.recordCount == 0) {
      tailDirty_                = true;
      currentEntry_.recordCount = pendingCommittedCount_;
      committedRecordCount_ -= pendingCommittedCount_;
      pendingCommittedCount_ = 0;
    }
    currentEntry_.recordCount++;
  } else {
    pendingCommittedCount_ = 0;
    flushCurrentEntry();
    currentEntry_.nullBitset  = nullBitsetParam;
    currentEntry_.recordCount = 1;
  }
}

void metaDataStream::onRecordModified(size_t recordIndex, const std::vector<bool> &nullBitsetParam) {
  auto [segIdx, offset]     = locateRecord(recordIndex);
  const bool inCurrentEntry = (segIdx == std::numeric_limits<size_t>::max());

  if (inCurrentEntry) {
    if (currentEntry_.nullBitset == nullBitsetParam) return;

    auto parts         = splitSegment(currentEntry_, offset, nullBitsetParam);
    IndexRecord newCur = parts.back();
    parts.pop_back();

    for (const auto &entry : parts) {
      appendEntry(entry);
      committedRecordCount_ += entry.recordCount;
    }

    pendingCommittedCount_ = 0;
    currentEntry_          = newCur;
    return;
  }

  auto allEntries = readCommittedEntries();
  auto &seg       = allEntries[segIdx];
  if (seg.nullBitset == nullBitsetParam) return;

  auto replacement = splitSegment(seg, offset, nullBitsetParam);
  allEntries.erase(allEntries.begin() + static_cast<std::ptrdiff_t>(segIdx));
  allEntries.insert(allEntries.begin() + static_cast<std::ptrdiff_t>(segIdx), replacement.begin(), replacement.end());

  rewriteFile(allEntries);
  pendingCommittedCount_ = 0;
  committedRecordCount_  = sumNonGapRecords(allEntries);
}

// ── Query interface ──────────────────────────────────────────────────

std::vector<bool> metaDataStream::getNullBitset(size_t recordIndex) const {
  auto [segIdx, offset] = locateRecord(recordIndex);
  if (segIdx == std::numeric_limits<size_t>::max()) return currentEntry_.nullBitset;
  return readCommittedEntries()[segIdx].nullBitset;
}

size_t metaDataStream::totalRecords() const { return committedRecordCount_ + currentEntry_.recordCount; }

std::vector<metaDataStream::IndexRecord> metaDataStream::segments() const {
  auto result = readCommittedEntries();
  if (currentEntry_.recordCount > 0) result.push_back(currentEntry_);
  return result;
}

// ── Transmission-gap interface ───────────────────────────────────────

void metaDataStream::onTransmissionGap(size_t gapDuration) {
  flushCurrentEntry();
  pendingCommittedCount_ = 0;

  IndexRecord gapMarker;
  gapMarker.nullBitset.resize(descriptorRef_->size(), true);
  gapMarker.recordCount = gapDuration;
  gapMarker.isGap       = true;
  appendEntry(gapMarker);
}

bool metaDataStream::isGapBefore(size_t recordIndex) const {
  if (recordIndex == 0) return false;

  size_t cumulative = 0;
  for (const auto &e : readCommittedEntries()) {
    if (e.isGap) {
      if (cumulative == recordIndex) return true;
      continue;
    }
    cumulative += e.recordCount;
  }
  return false;
}

bool metaDataStream::isEmpty() const { return totalRecords() == 0; }

void metaDataStream::reset() {
  createNullBitsetTemplate();
  committedRecordCount_  = 0;
  pendingCommittedCount_ = 0;
  tailDirty_             = false;
  saveHeader();
}

}  // namespace rdb
