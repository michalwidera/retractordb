#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

#include <boost/circular_buffer.hpp>
#include <memory>  // std::unique_ptr
#include <string>

#include "descriptor.h"
#include "faccbindev.h"
#include "faccfs.h"
#include "faccposix.h"
#include "faccposixprm.h"
#include "facctxtsrc.h"
#include "fainterface.h"
#include "payload.h"

namespace rdb {
enum class storageState { noDescriptor, attachedDescriptor, openExisting, openAndCreate };
enum class policyState { flux, freeze };

class storageAccessor {
  std::unique_ptr<FileAccessorInterface<uint8_t>> accessor;
  std::unique_ptr<rdb::payload> storagePayload;
  Descriptor descriptor;
  bool removeOnExit = true;
  size_t recordsCount = 0;
  size_t recordsSequence = 0;
  std::string descriptorFile = "";
  std::string storageFile = "";
  std::string storageType = "DEFAULT";
  void moveRef();
  void attachStorage();

  boost::circular_buffer<rdb::payload> circularBuffer{0};

  void abortIfStorageNotPrepared();
  void initializeAccessor();

 public:
  storageAccessor() = delete;
  storageAccessor(std::string fileNameDesc, std::string fileName = "");
  ~storageAccessor();

  storageState dataFileStatus = storageState::noDescriptor;
  policyState bufferPolicy = policyState::flux;

  void attachDescriptor(const Descriptor *descriptor = nullptr);

  // Read data from storage described as accessor
  // if var:destination is null read into storageAccessor payload
  bool read_(const size_t recordIndex, uint8_t *destination = nullptr);
  bool write(const size_t recordIndex = std::numeric_limits<size_t>::max());
  bool revRead(const size_t recordIndex, uint8_t *destination = nullptr);

  Descriptor &getDescriptor();
  std::unique_ptr<rdb::payload>::pointer getPayload();

  void setRemoveOnExit(bool value);
  const size_t getRecordsCount();
  size_t getRecordsSequence();
  bool descriptorFileExist();

  std::string getStorageName();

  bool isDeclared();

  void setCapacity(const int capacity);

  // technical function - for unit tests
  void reset();
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DACC_H_