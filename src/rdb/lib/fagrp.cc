#include "rdb/fagrp.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <regex>

namespace rdb {

std::ifstream::pos_type filesize(const std::string &filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

// fagrp.h -> typedef std::pair<segments_t, capacity_t> retention_t;

groupFile::groupFile(const std::string_view fileName,  //
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
    vec_.push_back(std::make_unique<posixBinaryFileWithShadow>(name(), recordSize_, percounter_));
  } else {
    auto min = std::numeric_limits<size_t>::max();
    auto max = std::numeric_limits<size_t>::min();
    for (const auto &entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
      auto filenameEx = entry.path().filename().string();
      std::smatch sm;
      if (regex_match(filenameEx, sm, std::regex(filename_ + "_segment_([0-9]+)"))) {
        auto val = atoi(sm[1].str().c_str());
        if (val < min) min = val;
        if (val > max) max = val;
      }
    }
    if (max == std::numeric_limits<size_t>::min() && min == std::numeric_limits<size_t>::max()) {
      // No existing segments found, start from 0 - only desc file may be found
      min = 0;
      max = 0;
    }
    removedSegments_ = min;
    for (auto i = min; i <= max; ++i) {
      currentSegment_  = i;
      currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);
      vec_.push_back(std::make_unique<posixBinaryFileWithShadow>(name(), recordSize_, percounter_));
      SPDLOG_INFO("Adding existing segment: {}", name());
      writeCount_ = vec_.back()->count();
    }
  }
}

groupFile::~groupFile() {}

auto groupFile::name() -> std::string & {
  if (retention_.noRetention()) {
    // pottenially consider change filename storage file here.
    return filename_;
  }
  return currentFilename_;
}

ssize_t groupFile::write(const uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);

  if (position == std::numeric_limits<size_t>::max()) writeCount_++;

  if (retention_.noRetention()) return vec_[0]->write(ptrData, position);

  assert(retention_.capacity != 0);

  // Purge operation
  if (ptrData == nullptr && position == 0) {
    for (auto &v : vec_) {
      std::filesystem::remove(v->name());
      std::filesystem::remove(v->name() + ".shadow");
    }
    vec_.clear();

    // Reset state
    writeCount_      = 0;
    currentSegment_  = 0;
    currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);
    vec_.push_back(std::make_unique<posixBinaryFileWithShadow>(name(), recordSize_, percounter_));
    removedSegments_ = 0;
    spdlog::info("Purged all segments, current segment is now 0.");
    assert(vec_.size() == 1 && "After purge, there should be only one segment left.");
    assert(vec_[0]->count() == 0 && "After purge, the segment should be empty.");
    spdlog::info("Purged all segments, current segment is now 0.");
    return 0;
  }

  if (writeCount_ > retention_.capacity) {
    // rotate segments

    currentSegment_++;
    currentFilename_ = filename_ + "_segment_" + std::to_string(currentSegment_);

    spdlog::info("Rotating segments: currentSegment={}", currentSegment_);

    vec_.push_back(std::make_unique<posixBinaryFileWithShadow>(name(), recordSize_, percounter_));

    writeCount_ = 0;

    if (retention_.segments != 0 && vec_.size() > retention_.segments) {
      auto segmentToRemove = vec_.front()->name();
      spdlog::info("Removing oldest segment: {}", segmentToRemove);
      std::filesystem::remove(segmentToRemove);
      std::filesystem::remove(segmentToRemove + ".shadow");
      vec_.erase(vec_.begin());
      removedSegments_++;
      assert(vec_.size() > 0);
    }
  }

  assert(currentSegment_ - removedSegments_ >= 0 && "Segment index after removing segments is out of bounds.");

  if (position != std::numeric_limits<size_t>::max()) {
    auto segmentIndex      = position / retention_.capacity;
    auto positionInSegment = position % retention_.capacity;

    assert(segmentIndex < vec_.size() && "Segment index out of bounds. Check retention settings and position.");
    assert(segmentIndex - removedSegments_ >= 0 && "Segment index after removing segments is out of bounds.");

    return vec_[segmentIndex - removedSegments_]->write(ptrData, positionInSegment);
  } else {
    return vec_[currentSegment_ - removedSegments_]->write(ptrData, position);
  }
}

ssize_t groupFile::read(uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);
  if (retention_.noRetention()) return vec_[0]->read(ptrData, position);

  assert(retention_.capacity != 0);

  auto segmentIndex      = position / retention_.capacity;
  auto positionInSegment = position % retention_.capacity;

  assert(segmentIndex - removedSegments_ >= 0 && "Segment index after removing segments is out of bounds.");
  return vec_[segmentIndex - removedSegments_]->read(ptrData, positionInSegment);
}

size_t groupFile::count() {
  size_t sumCount = 0;
  for (auto &v : vec_)
    sumCount += v->count();
  return sumCount + removedSegments_ * retention_.capacity;  // compensate for removed segments
}

}  // namespace rdb
