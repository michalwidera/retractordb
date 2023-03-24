#include "dataModel.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <regex>

#include "QStruct.h"  // coreInstance
#include "SOperations.h"
#include "iostream"

extern "C" qTree coreInstance;

// ctest -R '^ut-dataModel'

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
  fromPayload = std::make_unique<rdb::payload>(internalDescriptor);

  storage = std::make_unique<rdb::storageAccessor>(descriptorName, storageName);
  storage->attachDescriptor(&storageDescriptor);

  {
    std::stringstream strStream;
    strStream << storage->getDescriptor();
    SPDLOG_INFO("storage/external descriptor: {}", removeSpc(removeCRLF(strStream.str())));
  }
  {
    std::stringstream strStream;
    strStream << fromPayload->getDescriptor();
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

// TODO: TEST THIS FREAKING CODE ... ASAP
// ! Work in progress
// https://en.cppreference.com/w/cpp/numeric/math/div
void streamInstance::constructPayload(int offset, int length) {
  // First construct descriptor
  rdb::Descriptor descriptor;
  auto descriptorVecSize = storage->getDescriptor().size();
  for (auto i = offset; i < offset + length; i++) {
    descriptor | rdb::Descriptor{storage->getDescriptor()[std::div(i, descriptorVecSize).rem]};
  }
  // Second construct payload
  localPayload = std::make_unique<rdb::payload>(descriptor);
  std::cerr << "DESC:" << descriptor << std::endl;
  for (auto i : descriptor) std::cout << rdb::GetStringdescFld(std::get<rdb::rtype>(i)) << std::endl;
  // 3rd - fill payload with data from storage
  auto prevQuot = 0;
  for (auto i = 0; i < length; i++) {
    auto dv = std::div(i + offset, descriptorVecSize);
    std::cerr << i << "," << dv.quot << "," << dv.rem << std::endl;
    if (prevQuot != dv.quot) {
      prevQuot = dv.quot;
      std::cerr << "ReadReverse:" << dv.quot - 1 << std::endl;
      // storage->readReverse(dv.quot-1);
      // TODO: fix non existing data
    }
    std::cerr << "setItem:" << dv.rem << std::endl;

    std::any value = storage->getPayload()->getItem(dv.rem);
    try {
      localPayload->setItem(dv.rem, value);
    } catch (const std::bad_any_cast& e) {
      rdb::rfield p1{storage->getPayload()->getDescriptor()[dv.rem]};
      rdb::rfield p2{localPayload->getDescriptor()[dv.rem]};
      SPDLOG_ERROR("Bad cast in constructPayload {} -> {}",          //
                   rdb::GetStringdescFld(std::get<rdb::rtype>(p1)),  //
                   rdb::GetStringdescFld(std::get<rdb::rtype>(p2)));
    }
    // TODO: fix bad any_cast - intro checkUniform function?
  }
}

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

std::unique_ptr<rdb::payload>::pointer dataModel::getPayload(std::string instance, int offset) {
  qSet[instance]->storage->readReverse(offset);
  return qSet[instance]->storage->getPayload();
}

void dataModel::computeInstance(std::string instance) {
  auto qry = coreInstance[instance];
  token arg[3];
  int i = 0;
  for (auto tk : qry.lProgram) arg[i++] = tk;

  const command_id cmd = qry.lProgram.end()->getCommandID();
  switch (cmd) {
    case PUSH_STREAM: {
      // store in internal payload data from argument payload
      auto argumentQueryName = qry.lProgram.end()->getStr_();
      *(qSet[instance]->fromPayload) = *getPayload(instance);
      // invocation of payload &payload::operator=(payload &other) from payload.cc
      // TODO: Add check if storagePayload is empty or argumentQueryName exist?
    } break;
    case STREAM_TIMEMOVE: {
      // store in internal payload data from argument payload
      auto argumentQueryName = qry.lProgram.end()->getStr_();
      *(qSet[instance]->fromPayload) = *getPayload(instance, rational_cast<int>(arg[1].get()));
      // invocation of payload &payload::operator=(payload &other) from payload.cc
      // TODO: Add check if storagePayload is empty or argumentQueryName exist?
    } break;
    case STREAM_DEHASH_MOD:
    case STREAM_DEHASH_DIV: {
      auto streamNameArg = arg[0].getStr_();
      assert(streamNameArg != "");
      auto rationalArgument = arg[1].get();
      assert(rationalArgument > 0);
      // q.id - name of output stream
      // size[q.id] - count of record in output stream
      // q.rInterval - delta of output stream (rational) - contains
      // deltaDivMod( core0.delta , rationalArgument ) rationalArgument -
      // (2/3) argument of operation (rational)
      int newTimeOffset = -1;  // catch on assert(false);
      if (cmd == STREAM_DEHASH_DIV) newTimeOffset = Div(qry.rInterval, rationalArgument, 0);
      if (cmd == STREAM_DEHASH_MOD) newTimeOffset = Mod(rationalArgument, qry.rInterval, 0);
      if (newTimeOffset < 0) assert(false);
      *(qSet[instance]->fromPayload) = *getPayload(instance, newTimeOffset);
    } break;
    case STREAM_SUBSTRACT:
    case STREAM_AVG:
    case STREAM_MIN:
    case STREAM_MAX:
      *(qSet[instance]->fromPayload) = *getPayload(arg[0].getStr_());
      break;
    case STREAM_SUM:
      assert(false && "TODO");
    case STREAM_ADD:
      *(qSet[instance]->fromPayload) = *getPayload(arg[0].getStr_()) + *getPayload(arg[1].getStr_());
      // operator + from payload payload::operator+(payload &other) step into action here
      // TODO support renaming of double-same fields after merge
      break;
    case STREAM_AGSE:
      assert(false && "TODO");
    case STREAM_HASH:
      assert(false && "TODO");
    default:
      SPDLOG_ERROR("Undefined command_id:{}", cmd);
      abort();
  }
}
