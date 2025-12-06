#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

#include <boost/circular_buffer.hpp>
#include <memory>  // std::unique_ptr
#include <string>

#include "descriptor.h"
#include "faccbindev.h"
#include "faccfs.h"
#include "faccmemory.h"
#include "faccposix.h"
#include "facctxtsrc.h"
#include "fagrp.h"
#include "fainterface.h"
#include "payload.h"

namespace rdb {
enum class storageState { noDescriptor, attachedDescriptor, openAndCreate };
enum class sourceState { empty, flux, lock, armed };

class storageAccessor {
  std::unique_ptr<FileAccessorInterface> accessor_;
  std::unique_ptr<rdb::payload> storagePayload_;
  std::unique_ptr<rdb::payload> chamber_;
  Descriptor descriptor_;
  bool isDisposable_          = false;  // if true - storage and descriptor will be deleted after use
  bool isOneShot_             = false;  // if false - storage will be looped when end is reached
  size_t recordsCount_        = 0;
  std::string descriptorFile_ = "";
  std::string storageFile_    = "";
  std::string storageType_    = "DEFAULT";
  int percounter_             = -1;

  void moveRef();
  void attachStorage();

  boost::circular_buffer<rdb::payload> circularBuffer_{0};

  void abortIfStorageNotPrepared();
  void initializeAccessor();
  void readToChamber();

 public:
  storageAccessor() = delete;
  explicit storageAccessor(const std::string qryID,              //
                           const std::string fileName,           //
                           const std::string_view storageParam,  //
                           bool oneShot   = false,               //
                           int percounter = -1                   //
  );
  virtual ~storageAccessor();

  storageState dataFileStatus = storageState::noDescriptor;
  sourceState bufferState     = sourceState::empty;  // ? test lock

  void attachDescriptor(const Descriptor *descriptor = nullptr);

  bool write(const size_t recordIndex = std::numeric_limits<size_t>::max());
  bool revRead(const size_t recordIndexFromBack, uint8_t *destination = nullptr);
  bool read(const size_t recordIndexFromFront, uint8_t *destination = nullptr);
  void fire();
  void purge();

  Descriptor &getDescriptor();
  std::unique_ptr<rdb::payload>::pointer getPayload();

  void setDisposable(bool value);
  size_t getRecordsCount();
  bool descriptorFileExist();

  std::string getStorageName();

  bool isDeclared();

  void setCapacity(const int capacity);
  void cleanPayload(uint8_t *destination = nullptr);

  // technical function - for unit tests
  void reset();
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DACC_H_