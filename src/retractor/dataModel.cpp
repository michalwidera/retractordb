#include "dataModel.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <cstdlib>  // std::div
#include <regex>

#include "QStruct.h"  // coreInstance
#include "SOperations.h"
#include "iostream"

extern "C" qTree coreInstance;

// ctest -R '^ut-dataModel'

enum { noHexFormat = false, HexFormat = true };

/*
std::string removeCRLF(std::string input) { return std::regex_replace(input, std::regex("\\r\\n|\\r|\\n"), ""); }
std::string removeSpc(std::string input) { return std::regex_replace(input, std::regex(R"(\s+)"), " "); }
*/

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
    strStream << rdb::flat << storage->getDescriptor();
    SPDLOG_INFO("storage/external descriptor: {}", strStream.str());
  }
  {
    std::stringstream strStream;
    strStream << rdb::flat << fromPayload->getDescriptor();
    SPDLOG_INFO("image/internal descriptor: {}", strStream.str());
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
rdb::payload streamInstance::constructAgsePayload(int offset, int length) {
  // First construct descriptor
  rdb::Descriptor descriptor;
  auto descriptorVecSize = storage->getDescriptor().size();
  std::string storage_name = storage->getStorageName();
  for (auto i = offset; i < offset + length; i++) {
    rdb::rfield f{storage->getDescriptor()[std::div(i, descriptorVecSize).rem]};
    rdb::rfield x{std::make_tuple(storage_name + "_" + std::to_string(i),  //
                                  std::get<rdb::rlen>(f),                  //
                                  std::get<rdb::rtype>(f))};
    descriptor | rdb::Descriptor{x};
  }

  // Second construct payload
  std::unique_ptr<rdb::payload> localPayload = std::make_unique<rdb::payload>(descriptor);

  // std::cerr << "DESC:" << descriptor << std::endl;
  // for (auto i : descriptor) std::cout << rdb::GetStringdescFld(std::get<rdb::rtype>(i)) << std::endl;
  // 3rd - fill payload with data from storage
  auto prevQuot = 0;
  for (auto i = 0; i < length; i++) {
    auto dv = std::div(i + offset, descriptorVecSize);
    if (prevQuot != dv.quot) {
      prevQuot = dv.quot;
      storage->readReverse(dv.quot);
      // TODO: fix non existing data
    }

    auto locSrc = dv.rem;
    auto locDst = i;

    assert(i < descriptor.size());

    std::any value = storage->getPayload()->getItem(locSrc);
    localPayload->setItem(locDst, value);

    // TODO: fix bad any_cast - intro checkUniform function?
  }
  storage->readReverse(0);

  return *(localPayload.get());
}

dataModel::dataModel(/* args */) {}

dataModel::~dataModel() {}

void dataModel::loadCoreInstance(std::string compiledQueryFile) {
  std::ifstream ifs(compiledQueryFile.c_str(), std::ios::binary);
  assert(ifs);
  //
  // Load of compiled query from file
  //
  boost::archive::text_iarchive ia(ifs);
  ia >> coreInstance;
}

void dataModel::prepareQSet() {
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

std::unique_ptr<rdb::payload>::pointer dataModel::getPayload(std::string instance, int revOffset) {
  qSet[instance]->storage->readReverse(revOffset);
  return qSet[instance]->storage->getPayload();
}

void dataModel::computeInstance(std::string instance) {
  auto qry = coreInstance[instance];

  assert(qry.lProgram.size() < 4 && "all stream programs must be after optimization");

  token arg[3];
  int i = 0;
  for (auto tk : qry.lProgram) arg[i++] = tk;

  auto operation = qry.lProgram.back();  // Operation is always last element on stack

  const command_id cmd = operation.getCommandID();
  switch (cmd) {
    case PUSH_STREAM: {
      // store in internal payload data from argument payload
      auto argumentQueryName = operation.getStr_();
      *(qSet[instance]->fromPayload) = *getPayload(instance);
      // invocation of payload &payload::operator=(payload &other) from payload.cc
      // TODO: Add check if storagePayload is empty or argumentQueryName exist?
    } break;
    case STREAM_TIMEMOVE: {
      // store in internal payload data from argument payload
      auto argumentQueryName = operation.getStr_();
      *(qSet[instance]->fromPayload) = *getPayload(instance, rational_cast<int>(arg[1].getRI()));
      // invocation of payload &payload::operator=(payload &other) from payload.cc
      // TODO: Add check if storagePayload is empty or argumentQueryName exist?
    } break;
    case STREAM_DEHASH_MOD:
    case STREAM_DEHASH_DIV: {
      auto streamNameArg = arg[0].getStr_();
      assert(streamNameArg != "");
      auto rationalArgument = arg[1].getRI();
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
    case STREAM_AGSE: {                                      // (program sequence)
      bool mirror = operation.getRI() < 0;                   // PUSH_STREAM core -> delta_source (arg[0]) - operation
      int length = abs(rational_cast<int>(arg[1].getRI()));  // PUSH_VAL 2 -> window_length (arg[1])
      assert(length > 0);                                    //
      int step = rational_cast<int>(arg[2].getRI());         // STREAM_AGSE 3 -> window_step (arg[2])
      assert(step >= 0);                                     //

      *(qSet[instance]->fromPayload) = qSet[instance]->constructAgsePayload(step, length);
    } break;
    case STREAM_HASH:
      assert(false && "TODO");
    default:
      SPDLOG_ERROR("Undefined command_id:{}", cmd);
      abort();
  }
}
