#include "dataModel.h"

#include <cassert>
#include <regex>

#include "QStruct.h"  // coreInstance
#include "spdlog/spdlog.h"

// ctest -R '^unittest-test-schema'

extern "C" qTree coreInstance;

enum { noHexFormat = false, HexFormat = true };

std::string removeCRLF(std::string input) { return std::regex_replace(input, std::regex("\\r\\n|\\r|\\n"), ""); }

streamInstance::streamInstance(         //
    const std::string file,             //
    const rdb::Descriptor descStorage,  //
    const rdb::Descriptor descInternal  //
) {
  storage = std::make_unique<rdb::DataStorageAccessor<>>(file);
  storage->createDescriptor(descStorage);
  accessorStorage = std::make_unique<rdb::payLoadAccessor>(storage->getDescriptor(), storage->payload.get());

  payloadInternal = std::make_unique<std::byte[]>(descInternal.getSizeInBytes());
  accessorInternal = std::make_unique<rdb::payLoadAccessor>(descInternal, payloadInternal.get(), noHexFormat);

  {
    std::stringstream strStream;
    strStream << storage->getDescriptor();
    SPDLOG_INFO("storage/external descriptor: {}", removeCRLF(strStream.str()));
  }
  {
    std::stringstream strStream;
    strStream << accessorInternal->getDescriptor();
    SPDLOG_INFO("image/internal descriptor: {}", removeCRLF(strStream.str()));
  }
};
