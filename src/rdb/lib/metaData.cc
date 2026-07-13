#include "rdb/metaData.hpp"

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
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>

namespace rdb {

constexpr size_t kBitsPerByte = 8;  ///< also used below for entrySize_ (must match indexRecord.cc's packing)

// ── File I/O helpers ─────────────────────────────────────────────────

namespace {

constexpr size_t kHeaderSize = sizeof(int64_t);

void writeHeader(std::ostream &out, std::chrono::system_clock::time_point t) {
  int64_t ns       = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
  const auto bytes = std::as_bytes(std::span{&ns, 1});
  out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size_bytes()));
}

void writeEntry(std::ostream &out, const metaData::IndexRecord &entry) {
  auto buf = entry.serialize();
  out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

std::vector<metaData::IndexRecord> splitSegment(const metaData::IndexRecord &seg, size_t offset,
                                                const std::vector<bool> &newBitset) {
  std::vector<metaData::IndexRecord> result;
  if (offset > 0)
    result.emplace_back(metaData::IndexRecord{.nullBitset = seg.nullBitset, .recordCount = offset, .isGap = false});
  result.emplace_back(metaData::IndexRecord{.nullBitset = newBitset, .recordCount = 1, .isGap = false});
  if (offset + 1 < seg.recordCount)
    result.emplace_back(
        metaData::IndexRecord{.nullBitset = seg.nullBitset, .recordCount = seg.recordCount - offset - 1, .isGap = false});
  return result;
}

size_t sumNonGapRecords(const std::vector<metaData::IndexRecord> &entries) {
  return std::ranges::fold_left(entries, 0UZ, [](size_t acc, const auto &e) { return acc + (e.isGap ? 0UZ : e.recordCount); });
}

}  // namespace

// ── Private helpers ──────────────────────────────────────────────────

void metaData::createNullBitsetTemplate() {
  currentEntry_.nullBitset.resize(descriptorRef_->size(), false);
  currentEntry_.recordCount = 0;
}

void metaData::flushCurrentEntry() {
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

void metaData::overwriteLastEntry(const IndexRecord &entry) {
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

void metaData::saveHeader() {
  if (metaFilePath_.empty()) return;
  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::trunc);
  if (out.is_open()) writeHeader(out, creationTime_);
  cacheValid_ = false;
}

void metaData::appendEntry(const IndexRecord &entry) {
  if (metaFilePath_.empty()) return;
  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::app);
  if (out.is_open()) writeEntry(out, entry);
  cacheValid_ = false;
}

void metaData::rewriteFile(const std::vector<IndexRecord> &entries) {
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

std::vector<metaData::IndexRecord> metaData::readCommittedEntries() const {
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
    SPDLOG_WARN("metaData: unexpected payload alignment (payloadSize={}, entrySize={})", payloadSize, entrySize_);

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

void metaData::loadIndex() {
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

std::pair<std::optional<size_t>, size_t> metaData::locateRecord(size_t recordIndex) const {
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
    throw std::logic_error("metaData: committedRecordCount_ inconsistent with on-disk entries");
  }

  if (recordIndex < committedRecordCount_ + currentEntry_.recordCount)
    return {std::nullopt, recordIndex - committedRecordCount_};

  throw std::out_of_range("recordIndex out of range in metaData::locateRecord");
}

// ── Construction / destruction ───────────────────────────────────────

metaData::metaData(const Descriptor &descriptor, std::string metaFilePath)
    : metaFilePath_(std::move(metaFilePath)),
      descriptorRef_(std::make_shared<Descriptor>(descriptor)),
      creationTime_(std::chrono::system_clock::now()),
      entrySize_(sizeof(uint8_t) + (2 * sizeof(size_t)) + ((descriptor.size() + (kBitsPerByte - 1)) / kBitsPerByte)) {
  createNullBitsetTemplate();
  loadIndex();
  if (!metaFilePath_.empty() && !std::filesystem::exists(metaFilePath_)) saveHeader();
}

metaData::~metaData() { flushCurrentEntry(); }

void metaData::abandonFile() { metaFilePath_.clear(); }

// ── Core update interface ────────────────────────────────────────────

void metaData::onRecordAppended(const std::vector<bool> &nullBitsetParam) {
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

void metaData::onRecordModified(size_t recordIndex, const std::vector<bool> &nullBitsetParam) {
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

std::vector<bool> metaData::getNullBitset(size_t recordIndex) const {
  auto [segIdx, offset] = locateRecord(recordIndex);
  if (!segIdx) return currentEntry_.nullBitset;
  return readCommittedEntries()[*segIdx].nullBitset;
}

std::vector<bool> metaData::nullBitsetFor(size_t recordIndex) const {
  if (recordIndex < totalRecords()) return getNullBitset(recordIndex);
  return std::vector<bool>(descriptorRef_->size(), false);
}

size_t metaData::totalRecords() const { return committedRecordCount_ + currentEntry_.recordCount; }

std::vector<metaData::IndexRecord> metaData::segments() const {
  auto result = readCommittedEntries();
  if (currentEntry_.recordCount > 0) result.push_back(currentEntry_);
  return result;
}

// ── Transmission-gap interface ───────────────────────────────────────

void metaData::onTransmissionGap(size_t gapDuration) {
  flushCurrentEntry();
  tail_.reset();

  IndexRecord gapMarker{
      .nullBitset = std::vector<bool>(descriptorRef_->size(), true), .recordCount = gapDuration, .isGap = true};
  appendEntry(gapMarker);
}

bool metaData::isGapBefore(size_t recordIndex) const {
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

bool metaData::isEmpty() const { return totalRecords() == 0; }

void metaData::rotate(int percounter) {
  flushCurrentEntry();
  if (percounter >= 0 && !metaFilePath_.empty() && std::filesystem::exists(metaFilePath_)) {
    std::string rotatedPath = std::format("{}.old{}", metaFilePath_, percounter);
    std::error_code ec;
    std::filesystem::rename(metaFilePath_, rotatedPath, ec);
    if (ec) SPDLOG_WARN("metaData::rotate: failed to rename '{}' to '{}': {}", metaFilePath_, rotatedPath, ec.message());
  }
  creationTime_ = std::chrono::system_clock::now();
  reset();
}

void metaData::reset() {
  createNullBitsetTemplate();
  committedRecordCount_ = 0;
  tail_.reset();
  consecutiveNullCount_ = 0;
  activeGapDuration_    = 0;
  saveHeader();
}

// ── Gap-detection state machine (przejęte od storage) ────────────────

void metaData::configureGapDetection(int nullFillCount) {
  nullFillCount_          = nullFillCount;
  gapDetectionConfigured_ = true;
}

bool metaData::absorbAppend(const std::vector<bool> &nullBitset) {
  if (!gapDetectionConfigured_) return false;

  const bool isAllNull = !nullBitset.empty() && std::ranges::all_of(nullBitset, [](bool b) { return b; });
  if (isAllNull) {
    consecutiveNullCount_++;
    if (std::cmp_greater(consecutiveNullCount_, nullFillCount_)) {
      activeGapDuration_++;
      return true;
    }
  } else {
    flushPendingGap();
    consecutiveNullCount_ = 0;
  }
  return false;
}

void metaData::flushPendingGap() {
  if (activeGapDuration_ == 0) return;
  onTransmissionGap(activeGapDuration_);
  activeGapDuration_ = 0;
}

}  // namespace rdb
