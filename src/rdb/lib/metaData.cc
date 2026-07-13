#include "rdb/metaData.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <format>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <utility>

#include "rdb/rleSegment.hpp"

namespace rdb {

constexpr size_t kBitsPerByte = 8;  ///< used below to size store_'s entrySize (must match indexRecord.cc's packing)

// ── Private helpers ──────────────────────────────────────────────────

void metaData::createNullBitsetTemplate() {
  currentEntry_.nullBitset.resize(descriptorRef_->size(), false);
  currentEntry_.recordCount = 0;
}

void metaData::flushCurrentEntry() {
  if (currentEntry_.recordCount > 0) {
    if (tail_.shouldOverwrite()) {
      store_.overwriteLast(currentEntry_);
    } else {
      store_.appendEntry(currentEntry_);
    }
    committedRecordCount_ += currentEntry_.recordCount;
    tail_.markWritten(currentEntry_.recordCount);
    currentEntry_.recordCount = 0;
  }
}

void metaData::loadIndex() {
  auto allEntries = store_.readAll();

  if (!allEntries.empty() && !allEntries.back().isGap) {
    currentEntry_ = allEntries.back();
    allEntries.pop_back();
    store_.rewrite(allEntries);
  }

  committedRecordCount_ = sumNonGapRecords(allEntries);
}

std::pair<std::optional<size_t>, size_t> metaData::locateRecord(size_t recordIndex) const {
  if (recordIndex < committedRecordCount_) {
    auto entries        = store_.readAll();
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
    : store_(std::move(metaFilePath),
             sizeof(uint8_t) + (2 * sizeof(size_t)) + ((descriptor.size() + (kBitsPerByte - 1)) / kBitsPerByte)),
      descriptorRef_(std::make_shared<Descriptor>(descriptor)) {
  createNullBitsetTemplate();
  loadIndex();
  if (!store_.empty() && !store_.fileExists()) store_.saveHeader();
}

metaData::~metaData() { flushCurrentEntry(); }

void metaData::abandonFile() { store_.abandon(); }

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
        store_.overwriteLast(part);  // replace stale on-disk entry, not append after it
      } else {
        store_.appendEntry(part);
      }
      committedRecordCount_ += part.recordCount;
      isFirst = false;
    }

    tail_.reset();
    currentEntry_ = newCur;
    return;
  }

  auto allEntries = store_.readAll();
  // Gdy ostatni wpis na dysku jest przestarzały (został wciągnięty do currentEntry_
  // mechanizmem lazy overwrite), usuń go przed rewrite — w przeciwnym razie
  // logicznie nadpisany wpis zostałby utrwalony i policzony podwójnie.
  if (tail_.shouldOverwrite() && !allEntries.empty()) allEntries.pop_back();

  auto &seg = allEntries[*segIdx];
  if (seg.nullBitset == nullBitsetParam) return;

  auto replacement = splitSegment(seg, offset, nullBitsetParam);
  allEntries.erase(allEntries.begin() + static_cast<std::ptrdiff_t>(*segIdx));
  allEntries.insert(allEntries.begin() + static_cast<std::ptrdiff_t>(*segIdx), replacement.begin(), replacement.end());

  store_.rewrite(allEntries);
  tail_.reset();
  committedRecordCount_ = sumNonGapRecords(allEntries);
}

// ── Query interface ──────────────────────────────────────────────────

std::vector<bool> metaData::getNullBitset(size_t recordIndex) const {
  auto [segIdx, offset] = locateRecord(recordIndex);
  if (!segIdx) return currentEntry_.nullBitset;
  return store_.readAll()[*segIdx].nullBitset;
}

std::vector<bool> metaData::nullBitsetFor(size_t recordIndex) const {
  if (recordIndex < totalRecords()) return getNullBitset(recordIndex);
  return std::vector<bool>(descriptorRef_->size(), false);
}

size_t metaData::totalRecords() const { return committedRecordCount_ + currentEntry_.recordCount; }

std::vector<metaData::IndexRecord> metaData::segments() const {
  auto result = store_.readAll();
  if (currentEntry_.recordCount > 0) result.push_back(currentEntry_);
  return result;
}

// ── Transmission-gap interface ───────────────────────────────────────

void metaData::onTransmissionGap(size_t gapDuration) {
  flushCurrentEntry();
  tail_.reset();

  IndexRecord gapMarker{
      .nullBitset = std::vector<bool>(descriptorRef_->size(), true), .recordCount = gapDuration, .isGap = true};
  store_.appendEntry(gapMarker);
}

bool metaData::isGapBefore(size_t recordIndex) const {
  if (recordIndex == 0) return false;

  size_t cumulative = 0;
  for (const auto &e : store_.readAll()) {
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
  if (percounter >= 0 && !store_.empty() && store_.fileExists()) {
    std::string rotatedPath = std::format("{}.old{}", store_.path(), percounter);
    std::error_code ec;
    std::filesystem::rename(store_.path(), rotatedPath, ec);
    if (ec) SPDLOG_WARN("metaData::rotate: failed to rename '{}' to '{}': {}", store_.path(), rotatedPath, ec.message());
  }
  store_.setCreationTime(std::chrono::system_clock::now());
  reset();
}

void metaData::reset() {
  createNullBitsetTemplate();
  committedRecordCount_ = 0;
  tail_.reset();
  gapDetector_.resetCounters();
  store_.saveHeader();
}

// ── Gap-detection state machine (przejęte od storage) ────────────────

void metaData::configureGapDetection(int nullFillCount) { gapDetector_.configure(nullFillCount); }

bool metaData::absorbAppend(const std::vector<bool> &nullBitset) {
  if (!gapDetector_.enabled()) return false;

  const bool isAllNull = !nullBitset.empty() && std::ranges::all_of(nullBitset, [](bool b) { return b; });
  const bool absorbed  = gapDetector_.absorb(isAllNull);
  if (!absorbed && !isAllNull) flushPendingGap();
  return absorbed;
}

void metaData::flushPendingGap() {
  if (auto gap = gapDetector_.takePendingGap(); gap > 0) onTransmissionGap(gap);
}

}  // namespace rdb
