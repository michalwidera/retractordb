#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

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

class storageAccessor {
  std::unique_ptr<FileAccessorInterface<uint8_t>> accessor;
  std::unique_ptr<rdb::payload> storagePayload;
  Descriptor descriptor;
  bool reverse = false;
  bool removeOnExit = true;
  bool removeDescriptorOnExit = false;
  size_t recordsCount = 0;
  std::string descriptorFile;
  std::string storageFile;
  std::string storageType = "DEFAULT";
  void moveRef();
  void attachStorage();

 public:
  storageAccessor() = delete;
  storageAccessor(std::string fileNameDesc, std::string fileName = "");
  ~storageAccessor();

  storageState dataFileStatus = storageState::noDescriptor;

  void attachDescriptor(const Descriptor *descriptor = nullptr);

  bool read(const size_t recordIndex);
  bool write(const size_t recordIndex = std::numeric_limits<size_t>::max());
  bool readReverse(const size_t recordIndex);

  Descriptor &getDescriptor();
  std::unique_ptr<rdb::payload>::pointer getPayload();

  void setReverse(bool value);
  void setRemoveOnExit(bool value);
  void setRemoveDescriptorOnExit(bool value);
  const size_t getRecordsCount();
  bool peekStorage();
  bool peekDescriptor();

  std::string getStorageName();
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DACC_H_