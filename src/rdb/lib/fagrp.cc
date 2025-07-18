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
    : filename(fileName), recSize(recSize), retention(retention) {
  
  accessor = std::make_unique<posixBinaryFileAccessor<T>>(fileName, recSize);
  writeCount = 0;
}

template <class T>
groupFileAccessor<T>::~groupFileAccessor() {}

template <class T>
std::string groupFileAccessor<T>::fileName() {
  return filename;
}

template <class T>
ssize_t groupFileAccessor<T>::write(const T *ptrData, const size_t position) {
  assert(recSize != 0);

  if (position == std::numeric_limits<size_t>::max())
    writeCount++;

  if (retention.noRetention())
    return accessor->write(ptrData, position);

  assert(retention.capacity != 0);
  assert(retention.segments != 0);

  if (writeCount > retention.capacity)
  {
    // rotate segments

    currentSegment = (currentSegment + 1) % retention.segments;
    spdlog::info("Rotating segments: currentSegment={}", currentSegment);

    spdlog::info("Renaming file: {} to {}_segment_{}", filename, filename, currentSegment);

    std::filesystem::rename(
        filename,
        filename + "_segment_" + std::to_string(currentSegment)
        );

    accessor.reset();  // Close the current accessor

    std::unique_ptr<posixBinaryFileAccessor<T>> accessorNew = std::make_unique<posixBinaryFileAccessor<T>>(filename, recSize);

    accessor.swap(accessorNew);

    writeCount     = 0;
  }
  return accessor->write(ptrData, position);
}

template <class T>
ssize_t groupFileAccessor<T>::read(T *ptrData, const size_t position) {
  assert(recSize != 0);
  if (retention.noRetention())
    return accessor->read(ptrData, position);

  assert(retention.capacity != 0);
  assert(retention.segments != 0);

  return accessor->read(ptrData, position);
}

template <class T>
size_t groupFileAccessor<T>::count() {
  return writeCount;
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
