#include "rdb/storageacc.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstring>  //std::memset
#include <filesystem>
#include <fstream>
#include <iostream>

namespace rdb {

storage::storage(const std::string_view qryID,         //
                 const std::string_view fileName,      //
                 const std::string_view storageParam,  //
                 const std::string_view storageType,   //
                 bool oneShot,                         //
                 bool isHold,                          //
                 int percounter)
    : descriptorFile_(std::string(qryID) + ".desc"),
      storageFile_(fileName),
      percounter_(percounter),
      isOneShot_(oneShot),
      isHold_(isHold),
      storageType_(storageType) {
  assert(!qryID.empty());
  assert(!fileName.empty());

  if (isHold_) {
    SPDLOG_INFO("Stream file '{}' created in HOLD state", storageFile_);
  }

  metaIndexFile_ = storageFile_ + ".meta";

  if (storageParam.empty()) {
    SPDLOG_INFO("That's OK. Storage path is empty - no change of storageFile");
    SPDLOG_INFO("Meta index path {}", metaIndexFile_);
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
  metaIndexFile_  = storageFile_ + ".meta";

  SPDLOG_INFO("Meta index path {}", metaIndexFile_);
  SPDLOG_INFO("Storage path changed to {}", storageFile_);
  SPDLOG_INFO("Descriptor path changed to {}", descriptorFile_);
}

void storage::attachDescriptor(const Descriptor *descriptorParam) {
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

void storage::moveRef() {
  auto it = std::find_if(descriptor.begin(),
                         descriptor.end(),                                        //
                         [](const auto &item) { return item.rtype == rdb::REF; }  //
  );

  // Descriptor changes storageFile location
  if (it != descriptor.end()) {
    storageFile_   = (*it).rname;
    metaIndexFile_ = storageFile_ + ".meta";
    SPDLOG_INFO("Storage ref from descriptor changed to {}", storageFile_);
    SPDLOG_INFO("Meta index path changed to {}", metaIndexFile_);
  }

  // if storage object was created with default storage as ""
  // and there is no specified storage as REF in descriptor - we should
  // stop immediately.
  if (storageFile_ == "") {
    SPDLOG_ERROR("Storage file was not set in descriptor.");
    assert(false && "Storage file was not set in descriptor.");
    abort();
  }
}

void storage::attachStorage() {
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
    // Note: no meta index on declared sources, as they are read-only and not expected to have null values.
    return;
  }

  recordsCount_ = accessor_->count();
  SPDLOG_INFO("record count {} on {}", recordsCount_, storageFile_);

  metaDataStream_ = std::make_unique<rdb::metaDataStream>(descriptor, metaIndexFile_, rInterval_);
  SPDLOG_INFO("metaIndex file {} rInterval {}/{}", metaIndexFile_, rInterval_.numerator(), rInterval_.denominator());
}

storage::~storage() {
  flushPendingGap();
  if (isDisposable_) {
    if (storageFile_ != "") auto statusRemove1 = remove(storageFile_.c_str());
    if (descriptorFileExist()) remove(descriptorFile_.c_str());
    if (!metaIndexFile_.empty() && std::filesystem::exists(metaIndexFile_)) remove(metaIndexFile_.c_str());
    SPDLOG_INFO("Disposable - drop storage, drop descriptor, drop meta");
  }
}

bool storage::isDeclared() { return (storageType_ == "DEVICE") || (storageType_ == "TEXTSOURCE"); }

void storage::initializeAccessor() {
  assert(storageFile_ != "");
  assert(storageType_ != "");
  auto size = descriptor.getSizeInBytes();

  if (storageType_ == "DEFAULT") {
    accessor_ = std::make_unique<rdb::groupFile<posixBinaryFileWithShadow>>(storageFile_, size, descriptor.retention(), percounter_);
  } else if (storageType_ == "DIRECT") {
    accessor_ =
        std::make_unique<rdb::groupFile<posixBinaryFile>>(storageFile_, size, descriptor.retention(), percounter_);
  } else if (storageType_ == "MEMORY") {
    accessor_ = std::make_unique<rdb::memoryFile>(storageFile_, size, descriptor.policy());
  } else if (storageType_ == "POSIX") {
    accessor_ = std::make_unique<rdb::posixBinaryFile>(storageFile_, size, percounter_);
  } else if (storageType_ == "POSIXSHD") {
    accessor_ = std::make_unique<rdb::posixBinaryFileWithShadow>(storageFile_, size, percounter_);
  } else if (storageType_ == "GENERIC") {
    accessor_ = std::make_unique<rdb::genericBinaryFile>(storageFile_, size, percounter_);
  } else if (storageType_ == "DEVICE") {
    accessor_ = std::make_unique<rdb::binaryDeviceRO>(storageFile_, descriptor, !isOneShot_);
  } else if (storageType_ == "TEXTSOURCE") {
    accessor_ = std::make_unique<rdb::textSourceRO>(storageFile_, descriptor, !isOneShot_);
  } else {
    SPDLOG_INFO("Unsupported storage type {}", storageType_);
    assert(false && "Unsupported storage type");
    abort();
  }
}

void storage::reset() {
  consecutiveNullCount_ = 0;
  activeGapDuration_    = 0;
  assert(storageFile_ != "");

  if (!accessor_) return;  // no accessor initialized - no need to reset.

  auto resourceAlreadyExist = std::filesystem::exists(storageFile_);
  if (resourceAlreadyExist)
    if (!isDeclared()) remove(storageFile_.c_str());

  initializeAccessor();

  accessor_->write(nullptr, 0);
  recordsCount_ = 0;

  // Reset meta index if it exists
  if (metaDataStream_) {
    metaDataStream_->reset();
  }

  assert(recordsCount_ == accessor_->count());

  SPDLOG_INFO("reset - drop & recreate storage and meta index.");
}

void storage::cleanPayload(uint8_t *destination) {
  destination = (destination == nullptr)              //
                    ? storagePayload_->span().data()  //
                    : destination;
  auto size   = descriptor.getSizeInBytes();
  std::memset(destination, 0, size);
}

std::unique_ptr<rdb::payload>::pointer storage::getPayload() {
  if (!storagePayload_) {
    SPDLOG_ERROR("no payload attached");
    assert(false && "no payload attached");
    abort();
  }
  return storagePayload_.get();
}

bool storage::descriptorFileExist() { return std::filesystem::exists(descriptorFile_); }

void storage::setDisposable(bool value) { isDisposable_ = value; }

void storage::releaseOnHold() {
  if (isHold_ == true) {
    SPDLOG_INFO("RELEASE stream file '{}' from HOLD state", storageFile_);
  }
  isHold_ = false;
}

size_t storage::getRecordsCount() { return recordsCount_; }

void storage::abortIfStorageNotPrepared() {
  if (descriptor.empty()) {
    SPDLOG_ERROR("descriptor is Empty");
    assert(false && "Empty descriptor");
    abort();
  }
  if (!accessor_) {
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

void storage::fire() {
  assert(circularBuffer_.capacity() > 0);
  *storagePayload_ = *chamber_;
  recordsCount_++;
  circularBuffer_.push_front(*storagePayload_.get());  // only one place when buffer is feed.
}

void storage::purge() {
  abortIfStorageNotPrepared();

  consecutiveNullCount_ = 0;
  activeGapDuration_    = 0;

  accessor_->write(nullptr, 0);
  recordsCount_ = 0;

  // Reset meta index if it exists
  if (metaDataStream_) {
    metaDataStream_->reset();
    SPDLOG_INFO("metaDataStream reset during purge");
  }

  SPDLOG_INFO("purge - storage and meta index cleared");
}

void storage::markTransmissionGap(size_t gapDuration) {
  if (!metaDataStream_) {
    SPDLOG_WARN("markTransmissionGap called but metaDataStream is not initialized");
    return;
  }

  metaDataStream_->onTransmissionGap(gapDuration);
  SPDLOG_INFO("Transmission gap (duration={}) marked at record position {}", gapDuration, recordsCount_);
}

bool storage::hasGapBefore(size_t recordIndex) const {
  if (!metaDataStream_) {
    return false;
  }

  return metaDataStream_->isGapBefore(recordIndex);
}

bool storage::isMetaIndexEmpty() const {
  if (!metaDataStream_) {
    return recordsCount_ == 0;
  }

  return metaDataStream_->isEmpty();
}

bool storage::read(const size_t recordIndexFromFront, uint8_t *destination) {
  assert(!isDeclared());
  abortIfStorageNotPrepared();

  if (destination == nullptr) {
    destination = storagePayload_->span().data();
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
    if (metaDataStream_) {
      if (recordIndexFromFront < metaDataStream_->totalRecords())
        storagePayload_->setNullBitset(metaDataStream_->getNullBitset(recordIndexFromFront));
      else
        storagePayload_->setNullBitset(std::vector<bool>(descriptor.size(), false));
    }
    SPDLOG_INFO("read from file {} pos:{} rec-count:{}", accessor_->name(), recordIndexFromFront, recordsCount_);
  } else {
    std::memset(destination, 0, size);
    storagePayload_->setNullBitset(std::vector<bool>(descriptor.size(), false));
    SPDLOG_WARN("read fake {} - non existing data from pos:{} rec-count:{}", accessor_->name(), recordIndexFromFront,
                recordsCount_);
  }
  return result == 0;
}

bool storage::revRead(const size_t recordIndexFromBack, uint8_t *destination) {
  if (recordsCount_ == accessor_->count())
    SPDLOG_INFO("revRead {}: recordsCount:{} ->count():{}", storageFile_, recordsCount_, accessor_->count());
  else
    SPDLOG_ERROR("revRead {}: recordsCount:{} ->count():{}", storageFile_, recordsCount_, accessor_->count());

  if (isHold_) {
    SPDLOG_INFO("revRead on HOLD {} - fake data from pos:{} rec-count:{}", accessor_->name(), recordIndexFromBack,
                recordsCount_);
    destination = (destination == nullptr)              //
                      ? storagePayload_->span().data()  //
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
    auto result = accessor_->read(chamber_->span().data(), 0);
    SPDLOG_INFO("Physical read from source {} into chamber_ result={}", accessor_->name(), result);

    if (auto *textSource = dynamic_cast<rdb::textSourceRO *>(accessor_.get())) {
      chamber_->setNullBitset(textSource->lastNullBitset());
    } else if (auto *binarySource = dynamic_cast<rdb::binaryDeviceRO *>(accessor_.get())) {
      chamber_->setNullBitset(binarySource->lastNullBitset());
    } else if (result == EXIT_SUCCESS) {
      chamber_->setNullBitset(std::vector<bool>(descriptor.size(), false));
    } else {
      chamber_->setNullBitset(std::vector<bool>(descriptor.size(), true));
      std::fill(chamber_->span().begin(), chamber_->span().end(), 0);
    }

    if (result != EXIT_SUCCESS) {
      SPDLOG_WARN("Read failure from declared source {}. Returning null row.", accessor_->name());
    }

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
    destination = (destination == nullptr)              //
                      ? storagePayload_->span().data()  //
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

void storage::setCapacity(const int capacity) {
  if (isDeclared()) circularBuffer_.set_capacity(capacity == 0 ? 1 : capacity);
}

bool storage::write(const size_t recordIndex) {
  auto nullInfo = storagePayload_->getNullBitset();

  abortIfStorageNotPrepared();

  if (recordIndex >= recordsCount_ && gapDetectionConfigured_ && metaDataStream_) {
    const bool isAllNull = !nullInfo.empty() && std::all_of(nullInfo.begin(), nullInfo.end(), [](bool b) { return b; });
    if (isAllNull) {
      consecutiveNullCount_++;
      if (consecutiveNullCount_ > static_cast<size_t>(nullFillCount_)) {
        activeGapDuration_++;
        SPDLOG_INFO("Gap phase: skipping write, activeGapDuration={}", activeGapDuration_);
        return true;
      }
      SPDLOG_INFO("Nullfill phase: null record {}/{}", consecutiveNullCount_, nullFillCount_);
    } else {
      flushPendingGap();
      consecutiveNullCount_ = 0;
    }
  }

  assert(recordsCount_ == accessor_->count());

  auto size   = descriptor.getSizeInBytes();
  auto result = 0;
  if (recordIndex >= recordsCount_) {
    SPDLOG_INFO("append");

    result = accessor_->write(storagePayload_->span().data());  // <- Call to append Function
    assert(result == 0);
    if (result == 0) recordsCount_++;

    if (metaDataStream_) metaDataStream_->onRecordAppended(nullInfo);

  } else {
    SPDLOG_INFO("write {}", recordIndex);

    assert(recordsCount_ > 0);
    assert(recordIndex < recordsCount_);

    result = accessor_->write(storagePayload_->span().data(), recordIndex * size);
    assert(result == 0);

    if (metaDataStream_) metaDataStream_->onRecordModified(recordIndex, nullInfo);

    assert(recordsCount_ == accessor_->count());
  }
  return result == 0;
};

void storage::configureGapDetection(boost::rational<int> rInterval, int nullFillCount, int gapDetectionThreshold) {
  rInterval_              = rInterval;
  nullFillCount_          = nullFillCount;
  gapDetectionThreshold_  = gapDetectionThreshold;
  gapDetectionConfigured_ = true;

  // If metaDataStream already exists, recreate it with the new interval
  if (metaDataStream_) {
    metaDataStream_ = std::make_unique<rdb::metaDataStream>(descriptor, metaIndexFile_, rInterval_);
  }

  SPDLOG_INFO("configureGapDetection rInterval={}/{} nullFillCount={} gapDetectionThreshold={}", rInterval_.numerator(),
              rInterval_.denominator(), nullFillCount_, gapDetectionThreshold_);
}

void storage::flushPendingGap() {
  if (activeGapDuration_ == 0 || !metaDataStream_) return;
  markTransmissionGap(activeGapDuration_);
  SPDLOG_INFO("Flushed pending gap: duration={}", activeGapDuration_);
  activeGapDuration_ = 0;
}

}  // namespace rdb
