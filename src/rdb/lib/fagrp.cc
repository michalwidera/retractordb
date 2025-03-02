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

ccc::ccc(const std::string &fileName) : fileName(fileName) {}

void ccc::write(const size_t writeCountParam) {
  writeCount = writeCountParam;
  boost::json::object obj;
  obj["writeCount"] = writeCount;
  std::ofstream output(fileName + ".ccc");
  if (!output.is_open()) {
    throw std::runtime_error("Failed to open file for writing: " + fileName + ".ccc");
  }
  output << boost::json::serialize(obj) << std::endl;
  initialized = true;
}

size_t ccc::read() {
  if (!initialized) {
    initialized = true;

    auto fileExists = std::filesystem::exists(fileName + ".ccc");
    if (fileExists) {
      std::ifstream input(fileName + ".ccc");
      std::ostringstream buffer;
      buffer << input.rdbuf();
      boost::json::value json_data = boost::json::parse(buffer.str());
      writeCount                   = json_data.as_object().at("writeCount").as_int64();
    }
  }

  return writeCount;
};

std::ifstream::pos_type filesize(const std::string &filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

template <class T>
groupFileAccessor<T>::groupFileAccessor(const std::string &fileName,           //
                                        const size_t recSize,                  //
                                        const std::pair<int, int> &retention)  //
    : filename(fileName), recSize(recSize), retention(retention), cccFile(fileName) {
  if (retention == std::pair<int, int>{0, 0})
    vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fileName, recSize));
  else
    for (int segment = 0; segment < retention.second; segment++) {
      std::string fname_seq = fileName + "." + std::to_string(segment);
      vec.push_back(std::make_unique<posixBinaryFileAccessor<T>>(fname_seq, recSize));
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
  assert(recSize != 0);
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
      auto newPosition = (position / recSize) % retention.first;
      auto newVecIdx   = int((position / recSize) / (retention.first)) % (retention.second);
      // std::cerr << "W->[" << newVecIdx << "][" << newPosition << "]" << std::endl ;
      return vec[newVecIdx]->write(ptrData, newPosition);
    }
  }
  return 0;
}

template <class T>
ssize_t groupFileAccessor<T>::read(T *ptrData, const size_t position) {
  assert(recSize != 0);
  if (retention == std::pair<int, int>{0, 0})
    return vec[0]->read(ptrData, position);
  else {
    // Need to cover this with test - DOUBlECHECK
    assert(retention.second != 0);
    assert(retention.first != 0);
    auto newPosition = (position / recSize) % retention.first;
    auto newVecIdx   = int((position / recSize) / (retention.first)) % (retention.second);
    // std::cerr << "R<-[" << newVecIdx << "][" << newPosition << "]" << std::endl ;
    return vec[newVecIdx]->read(ptrData, newPosition * recSize);
  }
  return 0;  // proforma
}

template <class T>
size_t groupFileAccessor<T>::count() {
  size_t writeCount{cccFile.read()};

  for (auto &v : vec) writeCount += v->count();

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
