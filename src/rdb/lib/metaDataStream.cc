#include "rdb/metaDataStream.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>

namespace rdb {

constexpr size_t kBitsPerByte = 8;

// ── IndexRecord serialization ────────────────────────────────────────

std::vector<std::byte> metaDataStream::IndexRecord::serialize() const {
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
  const size_t byteCount = (bitsetSize + (kBitsPerByte - 1)) / kBitsPerByte;
  if (ptr + byteCount > end) throw std::runtime_error("Buffer underrun in bitset data");

  rec.nullBitset.resize(bitsetSize);
  for (size_t i = 0; i < bitsetSize; ++i)
    rec.nullBitset[i] = ((std::to_integer<uint8_t>(ptr[i / kBitsPerByte]) >> (i % kBitsPerByte)) & 1U) != 0;

  return rec;
}

// ── File I/O helpers ─────────────────────────────────────────────────

namespace {

constexpr size_t kHeaderSize = sizeof(int64_t);

void writeHeader(std::ostream &out, std::chrono::system_clock::time_point t) {
  int64_t ns       = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
  const auto bytes = std::as_bytes(std::span{&ns, 1});
  out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size_bytes()));
}

void writeEntry(std::ostream &out, const metaDataStream::IndexRecord &entry) {
  auto buf = entry.serialize();
  out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

std::vector<metaDataStream::IndexRecord> splitSegment(const metaDataStream::IndexRecord &seg, size_t offset,
                                                      const std::vector<bool> &newBitset) {
  std::vector<metaDataStream::IndexRecord> result;
  if (offset > 0)
    result.emplace_back(metaDataStream::IndexRecord{.nullBitset = seg.nullBitset, .recordCount = offset, .isGap = false});
  result.emplace_back(metaDataStream::IndexRecord{.nullBitset = newBitset, .recordCount = 1, .isGap = false});
  if (offset + 1 < seg.recordCount)
    result.emplace_back(
        metaDataStream::IndexRecord{.nullBitset = seg.nullBitset, .recordCount = seg.recordCount - offset - 1, .isGap = false});
  return result;
}

size_t sumNonGapRecords(const std::vector<metaDataStream::IndexRecord> &entries) {
  return std::ranges::fold_left(entries, 0UZ, [](size_t acc, const auto &e) { return acc + (e.isGap ? 0UZ : e.recordCount); });
}

}  // namespace

// ── Private helpers ──────────────────────────────────────────────────

void metaDataStream::createNullBitsetTemplate() {
  currentEntry_.nullBitset.resize(descriptorRef_->size(), false);
  currentEntry_.recordCount = 0;
}

void metaDataStream::flushCurrentEntry() {
  if (currentEntry_.recordCount > 0) {
    if (tail_.shouldOverwrite()) {
      overwriteLastEntry(currentEntry_);
    } else {
      appendEntry(currentEntry_);
    }
    committedRecordCount_ += currentEntry_.recordCount;
    tail_.markWritten(currentEntry_.recordCount);
    currentEntry_.recordCount = 0;
  }
}

void metaDataStream::overwriteLastEntry(const IndexRecord &entry) {
  if (metaFilePath_.empty()) return;
  std::fstream f(metaFilePath_, std::ios::binary | std::ios::in | std::ios::out);
  if (!f.is_open()) return;
  f.seekp(0, std::ios::end);
  // Compare stream positions as streamoff; cmp_less with streampos is ill-formed.
  const auto fileSize = static_cast<std::streamoff>(f.tellp());
  if (std::cmp_less(fileSize, kHeaderSize + entrySize_)) return;
  f.seekp(-static_cast<std::streamoff>(entrySize_), std::ios::end);
  auto buf = entry.serialize();
  f.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
  cacheValid_ = false;
}

void metaDataStream::saveHeader() {
  if (metaFilePath_.empty()) return;
  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::trunc);
  if (out.is_open()) writeHeader(out, creationTime_);
  cacheValid_ = false;
}

void metaDataStream::appendEntry(const IndexRecord &entry) {
  if (metaFilePath_.empty()) return;
  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::app);
  if (out.is_open()) writeEntry(out, entry);
  cacheValid_ = false;
}

void metaDataStream::rewriteFile(const std::vector<IndexRecord> &entries) {
  if (metaFilePath_.empty()) return;
  const std::string tmpPath = std::format("{}.tmp", metaFilePath_);
  std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) return;
  writeHeader(out, creationTime_);
  for (const auto &rec : entries)
    writeEntry(out, rec);
  out.close();
  std::filesystem::rename(tmpPath, metaFilePath_);
  cacheValid_ = false;
}

std::vector<metaDataStream::IndexRecord> metaDataStream::readCommittedEntries() const {
  if (cacheValid_) return entriesCache_;

  entriesCache_.clear();
  if (metaFilePath_.empty()) {
    cacheValid_ = true;
    return entriesCache_;
  }

  std::ifstream in(metaFilePath_, std::ios::binary);
  if (!in.is_open()) {
    cacheValid_ = true;
    return entriesCache_;
  }

  in.seekg(0, std::ios::end);
  // Compare stream positions as streamoff; cmp_less with streampos is ill-formed.
  const auto fileSize = static_cast<std::streamoff>(in.tellg());
  if (fileSize <= 0 || std::cmp_less_equal(fileSize, kHeaderSize)) {
    cacheValid_ = true;
    return entriesCache_;
  }

  const auto payloadSize = static_cast<size_t>(fileSize - static_cast<std::streamoff>(kHeaderSize));
  if (payloadSize % entrySize_ != 0)
    SPDLOG_WARN("metaDataStream: unexpected payload alignment (payloadSize={}, entrySize={})", payloadSize, entrySize_);

  in.seekg(static_cast<std::streamoff>(kHeaderSize), std::ios::beg);
  std::vector<std::byte> fileData(payloadSize);
  in.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(payloadSize));

  std::span<const std::byte> remaining(fileData);
  while (remaining.size() >= entrySize_) {
    entriesCache_.push_back(IndexRecord::deserialize(remaining.subspan(0, entrySize_)));
    remaining = remaining.subspan(entrySize_);
  }

  cacheValid_ = true;
  return entriesCache_;
}

void metaDataStream::loadIndex() {
  std::ifstream in(metaFilePath_, std::ios::binary);
  if (!in.is_open()) return;

  in.seekg(0, std::ios::end);
  // Compare stream positions as streamoff; cmp_less with streampos is ill-formed.
  const auto fileSize = static_cast<std::streamoff>(in.tellg());
  if (fileSize <= 0 || std::cmp_less(fileSize, kHeaderSize)) return;
  in.seekg(0, std::ios::beg);

  std::array<std::byte, sizeof(int64_t)> nsBuf{};
  in.read(reinterpret_cast<char *>(nsBuf.data()), static_cast<std::streamsize>(nsBuf.size()));
  in.close();
  int64_t creationTimeNs{};
  std::memcpy(&creationTimeNs, nsBuf.data(), sizeof(creationTimeNs));

  creationTime_ = std::chrono::system_clock::time_point(std::chrono::nanoseconds(creationTimeNs));

  auto allEntries = readCommittedEntries();

  if (!allEntries.empty() && !allEntries.back().isGap) {
    currentEntry_ = allEntries.back();
    allEntries.pop_back();
    rewriteFile(allEntries);
  }

  committedRecordCount_ = sumNonGapRecords(allEntries);
}

std::pair<std::optional<size_t>, size_t> metaDataStream::locateRecord(size_t recordIndex) const {
  if (recordIndex < committedRecordCount_) {
    auto entries        = readCommittedEntries();
    size_t cumulative   = 0;
    size_t segmentIndex = 0;
    for (const auto &entry : entries) {
      if (entry.isGap) {
        ++segmentIndex;
        continue;
      }
      if (recordIndex < cumulative + entry.recordCount) return {segmentIndex, recordIndex - cumulative};
      cumulative += entry.recordCount;
      ++segmentIndex;
    }
    throw std::logic_error("metaDataStream: committedRecordCount_ inconsistent with on-disk entries");
  }

  if (recordIndex < committedRecordCount_ + currentEntry_.recordCount)
    return {std::nullopt, recordIndex - committedRecordCount_};

  throw std::out_of_range("recordIndex out of range in metaDataStream::locateRecord");
}

// ── Construction / destruction ───────────────────────────────────────

metaDataStream::metaDataStream(const Descriptor &descriptor, std::string metaFilePath)
    : metaFilePath_(std::move(metaFilePath)),
      descriptorRef_(std::make_shared<Descriptor>(descriptor)),
      creationTime_(std::chrono::system_clock::now()),
      entrySize_(sizeof(uint8_t) + (2 * sizeof(size_t)) + ((descriptor.size() + (kBitsPerByte - 1)) / kBitsPerByte)) {
  shadowFilePath_ = metaFilePath_.empty() ? std::string{} : metaFilePath_ + ".shadow";
  createNullBitsetTemplate();
  loadIndex();
  if (!metaFilePath_.empty() && !std::filesystem::exists(metaFilePath_)) saveHeader();
}

metaDataStream::~metaDataStream() { flushCurrentEntry(); }

// ── Core update interface ────────────────────────────────────────────

void metaDataStream::onRecordAppended(const std::vector<bool> &nullBitsetParam) {
  if (currentEntry_.nullBitset == nullBitsetParam && (currentEntry_.recordCount > 0 || tail_.hasPending())) {
    if (currentEntry_.recordCount == 0) {
      // Re-absorb last committed entry for lazy extension (lazy overwrite)
      currentEntry_.recordCount = tail_.committedCount;
      committedRecordCount_ -= tail_.committedCount;
      tail_.markDirty();
    }
    currentEntry_.recordCount++;
  } else {
    tail_.clearPending();
    flushCurrentEntry();
    currentEntry_.nullBitset  = nullBitsetParam;
    currentEntry_.recordCount = 1;
  }
}

void metaDataStream::onRecordModified(size_t recordIndex, const std::vector<bool> &nullBitsetParam) {
  if (shadowMode_) {
    appendShadowOverride(recordIndex, nullBitsetParam);
    return;
  }
  applyModificationToMainIndex(recordIndex, nullBitsetParam);
}

void metaDataStream::applyModificationToMainIndex(size_t recordIndex, const std::vector<bool> &nullBitsetParam) {
  auto [segIdx, offset]     = locateRecord(recordIndex);
  const bool inCurrentEntry = !segIdx.has_value();

  if (inCurrentEntry) {
    if (currentEntry_.nullBitset == nullBitsetParam) return;

    auto parts         = splitSegment(currentEntry_, offset, nullBitsetParam);
    IndexRecord newCur = parts.back();
    parts.pop_back();

    bool isFirst = true;
    for (const auto &part : parts) {
      if (isFirst && tail_.shouldOverwrite()) {
        overwriteLastEntry(part);  // replace stale on-disk entry, not append after it
      } else {
        appendEntry(part);
      }
      committedRecordCount_ += part.recordCount;
      isFirst = false;
    }

    tail_.reset();
    currentEntry_ = newCur;
    return;
  }

  auto allEntries = readCommittedEntries();
  // Gdy ostatni wpis na dysku jest przestarzały (został wciągnięty do currentEntry_
  // mechanizmem lazy overwrite), usuń go przed rewriteFile — w przeciwnym razie
  // logicznie nadpisany wpis zostałby utrwalony i policzony podwójnie.
  if (tail_.shouldOverwrite() && !allEntries.empty()) allEntries.pop_back();

  auto &seg = allEntries[*segIdx];
  if (seg.nullBitset == nullBitsetParam) return;

  auto replacement = splitSegment(seg, offset, nullBitsetParam);
  allEntries.erase(allEntries.begin() + static_cast<std::ptrdiff_t>(*segIdx));
  allEntries.insert(allEntries.begin() + static_cast<std::ptrdiff_t>(*segIdx), replacement.begin(), replacement.end());

  rewriteFile(allEntries);
  tail_.reset();
  committedRecordCount_ = sumNonGapRecords(allEntries);
}

// ── Query interface ──────────────────────────────────────────────────

std::vector<bool> metaDataStream::getNullBitset(size_t recordIndex) const {
  if (shadowMode_)
    for (auto it = shadowOverrides_.rbegin(); it != shadowOverrides_.rend(); ++it)
      if (it->recordCount == recordIndex) return it->nullBitset;  // ostatni wpis dla pozycji jest obowiązujący

  auto [segIdx, offset] = locateRecord(recordIndex);
  if (!segIdx) return currentEntry_.nullBitset;
  return readCommittedEntries()[*segIdx].nullBitset;
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
  tail_.reset();

  IndexRecord gapMarker{
      .nullBitset = std::vector<bool>(descriptorRef_->size(), true), .recordCount = gapDuration, .isGap = true};
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

void metaDataStream::rotate(int percounter) {
  flushCurrentEntry();
  if (percounter >= 0 && !metaFilePath_.empty() && std::filesystem::exists(metaFilePath_)) {
    std::string rotatedPath = std::format("{}.old{}", metaFilePath_, percounter);
    std::error_code ec;
    std::filesystem::rename(metaFilePath_, rotatedPath, ec);
    if (ec) SPDLOG_WARN("metaDataStream::rotate: failed to rename '{}' to '{}': {}", metaFilePath_, rotatedPath, ec.message());
  }
  creationTime_ = std::chrono::system_clock::now();
  reset();
}

void metaDataStream::reset() {
  createNullBitsetTemplate();
  committedRecordCount_ = 0;
  tail_.reset();
  discardShadow();
  saveHeader();
}

// ── Shadow-index interface (.meta.shadow) ────────────────────────────

void metaDataStream::setShadowMode(bool enabled) {
  shadowMode_ = enabled;
  if (enabled) loadShadow();
}

void metaDataStream::appendShadowOverride(size_t recordIndex, const std::vector<bool> &nullBitsetParam) {
  if (recordIndex >= totalRecords())
    throw std::out_of_range("recordIndex out of range in metaDataStream::onRecordModified (shadow)");

  IndexRecord ov{.nullBitset  = nullBitsetParam,
                 .recordCount = recordIndex,
                 .isGap       = false};  // recordCount pełni rolę absolutnego indeksu rekordu
  shadowOverrides_.push_back(ov);

  if (shadowFilePath_.empty()) return;
  std::ofstream out(shadowFilePath_, std::ios::binary | std::ios::app);
  if (out.is_open()) writeEntry(out, ov);
}

void metaDataStream::loadShadow() {
  shadowOverrides_.clear();
  if (shadowFilePath_.empty()) return;

  std::ifstream in(shadowFilePath_, std::ios::binary);
  if (!in.is_open()) return;

  in.seekg(0, std::ios::end);
  const auto fileSize = static_cast<std::streamoff>(in.tellg());
  if (fileSize <= 0) return;

  const auto payloadSize = static_cast<size_t>(fileSize);
  if (payloadSize % entrySize_ != 0)
    SPDLOG_WARN("metaDataStream: unexpected shadow alignment (size={}, entrySize={})", payloadSize, entrySize_);

  in.seekg(0, std::ios::beg);
  std::vector<std::byte> fileData(payloadSize);
  in.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(payloadSize));

  std::span<const std::byte> remaining(fileData);
  while (remaining.size() >= entrySize_) {
    shadowOverrides_.push_back(IndexRecord::deserialize(remaining.subspan(0, entrySize_)));
    remaining = remaining.subspan(entrySize_);
  }
}

void metaDataStream::mergeShadow() {
  for (const auto &ov : shadowOverrides_)
    applyModificationToMainIndex(ov.recordCount, ov.nullBitset);
  discardShadow();
}

void metaDataStream::discardShadow() {
  shadowOverrides_.clear();
  if (shadowFilePath_.empty()) return;
  std::error_code ec;
  std::filesystem::remove(shadowFilePath_, ec);
}

}  // namespace rdb
