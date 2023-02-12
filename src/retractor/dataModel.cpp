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
    const std::string descriptorname,         //
    const std::string storagename,            //
    const rdb::Descriptor storageDescriptor,  //
    const rdb::Descriptor internalDescriptor) {
  SPDLOG_INFO("dataInstance desc:{} storage:{}", descriptorname, storagename);
  storagePayload = std::make_unique<rdb::payload>(storageDescriptor);
  internalPayload = std::make_unique<rdb::payload>(internalDescriptor);

  storage = std::make_unique<rdb::storageAccessor>(descriptorname, storagename);
  storage->attachDescriptor(&storageDescriptor);
  storage->attachStorage();
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

dataInstance::dataInstance(query &qry)
    :                                          //
      dataInstance(qry.id,                     // descriptor file
                   qry.filename,               // storage file
                   qry.descriptorFrom(),       //
                   qry.descriptorExpression()  //
                   )                           //
{
  SPDLOG_INFO("dataInstance -> qry");
};
