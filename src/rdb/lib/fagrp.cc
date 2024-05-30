#include "rdb/fagrp.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
namespace rdb {

template <class T>
groupFileAccessor<T>::groupFileAccessor(const std::string &fileName,       //
                                        const rdb::Descriptor &descriptor  //
                                        )
    : fileNameStr(fileName), descriptor(descriptor) {
  vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fileName));
}

template <class T>
groupFileAccessor<T>::~groupFileAccessor() {}

template <class T>
std::string groupFileAccessor<T>::fileName() {
  return fileNameStr;
}

template <class T>
ssize_t groupFileAccessor<T>::write(const T *ptrData, const size_t size, const size_t position) {
  return vec[0]->write(ptrData, size, position);
}

template <class T>
ssize_t groupFileAccessor<T>::read(T *ptrData, const size_t size, const size_t position) {
  return vec[0]->read(ptrData, size, position);
}

template class groupFileAccessor<uint8_t>;
template class groupFileAccessor<char>;

}  // namespace rdb
