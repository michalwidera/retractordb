#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

#include <cstddef>  // std::byte
#include <memory>   // std::unique_ptr
#include <string>

#include "descriptor.h"
#include "faccfs.h"
#include "faccposix.h"
#include "faccposixprm.h"
#include "fainterface.h"
#include "payload.h"

namespace rdb {
enum class storageState { noDescriptor, openExisting, openAndCreate };

class storageAccessor {
  std::unique_ptr<FileAccessorInterface<std::byte>> accessor;
  Descriptor descriptor;
  bool reverse = false;
  bool removeOnExit;
  size_t recordsCount;
  std::string filename;
  std::byte *payloadPtr;
  Descriptor *descriptorParam;

 public:
  storageAccessor() = delete;
  storageAccessor(std::string fileName, Descriptor* descriptorParam = nullptr);
  ~storageAccessor();

  storageState dataFileStatus;

  void createDescriptor(const Descriptor descriptor);

  void attachStorage();

  void attachPayloadPtr(std::byte *payloadPtrVal);
  void attachPayload(rdb::payload &payloadRef);

  bool read(const size_t recordIndex);
  bool write(const size_t recordIndex = std::numeric_limits<size_t>::max());

  Descriptor &getDescriptor();

  void setReverse(bool value);
  void setRemoveOnExit(bool value);
  const size_t getRecordsCount();
  bool storageAlreadyExisting();
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DACC_H_