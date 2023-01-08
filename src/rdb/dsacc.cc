#include "rdb/dsacc.h"

#include <cassert>
#include <cstring>  //std::memset
#include <filesystem>
#include <fstream>
#include <iostream>

namespace rdb {
template <class K>
DataStorageAccessor<K>::DataStorageAccessor(std::string fileName)
    : accessor(new K(fileName)),  //
      filename(fileName),         //
      removeOnExit(true),         //
      recordsCount(0),            //
      dataFileStatus(noData) {
  std::fstream myFile;
  myFile.rdbuf()->pubsetbuf(0, 0);
  std::string fileDesc(accessor->fileName());
  fileDesc.append(".desc");
  dataFileStatus = noDescriptor;
  // --
  if (std::filesystem::exists(fileDesc)) {
    myFile.open(fileDesc, std::ios::in);
    if (myFile.good()) myFile >> descriptor;
    myFile.close();
    if (descriptor.getSizeInBytes() > 0) {
      std::ifstream in(fileName.c_str(), std::ifstream::ate | std::ifstream::binary);
      if (in.good()) recordsCount = int(in.tellg() / descriptor.getSizeInBytes());
      payload.reset(new std::byte[descriptor.getSizeInBytes()]);
      memset(payload.get(), 0, descriptor.getSizeInBytes());
    }
    dataFileStatus = open;
  }
}

template <class K>
void DataStorageAccessor<K>::createDescriptor(const Descriptor descriptorParam) {
  if (dataFileStatus == open) abort();  // data file already opend and have attached descriptor
  descriptor = descriptorParam;
  std::fstream descFile;
  descFile.rdbuf()->pubsetbuf(0, 0);
  std::string fileDesc(accessor->fileName());
  fileDesc.append(".desc");
  // Creating .desc file
  descFile.open(fileDesc, std::ios::out);
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile << descriptor;
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile.close();
  if (descriptor.getSizeInBytes() > 0) {
    std::ifstream in(filename.c_str(), std::ifstream::ate | std::ifstream::binary);
    if (in.good()) recordsCount = int(in.tellg() / descriptor.getSizeInBytes());
    payload.reset(new std::byte[descriptor.getSizeInBytes()]);
    memset(payload.get(), 0, descriptor.getSizeInBytes());
  }
  dataFileStatus = open;
}

template <class K>
DataStorageAccessor<K>::~DataStorageAccessor() {
  if (removeOnExit) {
    // Constructor & Destructor does not fail - need to reconsider remove this
    // asserts or make this in another way.
    auto statusRemove1 = remove(filename.c_str());
    // assert(statusRemove1 == 0);
    auto descFilename(filename + ".desc");
    auto statusRemove2 = remove(descFilename.c_str());
    // assert(statusRemove2 == 0);
  }
}

template <class K>
Descriptor& DataStorageAccessor<K>::getDescriptor() {
  return descriptor;
}

template <class K>
void DataStorageAccessor<K>::setReverse(bool value) {
  reverse = value;
}

template <class K>
void DataStorageAccessor<K>::setRemoveOnExit(bool value) {
  removeOnExit = value;
}

template <class K>
const size_t DataStorageAccessor<K>::getRecordsCount() {
  return recordsCount;
}

template <class K>
bool DataStorageAccessor<K>::read(const size_t recordIndex) {
  if (descriptor.isDirty()) abort();
  if (dataFileStatus != open) abort();  // data file is not opened
  auto size = descriptor.getSizeInBytes();
  int result = 0;
  auto recordIndexRv = reverse ? (recordsCount - 1) - recordIndex : recordIndex;
  if (recordsCount > 0 && recordIndex < recordsCount) {
    result = accessor->read(payload.get(), size, recordIndexRv * size);
    assert(result == 0);
  }
  return result == 0;
}

template <class K>
bool DataStorageAccessor<K>::write(const size_t recordIndex) {
  if (descriptor.isDirty()) abort();
  if (dataFileStatus != open) abort();  // data file is not opened
  auto size = descriptor.getSizeInBytes();
  int result = 0;
  if (recordIndex == std::numeric_limits<size_t>::max()) {
    result = accessor->write(payload.get(), size);  // <- Call to append Function
    assert(result == 0);
    if (result == 0) recordsCount++;
  } else {
    if (recordsCount > 0 && recordIndex < recordsCount) {
      auto recordIndexRv = reverse ? (recordsCount - 1) - recordIndex : recordIndex;
      result = accessor->write(payload.get(), size, recordIndexRv * size);
      assert(result == 0);
    }
  }
  return result == 0;
};

template <class K>
const std::string DataStorageAccessor<K>::getStorageFilename() {
  return filename;
}

template class DataStorageAccessor<rdb::genericBinaryFileAccessor<std::byte>>;
template class DataStorageAccessor<rdb::posixBinaryFileAccessor<std::byte>>;
template class DataStorageAccessor<rdb::posixPrmBinaryFileAccessor<std::byte>>;
}  // namespace rdb