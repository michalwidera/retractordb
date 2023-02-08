#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

#include <cstddef>  // std::byte
#include <memory>   // std::unique_ptr
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

class storageAccessor {
  std::unique_ptr<FileAccessorInterface<std::byte>> accessor;
  Descriptor descriptor;
  bool reverse = false;
  bool removeOnExit = true;
  size_t recordsCount = 0;
  std::string descriptorFile;
  std::string storageFile;
  std::string storageType = "DEFAULT";
  std::byte *payloadPtr = nullptr;
  void moveRef();

 public:
  storageAccessor() = delete;
  storageAccessor(std::string fileName);
  ~storageAccessor();

  storageState dataFileStatus = storageState::noDescriptor;

  void attachDescriptor(const Descriptor *descriptor = nullptr);
  void attachStorage();

  void attachPayloadPtr(std::byte *payloadPtrVal);
  void attachPayload(rdb::payload &payloadRef);

  bool read(const size_t recordIndex);
  bool write(const size_t recordIndex = std::numeric_limits<size_t>::max());

  Descriptor &getDescriptor();

  void setReverse(bool value);
  void setRemoveOnExit(bool value);
  const size_t getRecordsCount();
  bool peekStorage();
  bool peekDescriptor();
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DACC_H_