#ifndef EXTERMALACC_RDB_INCLUDE_DACC_H_
#define EXTERMALACC_RDB_INCLUDE_DACC_H_

#include <cstddef>  // std::byte
#include <string>

#include "descriptor.h"
#include "payload.h"

namespace rdb {
class externalAccessor {
  Descriptor descriptor;
  std::string filename;
  std::byte *payloadPtr;

 public:
  externalAccessor() = delete;
  externalAccessor(std::string fileName);

  void attachDescriptor(const Descriptor descriptor);

  void attachPayloadPtr(std::byte *payloadPtrVal);
  void attachPayload(rdb::payload &payloadRef);

  bool read();

  Descriptor &getDescriptor();
};
}  // namespace rdb

#endif
