#include "rdb/storageacc.h"

#include <algorithm>
#include <cassert>
#include <cstring>  //std::memset
#include <filesystem>
#include <fstream>
#include <iostream>

#include "spdlog/spdlog.h"

namespace rdb {

bool isOpen(const storageState val) { return (val == storageState::openExisting || val == storageState::openAndCreate); };

storageAccessor::storageAccessor(std::string fileName)
    : filename(fileName),                         //
      removeOnExit(true),                         //
      recordsCount(0),                            //
      payloadPtr(nullptr),                        //
      dataFileStatus(storageState::noDescriptor)  //
{}

void storageAccessor::attachStorage() {
  auto resourceAlreadyExist = std::filesystem::exists(filename);

  accessor = std::make_unique<rdb::posixPrmBinaryFileAccessor<std::byte>>(filename);

  if (resourceAlreadyExist) {
    std::ifstream in(filename.c_str(), std::ifstream::ate | std::ifstream::binary);
    if (in.good()) recordsCount = int(in.tellg() / descriptor.getSizeInBytes());
    SPDLOG_INFO("record count {} on {}", recordsCount, filename);
    dataFileStatus = storageState::openExisting;
    return;
  }

  SPDLOG_INFO("Storage created.", recordsCount, filename);
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

void storageAccessor::attachDescriptor(const Descriptor* descriptorParam) {
  auto descriptorFile(filename + ".desc");

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

    auto it = std::find_if(descriptor.begin(),
                           descriptor.end(),                                             //
                           [](auto& item) { return std::get<rtype>(item) == rdb::REF; }  //
    );

    if (it != descriptor.end()) {
      filename = std::get<rname>(*it);
      SPDLOG_INFO("Storage ref from descriptor {}", filename);
    }

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

  SPDLOG_INFO("Descriptor created.");
}

storageAccessor::~storageAccessor() {
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

Descriptor& storageAccessor::getDescriptor() { return descriptor; }

bool storageAccessor::peekDescriptor() {
  auto descriptorFile(filename + ".desc");
  return std::filesystem::exists(descriptorFile);
}

void storageAccessor::setReverse(bool value) { reverse = value; }

void storageAccessor::setRemoveOnExit(bool value) { removeOnExit = value; }

const size_t storageAccessor::getRecordsCount() { return recordsCount; }

bool storageAccessor::read(const size_t recordIndex) {
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

bool storageAccessor::write(const size_t recordIndex) {
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

bool storageAccessor::peekStorage() {  //
  return dataFileStatus == storageState::openExisting;
}

}  // namespace rdb