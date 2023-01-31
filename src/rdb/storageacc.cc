#include "rdb/storageacc.h"

#include <cassert>
#include <cstring>  //std::memset
#include <filesystem>
#include <fstream>
#include <iostream>

#include "spdlog/spdlog.h"

namespace rdb {
template <class K>
storageAccessor<K>::storageAccessor(std::string fileName)
    : accessor(new K(fileName)),  //
      filename(fileName),         //
      removeOnExit(true),         //
      recordsCount(0),            //
      payloadPtr(nullptr),        //
      dataFileStatus(noDescriptor) {
  std::fstream myFile;
  myFile.rdbuf()->pubsetbuf(0, 0);
  auto fileDesc(accessor->fileName().append(".desc"));
  // --
  if (!std::filesystem::exists(fileDesc)) {
    SPDLOG_WARN("construct: no descriptor found - expect fn createDescriptor, done.");
    return;
  }

  myFile.open(fileDesc, std::ios::in);  // Open existing descriptor
  if (myFile.good()) myFile >> descriptor;
  myFile.close();

  if (descriptor.getSizeInBytes() == 0) {
    SPDLOG_ERROR("construct: descriptor found - but struct empty, done.");
    return;
  }

  std::ifstream in(fileName.c_str(), std::ifstream::ate | std::ifstream::binary);
  if (in.good()) recordsCount = int(in.tellg() / descriptor.getSizeInBytes());

  dataFileStatus = open;

  SPDLOG_INFO("construct: Success, Descriptor&Storage [open]");
}

template <class K>
void storageAccessor<K>::attachPayloadPtr(std::byte *payloadPtrVal) {
  SPDLOG_INFO("construct: Payload [attached]");
  payloadPtr = payloadPtrVal;
}

template <class K>
void storageAccessor<K>::createDescriptor(const Descriptor descriptorParam) {
  if (dataFileStatus == open) abort();  // data file already opend and have attached descriptor
  descriptor = descriptorParam;
  std::fstream descFile;
  descFile.rdbuf()->pubsetbuf(0, 0);
  auto fileDesc(accessor->fileName().append(".desc"));
  // Creating .desc file
  descFile.open(fileDesc, std::ios::out);
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile << descriptor;
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile.close();

  if (descriptor.getSizeInBytes() == 0) {
    SPDLOG_ERROR("construct: descriptor found - but struct empty, done.");
    return;
  }

  std::ifstream in(filename.c_str(), std::ifstream::ate | std::ifstream::binary);
  if (in.good()) recordsCount = int(in.tellg() / descriptor.getSizeInBytes());

  dataFileStatus = open;

  SPDLOG_INFO("fn createDescriptor: Success, Descriptor&Storage [open]");
}

template <class K>
storageAccessor<K>::~storageAccessor() {
  if (removeOnExit) {
    // Constructor & Destructor does not fail - need to reconsider remove this
    // asserts or make this in another way.
    auto statusRemove1 = remove(filename.c_str());
    // assert(statusRemove1 == 0);
    auto descFilename(filename + ".desc");
    auto statusRemove2 = remove(descFilename.c_str());
    // assert(statusRemove2 == 0);
    SPDLOG_INFO("drop storage");
  }
}

template <class K>
Descriptor& storageAccessor<K>::getDescriptor() {
  return descriptor;
}

template <class K>
void storageAccessor<K>::setReverse(bool value) {
  reverse = value;
}

template <class K>
void storageAccessor<K>::setRemoveOnExit(bool value) {
  removeOnExit = value;
}

template <class K>
const size_t storageAccessor<K>::getRecordsCount() {
  return recordsCount;
}

template <class K>
bool storageAccessor<K>::read(const size_t recordIndex) {
  if (descriptor.isDirty()) abort();
  if (dataFileStatus != open) abort();  // data file is not opened
  if (payloadPtr == nullptr) abort();  // no payload attached
  auto size = descriptor.getSizeInBytes();
  auto result = 0;
  auto recordIndexRv = reverse ? (recordsCount - 1) - recordIndex : recordIndex;
  if (recordsCount > 0 && recordIndex < recordsCount) {
    result = accessor->read(payloadPtr, size, recordIndexRv * size);
    assert(result == 0);
    SPDLOG_INFO("read {}", recordIndexRv);
  }
  return result == 0;
}

template <class K>
bool storageAccessor<K>::write(const size_t recordIndex) {
  if (descriptor.isDirty()) abort();
  if (dataFileStatus != open) abort();  // data file is not opened
  if (payloadPtr == nullptr) abort();  // no payload attached
  auto size = descriptor.getSizeInBytes();
  auto result = 0;
  if (recordIndex >= recordsCount) {
    result = accessor->write(payloadPtr, size);  // <- Call to append Function
    assert(result == 0);
    if (result == 0) recordsCount++;
    SPDLOG_INFO("append");
  } else {
    if (recordsCount > 0 && recordIndex < recordsCount) {
      auto recordIndexRv = reverse ? (recordsCount - 1) - recordIndex : recordIndex;
      result = accessor->write(payloadPtr, size, recordIndexRv * size);
      assert(result == 0);
      SPDLOG_INFO("write {}", recordIndexRv);
    }
  }
  return result == 0;
};

template <class K>
const std::string storageAccessor<K>::getStorageFilename() {
  return filename;
}

template class storageAccessor<rdb::genericBinaryFileAccessor<std::byte>>;
template class storageAccessor<rdb::posixBinaryFileAccessor<std::byte>>;
template class storageAccessor<rdb::posixPrmBinaryFileAccessor<std::byte>>;
}  // namespace rdb