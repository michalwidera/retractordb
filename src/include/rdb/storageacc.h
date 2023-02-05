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

template <class K = rdb::posixPrmBinaryFileAccessor<std::byte>>
class storageAccessor {
  std::unique_ptr<K> accessor;
  Descriptor descriptor;
  bool reverse = false;
  bool removeOnExit;
  size_t recordsCount;
  std::string filename;
  std::byte *payloadPtr;

 public:
  storageAccessor() = delete;
  storageAccessor(std::string fileName);
  ~storageAccessor();

  storageState dataFileStatus;

  void createDescriptor(const Descriptor descriptor);

  void attachPayloadPtr(std::byte *payloadPtrVal);
  void attachPayload(rdb::payload &payloadRef);

  bool read(const size_t recordIndex);
  bool write(const size_t recordIndex = std::numeric_limits<size_t>::max());

  Descriptor &getDescriptor();

  void setReverse(bool value);
  void setRemoveOnExit(bool value);
  const size_t getRecordsCount();
  const std::string getStorageFilename();
  bool storageAlreadyExisting();
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DACC_H_