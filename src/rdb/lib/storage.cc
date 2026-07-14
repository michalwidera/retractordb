#include "rdb/storage.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

#include <cstring>  //std::memset
#include <filesystem>
#include <ranges>
#include "fatalError.hpp"
#include "rdb/accessorFactory.hpp"
#include "rdb/descriptorIO.hpp"

namespace rdb {

storage::storage(const std::string_view qryID,         //
                 const std::string_view fileName,      //
                 const std::string_view storageParam,  //
                 const std::string_view storageType,   //
                 bool oneShot,                         //
                 bool isHold,                          //
                 int percounter)
    : isOneShot_(oneShot),
      isHold_(isHold),
      paths_(qryID, fileName, storageParam),
      storageType_(storageType),
      percounter_(percounter) {}

void storage::attachDescriptor(const Descriptor *descriptorParam) {
  if (descriptorFileExist()) {
    descriptor = loadDescriptorFile(paths_.descriptorFile());
    if (descriptorParam != nullptr) verifyDescriptorMatch(*descriptorParam, descriptor, paths_.descriptorFile());
  } else {
    if (descriptorParam == nullptr) {
      FatalError("storage: no descriptor file and no descriptor provided");
    }
    descriptor = *descriptorParam;
    saveDescriptorFile(paths_.descriptorFile(), descriptor);
  }

  paths_.relocateFromRef(descriptor);
  storagePayload_ = std::make_unique<rdb::payload>(descriptor);
  buffer_.attach(descriptor);

  attachStorage();
}

void storage::attachStorage() {
  if (paths_.storageFile().empty()) FatalError("storage: storage file path is empty — storage not properly configured");

  auto it1 = std::ranges::find_if(descriptor,  //
                                  [](const auto &item) { return item.rtype == rdb::TYPE; });

  if (it1 != descriptor.end()) {
    storageType_ = (*it1).rname;
  }

  initializeAccessor();

  // Wstrzyknięcie wariantu metadanych — dobór wariantu (inertny/cień indeksu/bazowy) realizuje fabryka.
  metaData_ = makeMetaIndex(isDeclared(), accessor_->hasShadow(), descriptor, paths_.metaIndexFile());

  if (isDeclared()) return;

  recordsCount_ = accessor_->count();
  detectStartupState();
}

storage::~storage() {
  // Jedyny strażnik null: storage mógł zostać zniszczony przed attachDescriptor().
  // Pending gap musi trafić na dysk PRZED ewentualnym kasowaniem plików disposable.
  if (metaData_) metaData_->flushPendingGap();
  if (isDisposable_) {
    // Odłączenie od pliku PRZED usunięciem: bez tego destruktor metaData_ (wywołany automatycznie
    // po zakończeniu tego ciała) odtworzyłby właśnie skasowany plik .meta przez flushCurrentEntry().
    if (metaData_) metaData_->abandonFile();
    paths_.removeAllFiles();
  }
}

bool storage::isDeclared() const { return isDeclaredType(storageType_); }

void storage::initializeAccessor() {
  accessor_ = makeAccessor(storageType_, paths_.storageFile(), descriptor, isOneShot_, percounter_);
}

void storage::resetForUnitTest() {
  if (paths_.storageFile().empty()) FatalError("storage: storage file path is empty — storage not properly configured");

  if (!accessor_) return;  // no accessor initialized - no need to reset.

  auto resourceAlreadyExist = std::filesystem::exists(paths_.storageFile());
  if (resourceAlreadyExist)
    if (!isDeclared()) remove(paths_.storageFile().c_str());

  initializeAccessor();

  accessor_->write(nullptr, 0);
  recordsCount_ = 0;

  if (metaData_) metaData_->reset();

  if (recordsCount_ != accessor_->count()) {
    FatalError("storage: internal record count mismatch: recordsCount_={} count()={} in {}", recordsCount_, accessor_->count(),
               paths_.storageFile());
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

bool storage::descriptorFileExist() { return std::filesystem::exists(paths_.descriptorFile()); }

void storage::setDisposable(bool value) { isDisposable_ = value; }

void storage::releaseOnHold() { isHold_ = false; }

size_t storage::getRecordsCount() const { return recordsCount_; }

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
  if (!metaData_) {
    FatalError("storage: meta index not attached — attachDescriptor() not called");
  }
}

void storage::fire() {
  buffer_.fire(*storagePayload_);
  recordsCount_++;
}

void storage::purge() {
  abortIfStorageNotPrepared();

  accessor_->write(nullptr, 0);
  recordsCount_ = 0;

  metaData_->reset();  // czyści indeks oraz liczniki maszyny gap
}

void storage::markTransmissionGap(size_t gapDuration) { metaData_->onTransmissionGap(gapDuration); }

bool storage::hasGapBefore(size_t recordIndex) const { return metaData_->isGapBefore(recordIndex); }

bool storage::isMetaIndexEmpty() const {
  // Źródła deklarowane mają inertny indeks — o pustości decyduje licznik rekordów storage.
  if (isDeclared()) return recordsCount_ == 0;
  return metaData_->isEmpty();
}

bool storage::read(const size_t recordIndexFromFront, uint8_t *destination) {
  if (isDeclared()) FatalError("storage::read: cannot read directly from declared (device/textsource) storage");
  abortIfStorageNotPrepared();

  if (destination == nullptr) {
    destination = storagePayload_->span().data();
  }

  if (destination == nullptr) FatalError("storage::read: destination pointer is null (payload span is empty)");
  auto size      = descriptor.getSizeInBytes();
  ssize_t result = 0;

  if (recordsCount_ != accessor_->count()) {
    FatalError("storage: internal record count mismatch: recordsCount_={} count()={} in {}", recordsCount_, accessor_->count(),
               paths_.storageFile());
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
    storagePayload_->setNullBitset(metaData_->nullBitsetFor(recordIndexFromFront));
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
    SPDLOG_ERROR("revRead {}: recordsCount:{} ->count():{}", paths_.storageFile(), recordsCount_, accessor_->count());

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
                 paths_.storageFile());
    }
    const auto recordPositionFromBack = recordsCount_ - recordIndexFromBack - 1;
    return read(recordPositionFromBack, destination);
  }

  // For all _DECLARED_ data sources buffer capacity at least _MUST_ be 1
  // In order to maintain the consistency of declared data sources,
  // it is necessary to maintain a buffer of at least 1

  if (buffer_.capacity() == 0) FatalError("storage::revRead: circular buffer capacity is zero for declared source");

  if (recordIndexFromBack == 0 && bufferState == sourceState::flux) {
    buffer_.readCurrent(*accessor_, *storagePayload_);
    bufferState = sourceState::armed;
    return true;
  }
  // recordIndexFromBack is size_t (unsigned), always >= 0

  // Read data from Circular Buffer instead of data source
  // - only for declared data sources
  // - only for data sources that have buffer declared
  // - only for recordIndex > 0 if sourceState::flux
  // - also for recordIndex == 0

  if (recordIndexFromBack >= buffer_.capacity()) {
    FatalError("storage::revRead: recordIndexFromBack {} >= circularBuffer_.capacity() {} in '{}'", recordIndexFromBack,
               buffer_.capacity(), accessor_->name());
  }

  // in case of accessing buffer that has no data yet - zeros are returned

  if (recordIndexFromBack >= buffer_.size()) {
    destination = (destination == nullptr)              //
                      ? storagePayload_->span().data()  //
                      : destination;

    if (destination == nullptr) FatalError("storage::revRead: destination pointer is null in buffer fallback path");
    auto size = descriptor.getSizeInBytes();
    std::memset(destination, 0, size);
    SPDLOG_WARN("read buffer fn {} - non existing data from [pos:{} cap:{} size:{}]", accessor_->name(), recordIndexFromBack,
                buffer_.capacity(), buffer_.size());
    return true;
  }

  // Note: the previous if-block handles the case where recordIndexFromBack >= buffer_.size()
  // so here recordIndexFromBack < buffer_.size() is guaranteed

  *(storagePayload_) = buffer_.history(recordIndexFromBack);
  return true;
}

void storage::setCapacity(const int capacity) {
  if (isDeclared()) buffer_.setCapacity(capacity);
}

bool storage::write(const size_t recordIndex) {
  abortIfStorageNotPrepared();
  const auto nullInfo = storagePayload_->getNullBitset();

  // Maszyna detekcji gap żyje w metaData: rekord all-null poza fazą nullfill jest pochłaniany
  // (nie trafia do fizycznego magazynu), a jego brak zostanie oznaczony wpisem gap.
  if (recordIndex >= recordsCount_ && metaData_->absorbAppend(nullInfo)) return true;

  if (recordsCount_ != accessor_->count()) {
    FatalError("storage: internal record count mismatch: recordsCount_={} count()={} in {}", recordsCount_, accessor_->count(),
               paths_.storageFile());
  }

  ssize_t result = 0;
  if (recordIndex >= recordsCount_) {
    result = accessor_->write(storagePayload_->span().data());  // <- Call to append Function
    if (result != 0) {
      FatalError("storage::write: append to '{}' failed (result={})", paths_.storageFile(), result);
    }
    recordsCount_++;

    metaData_->onRecordAppended(nullInfo);
    metaData_->flushCurrentEntry();
  } else {
    result = accessor_->write(storagePayload_->span().data(), recordIndex * descriptor.getSizeInBytes());
    if (result != 0) {
      FatalError("storage::write: overwrite to '{}' at index {} failed (result={})", paths_.storageFile(), recordIndex, result);
    }

    metaData_->onRecordModified(recordIndex, nullInfo);  // polimorficznie: cień indeksu albo główny indeks
  }
  return result == 0;
};

void storage::configureGapDetection(boost::rational<int> rInterval, int nullFillCount) {
  rInterval_ = rInterval;
  if (metaData_) metaData_->configureGapDetection(nullFillCount);

  detectStartupState();
}

void storage::detectStartupState() {
  if (!metaData_ || !metaData_->gapDetectionEnabled()) return;

  // Detect rotation: data file is fresh/empty but meta index has records from previous run
  if (recordsCount_ == 0 && !metaData_->isEmpty()) {
    metaData_->rotate(percounter_);
    return;
  }

  // Fresh start with no previous records — nothing to gap from
  if (metaData_->isEmpty()) return;

  // Existing data: compute gap since data file was last written
  if (!std::filesystem::exists(paths_.storageFile()) || rInterval_.numerator() <= 0) return;

  std::error_code ec;
  auto lastWriteFT = std::filesystem::last_write_time(paths_.storageFile(), ec);
  if (ec) return;

  auto lastWriteSC     = std::chrono::file_clock::to_sys(lastWriteFT);
  auto now             = std::chrono::system_clock::now();
  const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastWriteSC).count();
  if (elapsedNs <= 0) return;

  const auto intervalNs =
      static_cast<int64_t>(static_cast<double>(rInterval_.numerator()) / rInterval_.denominator() * 1'000'000'000.0);
  if (intervalNs <= 0) return;

  const auto gapDuration = static_cast<size_t>(elapsedNs / intervalNs);
  if (gapDuration > 0) metaData_->onTransmissionGap(gapDuration);
}

boost::rational<int> storage::getSamplingInterval() const { return rInterval_; }

}  // namespace rdb
