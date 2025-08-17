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
  std::unique_ptr<FileAccessorInterface> accessor;
  std::unique_ptr<rdb::payload> storagePayload;
  std::unique_ptr<rdb::payload> chamber;
  Descriptor descriptor;
  bool removeOnExit          = true;
  size_t recordsCount        = 0;
  std::string descriptorFile = "";
  std::string storageFile    = "";
  std::string storageType    = "DEFAULT";

  void moveRef();
  void attachStorage();

  boost::circular_buffer<rdb::payload> circularBuffer{0};

  void abortIfStorageNotPrepared();
  void initializeAccessor();

  // Read data from storage described as accessor
  // if var:destination is null read into storageAccessor payload
  bool read_();  // read from device into chamber

 public:
  storageAccessor() = delete;
  explicit storageAccessor(const std::string fileNameDesc, const std::string fileName = "");
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

  void setRemoveOnExit(bool value);
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