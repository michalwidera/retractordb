#include "rdb/fagrp.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
namespace rdb {

template <class T>
groupFileAccessor<T>::groupFileAccessor(const std::string &fileName,          //
                                        const std::pair<int, int> &retention  //
                                        )
    : fileNameStr(fileName), retention(retention) {
  if (retention == std::pair<int,int>{0,0})
    vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fileName));
  else
    for (int segment = 0 ; segment < retention.first ; segment ++ ) {   // first == segments
      std::string fname_seq = fileName + "." + std::to_string(segment);
      vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fname_seq));
    }
}

template <class T>
groupFileAccessor<T>::~groupFileAccessor() {}

template <class T>
std::string groupFileAccessor<T>::fileName() {
  return fileNameStr;
}

template <class T>
ssize_t groupFileAccessor<T>::write(const T *ptrData, const size_t size, const size_t position) {
  if (retention == std::pair<int,int>{0,0})
    return vec[0]->write(ptrData, size, position);
  else
    ; // TODO
}

template <class T>
ssize_t groupFileAccessor<T>::read(T *ptrData, const size_t size, const size_t position) {
  if (retention == std::pair<int,int>{0,0})
    return vec[0]->read(ptrData, size, position);
  else
    ; // TODO
}

template class groupFileAccessor<uint8_t>;
template class groupFileAccessor<char>;

}  // namespace rdb
