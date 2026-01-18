#include "rdb/storageacc.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstring>  //std::memset
#include <filesystem>
#include <fstream>
#include <iostream>

namespace rdb {

bool isOpen(const storageState val) { return (val == storageState::openAndCreate); };

storageAccessor::storageAccessor(const std::string qryID,              //
                                 const std::string fileName,           //
                                 const std::string_view storageParam,  //
                                 bool oneShot,                         //
                                 bool isHold,                          //
                                 int percounter)
    : descriptorFile_(qryID + ".desc"), storageFile_(fileName), percounter_(percounter), isOneShot_(oneShot), isHold_(isHold) {
  assert(!qryID.empty());
  assert(!fileName.empty());

  if (isHold_) {
    SPDLOG_INFO("Stream file '{}' created in HOLD state", storageFile_);
  }

  if (storageParam.empty()) {
    SPDLOG_INFO("That's OK. Storage path is empty - no change of storageFile");
    return;  // no change
  }

  if (!std::filesystem::is_directory(storageParam)) {
    SPDLOG_ERROR("Storage path {} does not point to directory.", storageParam);
    assert(false && "Storage path does not point to directory.");
    abort();
  }

  SPDLOG_INFO("Storage path before {}", storageFile_);
  SPDLOG_INFO("Descriptor path before {}", descriptorFile_);

  descriptorFile_ = std::filesystem::path(storageParam) / std::filesystem::path(descriptorFile_);
  storageFile_    = std::filesystem::path(storageParam) / std::filesystem::path(storageFile_);
  SPDLOG_INFO("Storage path changed to {}", storageFile_);
  SPDLOG_INFO("Descriptor path changed to {}", descriptorFile_);
}

void storageAccessor::attachDescriptor(const Descriptor *descriptorParam) {
  if (descriptorFileExist()) {
    std::fstream myFile;
    myFile.rdbuf()->pubsetbuf(nullptr, 0);
    myFile.open(descriptorFile_, std::ios::in);  // Open existing descriptor
    if (myFile.good()) myFile >> descriptor;
    myFile.close();

    if (descriptor.getSizeInBytes() == 0) {
      SPDLOG_ERROR("Empty descriptor in file.");
      std::cerr << "Error, empty descriptor file:" << storageFile_ << ".desc\n";
      assert(false && "Empty descriptor in file.");
      abort();
    }

    if (descriptorParam != nullptr && *descriptorParam != descriptor) {
      SPDLOG_ERROR("Descriptors do not match.");
      std::cerr << "Error in data descriptor file: " << storageFile_ << ".desc\n";
      std::cerr << "Provided Descriptor:\n" << *descriptorParam << "\nExisting Descriptor:\n" << descriptor << std::endl;
      assert(false && "Descriptors dont match - previous one have different schema? remove&restart.");
      abort();
    }

    moveRef();
    storagePayload_ = std::make_unique<rdb::payload>(descriptor);
    chamber_        = std::make_unique<rdb::payload>(descriptor);

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
  descFile.open(descriptorFile_, std::ios::out);
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile << descriptor;
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile.close();

  moveRef();
  storagePayload_ = std::make_unique<rdb::payload>(descriptor);
  chamber_        = std::make_unique<rdb::payload>(descriptor);

  SPDLOG_INFO("Payload & Descriptor created.");
  attachStorage();
}

void storageAccessor::moveRef() {
  auto it = std::find_if(descriptor.begin(),
                         descriptor.end(),                                        //
                         [](const auto &item) { return item.rtype == rdb::REF; }  //
  );

  // Descriptor changes storageFile location
  if (it != descriptor.end()) {
    storageFile_ = (*it).rname;
    SPDLOG_INFO("Storage ref from descriptor changed to {}", storageFile_);
  }

  // if storageAccessor object was created with default storage as ""
  // and there is no specified storage as REF in descriptor - we should
  // stop immediately.
  if (storageFile_ == "") {
    SPDLOG_ERROR("Storage file was not set in descriptor.");
    assert(false && "Storage file was not set in descriptor.");
    abort();
  }
}

void storageAccessor::attachStorage() {
  assert(storageFile_ != "");

  auto it1 = std::find_if(descriptor.begin(),
                          descriptor.end(),                                         //
                          [](const auto &item) { return item.rtype == rdb::TYPE; }  //
  );

  if (it1 != descriptor.end()) {
    storageType_ = (*it1).rname;
    SPDLOG_INFO("Storage type from descriptor {}", storageType_);
  }

  initializeAccessor();

  if (isDeclared()) {
    SPDLOG_INFO("records declared source on {}", storageFile_);
    return;
  }

  recordsCount_ = accessor_->count();
  SPDLOG_INFO("record count {} on {}", recordsCount_, storageFile_);

  dataFileStatus = storageState::openAndCreate;
}

storageAccessor::~storageAccessor() {
  if (isDisposable_) {
    if (storageFile_ != "") auto statusRemove1 = remove(storageFile_.c_str());
    if (descriptorFileExist()) remove(descriptorFile_.c_str());
    SPDLOG_INFO("Disposable - drop storage, drop descriptor");
  }
}

bool storageAccessor::isDeclared() { return (storageType_ == "DEVICE") || (storageType_ == "TEXTSOURCE"); }

void storageAccessor::initializeAccessor() {
  assert(storageFile_ != "");
  assert(storageType_ != "");
  auto size = descriptor.getSizeInBytes();

  if (storageType_ == "DEFAULT") {
    accessor_ = std::make_unique<rdb::groupFileAccessor>(storageFile_, size, descriptor.retention(), percounter_);
  } else if (storageType_ == "MEMORY") {
    accessor_ = std::make_unique<rdb::memoryFileAccessor>(storageFile_, size, descriptor.policy());
  } else if (storageType_ == "POSIX") {
    accessor_ = std::make_unique<rdb::posixBinaryFileAccessor>(storageFile_, size, percounter_);
  } else if (storageType_ == "GENERIC") {
    accessor_ = std::make_unique<rdb::genericBinaryFileAccessor>(storageFile_, size, percounter_);
  } else if (storageType_ == "DEVICE") {
    accessor_ = std::make_unique<rdb::binaryDeviceAccessorRO>(storageFile_, size, !isOneShot_);
  } else if (storageType_ == "TEXTSOURCE") {
    accessor_ = std::make_unique<rdb::textSourceAccessorRO>(storageFile_, size, descriptor, !isOneShot_);
  } else {
    SPDLOG_INFO("Unsupported storage type {}", storageType_);
    assert(false && "Unsupported storage type");
    abort();
  }
}

void storageAccessor::reset() {
  assert(storageFile_ != "");

  if (!accessor_) return;  // no accessor initialized - no need to reset.

  auto resourceAlreadyExist = std::filesystem::exists(storageFile_);
  if (resourceAlreadyExist)
    if (!isDeclared()) remove(storageFile_.c_str());

  initializeAccessor();

  accessor_->write(nullptr, 0);
  recordsCount_ = 0;

  assert(recordsCount_ == accessor_->count());

  SPDLOG_INFO("reset - drop & recreate storage.");
}

void storageAccessor::cleanPayload(uint8_t *destination) {
  destination = (destination == nullptr)                              //
                    ? static_cast<uint8_t *>(storagePayload_->get())  //
                    : destination;
  auto size   = descriptor.getSizeInBytes();
  std::memset(destination, 0, size);
}

std::unique_ptr<rdb::payload>::pointer storageAccessor::getPayload() {
  if (!storagePayload_) {
    SPDLOG_ERROR("no payload attached");
    assert(false && "no payload attached");
    abort();
  }
  return storagePayload_.get();
}

bool storageAccessor::descriptorFileExist() { return std::filesystem::exists(descriptorFile_); }

void storageAccessor::setDisposable(bool value) { isDisposable_ = value; }

void storageAccessor::releaseOnHold() {
  if (isHold_ == true) {
    SPDLOG_INFO("RELEASE stream file '{}' from HOLD state", storageFile_);
  }
  isHold_ = false;
}

size_t storageAccessor::getRecordsCount() { return recordsCount_; }

std::string storageAccessor::getStorageName() { return storageFile_; }

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
  if (!storagePayload_) {
    SPDLOG_ERROR("no payload attached");
    assert(false && "payload not attached");
    abort();
  }
}

void storageAccessor::fire() {
  assert(circularBuffer_.capacity() > 0);
  *storagePayload_ = *chamber_;
  recordsCount_++;
  circularBuffer_.push_front(*storagePayload_.get());  // only one place when buffer is feed.
}

void storageAccessor::purge() { accessor_->write(nullptr, 0); }

bool storageAccessor::read(const size_t recordIndexFromFront, uint8_t *destination) {
  assert(!isDeclared());
  abortIfStorageNotPrepared();

  if (destination == nullptr) {
    destination = static_cast<uint8_t *>(storagePayload_->get());
  }

  assert(destination != nullptr);
  auto size   = descriptor.getSizeInBytes();
  auto result = 0;

  assert(recordsCount_ == accessor_->count());

  if (isHold_) {
    std::memset(destination, 0, size);
    SPDLOG_INFO("read on HOLD {} - fake data from pos:{} rec-count:{}", accessor_->name(), recordIndexFromFront, recordsCount_);
    return true;
  }

  if (recordsCount_ > 0 && recordIndexFromFront < recordsCount_) {
    result = accessor_->read(destination, recordIndexFromFront * size);
    assert(result == 0);
    SPDLOG_INFO("read from file {} pos:{} rec-count:{}", accessor_->name(), recordIndexFromFront, recordsCount_);
  } else {
    std::memset(destination, 0, size);
    SPDLOG_WARN("read fake {} - non existing data from pos:{} rec-count:{}", accessor_->name(), recordIndexFromFront,
                recordsCount_);
  }
  return result == 0;
}

bool storageAccessor::revRead(const size_t recordIndexFromBack, uint8_t *destination) {
  if (recordsCount_ == accessor_->count())
    SPDLOG_INFO("revRead {}: recordsCount:{} ->count():{}", storageFile_, recordsCount_, accessor_->count());
  else
    SPDLOG_ERROR("revRead {}: recordsCount:{} ->count():{}", storageFile_, recordsCount_, accessor_->count());

  if (isHold_) {
    SPDLOG_INFO("revRead on HOLD {} - fake data from pos:{} rec-count:{}", accessor_->name(), recordIndexFromBack,
                recordsCount_);
    destination = (destination == nullptr)                              //
                      ? static_cast<uint8_t *>(storagePayload_->get())  //
                      : destination;

    assert(destination != nullptr);
    auto size = descriptor.getSizeInBytes();
    std::memset(destination, 0, size);
    bufferState = sourceState::armed;  // fake armed on hold position
    return true;
  }

  if (!isDeclared()) {
    assert(recordsCount_ == accessor_->count());
    const auto recordPositionFromBack = recordsCount_ - recordIndexFromBack - 1;
    return read(recordPositionFromBack, destination);
  }

  // For all _DECLARED_ data sources buffer capacity at least _MUST_ be 1
  // In order to maintain the consistency of declared data sources,
  // it is necessary to maintain a buffer of at least 1

  assert(circularBuffer_.capacity() > 0);
  assert(isDeclared());

  if (recordIndexFromBack == 0 && bufferState == sourceState::flux) {
    //
    // THIS IS ONLY ONE PLACE WHERE DATA ARE READ FROM SOURCE
    //
    auto result = accessor_->read(static_cast<uint8_t *>(chamber_->get()), 0);
    SPDLOG_INFO("Physical read from source {} into chamber_ result={}", accessor_->name(), result);
    assert(result == EXIT_SUCCESS && "read failure from data source");
    bufferState              = sourceState::armed;
    *(storagePayload_.get()) = *chamber_;
    return true;
  }
  assert(recordIndexFromBack >= 0);

  // Read data from Circular Buffer instead of data source
  // - only for declared data sources
  // - only for data sources that have buffer declared
  // - only for recordIndex > 0 if sourceState::flux
  // - also for recordIndex == 0

  SPDLOG_INFO("Buffer capacity = {}, recordIndex = {}, file = {}", circularBuffer_.capacity(), recordIndexFromBack,
              storageFile_);
  assert((recordIndexFromBack < circularBuffer_.capacity()) && "Stop if we are accessing over Circular Buffer Size.");

  // in case of accessing buffer that has no data yet - zeros are returned

  if (recordIndexFromBack >= circularBuffer_.size()) {
    destination = (destination == nullptr)                              //
                      ? static_cast<uint8_t *>(storagePayload_->get())  //
                      : destination;

    assert(destination != nullptr);
    auto size = descriptor.getSizeInBytes();
    std::memset(destination, 0, size);
    SPDLOG_WARN("read buffer fn {} - non existing data from [pos:{} cap:{} size:{}]", accessor_->name(), recordIndexFromBack,
                circularBuffer_.capacity(), circularBuffer_.size());
    return true;
  }

  assert((recordIndexFromBack < circularBuffer_.size()) && "Stop if we have not enough elements in buffer (? - zeros?)");

  *(storagePayload_.get()) = circularBuffer_[recordIndexFromBack];
  return true;
}

void storageAccessor::setCapacity(const int capacity) {
  if (isDeclared()) circularBuffer_.set_capacity(capacity == 0 ? 1 : capacity);
}

bool storageAccessor::write(const size_t recordIndex) {
  abortIfStorageNotPrepared();

  assert(recordsCount_ == accessor_->count());

  auto size   = descriptor.getSizeInBytes();
  auto result = 0;
  if (recordIndex >= recordsCount_) {
    result = accessor_->write(static_cast<uint8_t *>(storagePayload_->get()));  // <- Call to append Function
    assert(result == 0);
    if (result == 0) recordsCount_++;

    SPDLOG_INFO("append");
    return result == 0;
  }

  if (recordsCount_ > 0 && recordIndex < recordsCount_) {
    result = accessor_->write(static_cast<uint8_t *>(storagePayload_->get()), recordIndex * size);
    assert(result == 0);
    SPDLOG_INFO("write {}", recordIndex);
  }

  assert(recordsCount_ == accessor_->count());

  return result == 0;
};

}  // namespace rdb