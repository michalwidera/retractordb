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

storageAccessor::storageAccessor(std::string fileNameDesc, const std::string fileName)
    :  //
      descriptorFile(fileNameDesc + ".desc"),
      storageFile(fileName) {
  SPDLOG_INFO("descriptorFile-> {} | storageFile-> {}", descriptorFile, fileName);
}

void storageAccessor::attachDescriptor(const Descriptor* descriptorParam) {
  if (descriptorFileExist()) {
    std::fstream myFile;
    myFile.rdbuf()->pubsetbuf(nullptr, 0);
    myFile.open(descriptorFile, std::ios::in);  // Open existing descriptor
    if (myFile.good()) myFile >> descriptor;
    myFile.close();

    if (descriptor.getSizeInBytes() == 0) {
      SPDLOG_ERROR("Empty descriptor in file.");
      assert(false && "Empty descriptor in file.");
      abort();
    }

    if (descriptorParam != nullptr && *descriptorParam != descriptor) {
      SPDLOG_ERROR("Descriptors do not match.");
      assert(false && "Descriptors dont match - previous one have different schema? remove&restart.");
      abort();
    }

    moveRef();
    storagePayload = std::make_unique<rdb::payload>(descriptor);
    chamber = std::make_unique<rdb::payload>(descriptor);

    SPDLOG_INFO("Payload created, Descriptor from file used.");

    attachStorage();
    return;
  }

  if (descriptorParam == nullptr) {
    SPDLOG_ERROR("No descriptor file found, no descriptor provided.");
    assert(false && "No descriptor found");
    abort();
  }

  descriptor = *descriptorParam;

  // Create descriptor file instance
  std::fstream descFile;
  descFile.rdbuf()->pubsetbuf(nullptr, 0);
  descFile.open(descriptorFile, std::ios::out);
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile << descriptor;
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile.close();

  moveRef();
  storagePayload = std::make_unique<rdb::payload>(descriptor);
  chamber = std::make_unique<rdb::payload>(descriptor);

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
    assert(false && "Storage file was not set in descriptor.");
    abort();
  }
}

void storageAccessor::attachStorage() {
  assert(storageFile != "");

  auto resourceAlreadyExist = std::filesystem::exists(storageFile);

  auto it = std::find_if(descriptor.begin(),
                         descriptor.end(),                                              //
                         [](auto& item) { return std::get<rtype>(item) == rdb::TYPE; }  //
  );

  if (it != descriptor.end()) {
    storageType = std::get<rname>(*it);
    SPDLOG_INFO("Storage type from descriptor {}", storageType);
  }

  initializeAccessor();

  if (isDeclared()) {
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

void storageAccessor::initializeAccessor() {
  assert(storageFile != "");
  assert(storageType != "");

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
    assert(false && "Unsupported storage type");
    abort();
  }
}

void storageAccessor::reset() {
  assert(storageFile != "");

  if (!accessor) return;  // no accessor initialized - no need to reset.

  auto resourceAlreadyExist = std::filesystem::exists(storageFile);
  if (resourceAlreadyExist)
    if (!isDeclared()) remove(storageFile.c_str());

  initializeAccessor();

  recordsCount = 0;
  SPDLOG_INFO("reset - drop & recreate storage.");
}

void storageAccessor::cleanPayload(uint8_t* destination) {
  destination = (destination == nullptr)                            //
                    ? static_cast<uint8_t*>(storagePayload->get())  //
                    : destination;
  auto size = descriptor.getSizeInBytes();
  std::memset(destination, 0, size);
}

Descriptor& storageAccessor::getDescriptor() { return descriptor; }

std::unique_ptr<rdb::payload>::pointer storageAccessor::getPayload() {
  if (!storagePayload) {
    SPDLOG_ERROR("no payload attached");
    assert(false && "no payload attached");
    abort();
  }
  return storagePayload.get();
}

bool storageAccessor::descriptorFileExist() { return std::filesystem::exists(descriptorFile); }

void storageAccessor::setRemoveOnExit(bool value) { removeOnExit = value; }

const size_t storageAccessor::getRecordsCount() { return recordsCount; }

std::string storageAccessor::getStorageName() { return storageFile; }

void storageAccessor::abortIfStorageNotPrepared() {
  if (descriptor.isEmpty()) {
    SPDLOG_ERROR("descriptor is Empty");
    assert(false && "Empty descriptor");
    abort();
  }
  if (!isOpen(dataFileStatus)) {
    SPDLOG_ERROR("data file is not opened");
    assert(false && "data file didn't opened");
    abort();
  }
  if (!storagePayload) {
    SPDLOG_ERROR("no payload attached");
    assert(false && "payload not attached");
    abort();
  }
}

void storageAccessor::fire() {
  assert(circularBuffer.capacity() > 0);
  *storagePayload = *chamber;
  circularBuffer.push_front(*storagePayload.get());  // only one place when buffer is feed.
  bufferState = sourceState::lock;
}

bool storageAccessor::read_() {
  assert(isDeclared());
  uint8_t* destination = static_cast<uint8_t*>(chamber->get());
  recordsCount++;

  auto size = descriptor.getSizeInBytes();
  auto result = accessor->read(destination, size, 0);
  return result == 0;
}

bool storageAccessor::read_(const size_t recordIndex, uint8_t* destination) {
  assert(!isDeclared());
  abortIfStorageNotPrepared();

  if (destination == nullptr) {
    destination = static_cast<uint8_t*>(storagePayload->get());
  }

  assert(destination != nullptr);
  auto size = descriptor.getSizeInBytes();
  auto result = 0;

  auto recordIndexRv{0};

  recordIndexRv = recordIndex;

  if (recordsCount > 0 && recordIndexRv < recordsCount) {
    result = accessor->read(destination, size, recordIndexRv * size);
    assert(result == 0);
    SPDLOG_INFO("read from file {} pos:{} rec-count:{}", accessor->fileName(), recordIndexRv, recordsCount);
  } else {
    std::memset(destination, 0, size);
    SPDLOG_WARN("read fake {} - non existing data from pos:{} rec-count:{}", accessor->fileName(), recordIndexRv, recordsCount);
  }
  return result == 0;
}

bool storageAccessor::revRead(const size_t recordIndex, uint8_t* destination) {
  const auto recordPositionFromBack = getRecordsCount() - recordIndex - 1;

  if (!isDeclared()) return read_(recordPositionFromBack, destination);

  // For all _DECLARED_ data sources buffer capacity at least _MUST_ be 1
  // In order to maintain the consistency of declared data sources,
  // it is necessary to maintain a buffer of at least 1

  assert(circularBuffer.capacity() > 0);
  assert(isDeclared());

  if (recordIndex == 0 && bufferState == sourceState::flux) {
    //
    // THIS IS ONLY ONE PLACE WHERE DATA ARE READ FROM SOURCE
    //
    auto result = read_();
    assert(result && "Failure during read.");
    bufferState = sourceState::armed;
  }
  assert(recordIndex >= 0);

  // Read data from Circular Buffer instead of data source
  // - only for declared data sources
  // - only for data sources that have buffer declared
  // - only for recordIndex > 0 if sourceState::flux
  // - also for recordIndex == 0 if sourceState::lock

  assert((recordIndex < circularBuffer.capacity()) && "Stop if we are accessing over Circular Buffer Size.");

  // in case of accessing buffer that has no data yet - zeros are returned

  if (recordIndex >= circularBuffer.size()) {
    destination = (destination == nullptr)                            //
                      ? static_cast<uint8_t*>(storagePayload->get())  //
                      : destination;

    assert(destination != nullptr);
    auto size = descriptor.getSizeInBytes();
    std::memset(destination, 0, size);
    SPDLOG_WARN("read buffer fn {} - non existing data from pos:{} capacity:{}", accessor->fileName(), recordIndex,
                circularBuffer.capacity());
    return true;
  }

  assert((recordIndex < circularBuffer.size()) && "Stop if we have not enough elements in buffer (? - zeros?)");

  *(storagePayload.get()) = circularBuffer[recordIndex];
  return true;
}

void storageAccessor::setCapacity(const int capacity) {
  if (isDeclared()) circularBuffer.set_capacity(capacity == 0 ? 1 : capacity);
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