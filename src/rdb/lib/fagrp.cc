#include "rdb/fagrp.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ranges>
#include <string_view>
#include "fatalError.hpp"

namespace rdb {

namespace {

constexpr int kDecimalBase              = 10;
constexpr size_t kDefaultSegmentReserve = 8;

auto parseSegmentIndex(const std::string &candidate, const std::string &baseName) -> std::optional<size_t> {
  const std::string prefix = baseName + "_segment_";
  if (!candidate.starts_with(prefix)) return std::nullopt;

  const std::string suffix = candidate.substr(prefix.size());
  if (suffix.empty()) return std::nullopt;
  if (!std::ranges::all_of(suffix, ::isdigit)) return std::nullopt;

  return static_cast<size_t>(std::strtoull(suffix.c_str(), nullptr, kDecimalBase));
}

}  // namespace

std::ifstream::pos_type filesize(const std::string &filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

// fagrp.hpp -> typedef std::pair<segments_t, capacity_t> retention_t;
template <typename T>
groupFile<T>::groupFile(const std::string_view fileName,  //
                        const Descriptor &descriptor,     //
                        const retention_t &retention,     //
                        int percounter)                   //
    : filename_(std::string(fileName)),
      descriptor_(descriptor),
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),
      retention_(retention),
      percounter_(percounter) {
  writeCount_      = 0;
  currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);

  if (retention.noRetention()) {
    vec_.push_back(std::make_unique<T>(name(), descriptor_, percounter_));
  } else {
    std::vector<size_t> existingSegments;
    existingSegments.reserve(retention_.segments == 0 ? kDefaultSegmentReserve : retention_.segments);

    for (const auto &entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
      const auto filenameEx = entry.path().filename().string();
      if (const auto segIdx = parseSegmentIndex(filenameEx, filename_); segIdx.has_value()) {
        existingSegments.push_back(*segIdx);
      }
    }

    if (existingSegments.empty()) {
      // No existing segments found, start from 0 - only desc file may be found
      existingSegments.push_back(0);
    }

    std::ranges::sort(existingSegments);
    existingSegments.erase(std::ranges::unique(existingSegments).begin(), existingSegments.end());

    // Restore only a contiguous suffix ending at the latest segment. This guarantees
    // global-position mapping remains valid after restart even if older files are missing.
    auto firstContiguousIdx = existingSegments.size() - 1;
    while (firstContiguousIdx > 0) {
      if (existingSegments[firstContiguousIdx - 1] + 1 != existingSegments[firstContiguousIdx]) {
        break;
      }
      --firstContiguousIdx;
    }

    std::vector<size_t> restoredSegments(existingSegments.begin() + static_cast<std::ptrdiff_t>(firstContiguousIdx),
                                         existingSegments.end());

    if (retention_.segments != 0 && restoredSegments.size() > retention_.segments) {
      restoredSegments.erase(restoredSegments.begin(),
                             restoredSegments.begin() + (restoredSegments.size() - retention_.segments));
    }

    removedSegments_ = restoredSegments.front();
    for (const auto i : restoredSegments) {
      currentSegment_  = i;
      currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);
      vec_.push_back(std::make_unique<T>(name(), descriptor_, percounter_));
      writeCount_ = vec_.back()->count();
    }
  }
}
template <typename T>
groupFile<T>::~groupFile() = default;

template <typename T>
auto groupFile<T>::name() -> std::string & {
  if (retention_.noRetention()) {
    // pottenially consider change filename storage file here.
    return filename_;
  }
  return currentFilename_;
}

template <typename T>
ssize_t groupFile<T>::purge() {
  for (auto &v : vec_) {
    v->write(nullptr, 0);  // purge files using special write command - this deltes the files
  }
  vec_.clear();

  // Reset state
  writeCount_      = 0;
  currentSegment_  = 0;
  removedSegments_ = 0;
  currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);
  vec_.push_back(std::make_unique<T>(name(), descriptor_, percounter_));

  SPDLOG_DEBUG("Purged all segments and reset group state.");
  if (vec_.size() != 1) FatalError("fagrp::purge: expected exactly one segment after purge");
  if (vec_[0]->count() != 0) FatalError("fagrp::purge: segment is not empty after purge");

  return EXIT_SUCCESS;
}

template <typename T>
ssize_t groupFile<T>::write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) FatalError("groupFile::write: recordSize_ is zero");

  if (ptrData == nullptr && position == 0) {
    return purge();
  }

  if (retention_.noRetention()) return static_cast<FileInterface *>(vec_[0].get())->write(ptrData, nullBitset, position);

  if (retention_.capacity == 0) FatalError("groupFile::write: retention capacity is zero");

  if (position == std::numeric_limits<size_t>::max()) {
    if (writeCount_ >= retention_.capacity) {
      currentSegment_++;
      currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);
      SPDLOG_DEBUG("Rotating segments: currentSegment={}", currentSegment_);
      vec_.push_back(std::make_unique<T>(name(), descriptor_, percounter_));
      writeCount_ = 0;
      if (retention_.segments != 0 && vec_.size() > retention_.segments) {
        auto segmentToRemove = vec_.front()->name();
        SPDLOG_DEBUG("Removing oldest segment: {}", segmentToRemove);
        std::filesystem::remove(segmentToRemove);
        if (std::filesystem::exists(segmentToRemove + ".shadow")) std::filesystem::remove(segmentToRemove + ".shadow");
        vec_.erase(vec_.begin());
        removedSegments_++;
        if (vec_.empty()) FatalError("groupFile::write: no segments remain after removing oldest");
      }
    }
    const auto rc =
        static_cast<FileInterface *>(vec_[currentSegment_ - removedSegments_].get())->write(ptrData, nullBitset, position);
    if (rc == EXIT_SUCCESS) {
      writeCount_++;
    }
    return rc;
  }

  auto segmentIndex      = position / retention_.capacity;
  auto positionInSegment = position % retention_.capacity;

  if (segmentIndex < removedSegments_) return EXIT_FAILURE;

  const auto localSegmentIndex = segmentIndex - removedSegments_;
  if (localSegmentIndex >= vec_.size()) return EXIT_FAILURE;

  return static_cast<FileInterface *>(vec_[localSegmentIndex].get())->write(ptrData, nullBitset, positionInSegment);
}

template <typename T>
ssize_t groupFile<T>::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) FatalError("groupFile::read: recordSize_ is zero");
  if (retention_.noRetention()) return static_cast<FileInterface *>(vec_[0].get())->read(ptrData, nullBitset, position);

  if (retention_.capacity == 0) FatalError("groupFile::read: retention capacity is zero");

  auto segmentIndex      = position / retention_.capacity;
  auto positionInSegment = position % retention_.capacity;

  if (segmentIndex < removedSegments_) return EXIT_FAILURE;

  const auto localSegmentIndex = segmentIndex - removedSegments_;
  if (localSegmentIndex >= vec_.size()) return EXIT_FAILURE;

  return static_cast<FileInterface *>(vec_[localSegmentIndex].get())->read(ptrData, nullBitset, positionInSegment);
}

template <typename T>
size_t groupFile<T>::count() {
  size_t sumCount = 0;
  for (auto &v : vec_)
    sumCount += v->count();
  return sumCount + (removedSegments_ * retention_.capacity);  // compensate for removed segments
}

}  // namespace rdb
