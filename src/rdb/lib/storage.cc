#include "rdb/storage.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

#include <cstring>  //std::memset
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include "fatalError.hpp"
#include "rdb/storageShadow.hpp"

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
      std::cerr << "Provided Descriptor:\n" << *descriptorParam << "\nExisting Descriptor:\n" << descriptor << '\n';
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
  auto it = std::ranges::find_if(descriptor,  //
                                 [](const auto &item) { return item.rtype == rdb::REF; });

  // Descriptor changes storageFile location
  if (it != descriptor.end()) {
    storageFile_   = (*it).rname;
    metaIndexFile_ = storageFile_ + ".meta";
  }

  // if storage object was created with default storage as ""
  // and there is no specified storage as REF in descriptor - we should
  // stop immediately.
  if (storageFile_.empty()) {
    FatalError("storage: storage file not set in descriptor (missing REF field or :STORAGE directive)");
  }
}

void storage::attachStorage() {
  if (storageFile_.empty()) FatalError("storage: storageFile_ is empty — storage not properly configured");

  auto it1 = std::ranges::find_if(descriptor,  //
                                  [](const auto &item) { return item.rtype == rdb::TYPE; });

  if (it1 != descriptor.end()) {
    storageType_ = (*it1).rname;
  }

  initializeAccessor();

  if (isDeclared()) {
    // Źródła deklarowane są tylko do odczytu — wstrzykiwany jest wariant inertny (pusta ścieżka = bez persystencji).
    metaData_ = std::make_unique<rdb::metaData>(descriptor, "");
    return;
  }

  recordsCount_ = accessor_->count();

  // Wstrzyknięcie wariantu metadanych. Posiadanie pliku cienia danych (accessor_->hasShadow()) jest
  // niezależne od posiadania metaindeksu: cień chroni oryginalną zarejestrowaną zawartość danych,
  // metaindeks rejestruje wartości null i przerwy w transmisji. Magazyny z cieniem danych dostają
  // storageShadow (aktualizacje → cień indeksu .meta.shadow); pozostałe — bazowy metaData.
  if (accessor_->hasShadow()) {
    metaData_ = std::make_unique<rdb::storageShadow>(descriptor, metaIndexFile_);
  } else {
    metaData_ = std::make_unique<rdb::metaData>(descriptor, metaIndexFile_);
  }
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
    if (!storageFile_.empty()) (void)remove(storageFile_.c_str());
    if (descriptorFileExist()) remove(descriptorFile_.c_str());
    if (!metaIndexFile_.empty() && std::filesystem::exists(metaIndexFile_)) remove(metaIndexFile_.c_str());
    const std::string metaShadowFile = storageShadow::metaShadowFilePath(metaIndexFile_);
    if (!metaIndexFile_.empty() && std::filesystem::exists(metaShadowFile)) remove(metaShadowFile.c_str());
  }
}

bool storage::isDeclared() const { return (storageType_ == "DEVICE") || (storageType_ == "TEXTSOURCE"); }

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
  if (storageFile_.empty()) FatalError("storage: storageFile_ is empty — storage not properly configured");

  if (!accessor_) return;  // no accessor initialized - no need to reset.

  auto resourceAlreadyExist = std::filesystem::exists(storageFile_);
  if (resourceAlreadyExist)
    if (!isDeclared()) remove(storageFile_.c_str());

  initializeAccessor();

  accessor_->write(nullptr, 0);
  recordsCount_ = 0;

  if (metaData_) metaData_->reset();

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
  if (circularBuffer_.capacity() == 0) FatalError("storage::fire: circular buffer capacity is zero");
  *storagePayload_ = *chamber_;
  recordsCount_++;
  circularBuffer_.push_front(*storagePayload_);  // only one place when buffer is feed.
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

    bufferState        = sourceState::armed;
    *(storagePayload_) = *chamber_;
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

  *(storagePayload_) = circularBuffer_[recordIndexFromBack];
  return true;
}

void storage::setCapacity(const int capacity) {
  if (isDeclared()) circularBuffer_.set_capacity(capacity == 0 ? 1 : capacity);
}

bool storage::write(const size_t recordIndex) {
  abortIfStorageNotPrepared();
  const auto nullInfo = storagePayload_->getNullBitset();

  // Maszyna detekcji gap żyje w metaData: rekord all-null poza fazą nullfill jest pochłaniany
  // (nie trafia do fizycznego magazynu), a jego brak zostanie oznaczony wpisem gap.
  if (recordIndex >= recordsCount_ && metaData_->absorbAppend(nullInfo)) return true;

  if (recordsCount_ != accessor_->count()) {
    FatalError("storage: internal record count mismatch: recordsCount_={} count()={} in {}", recordsCount_, accessor_->count(),
               storageFile_);
  }

  ssize_t result = 0;
  if (recordIndex >= recordsCount_) {
    result = accessor_->write(storagePayload_->span().data());  // <- Call to append Function
    if (result != 0) {
      FatalError("storage::write: append to '{}' failed (result={})", storageFile_, result);
    }
    recordsCount_++;

    metaData_->onRecordAppended(nullInfo);
    metaData_->flushCurrentEntry();
  } else {
    result = accessor_->write(storagePayload_->span().data(), recordIndex * descriptor.getSizeInBytes());
    if (result != 0) {
      FatalError("storage::write: overwrite to '{}' at index {} failed (result={})", storageFile_, recordIndex, result);
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
  if (gapDuration > 0) metaData_->onTransmissionGap(gapDuration);
}

boost::rational<int> storage::getSamplingInterval() const { return rInterval_; }

}  // namespace rdb
