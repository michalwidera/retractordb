#include "dataModel.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <regex>

#include "QStruct.h"  // coreInstance

// ctest -R '^unittest-test-schema'

enum { noHexFormat = false, HexFormat = true };

std::string removeCRLF(std::string input) { return std::regex_replace(input, std::regex("\\r\\n|\\r|\\n"), ""); }

std::string removeSpc(std::string input) { return std::regex_replace(input, std::regex(R"(\s+)"), " "); }

dataInstance::dataInstance(                   //
    const std::string descriptorname,         //
    const std::string storagenameParam,       //
    const rdb::Descriptor storageDescriptor,  //
    const rdb::Descriptor internalDescriptor) {
  // only objects with REF has storagenameParam filled.
  const auto storagename{storagenameParam == "" ? descriptorname : storagenameParam};
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

dataInstance::dataInstance(                    //
    const std::string idAndStorageName,        //
    const rdb::Descriptor storageDescriptor,   //
    const rdb::Descriptor internalDescriptor)  //
    : dataInstance(idAndStorageName,           // descriptor file
                   idAndStorageName,           // storage file
                   storageDescriptor,          //
                   internalDescriptor          //
      ) {
  SPDLOG_INFO("dataInstance - storage and id are the same");
}

dataInstance::dataInstance(query &qry)
    : dataInstance(qry.id,                     // descriptor file
                   qry.filename,               // storage file
                   qry.descriptorFrom(),       //
                   qry.descriptorExpression()  //
      ) {
  SPDLOG_INFO("dataInstance <- qry");
};
