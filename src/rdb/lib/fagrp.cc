#include "rdb/fagrp.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>

namespace rdb {

namespace {

auto parseSegmentIndex(const std::string &candidate, const std::string &baseName) -> std::optional<size_t> {
  const std::string prefix = baseName + "_segment_";
  if (!candidate.starts_with(prefix)) return std::nullopt;

  const std::string suffix = candidate.substr(prefix.size());
  if (suffix.empty()) return std::nullopt;
  if (!std::all_of(suffix.begin(), suffix.end(), ::isdigit)) return std::nullopt;

  return static_cast<size_t>(std::strtoull(suffix.c_str(), nullptr, 10));
}

}  // namespace

std::ifstream::pos_type filesize(const std::string &filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

// fagrp.hpp -> typedef std::pair<segments_t, capacity_t> retention_t;
template <typename T>
groupFile<T>::groupFile(const std::string_view fileName,  //
                        const ssize_t recordSize,         //
                        const retention_t &retention,     //
                        int percounter)                   //
    : filename_(std::string(fileName)),
      recordSize_(recordSize),
      retention_(retention),
      percounter_(percounter) {
  writeCount_      = 0;
  currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);

  if (retention.noRetention()) {
    vec_.push_back(std::make_unique<T>(name(), recordSize_, percounter_));
  } else {
    std::vector<size_t> existingSegments;
    existingSegments.reserve(retention_.segments == 0 ? 8 : retention_.segments);

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

    std::sort(existingSegments.begin(), existingSegments.end());
    existingSegments.erase(std::unique(existingSegments.begin(), existingSegments.end()), existingSegments.end());

    // Restore only a contiguous suffix ending at the latest segment. This guarantees
    // global-position mapping remains valid after restart even if older files are missing.
    auto firstContiguousIdx = existingSegments.size() - 1;
    while (firstContiguousIdx > 0) {
      if (existingSegments[firstContiguousIdx - 1] + 1 != existingSegments[firstContiguousIdx]) {
        break;
      }
      --firstContiguousIdx;
    }

    std::vector<size_t> restoredSegments(existingSegments.begin() + firstContiguousIdx, existingSegments.end());

    if (retention_.segments != 0 && restoredSegments.size() > retention_.segments) {
      restoredSegments.erase(restoredSegments.begin(),
                             restoredSegments.begin() + (restoredSegments.size() - retention_.segments));
    }

    removedSegments_ = restoredSegments.front();
    for (const auto i : restoredSegments) {
      currentSegment_  = i;
      currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);
      vec_.push_back(std::make_unique<T>(name(), recordSize_, percounter_));
      SPDLOG_INFO("Adding existing segment: {}", name());
      writeCount_ = vec_.back()->count();
    }
  }
}
template <typename T>
groupFile<T>::~groupFile() {}

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
  vec_.push_back(std::make_unique<T>(name(), recordSize_, percounter_));

  spdlog::info("Purged all segments and reset group state.");
  assert(vec_.size() == 1 && "After purge, there should be only one segment left.");
  assert(vec_[0]->count() == 0 && "After purge, the segment should be empty.");

  return EXIT_SUCCESS;
}

template <typename T>
ssize_t groupFile<T>::write(const uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);

  // Keep backward compatibility: special write request triggers full purge.
  if (ptrData == nullptr && position == 0) {
    return purge();
  }

  if (retention_.noRetention()) return vec_[0]->write(ptrData, position);

  assert(retention_.capacity != 0);

  if (position == std::numeric_limits<size_t>::max()) {
    if (writeCount_ >= retention_.capacity) {
      // rotate segments exactly at capacity boundary
      currentSegment_++;
      currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);

      spdlog::info("Rotating segments: currentSegment={}", currentSegment_);

      vec_.push_back(std::make_unique<T>(name(), recordSize_, percounter_));
      writeCount_ = 0;

      if (retention_.segments != 0 && vec_.size() > retention_.segments) {
        auto segmentToRemove = vec_.front()->name();
        spdlog::info("Removing oldest segment: {}", segmentToRemove);
        std::filesystem::remove(segmentToRemove);
        if (std::filesystem::exists(segmentToRemove + ".shadow"))
          std::filesystem::remove(segmentToRemove + ".shadow");
        vec_.erase(vec_.begin());
        removedSegments_++;
        assert(vec_.size() > 0);
      }
    }

    const auto rc = vec_[currentSegment_ - removedSegments_]->write(ptrData, position);
    if (rc == EXIT_SUCCESS) {
      writeCount_++;
    }
    return rc;
  }

  auto segmentIndex      = position / retention_.capacity;
  auto positionInSegment = position % retention_.capacity;

  if (segmentIndex < removedSegments_) {
    return EXIT_FAILURE;
  }

  const auto localSegmentIndex = segmentIndex - removedSegments_;
  if (localSegmentIndex >= vec_.size()) {
    return EXIT_FAILURE;
  }

  return vec_[localSegmentIndex]->write(ptrData, positionInSegment);
}

template <typename T>
ssize_t groupFile<T>::read(uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);
  if (retention_.noRetention()) return vec_[0]->read(ptrData, position);

  assert(retention_.capacity != 0);

  auto segmentIndex      = position / retention_.capacity;
  auto positionInSegment = position % retention_.capacity;

  if (segmentIndex < removedSegments_) {
    return EXIT_FAILURE;
  }

  const auto localSegmentIndex = segmentIndex - removedSegments_;
  if (localSegmentIndex >= vec_.size()) {
    return EXIT_FAILURE;
  }

  return vec_[localSegmentIndex]->read(ptrData, positionInSegment);
}

template <typename T>
size_t groupFile<T>::count() {
  size_t sumCount = 0;
  for (auto &v : vec_)
    sumCount += v->count();
  return sumCount + removedSegments_ * retention_.capacity;  // compensate for removed segments
}

}  // namespace rdb
