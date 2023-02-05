#ifndef EXTERMALACC_RDB_INCLUDE_DACC_H_
#define EXTERMALACC_RDB_INCLUDE_DACC_H_

#include <string>
#include <cstddef>  // std::byte

#include "descriptor.h"

namespace rdb {
class externalAccessor {
  std::string filename;
  std::byte *payloadPtr;
 public:
  externalAccessor() = delete;
  externalAccessor(std::string fileName);

  void createDescriptor(const Descriptor descriptor);
  bool read();

  Descriptor &getDescriptor();
};
}  // namespace rdb

#endif