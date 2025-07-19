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
    : filename(fileName), prevFileName(fileName), recSize(recSize), retention(retention) {
  vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fileName, recSize));
  writeCount = 0;
}

template <class T>
groupFileAccessor<T>::~groupFileAccessor() {}

template <class T>
auto groupFileAccessor<T>::name() const -> const std::string & {
  return filename;
}

template <class T>
auto groupFileAccessor<T>::name() -> std::string & {
  // setter
  spdlog::info("Setting name of groupFileAccessor from {} to {}", prevFileName, filename);
  std::filesystem::rename(prevFileName, filename);
  prevFileName = filename;
  return filename;
}

template <class T>
ssize_t groupFileAccessor<T>::write(const T *ptrData, const size_t position) {
  assert(recSize != 0);

  if (position == std::numeric_limits<size_t>::max()) writeCount++;

  if (retention.noRetention()) return vec[currentSegment - removedSegments]->write(ptrData, position);

  assert(retention.capacity != 0);

  if (writeCount > retention.capacity) {
    // rotate segments

    currentSegment++;
    spdlog::info("Rotating segments: currentSegment={}", currentSegment);

    spdlog::info("Renaming file: {} to {}_segment_{}", filename, filename, currentSegment);

    name() = filename + "_segment_" + std::to_string(currentSegment);

    vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(filename, recSize));

    writeCount = 0;

    if (retention.segments != 0 && vec.size() > retention.segments) {
      spdlog::info("Removing oldest segment: {}", vec.front()->name());
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
  } else
    return vec[currentSegment - removedSegments]->write(ptrData, position);
}

template <class T>
ssize_t groupFileAccessor<T>::read(T *ptrData, const size_t position) {
  assert(recSize != 0);
  if (retention.noRetention()) return vec[currentSegment - removedSegments]->read(ptrData, position);

  assert(retention.capacity != 0);

  auto segmentIndex      = position / retention.capacity;
  auto positionInSegment = position % retention.capacity;

  assert(segmentIndex - removedSegments >= 0 && "Segment index after removing segments is out of bounds.");

  return vec[segmentIndex - removedSegments]->read(ptrData, positionInSegment);
}

template <class T>
size_t groupFileAccessor<T>::count() {
  // return writeCount;
  size_t sumCount = 0;
  for (auto &v : vec) sumCount += v->count();
  return sumCount + removedSegments * retention.capacity;  // compensate for removed segments
}

template class groupFileAccessor<uint8_t>;
template class groupFileAccessor<char>;

}  // namespace rdb

/*
PYTHON MODEL

window = 3
silos = 2

for i in range(0,20): print(i, i% window)

*/
