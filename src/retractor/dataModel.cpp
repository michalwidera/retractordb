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

dataInstance::dataInstance(                   //
    const std::string file,                   //
    const rdb::Descriptor storageDescriptor,  //
    const rdb::Descriptor internalDescriptor) {
  storagePayload = std::make_unique<rdb::payload>(storageDescriptor);
  internalPayload = std::make_unique<rdb::payload>(internalDescriptor);

  storage = std::make_unique<rdb::storageAccessor>(file);
  storage->attachStorage();
  storage->createDescriptor(storageDescriptor);
  storage->attachPayload(*storagePayload);

  {
    std::stringstream strStream;
    strStream << storagePayload->getDescriptor();
    SPDLOG_INFO("storage/external descriptor: {}", removeSpc(removeCRLF(strStream.str())));
  }
  {
    std::stringstream strStream;
    strStream << internalPayload->getDescriptor();
    SPDLOG_INFO("image/internal descriptor: {}", removeSpc(removeCRLF(strStream.str())));
  }
};

/*
dataInstance::dataInstance(                   //
    const std::string file,                   //
    const rdb::Descriptor storageDescriptor   //
) {
  storagePayload = std::make_unique<rdb::payload>(storageDescriptor);
}
*/

dataInstance::dataInstance(query &qry)
    :                                           //
      dataInstance(qry.filename,                //
                   qry.descriptorFrom(),        //
                   qry.descriptorExpression())  //
      {};
