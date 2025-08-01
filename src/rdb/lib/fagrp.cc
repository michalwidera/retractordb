#include "rdb/fagrp.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/json.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

namespace rdb {

std::ifstream::pos_type filesize(const std::string &filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

// fagrp.h -> typedef std::pair<segments_t, capacity_t> retention_t;

template <class T>
groupFileAccessor<T>::groupFileAccessor(const std::string &fileName,   //
                                        const size_t recSize,          //
                                        const retention_t &retention)  //
    : filename(fileName), recSize(recSize), retention(retention) {
  writeCount      = 0;
  currentFilename = filename + "_segment_" + std::to_string(currentSegment);
  if (retention.noRetention()) {
    vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(name(), recSize));
  } else {
    auto min = std::numeric_limits<size_t>::max();
    auto max = std::numeric_limits<size_t>::min();
    for (const auto &entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
      auto filename = entry.path().filename().string();
      std::smatch sm;
      if (regex_match(filename, sm, std::regex(fileName + "_segment_([0-9]+)"))) {
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
    removedSegments = min;
    for (auto i = min; i <= max; ++i) {
      currentSegment  = i;
      currentFilename = filename + "_segment_" + std::to_string(currentSegment);
      vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(name(), recSize));
      SPDLOG_INFO("Adding existing segment: {}", name());
      writeCount = vec.back()->count();
    }
  }
}

template <class T>
groupFileAccessor<T>::~groupFileAccessor() {}

template <class T>
auto groupFileAccessor<T>::name() const -> const std::string & {
  if (retention.noRetention()) return filename;
  return currentFilename;
}

template <class T>
auto groupFileAccessor<T>::name() -> std::string & {
  if (retention.noRetention()) {
    // pottenially consider change filename storage file here.
    return filename;
  }
  return currentFilename;
}

template <class T>
ssize_t groupFileAccessor<T>::write(const T *ptrData, const size_t position) {
  assert(recSize != 0);

  if (position == std::numeric_limits<size_t>::max()) writeCount++;

  if (retention.noRetention()) return vec[0]->write(ptrData, position);

  assert(retention.capacity != 0);

  // Purge operation
  if (ptrData == nullptr && position == 0) {
    for (auto &v : vec) std::filesystem::remove(v->name());
    vec.clear();

    // Reset state
    writeCount      = 0;
    currentSegment  = 0;
    currentFilename = filename + "_segment_" + std::to_string(currentSegment);
    vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(name(), recSize));
    removedSegments = 0;
    spdlog::info("Purged all segments, current segment is now 0.");
    assert(vec.size() == 1 && "After purge, there should be only one segment left.");
    assert(vec[0]->count() == 0 && "After purge, the segment should be empty.");
    spdlog::info("Purged all segments, current segment is now 0.");
    return 0;
  }

  if (writeCount > retention.capacity) {
    // rotate segments

    currentSegment++;
    currentFilename = filename + "_segment_" + std::to_string(currentSegment);

    spdlog::info("Rotating segments: currentSegment={}", currentSegment);

    vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(name(), recSize));

    writeCount = 0;

    if (retention.segments != 0 && vec.size() > retention.segments) {
      auto segmentToRemove = vec.front()->name();
      spdlog::info("Removing oldest segment: {}", segmentToRemove);
      std::filesystem::remove(segmentToRemove);
      vec.erase(vec.begin());
      removedSegments++;
      assert(vec.size() > 0);
    }
  }

  assert(currentSegment - removedSegments >= 0 && "Segment index after removing segments is out of bounds.");

  if (position != std::numeric_limits<size_t>::max()) {
    auto segmentIndex      = position / retention.capacity;
    auto positionInSegment = position % retention.capacity;

    assert(segmentIndex < vec.size() && "Segment index out of bounds. Check retention settings and position.");
    assert(segmentIndex - removedSegments >= 0 && "Segment index after removing segments is out of bounds.");

    return vec[segmentIndex - removedSegments]->write(ptrData, positionInSegment);
  } else {
    return vec[currentSegment - removedSegments]->write(ptrData, position);
  }
}

template <class T>
ssize_t groupFileAccessor<T>::read(T *ptrData, const size_t position) {
  assert(recSize != 0);
  if (retention.noRetention()) return vec[0]->read(ptrData, position);

  assert(retention.capacity != 0);

  auto segmentIndex      = position / retention.capacity;
  auto positionInSegment = position % retention.capacity;

  assert(segmentIndex - removedSegments >= 0 && "Segment index after removing segments is out of bounds.");
  return vec[segmentIndex - removedSegments]->read(ptrData, positionInSegment);
}

template <class T>
size_t groupFileAccessor<T>::count() {
  size_t sumCount = 0;
  for (auto &v : vec) sumCount += v->count();
  return sumCount + removedSegments * retention.capacity;  // compensate for removed segments
}

template class groupFileAccessor<uint8_t>;
template class groupFileAccessor<char>;

}  // namespace rdb
