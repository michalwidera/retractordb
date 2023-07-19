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
  SPDLOG_INFO("descriptorFile-> {} | storageFile-> {}", descriptorFile, fileName);
}

void storageAccessor::attachDescriptor(const Descriptor* descriptorParam) {
  if (descriptorFileExist()) {
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
    storagePayload = std::make_unique<rdb::payload>(descriptor);

    SPDLOG_INFO("Payload created, Descriptor from file used.");

    attachStorage();
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
  storagePayload = std::make_unique<rdb::payload>(descriptor);

  SPDLOG_INFO("Payload & Descriptor created.");
  attachStorage();
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

  // if storageAccessor object was created with default storage as ""
  // and there is no specified storage as REF in descriptor - we should
  // stop immediately.
  if (storageFile == "") {
    SPDLOG_ERROR("Storage file was not set in descriptor.");
    abort();
  }
}

void storageAccessor::attachStorage() {
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
    accessor = std::make_unique<rdb::posixPrmBinaryFileAccessor<uint8_t>>(storageFile);
  } else if (storageType == "GENERIC") {
    accessor = std::make_unique<rdb::genericBinaryFileAccessor<uint8_t>>(storageFile);
  } else if (storageType == "POSIX") {
    accessor = std::make_unique<rdb::posixBinaryFileAccessor<uint8_t>>(storageFile);
  } else if (storageType == "DEVICE") {
    accessor = std::make_unique<rdb::binaryDeviceAccessor<uint8_t>>(storageFile);
  } else if (storageType == "TEXTSOURCE") {
    accessor = std::make_unique<rdb::textSourceAccessor<uint8_t>>(storageFile);
    accessor->fctrl(&descriptor, 0);
  } else {
    SPDLOG_INFO("Unsupported storage type {}", storageType);
    abort();
  }

  if (isDeclared()) {
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

storageAccessor::~storageAccessor() {
  if (removeOnExit) {
    if (storageFile != "") auto statusRemove1 = remove(storageFile.c_str());
    if (descriptorFileExist()) remove(descriptorFile.c_str());
    SPDLOG_INFO("drop storage, drop descriptor");
  }
}

bool storageAccessor::isDeclared() { return (storageType == "DEVICE") || (storageType == "TEXTSOURCE"); }

void storageAccessor::reset() {
  assert(storageFile != "");

  if (!accessor) return;  // no accessor initialized - no need to reset.

  auto resourceAlreadyExist = std::filesystem::exists(storageFile);
  if (resourceAlreadyExist)
    if (!isDeclared()) remove(storageFile.c_str());

  if (storageType == "DEFAULT") {
    accessor = std::make_unique<rdb::posixPrmBinaryFileAccessor<uint8_t>>(storageFile);
  } else if (storageType == "GENERIC") {
    accessor = std::make_unique<rdb::genericBinaryFileAccessor<uint8_t>>(storageFile);
  } else if (storageType == "POSIX") {
    accessor = std::make_unique<rdb::posixBinaryFileAccessor<uint8_t>>(storageFile);
  } else if (storageType == "DEVICE") {
    accessor = std::make_unique<rdb::binaryDeviceAccessor<uint8_t>>(storageFile);
  } else if (storageType == "TEXTSOURCE") {
    accessor = std::make_unique<rdb::textSourceAccessor<uint8_t>>(storageFile);
    accessor->fctrl(&descriptor, 0);
  } else {
    SPDLOG_INFO("Unsupported storage type {}", storageType);
    abort();
  }

  if (isDeclared())
    recordsCount = 1;
  else
    recordsCount = 0;
  SPDLOG_INFO("reset - drop & recreate storage.");
}

Descriptor& storageAccessor::getDescriptor() { return descriptor; }

std::unique_ptr<rdb::payload>::pointer storageAccessor::getPayload() {
  if (!storagePayload) {
    SPDLOG_ERROR("no payload attached");
    abort();  // no payload attached
  }
  return storagePayload.get();
}

bool storageAccessor::descriptorFileExist() { return std::filesystem::exists(descriptorFile); }

void storageAccessor::setRemoveOnExit(bool value) { removeOnExit = value; }

const size_t storageAccessor::getRecordsCount() {
  if (isDeclared())
    return circularBuffer.size() == 0 ? 1 : circularBuffer.size();
  else
    return recordsCount;
}

std::string storageAccessor::getStorageName() { return storageFile; }

void storageAccessor::abortIfStorageNotPrepared() {
  if (descriptor.isEmpty()) {
    SPDLOG_ERROR("descriptor is Empty");
    abort();
  }
  if (!isOpen(dataFileStatus)) {
    SPDLOG_ERROR("data file is not opened");
    abort();
  }
  if (!storagePayload) {
    SPDLOG_ERROR("no payload attached");
    abort();
  }
}

bool storageAccessor::read(const size_t recordIndex, uint8_t* destination) {
  abortIfStorageNotPrepared();
  destination = (destination == nullptr)                            //
                    ? static_cast<uint8_t*>(storagePayload->get())  //
                    : destination;

  assert(destination != nullptr);
  auto size = descriptor.getSizeInBytes();
  auto result = 0;

  auto recordIndexRv{0};
  if (!isDeclared()) recordIndexRv = recordIndex;  // In case of device type - is works only on first record.

  if (recordsCount > 0 && recordIndexRv < recordsCount) {
    result = accessor->read(destination, size, recordIndexRv * size);
    if (circularBuffer.capacity() > 0) circularBuffer.push_front(*storagePayload.get());
    assert(result == 0);
    SPDLOG_INFO("read fn {} from pos:{} limit:{}", accessor->fileName(), recordIndexRv, recordsCount);
  } else {
    std::memset(destination, 0, size);
    SPDLOG_WARN("read fn {} - non existing data from pos:{} limit:{}", accessor->fileName(), recordIndexRv, recordsCount);
  }
  return result == 0;
}

bool storageAccessor::readReverse(const size_t recordIndex, uint8_t* destination) {
  if (!isDeclared()) return read(getRecordsCount() - recordIndex - 1, destination);

  if (circularBuffer.capacity() == 0) return read(0, destination);
  if (recordIndex == 0 && bufferPolicy == policyState::flux) return read(0, destination);

  assert(circularBuffer.capacity() > 0);
  assert(recordIndex >= 0);

  // Read data from Circular Buffer instead of data source

  assert((recordIndex <= circularBuffer.capacity()) && "Stop if we are accessing over Circular Buffer Size.");
  assert((recordIndex <= circularBuffer.size()) && "Stop if we have not enough elements in buffer (? - zeros?)");

  destination = (destination == nullptr)                            //
                    ? static_cast<uint8_t*>(storagePayload->get())  //
                    : destination;
  assert(destination != nullptr);
  auto size = descriptor.getSizeInBytes();

  *(storagePayload.get()) = circularBuffer[recordIndex];
  return true;
}

void storageAccessor::setCapacity(const int capacity) {
  assert(isDeclared());
  circularBuffer.set_capacity(capacity);
}

bool storageAccessor::write(const size_t recordIndex) {
  abortIfStorageNotPrepared();

  auto size = descriptor.getSizeInBytes();
  auto result = 0;
  if (recordIndex >= recordsCount) {
    result = accessor->write(static_cast<uint8_t*>(storagePayload->get()),
                             size);  // <- Call to append Function
    assert(result == 0);
    if (result == 0) recordsCount++;
    SPDLOG_INFO("append");
    return result == 0;
  }

  if (recordsCount > 0 && recordIndex < recordsCount) {
    result = accessor->write(static_cast<uint8_t*>(storagePayload->get()), size, recordIndex * size);
    assert(result == 0);
    SPDLOG_INFO("write {}", recordIndex);
  }
  return result == 0;
};

}  // namespace rdb