#include "rdb/fagrp.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <iostream>

namespace rdb {

template <class T>
groupFileAccessor<T>::groupFileAccessor(const std::string &fileName,           //
                                        const size_t size,                     //
                                        const std::pair<int, int> &retention)  //
    : filename(fileName), size(size), retention(retention) {
  if (retention == std::pair<int, int>{0, 0})
    vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fileName, size));
  else
    for (int segment = 0; segment < retention.second; segment++) {
      std::string fname_seq = fileName + "." + std::to_string(segment);
      vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fname_seq, size));
    }

  writeCount = count();
}

template <class T>
groupFileAccessor<T>::~groupFileAccessor() {}

template <class T>
std::string groupFileAccessor<T>::fileName() {
  return filename;
}

template <class T>
ssize_t groupFileAccessor<T>::write(const T *ptrData, const size_t position) {
  assert(size != 0);
  if (retention == std::pair<int, int>{0, 0})
    return vec[0]->write(ptrData, position);
  else {
    // Need to cover this with test - DOUBlECHECK
    assert(retention.second != 0);
    assert(retention.first != 0);
    if (position == std::numeric_limits<size_t>::max()) {
      // append procedure
      auto newVecIdx1 = (writeCount / retention.second) % retention.first;
      writeCount++;
      auto newVecIdx2 = (writeCount / retention.second) % retention.first;
      if (newVecIdx1 != newVecIdx2) vec[newVecIdx2]->write(nullptr, 0);  // delete all data from one
      return vec[newVecIdx2]->write(ptrData, position);
    } else if (position == 0 && ptrData == nullptr) {
      for (auto &v : vec) v->write(nullptr, 0);  // purge all
    } else {
      // write at position procedure
      auto newPosition = (position / size) % retention.first;
      auto newVecIdx   = int((position / size) / (retention.first)) % (retention.second);
      // std::cerr << "W->[" << newVecIdx << "][" << newPosition << "]" << std::endl ;
      return vec[newVecIdx]->write(ptrData, newPosition);
    }
  }
  return 0;
}

template <class T>
ssize_t groupFileAccessor<T>::read(T *ptrData, const size_t position) {
  assert(size != 0);
  if (retention == std::pair<int, int>{0, 0})
    return vec[0]->read(ptrData, position);
  else {
    // Need to cover this with test - DOUBlECHECK
    assert(retention.second != 0);
    assert(retention.first != 0);
    auto newPosition = (position / size) % retention.first;
    auto newVecIdx   = int((position / size) / (retention.first)) % (retention.second);
    // std::cerr << "R<-[" << newVecIdx << "][" << newPosition << "]" << std::endl ;
    return vec[newVecIdx]->read(ptrData, newPosition * size);
  }
  return 0;  // proforma
}

template <class T>
size_t groupFileAccessor<T>::count() {
  size_t sumCount{0};
  for (auto &v : vec) sumCount += v->count();
  return sumCount;
}

template class groupFileAccessor<uint8_t>;
template class groupFileAccessor<char>;

}  // namespace rdb
