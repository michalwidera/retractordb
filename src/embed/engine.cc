#include "rdb/embed/engine.hpp"

#include "fatalError.hpp"

namespace rdb::embed {

std::unique_ptr<storage> Engine::openStorage(const std::string_view qryID,         //
                                             const std::string_view fileName,      //
                                             const std::string_view storageParam,  //
                                             const std::string_view storageType,   //
                                             const bool oneShot,                   //
                                             const bool isHold,                    //
                                             const int percounter) {
  const ThrowOnFatalScope throwOnFatal;
  return std::make_unique<storage>(qryID, fileName, storageParam, storageType, oneShot, isHold, percounter, &memory_);
}

}  // namespace rdb::embed
