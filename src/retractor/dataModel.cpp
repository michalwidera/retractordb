#include "dataModel.h"

#include <cassert>
#include <regex>

#include "QStruct.h"  // coreInstance
#include "spdlog/spdlog.h"

// ctest -R '^unittest-test-schema'

extern "C" qTree coreInstance;

enum { noHexFormat = false, HexFormat = true };

std::string removeCRLF(std::string input) { return std::regex_replace(input, std::regex("\\r\\n|\\r|\\n"), ""); }

std::string removeSpc(std::string input) { return std::regex_replace(input, std::regex(R"(\s+)"), " "); }

streamInstance::streamInstance(               //
    const std::string file,                   //
    const rdb::Descriptor storageDescriptor,  //
    const rdb::Descriptor internalDescriptor  //
) {
  storage = std::make_unique<rdb::storageAccessor<>>(file);
  storage->createDescriptor(storageDescriptor);
  storagePayload = std::make_unique<rdb::payload>(storageDescriptor);
  InternalPayload = std::make_unique<rdb::payload>(internalDescriptor);

  {
    std::stringstream strStream;
    strStream << storagePayload->getDescriptor();
    SPDLOG_INFO("storage/external descriptor: {}", removeSpc(removeCRLF(strStream.str())));
  }
  {
    std::stringstream strStream;
    strStream << InternalPayload->getDescriptor();
    SPDLOG_INFO("image/internal descriptor: {}", removeSpc(removeCRLF(strStream.str())));
  }
};
