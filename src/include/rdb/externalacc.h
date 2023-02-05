#ifndef EXTERMALACC_RDB_INCLUDE_DACC_H_
#define EXTERMALACC_RDB_INCLUDE_DACC_H_

#include <string>

namespace rdb {
class externalAccessor {
 public:
  externalAccessor() = delete;
  externalAccessor(std::string fileName);
};
}  // namespace rdb

#endif