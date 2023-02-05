#include "rdb/storageacc.h"

#include <cassert>
#include <cstring>  //std::memset
#include <filesystem>
#include <fstream>
#include <iostream>

#include "spdlog/spdlog.h"

namespace rdb {

bool isOpen(const storageState val) { return (val == storageState::openExisting || val == storageState::openAndCreate); };

template <class K>
storageAccessor<K>::storageAccessor(std::string fileName)
    : filename(fileName),   //
      removeOnExit(true),   //
      recordsCount(0),      //
      payloadPtr(nullptr),  //
      dataFileStatus(storageState::noDescriptor) {
  auto storageExistsBefore = std::filesystem::exists(fileName);
  accessor = std::make_unique<K>(fileName);
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

  if (storageExistsBefore) {
    dataFileStatus = storageState::openExisting;
    SPDLOG_INFO("construct: Success, Descriptor&Storage [openExisting]");
    return;
  }

  dataFileStatus = storageState::openAndCreate;
  SPDLOG_INFO("construct: Success, Descriptor&Storage [openAndCreate]");
}

template <class K>
void storageAccessor<K>::attachPayloadPtr(std::byte* payloadPtrVal) {
  SPDLOG_INFO("required: PayloadPtr [attached]");
  payloadPtr = payloadPtrVal;
}

template <class K>
void storageAccessor<K>::attachPayload(rdb::payload& payloadRef) {
  SPDLOG_INFO("required: Payload [attached]");
  payloadPtr = payloadRef.get();
}

template <class K>
void storageAccessor<K>::createDescriptor(const Descriptor descriptorParam) {
  if (isOpen(dataFileStatus)) {
    if (descriptorParam == descriptor) {
      SPDLOG_INFO("createDescriptor: descriptor param match - ok, pass.");
      return;
    }
    abort();  // data file already opend and have attached different descriptor
  }
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
    SPDLOG_ERROR("createDescriptor: descriptor found - but struct empty, done.");
    return;
  }

  std::ifstream in(filename.c_str(), std::ifstream::ate | std::ifstream::binary);
  if (in.good()) recordsCount = int(in.tellg() / descriptor.getSizeInBytes());

  dataFileStatus = storageState::openAndCreate;

  SPDLOG_INFO("createDescriptor: Success, Descriptor&Storage [openAndCreate]");
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
  if (!isOpen(dataFileStatus)) abort();  // data file is not opened
  if (payloadPtr == nullptr) abort();    // no payload attached
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
  if (!isOpen(dataFileStatus)) abort();  // data file is not opened
  if (payloadPtr == nullptr) abort();    // no payload attached
  auto size = descriptor.getSizeInBytes();
  auto result = 0;
  if (recordIndex >= recordsCount) {
    result = accessor->write(payloadPtr, size);  // <- Call to append Function
    assert(result == 0);
    if (result == 0) recordsCount++;
    SPDLOG_INFO("append");
    return result == 0;
  }

  if (recordsCount > 0 && recordIndex < recordsCount) {
    auto recordIndexRv = reverse ? (recordsCount - 1) - recordIndex : recordIndex;
    result = accessor->write(payloadPtr, size, recordIndexRv * size);
    assert(result == 0);
    SPDLOG_INFO("write {}", recordIndexRv);
  }
  return result == 0;
};

template <class K>
bool storageAccessor<K>::storageAlreadyExisting() {
  return dataFileStatus == storageState::openExisting;
}

template class storageAccessor<rdb::genericBinaryFileAccessor<std::byte>>;
template class storageAccessor<rdb::posixBinaryFileAccessor<std::byte>>;
template class storageAccessor<rdb::posixPrmBinaryFileAccessor<std::byte>>;
}  // namespace rdb