#include "rdb/memoryStore.hpp"

namespace rdb {

MemoryStore &MemoryStore::processDefault() {
  static MemoryStore instance;
  return instance;
}

}  // namespace rdb
