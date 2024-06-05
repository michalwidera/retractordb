#include "rdb/fagrp.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

#include <iostream>

namespace rdb {

template <class T>
groupFileAccessor<T>::groupFileAccessor(const std::string &fileName,          //
                                        const std::pair<int, int> &retention  //
                                        )
    : fileNameStr(fileName), retention(retention) {
  if (retention == std::pair<int, int>{0, 0})
    vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fileName));
  else
    for (int segment = 0; segment < retention.first; segment++) {  // first == segments
      std::string fname_seq = fileName + "." + std::to_string(segment);
      vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fname_seq));
    }

  std::cerr << "fagrp:" << vec.size() << std::endl;
  std::cerr << "fagrp:" << fileName << std::endl;
}

template <class T>
groupFileAccessor<T>::~groupFileAccessor() {}

template <class T>
std::string groupFileAccessor<T>::fileName() {
  return fileNameStr;
}

template <class T>
ssize_t groupFileAccessor<T>::write(const T *ptrData, const size_t size, const size_t position) {
  if (retention == std::pair<int, int>{0, 0})
    return vec[0]->write(ptrData, size, position);
  else {
    // Need to cover this with test - DOUBlECHECK
    assert(retention.second != 0);
    assert(retention.first != 0);
    if (position == std::numeric_limits<size_t>::max()) {
      // append procedure
      auto newVecIdx1 = (writeCount / retention.second) % retention.first;
      writeCount++;
      auto newVecIdx2 = (writeCount / retention.second) % retention.first;
      if (newVecIdx1 != newVecIdx2) vec[newVecIdx2]->write(nullptr, 0, 0);  // delete all data
      return vec[newVecIdx2]->write(ptrData, size, position);
    } else {
      // write at position procedure
      auto newPosition = position % retention.second;  // second == capacity
      auto newVecIdx   = (position / retention.second) % retention.first;
      return vec[newVecIdx]->write(ptrData, size, newPosition);
    }
  }
}

template <class T>
ssize_t groupFileAccessor<T>::read(T *ptrData, const size_t size, const size_t position) {
  if (retention == std::pair<int, int>{0, 0})
    return vec[0]->read(ptrData, size, position);
  else {
    // Need to cover this with test - DOUBlECHECK
    assert(retention.second != 0);
    assert(retention.first != 0);
    auto newPosition = position % retention.second;  // second == capacity
    auto newVecIdx   = (position / retention.second) % retention.first;
    return vec[newVecIdx]->read(ptrData, size, newPosition);
  }
}

template <class T>
size_t groupFileAccessor<T>::count() {
  return 0;
}

template class groupFileAccessor<uint8_t>;
template class groupFileAccessor<char>;

}  // namespace rdb
