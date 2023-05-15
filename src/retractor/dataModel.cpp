#include "dataModel.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <cstdlib>  // std::div
#include <regex>

#include "QStruct.h"  // coreInstance
#include "SOperations.h"
#include "convertTypes.h"
#include "iostream"

// extern "C" qTree coreInstance;

// ctest -R '^ut-dataModel' -V

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
    : streamInstance(qry.id,                   // descriptor file
                     qry.filename,             // storage file
                     qry.descriptorStorage(),  //
                     qry.descriptorFrom()      //
      ) {
  SPDLOG_INFO("streamInstance <- qry");
};

// TODO: TEST THIS FREAKING CODE ... ASAP
// ! Work in progress
// https://en.cppreference.com/w/cpp/numeric/math/div
rdb::payload streamInstance::constructAgsePayload(int length, int offset) {
  assert(offset > 0);
  // First construct descriptor
  rdb::Descriptor descriptor;
  bool flip = (length < 0);
  length = abs(length);
  auto descriptorVecSize = storage->getDescriptor().size();
  std::string storage_name = storage->getStorageName();

  auto maxType = storage->getDescriptor().getMaxType();
  auto maxLen = GetFieldLenFromType(maxType);
  for (auto i = offset; i < offset + length; i++) {
    rdb::rfield x{std::make_tuple(storage_name + "_" + std::to_string(i),  //
                                  maxLen,                                  //
                                  maxType)};
    descriptor | rdb::Descriptor{x};
  }

  // Second construct payload
  std::unique_ptr<rdb::payload> localPayload = std::make_unique<rdb::payload>(descriptor);

  // std::cerr << "DESC:" << descriptor << std::endl;
  // for (auto i : descriptor) std::cout << rdb::GetStringdescFld(std::get<rdb::rtype>(i)) << std::endl;
  // 3rd - fill payload with data from storage
  auto prevQuot{-1};
  for (auto i = 0; i < length; i++) {
    auto dv = std::div(i + offset, descriptorVecSize);
    if (prevQuot != dv.quot) {
      prevQuot = dv.quot;
      storage->readReverse(dv.quot);
      // TODO: fix non existing data
    }

    auto locSrc = descriptorVecSize - dv.rem - 1;
    auto locDst = (!flip) ? i : length - i - 1;  // * Flipping is here

    assert(i < descriptor.size());

    std::any value = storage->getPayload()->getItem(locSrc);
    localPayload->setItem(locDst, value);
  }

  return *(localPayload.get());
}

dataModel::dataModel(qTree& coreInstance) : coreInstance(coreInstance) {
  //
  // Special parameters support in query set
  // fetch all ':*' - and remove them from coreInstance
  //
  for (auto qry : coreInstance) {
    if (qry.id == ":STORAGE") storagePath = qry.filename;
  }

  auto new_end = std::remove_if(coreInstance.begin(), coreInstance.end(),  //
                                [](const query& qry) { return qry.id[0] == ':'; });
  coreInstance.erase(new_end, coreInstance.end());

  SPDLOG_INFO("Create struct on CORE INSTANCE");

  for (auto& qry : coreInstance) qSet.emplace(qry.id, std::make_unique<streamInstance>(qry));
  for (auto const& [key, val] : qSet) val->storage->setRemoveOnExit(false);
}

dataModel::~dataModel() {}

std::unique_ptr<rdb::payload>::pointer dataModel::getPayload(std::string instance, int revOffset) {
  qSet[instance]->storage->readReverse(revOffset);
  return qSet[instance]->storage->getPayload();
}

// TODO: work area
void dataModel::processRows(std::set<std::string> inSet) {
  for (auto q : coreInstance) {
    if (inSet.find(q.id) == inSet.end()) continue;  // Drop off rows that not computed now
    if (q.isDeclaration()) continue;                // Declarations are not need to process
    computeInstance(q.id);
  }
}

void dataModel::computeInstance(std::string instance) {
  auto qry = coreInstance[instance];

  assert(qry.lProgram.size() < 4 && "all stream programs must be after optimization");
  assert(qry.lProgram.size() > 0 && "all stream are not declarations");

  std::vector<token> arg;
  for (auto tk : qry.lProgram) arg.push_back(tk);

  auto operation = qry.lProgram.back();  // Operation is always last element on stack

  const command_id cmd = operation.getCommandID();
  switch (cmd) {
    case PUSH_STREAM: {
      // 	:- PUSH_STREAM(core0)
      const auto nameSrc = operation.getStr_();
      *(qSet[instance]->fromPayload) = *getPayload(nameSrc);
    } break;
    case STREAM_TIMEMOVE: {
      // store in internal payload data from argument payload
      const auto nameSrc = arg[0].getStr_();
      const auto timeOffset = std::get<int>(operation.getVT());
      *(qSet[instance]->fromPayload) = *getPayload(nameSrc, timeOffset);
      // invocation of payload &payload::operator=(payload &other) from payload.cc
      // TODO: Add check if storagePayload is empty or argumentQueryName exist?
    } break;
    case STREAM_DEHASH_MOD:
    case STREAM_DEHASH_DIV: {
      const auto nameSrc = arg[0].getStr_();
      assert(nameSrc != "");
      const auto rationalArgument = arg[1].getRI();
      assert(rationalArgument > 0);
      // q.id - name of output stream
      // size[q.id] - count of record in output stream
      // q.rInterval - delta of output stream (rational) - contains
      // deltaDivMod( core0.delta , rationalArgument ) rationalArgument -
      // (2/3) argument of operation (rational)
      int timeOffset = -1;  // catch on assert(false);
      if (cmd == STREAM_DEHASH_DIV) timeOffset = Div(qry.rInterval, rationalArgument, 0);
      if (cmd == STREAM_DEHASH_MOD) timeOffset = Mod(rationalArgument, qry.rInterval, 0);
      if (timeOffset < 0) assert(false);
      *(qSet[instance]->fromPayload) = *getPayload(nameSrc, timeOffset);
    } break;
    case STREAM_AVG:
    case STREAM_MIN:
    case STREAM_MAX: {
      assert(false && "TODO");
      *(qSet[instance]->fromPayload) = *getPayload(arg[0].getStr_());
    } break;
    case STREAM_SUBSTRACT: {
      //  :- PUSH_STREAM(core0)
      //  :- STREAM_SUBSTRACT(1/2)
      const auto nameSrc = arg[0].getStr_();
      auto rationalArgument = arg[1].getRI();
      const auto lengthOfSrc = qSet[nameSrc]->storage->getRecordsCount();
      const auto timeOffset = Subtract(getQuery(nameSrc).rInterval, rationalArgument, lengthOfSrc);
      *(qSet[instance]->fromPayload) = *getPayload(nameSrc, timeOffset);
    } break;
    case STREAM_SUM:
      assert(false && "TODO");
    case STREAM_ADD: {
      // 	:- PUSH_STREAM(core0)
      //  :- PUSH_STREAM(core1)
      //  :- STREAM_ADD
      //
      const auto nameSrc1 = arg[0].getStr_();
      const auto nameSrc2 = arg[1].getStr_();
      // operator + from payload payload::operator+(payload &other) step into
      // action here
      // TODO support renaming of double-same fields after merge?

      *(qSet[instance]->fromPayload) = *getPayload(nameSrc1) + *getPayload(nameSrc2);
    } break;
    case STREAM_AGSE: {
      // 	:- PUSH_STREAM core -> delta_source (arg[0]) - operation
      //  :- STREAM_AGSE 2,3 -> window_length, window_step (arg[1])
      assert(arg.size() == 2);
      const auto nameSrc = arg[0].getStr_();
      auto [step, length] = get<std::pair<int, int>>(operation.getVT());
      assert(step >= 0);

      *(qSet[instance]->fromPayload) = qSet[nameSrc]->constructAgsePayload(step, length);
    } break;
    case STREAM_HASH: {
      // 	:- PUSH_STREAM(core0)
      //  :- PUSH_STREAM(core1)
      //  :- STREAM_HASH
      assert(arg.size() == 3);
      const auto nameSrc1 = arg[0].getStr_();
      const auto nameSrc2 = arg[1].getStr_();
      const auto intervalSrc1 = getQuery(nameSrc1).rInterval;
      const auto intervalSrc2 = getQuery(nameSrc2).rInterval;

      const auto recordOffset = qSet[instance]->storage->getRecordsCount();

      int retPos;
      if (Hash(intervalSrc1, intervalSrc2, recordOffset, retPos)) {
        qSet[nameSrc1]->storage->read(retPos);
        *(qSet[instance]->fromPayload) = *getPayload(nameSrc1);
      } else {
        qSet[nameSrc2]->storage->read(retPos);
        *(qSet[instance]->fromPayload) = *getPayload(nameSrc2);
      }
    } break;
    default:
      SPDLOG_ERROR("Undefined command_id:{}", cmd);
      abort();
  }
}
