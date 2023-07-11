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
    if (peekDescriptor()) remove(descriptorFile.c_str());
    SPDLOG_INFO("drop storage, drop descriptor");
  }
  if (removeDescriptorOnExit) {
    if (peekDescriptor()) remove(descriptorFile.c_str());
    SPDLOG_INFO("drop descriptor");
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

bool storageAccessor::peekDescriptor() { return std::filesystem::exists(descriptorFile); }

void storageAccessor::setReverse(bool value) { reverse = value; }

void storageAccessor::setRemoveOnExit(bool value) { removeOnExit = value; }

void storageAccessor::setRemoveDescriptorOnExit(bool value) { removeDescriptorOnExit = value; }

const size_t storageAccessor::getRecordsCount() { return recordsCount; }

std::string storageAccessor::getStorageName() { return storageFile; }

bool storageAccessor::read(const size_t recordIndex, uint8_t* destination) {
  if (descriptor.isEmpty()) {
    SPDLOG_ERROR("descriptor is Empty");
    abort();
  }
  if (!isOpen(dataFileStatus)) {
    SPDLOG_ERROR("store is not open");
    abort();  // data file is not opened
  }
  if (!storagePayload) {
    SPDLOG_ERROR("no payload attached");
    abort();  // no payload attached
  }
  destination = (destination == nullptr)                            //
                    ? static_cast<uint8_t*>(storagePayload->get())  //
                    : destination;                                  //
  assert(destination != nullptr);
  auto size = descriptor.getSizeInBytes();
  auto result = 0;

  auto recordIndexRv{0};
  if (storageType != "DEVICE") recordIndexRv = reverse ? (recordsCount - 1) - recordIndex : recordIndex;

  if (recordsCount > 0 && recordIndexRv < recordsCount) {
    result = accessor->read(destination, size, recordIndexRv * size);
    assert(result == 0);
    SPDLOG_INFO("read fn from pos:{} limit:{}", recordIndexRv, recordsCount);
  } else {
    std::memset(destination, 0, size);
    SPDLOG_WARN("read fn - non existing data from pos:{} limit:{}", recordIndexRv, recordsCount);
  }
  return result == 0;
}

bool storageAccessor::readReverse(const size_t recordIndex, uint8_t* destination) {
  auto reversePrevValue = reverse;
  reverse = true;
  auto return_value = read(recordIndex, destination);
  reverse = reversePrevValue;
  return return_value;
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
  if (!storagePayload) {
    SPDLOG_ERROR("no payload attached");
    abort();  // no payload attached
  }
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
    auto recordIndexRv = reverse ? (recordsCount - 1) - recordIndex : recordIndex;
    result = accessor->write(static_cast<uint8_t*>(storagePayload->get()), size, recordIndexRv * size);
    assert(result == 0);
    SPDLOG_INFO("write {}", recordIndexRv);
  }
  return result == 0;
};

bool storageAccessor::peekStorage() {  //
  return dataFileStatus == storageState::openExisting;
}

}  // namespace rdb