#include "rdb/storageacc.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

#include <cstring>  //std::memset
#include <filesystem>
#include <fstream>
#include <iostream>
#include "fatalError.hpp"

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
  if (qryID.empty()) FatalError("storage: qryID must not be empty");
  if (fileName.empty()) FatalError("storage: fileName must not be empty");

  metaIndexFile_ = storageFile_ + ".meta";

  if (storageParam.empty()) {
    return;  // no change
  }

  if (!std::filesystem::is_directory(storageParam)) {
    FatalError("storage: path '{}' is not a directory", storageParam);
  }

  descriptorFile_ = std::filesystem::path(storageParam) / std::filesystem::path(descriptorFile_);
  storageFile_    = std::filesystem::path(storageParam) / std::filesystem::path(storageFile_);
  metaIndexFile_  = storageFile_ + ".meta";
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
      FatalError("storage: empty descriptor file");
    }

    if (descriptorParam != nullptr && *descriptorParam != descriptor) {
      SPDLOG_ERROR("Descriptors do not match.");
      std::cerr << "Error in data descriptor file: " << storageFile_ << ".desc\n";
      std::cerr << "Provided Descriptor:\n" << *descriptorParam << "\nExisting Descriptor:\n" << descriptor << std::endl;
      FatalError("storage: descriptor schema mismatch — remove data files and restart");
    }

    moveRef();
    storagePayload_ = std::make_unique<rdb::payload>(descriptor);
    chamber_        = std::make_unique<rdb::payload>(descriptor);

    attachStorage();
    return;
  }

  if (descriptorParam == nullptr) {
    FatalError("storage: no descriptor file and no descriptor provided");
  }

  descriptor = *descriptorParam;

  // Create descriptor file instance
  std::fstream descFile;
  descFile.rdbuf()->pubsetbuf(nullptr, 0);
  descFile.open(descriptorFile_, std::ios::out);
  if ((descFile.rdstate() & std::ofstream::failbit) != 0) {
    FatalError("storage: failed to open descriptor file for writing: {}", descriptorFile_);
  }
  descFile << descriptor;
  if ((descFile.rdstate() & std::ofstream::failbit) != 0) {
    FatalError("storage: failed to write descriptor file: {}", descriptorFile_);
  }
  descFile.close();

  moveRef();
  storagePayload_ = std::make_unique<rdb::payload>(descriptor);
  chamber_        = std::make_unique<rdb::payload>(descriptor);

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
  }

  // if storage object was created with default storage as ""
  // and there is no specified storage as REF in descriptor - we should
  // stop immediately.
  if (storageFile_ == "") {
    FatalError("storage: storage file not set in descriptor (missing REF field or :STORAGE directive)");
  }
}

void storage::attachStorage() {
  if (storageFile_.empty()) FatalError("storage: storageFile_ is empty — storage not properly configured");

  auto it1 = std::find_if(descriptor.begin(),
                          descriptor.end(),                                         //
                          [](const auto &item) { return item.rtype == rdb::TYPE; }  //
  );

  if (it1 != descriptor.end()) {
    storageType_ = (*it1).rname;
  }

  initializeAccessor();

  if (isDeclared()) {
    // Note: no meta index on declared sources, as they are read-only and not expected to have null values.
    return;
  }

  recordsCount_ = accessor_->count();

  metaDataStream_ = std::make_unique<rdb::metaDataStream>(descriptor, metaIndexFile_);
  detectStartupState();
}

storage::~storage() {
  flushPendingGap();
  if (isDisposable_) {
    if (storageFile_ != "") auto statusRemove1 = remove(storageFile_.c_str());
    if (descriptorFileExist()) remove(descriptorFile_.c_str());
    if (!metaIndexFile_.empty() && std::filesystem::exists(metaIndexFile_)) remove(metaIndexFile_.c_str());
  }
}

bool storage::isDeclared() { return (storageType_ == "DEVICE") || (storageType_ == "TEXTSOURCE"); }

void storage::initializeAccessor() {
  if (storageFile_.empty()) FatalError("storage: storageFile_ is empty — storage not properly configured");
  if (storageType_.empty()) FatalError("storage: storageType_ is empty — storage type not set");

  if (storageType_ == "DEFAULT") {
    accessor_ = std::make_unique<rdb::groupFile<posixBinaryFileWithShadow>>(storageFile_, descriptor, descriptor.retention(),
                                                                            percounter_);
  } else if (storageType_ == "DIRECT") {
    accessor_ = std::make_unique<rdb::groupFile<posixBinaryFile>>(storageFile_, descriptor, descriptor.retention(), percounter_);
  } else if (storageType_ == "MEMORY") {
    accessor_ = std::make_unique<rdb::memoryFile>(storageFile_, descriptor, descriptor.storagePolicy());
  } else if (storageType_ == "POSIX") {
    accessor_ = std::make_unique<rdb::posixBinaryFile>(storageFile_, descriptor, percounter_);
  } else if (storageType_ == "POSIXSHD") {
    accessor_ = std::make_unique<rdb::posixBinaryFileWithShadow>(storageFile_, descriptor, percounter_);
  } else if (storageType_ == "GENERIC") {
    accessor_ = std::make_unique<rdb::genericBinaryFile>(storageFile_, descriptor, percounter_);
  } else if (storageType_ == "DEVICE") {
    accessor_ = std::make_unique<rdb::binaryDeviceRO>(storageFile_, descriptor, !isOneShot_);
  } else if (storageType_ == "TEXTSOURCE") {
    accessor_ = std::make_unique<rdb::textSourceRO>(storageFile_, descriptor, !isOneShot_);
  } else {
    FatalError("storage: unsupported storage type '{}'", storageType_);
  }
}

void storage::resetForUnitTest() {
  consecutiveNullCount_ = 0;
  activeGapDuration_    = 0;
  if (storageFile_.empty()) FatalError("storage: storageFile_ is empty — storage not properly configured");

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

  if (recordsCount_ != accessor_->count()) {
    FatalError("storage: internal record count mismatch: recordsCount_={} count()={} in {}", recordsCount_, accessor_->count(),
               storageFile_);
  }
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
    FatalError("storage::getPayload: payload not attached");
  }
  return storagePayload_.get();
}

bool storage::descriptorFileExist() { return std::filesystem::exists(descriptorFile_); }

void storage::setDisposable(bool value) { isDisposable_ = value; }

void storage::releaseOnHold() { isHold_ = false; }

size_t storage::getRecordsCount() { return recordsCount_; }

void storage::abortIfStorageNotPrepared() {
  if (descriptor.empty()) {
    FatalError("storage: descriptor is empty — storage not initialized");
  }
  if (!accessor_) {
    FatalError("storage: data file not opened — accessor not initialized");
  }
  if (!storagePayload_) {
    FatalError("storage: payload not attached");
  }
}

void storage::fire() {
  if (circularBuffer_.capacity() == 0) FatalError("storage::fire: circular buffer capacity is zero");
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
  }
}

void storage::markTransmissionGap(size_t gapDuration) {
  if (!metaDataStream_) {
    SPDLOG_WARN("markTransmissionGap called but metaDataStream is not initialized");
    return;
  }

  metaDataStream_->onTransmissionGap(gapDuration);
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
  if (isDeclared()) FatalError("storage::read: cannot read directly from declared (device/textsource) storage");
  abortIfStorageNotPrepared();

  if (destination == nullptr) {
    destination = storagePayload_->span().data();
  }

  if (destination == nullptr) FatalError("storage::read: destination pointer is null (payload span is empty)");
  auto size   = descriptor.getSizeInBytes();
  auto result = 0;

  if (recordsCount_ != accessor_->count()) {
    FatalError("storage: internal record count mismatch: recordsCount_={} count()={} in {}", recordsCount_, accessor_->count(),
               storageFile_);
  }

  if (isHold_) {
    std::memset(destination, 0, size);
    return true;
  }

  if (recordsCount_ > 0 && recordIndexFromFront < recordsCount_) {
    result = accessor_->read(destination, recordIndexFromFront * size);
    if (result != 0) {
      FatalError("storage::read: read from '{}' at pos {} failed (result={})", accessor_->name(), recordIndexFromFront, result);
    }
    if (metaDataStream_) {
      if (recordIndexFromFront < metaDataStream_->totalRecords())
        storagePayload_->setNullBitset(metaDataStream_->getNullBitset(recordIndexFromFront));
      else
        storagePayload_->setNullBitset(std::vector<bool>(descriptor.size(), false));
    }
  } else {
    std::memset(destination, 0, size);
    storagePayload_->setNullBitset(std::vector<bool>(descriptor.size(), false));
    SPDLOG_WARN("read fake {} - non existing data from pos:{} rec-count:{}", accessor_->name(), recordIndexFromFront,
                recordsCount_);
  }
  return result == 0;
}

bool storage::revRead(const size_t recordIndexFromBack, uint8_t *destination) {
  if (recordsCount_ != accessor_->count())
    SPDLOG_ERROR("revRead {}: recordsCount:{} ->count():{}", storageFile_, recordsCount_, accessor_->count());

  if (isHold_) {
    destination = (destination == nullptr)              //
                      ? storagePayload_->span().data()  //
                      : destination;

    if (destination == nullptr) FatalError("storage::revRead: destination pointer is null in hold path");
    auto size = descriptor.getSizeInBytes();
    std::memset(destination, 0, size);
    bufferState = sourceState::armed;  // fake armed on hold position
    return true;
  }

  if (!isDeclared()) {
    if (recordsCount_ != accessor_->count()) {
      FatalError("storage: internal record count mismatch: recordsCount_={} count()={} in {}", recordsCount_, accessor_->count(),
                 storageFile_);
    }
    const auto recordPositionFromBack = recordsCount_ - recordIndexFromBack - 1;
    return read(recordPositionFromBack, destination);
  }

  // For all _DECLARED_ data sources buffer capacity at least _MUST_ be 1
  // In order to maintain the consistency of declared data sources,
  // it is necessary to maintain a buffer of at least 1

  if (circularBuffer_.capacity() == 0) FatalError("storage::revRead: circular buffer capacity is zero for declared source");
  if (!isDeclared()) FatalError("storage::revRead: expected declared source (DEVICE/TEXTSOURCE)");

  if (recordIndexFromBack == 0 && bufferState == sourceState::flux) {
    //
    // THIS IS ONLY ONE PLACE WHERE DATA ARE READ FROM SOURCE
    //
    auto result = accessor_->read(chamber_->span().data(), 0);

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
  // recordIndexFromBack is size_t (unsigned), always >= 0

  // Read data from Circular Buffer instead of data source
  // - only for declared data sources
  // - only for data sources that have buffer declared
  // - only for recordIndex > 0 if sourceState::flux
  // - also for recordIndex == 0

  if (recordIndexFromBack >= circularBuffer_.capacity()) {
    FatalError("storage::revRead: recordIndexFromBack {} >= circularBuffer_.capacity() {} in '{}'", recordIndexFromBack,
               circularBuffer_.capacity(), accessor_->name());
  }

  // in case of accessing buffer that has no data yet - zeros are returned

  if (recordIndexFromBack >= circularBuffer_.size()) {
    destination = (destination == nullptr)              //
                      ? storagePayload_->span().data()  //
                      : destination;

    if (destination == nullptr) FatalError("storage::revRead: destination pointer is null in buffer fallback path");
    auto size = descriptor.getSizeInBytes();
    std::memset(destination, 0, size);
    SPDLOG_WARN("read buffer fn {} - non existing data from [pos:{} cap:{} size:{}]", accessor_->name(), recordIndexFromBack,
                circularBuffer_.capacity(), circularBuffer_.size());
    return true;
  }

  // Note: the previous if-block handles the case where recordIndexFromBack >= circularBuffer_.size()
  // so here recordIndexFromBack < circularBuffer_.size() is guaranteed

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
        return true;
      }
    } else {
      flushPendingGap();
      consecutiveNullCount_ = 0;
    }
  }

  if (recordsCount_ != accessor_->count()) {
    FatalError("storage: internal record count mismatch: recordsCount_={} count()={} in {}", recordsCount_, accessor_->count(),
               storageFile_);
  }

  auto size   = descriptor.getSizeInBytes();
  auto result = 0;
  if (recordIndex >= recordsCount_) {
    result = accessor_->write(storagePayload_->span().data());  // <- Call to append Function
    if (result != 0) {
      FatalError("storage::write: append to '{}' failed (result={})", storageFile_, result);
    }
    recordsCount_++;

    if (metaDataStream_) {
      metaDataStream_->onRecordAppended(nullInfo);
      metaDataStream_->flushCurrentEntry();
    }

  } else {
    if (recordsCount_ == 0) FatalError("storage::write: recordsCount_ is zero but recordIndex implies overwrite");
    if (recordIndex >= recordsCount_) {
      FatalError("storage::write: recordIndex {} >= recordsCount_ {} in '{}'", recordIndex, recordsCount_, storageFile_);
    }

    result = accessor_->write(storagePayload_->span().data(), recordIndex * size);
    if (result != 0) {
      FatalError("storage::write: overwrite to '{}' at index {} failed (result={})", storageFile_, recordIndex, result);
    }

    if (metaDataStream_) metaDataStream_->onRecordModified(recordIndex, nullInfo);

    if (recordsCount_ != accessor_->count()) {
      FatalError("storage: internal record count mismatch: recordsCount_={} count()={} in {}", recordsCount_, accessor_->count(),
                 storageFile_);
    }
  }
  return result == 0;
};

void storage::configureGapDetection(boost::rational<int> rInterval, int nullFillCount) {
  rInterval_              = rInterval;
  nullFillCount_          = nullFillCount;
  gapDetectionConfigured_ = true;

  detectStartupState();
}

void storage::detectStartupState() {
  if (!metaDataStream_ || !gapDetectionConfigured_) return;

  // Detect rotation: data file is fresh/empty but meta index has records from previous run
  if (recordsCount_ == 0 && !metaDataStream_->isEmpty()) {
    metaDataStream_->rotate(percounter_);
    return;
  }

  // Fresh start with no previous records — nothing to gap from
  if (metaDataStream_->isEmpty()) return;

  // Existing data: compute gap since data file was last written
  if (!std::filesystem::exists(storageFile_) || rInterval_.numerator() <= 0) return;

  std::error_code ec;
  auto lastWriteFT = std::filesystem::last_write_time(storageFile_, ec);
  if (ec) return;

  auto lastWriteSC     = std::chrono::file_clock::to_sys(lastWriteFT);
  auto now             = std::chrono::system_clock::now();
  const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastWriteSC).count();
  if (elapsedNs <= 0) return;

  const auto intervalNs =
      static_cast<int64_t>(static_cast<double>(rInterval_.numerator()) / rInterval_.denominator() * 1'000'000'000.0);
  if (intervalNs <= 0) return;

  const auto gapDuration = static_cast<size_t>(elapsedNs / intervalNs);
  if (gapDuration > 0) metaDataStream_->onTransmissionGap(gapDuration);
}

boost::rational<int> storage::getSamplingInterval() const { return rInterval_; }

void storage::flushPendingGap() {
  if (activeGapDuration_ == 0 || !metaDataStream_) return;
  markTransmissionGap(activeGapDuration_);
  activeGapDuration_ = 0;
}

}  // namespace rdb
