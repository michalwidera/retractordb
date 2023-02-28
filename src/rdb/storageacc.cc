#include "rdb/storageacc.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstring>  //std::memset
#include <filesystem>
#include <fstream>
#include <iostream>

namespace rdb {

bool isOpen(const storageState val) { return (val == storageState::openExisting || val == storageState::openAndCreate); };

storageAccessor::storageAccessor(std::string fileNameDesc, std::string fileName)
    :  //
      descriptorFile(fileNameDesc + ".desc"),
      storageFile(fileName) {
  SPDLOG_INFO("descriptorFile -> {} | storageFile-> {}", descriptorFile, storageFile);
}

void storageAccessor::attachDescriptor(const Descriptor* descriptorParam) {
  if (peekDescriptor()) {
    std::fstream myFile;
    myFile.rdbuf()->pubsetbuf(0, 0);
    myFile.open(descriptorFile, std::ios::in);  // Open existing descriptor
    if (myFile.good()) myFile >> descriptor;
    myFile.close();

    if (descriptor.getSizeInBytes() == 0) {
      SPDLOG_ERROR("Empty descriptor in file.");
      abort();
    }

    if (descriptorParam != nullptr && *descriptorParam != descriptor) {
      SPDLOG_ERROR("Descriptors not match.");
      abort();
    }

    moveRef();

    SPDLOG_INFO("Descriptor from file used.");
    return;
  }

  if (descriptorParam == nullptr) {
    SPDLOG_ERROR("No descriptor file found, no descriptor provided.");
    abort();
  }

  descriptor = *descriptorParam;

  // Create descriptor file instance
  std::fstream descFile;
  descFile.rdbuf()->pubsetbuf(0, 0);
  descFile.open(descriptorFile, std::ios::out);
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile << descriptor;
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile.close();

  moveRef();

  SPDLOG_INFO("Descriptor created.");
}

void storageAccessor::moveRef() {
  auto it = std::find_if(descriptor.begin(),
                         descriptor.end(),                                             //
                         [](auto& item) { return std::get<rtype>(item) == rdb::REF; }  //
  );

  // Descriptor changes storageFile location
  if (it != descriptor.end()) {
    storageFile = std::get<rname>(*it);
    SPDLOG_INFO("Storage ref from descriptor changed to {}", storageFile);
  }
}

void storageAccessor::attachStorage() {
  // assert(descriptor.len() != 0);

  auto resourceAlreadyExist = std::filesystem::exists(storageFile);

  auto it = std::find_if(descriptor.begin(),
                         descriptor.end(),                                              //
                         [](auto& item) { return std::get<rtype>(item) == rdb::TYPE; }  //
  );

  if (it != descriptor.end()) {
    storageType = std::get<rname>(*it);
    SPDLOG_INFO("Storage type from descriptor {}", storageType);
  }

  if (storageType == "DEFAULT") {
    accessor = std::make_unique<rdb::posixPrmBinaryFileAccessor<std::byte>>(storageFile);
  } else if (storageType == "GENERIC") {
    accessor = std::make_unique<rdb::genericBinaryFileAccessor<std::byte>>(storageFile);
  } else if (storageType == "POSIX") {
    accessor = std::make_unique<rdb::posixBinaryFileAccessor<std::byte>>(storageFile);
  } else if (storageType == "DEVICE") {
    accessor = std::make_unique<rdb::binaryDeviceAccessor<std::byte>>(storageFile);
  } else if (storageType == "TEXTSOURCE") {
    accessor = std::make_unique<rdb::textSrouceAccessor<std::byte>>(storageFile);
    accessor->fctrl(&descriptor, 0);
  } else {
    SPDLOG_INFO("Unsupported storage type {}", storageType);
    abort();
  }

  bool readOnlyStorage = (storageType == "DEVICE") || (storageType == "TEXTSOURCE");

  if (readOnlyStorage) {
    recordsCount = 1;
    SPDLOG_INFO("records source on {}", storageFile);
    dataFileStatus = storageState::openExisting;
    return;
  }

  if (resourceAlreadyExist) {
    std::ifstream in(storageFile.c_str(), std::ifstream::ate | std::ifstream::binary);
    if (in.good()) recordsCount = int(in.tellg() / descriptor.getSizeInBytes());
    SPDLOG_INFO("record count {} on {}", recordsCount, storageFile);
    dataFileStatus = storageState::openExisting;
    return;
  }

  SPDLOG_INFO("Storage created.", recordsCount, storageFile);
  dataFileStatus = storageState::openAndCreate;
}

void storageAccessor::attachPayloadPtr(std::byte* payloadPtrVal) {
  SPDLOG_INFO("required: PayloadPtr [attached]");
  payloadPtr = payloadPtrVal;
}

void storageAccessor::attachPayload(rdb::payload& payloadRef) {
  SPDLOG_INFO("required: Payload [attached]");
  payloadPtr = payloadRef.get();
}

storageAccessor::~storageAccessor() {
  if (removeOnExit) {
    // Constructor & Destructor does not fail - need to reconsider remove this
    // asserts or make this in another way.
    auto statusRemove1 = remove(storageFile.c_str());
    // assert(statusRemove1 == 0);
    auto statusRemove2 = remove(descriptorFile.c_str());
    // assert(statusRemove2 == 0);
    SPDLOG_INFO("drop storage");
  }
}

Descriptor& storageAccessor::getDescriptor() { return descriptor; }

bool storageAccessor::peekDescriptor() { return std::filesystem::exists(descriptorFile); }

void storageAccessor::setReverse(bool value) { reverse = value; }

void storageAccessor::setRemoveOnExit(bool value) { removeOnExit = value; }

const size_t storageAccessor::getRecordsCount() { return recordsCount; }

bool storageAccessor::read(const size_t recordIndex) {
  if (descriptor.isEmpty()) {
    SPDLOG_ERROR("descriptor is Empty");
    abort();
  }
  if (!isOpen(dataFileStatus)) {
    SPDLOG_ERROR("store is not open");
    abort();  // data file is not opened
  }
  if (payloadPtr == nullptr) {
    SPDLOG_ERROR("no payload attached");
    abort();  // no payload attached
  }
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

bool storageAccessor::write(const size_t recordIndex) {
  if (descriptor.isEmpty()) {
    SPDLOG_ERROR("descriptor is Empty");
    abort();
  }
  if (!isOpen(dataFileStatus)) {
    SPDLOG_ERROR("store is not open");
    abort();  // data file is not opened
  }
  if (payloadPtr == nullptr) {
    SPDLOG_ERROR("no payload attached");
    abort();  // no payload attached
  }
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

bool storageAccessor::peekStorage() {  //
  return dataFileStatus == storageState::openExisting;
}

}  // namespace rdb