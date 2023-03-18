#include "dataModel.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <regex>

#include "QStruct.h"  // coreInstance

extern "C" qTree coreInstance;

// ctest -R '^unittest-test-schema'

enum { noHexFormat = false, HexFormat = true };

std::string removeCRLF(std::string input) { return std::regex_replace(input, std::regex("\\r\\n|\\r|\\n"), ""); }

std::string removeSpc(std::string input) { return std::regex_replace(input, std::regex(R"(\s+)"), " "); }

streamInstance::streamInstance(               //
    const std::string descriptorName,         //
    const std::string storageNameParam,       //
    const rdb::Descriptor storageDescriptor,  //
    const rdb::Descriptor internalDescriptor) {
  // only objects with REF has storageNameParam filled.
  const auto storageName{storageNameParam == "" ? descriptorName : storageNameParam};
  SPDLOG_INFO("streamInstance desc:{} storage:{}", descriptorName, storageName);
  storagePayload = std::make_unique<rdb::payload>(storageDescriptor);
  internalPayload = std::make_unique<rdb::payload>(internalDescriptor);

  storage = std::make_unique<rdb::storageAccessor>(descriptorName, storageName);
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

streamInstance::streamInstance(                //
    const std::string idAndStorageName,        //
    const rdb::Descriptor storageDescriptor,   //
    const rdb::Descriptor internalDescriptor)  //
    : streamInstance(idAndStorageName,         // descriptor file
                     idAndStorageName,         // storage file
                     storageDescriptor,        //
                     internalDescriptor        //
      ) {
  SPDLOG_INFO("streamInstance - storage and id are the same");
}

streamInstance::streamInstance(query& qry)
    : streamInstance(qry.id,                     // descriptor file
                     qry.filename,               // storage file
                     qry.descriptorFrom(),       //
                     qry.descriptorExpression()  //
      ) {
  SPDLOG_INFO("streamInstance <- qry");
};

dataModel::dataModel(/* args */) {}

dataModel::~dataModel() {}

void dataModel::load(std::string compiledQueryFile) {
  std::ifstream ifs(compiledQueryFile.c_str(), std::ios::binary);
  assert(ifs);
  //
  // Load of compiled query from file
  //
  boost::archive::text_iarchive ia(ifs);
  ia >> coreInstance;
}

void dataModel::prepare() {
  //
  // Special parameters support in query set
  // fetch all ':*' - and remove them from coreInstance
  //
  for (auto qry : coreInstance)
    if (qry.id == ":STORAGE") storagePath = qry.filename;

  auto new_end = std::remove_if(coreInstance.begin(), coreInstance.end(),  //
                                [](const query& qry) { return qry.id[0] == ':'; });
  coreInstance.erase(new_end, coreInstance.end());

  SPDLOG_INFO("Create struct on CORE INSTANCE");

  for (auto& qry : coreInstance) qSet.emplace(qry.id, std::make_unique<streamInstance>(qry));
  for (auto const& [key, val] : qSet) val->storage->setRemoveOnExit(false);
}

void dataModel::computeInstance(std::string instance) {
  auto qry = coreInstance[instance];
  const command_id cmd = qry.lProgram.end()->getCommandID();
  switch (cmd) {
    case PUSH_STREAM: {
      // store in internal payload data from argument payload
      auto argumentQueryName = qry.lProgram.end()->getStr_();
      *(qSet[instance]->internalPayload) = *(qSet[argumentQueryName]->storagePayload);
      // invocation of payload &payload::operator=(payload &other) from payload.cc
      // TODO: Add check if storagePayload is empty or argumentQueryName exist?
    } break;
    case STREAM_TIMEMOVE:
    case STREAM_DEHASH_MOD:
    case STREAM_DEHASH_DIV:
    case STREAM_SUBSTRACT:
    case STREAM_AVG:
    case STREAM_MIN:
    case STREAM_MAX:
    case STREAM_SUM:
    case STREAM_ADD:
    case STREAM_AGSE:
    case STREAM_HASH:
    default:
      SPDLOG_ERROR("Undefined command_id:{}", cmd);
      abort();
  }
}
