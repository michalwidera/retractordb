#include "dataModel.h"

#include <rdb/convertTypes.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <cstdlib>  // std::div
#include <iostream>
#include <memory>  // unique_ptr
#include <regex>

#include "QStruct.h"  // coreInstance
#include "SOperations.h"
#include "expressionEvaluator.h"
#include "rdb/convertTypes.h"

// ctest -R '^ut-dataModel' -V

extern "C" qTree coreInstance;

enum { noHexFormat = false, HexFormat = true };

dataModel* pProc = nullptr;

/*
std::string removeCRLF(std::string input) { return std::regex_replace(input, std::regex("\\r\\n|\\r|\\n"), ""); }
std::string removeSpc(std::string input) { return std::regex_replace(input, std::regex(R"(\s+)"), " "); }
*/

streamInstance::streamInstance(               //
    const std::string descriptorName,         // q.id
    const std::string storageNameParam,       // filename
    const rdb::Descriptor storageDescriptor,  //
    const rdb::Descriptor internalDescriptor) {
  // only objects with REF has storageNameParam filled.
  const auto storageName{storageNameParam == "" ? descriptorName : storageNameParam};
  SPDLOG_INFO("streamInstance desc:{} storage:{}", descriptorName, storageName);
  inputPayload = std::make_unique<rdb::payload>(internalDescriptor);

  outputPayload = std::make_unique<rdb::storageAccessor>(descriptorName, storageName);
  outputPayload->attachDescriptor(&storageDescriptor);

  auto requestedCapacity = coreInstance.maxCapacity[descriptorName];
  outputPayload->setCapacity(requestedCapacity);

  {
    std::stringstream strStream;
    strStream << rdb::flat << outputPayload->getDescriptor();
    SPDLOG_INFO("storage/external descriptor: {}", strStream.str());
  }
  {
    std::stringstream strStream;
    strStream << rdb::flat << inputPayload->getDescriptor();
    SPDLOG_INFO("image/internal descriptor: {}", strStream.str());
  }
};

streamInstance::streamInstance(                //
    const std::string idAndStorageName,        // q.id = filename
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
    : streamInstance(qry.id,                   // descriptor file (q.id)
                     qry.filename,             // storage file (filename)
                     qry.descriptorStorage(),  //
                     qry.descriptorFrom()      //
      ) {
  SPDLOG_INFO("streamInstance <- qry");
};

// https://en.cppreference.com/w/cpp/numeric/math/div
rdb::payload streamInstance::constructAgsePayload(const int length, const int offset, std::string instance) {
  assert(offset > 0);
  // First construct descriptor
  rdb::Descriptor descriptor;
  bool flip = (length < 0);
  auto lengthAbs = abs(length);

  outputPayload->readReverse(0);

  auto descriptorVecSize = outputPayload->getDescriptor().sizeFlat();
  auto [maxType, maxLen] = outputPayload->getDescriptor().getMaxType();
  for (auto i = 0; i < lengthAbs; i++) {
    rdb::rField x{std::make_tuple(instance + "_" + std::to_string(i),  //
                                  maxLen,                              //
                                  1,                                   // TODO: Check
                                  maxType)};
    descriptor | rdb::Descriptor{x};
  }

  // Second construct payload
  std::unique_ptr<rdb::payload> localPayload = std::make_unique<rdb::payload>(descriptor);

  auto prevQuot{-1};
  for (auto i = 0; i < lengthAbs; i++) {
    auto dv = std::div(i + offset, descriptorVecSize);
    if (prevQuot != dv.quot) {
      prevQuot = dv.quot;
      outputPayload->readReverse(dv.quot);
      // TODO: check&fix non existing data?
    }

    auto locSrc = descriptorVecSize - dv.rem - 1;
    auto locDst = (!flip) ? i : lengthAbs - i - 1;  // * Flipping is here

    assert(i < descriptor.size());

    std::any value = outputPayload->getPayload()->getItem(locSrc);
    localPayload->setItem(locDst, value);
  }

  return *(localPayload.get());
}

enum opType { maxop, minop, sumop, avgop };

template <typename T>
void fnOp(opType op, std::any value, std::any& valueRet) {
  T val1 = std::any_cast<T>(valueRet);
  T val2 = std::any_cast<T>(value);
  switch (op) {
    case maxop:
      if (val1 < val2) valueRet = value;
      break;
    case minop:
      if (val1 > val2) valueRet = value;
      break;
    case sumop:
      valueRet = val1 + val2;
      break;
    case avgop:
      valueRet = val1 / val2;
      break;
    default:
      SPDLOG_ERROR("non supported opType");
      assert(false);
  }
}

rdb::payload streamInstance::constructAggregate(command_id cmd, std::string instance) {
  assert(cmd == STREAM_MAX || cmd == STREAM_MIN || cmd == STREAM_SUM || cmd == STREAM_AVG);

  // First construct descriptor
  outputPayload->readReverse(0);

  auto [maxType, maxLen] = outputPayload->getDescriptor().getMaxType();
  rdb::rField x{std::make_tuple(instance, maxLen, 1, maxType)};  // TODO - Check 1
  rdb::Descriptor descriptor{x};
  // same as core[instance].descriptorFrom()

  // Second construct payload
  std::unique_ptr<rdb::payload> localPayload = std::make_unique<rdb::payload>(descriptor);

  if (maxType > rdb::DOUBLE) {  // fldlist.h -  rdb types are in sequence
    SPDLOG_INFO("Not supported aggregation on type");
    assert(false);
    return *(localPayload.get());  // uninitialized payload
  }

  // choose aggregate operation
  cast<std::any> castAny;
  std::any valueRet = castAny(0, maxType);

  opType op;

  if (cmd == STREAM_MAX)
    op = maxop;
  else if (cmd == STREAM_MIN)
    op = minop;
  else if (cmd == STREAM_SUM || cmd == STREAM_AVG)
    op = sumop;  // <- STREAM_AVG: this is not an error
  else
    assert(false);

  auto i{0};
  for (auto const it : outputPayload->getDescriptor()) {
    if (std::get<rdb::rtype>(it) == rdb::REF) continue;
    if (std::get<rdb::rtype>(it) == rdb::TYPE) continue;

    std::any value = castAny(outputPayload->getPayload()->getItem(i++), maxType);
    switch (maxType) {
      case rdb::BYTE:
        fnOp<uint8_t>(op, value, valueRet);
        break;
      case rdb::INTEGER:
        fnOp<int>(op, value, valueRet);
        break;
      case rdb::UINT:
        fnOp<unsigned>(op, value, valueRet);
        break;
      case rdb::FLOAT:
        fnOp<float>(op, value, valueRet);
        break;
      case rdb::DOUBLE:
        fnOp<double>(op, value, valueRet);
        break;
      case rdb::RATIONAL:
        fnOp<boost::rational<int>>(op, value, valueRet);
        break;
      default:
        SPDLOG_ERROR("Not supported aggregation type");
        assert(false);
        break;
    }
  }

  if (cmd == STREAM_AVG) {
    std::any value = castAny(outputPayload->getPayload()->getDescriptor().size(), maxType);
    switch (maxType) {
      case rdb::BYTE:
        fnOp<uint8_t>(avgop, value, valueRet);  // <- valueRet/value
        break;
      case rdb::INTEGER:
        fnOp<int>(avgop, value, valueRet);
        break;
      case rdb::UINT:
        fnOp<unsigned>(avgop, value, valueRet);
        break;
      case rdb::FLOAT:
        fnOp<float>(avgop, value, valueRet);
        break;
      case rdb::DOUBLE:
        fnOp<double>(avgop, value, valueRet);
        break;
      case rdb::RATIONAL:
        fnOp<boost::rational<int>>(avgop, value, valueRet);
        break;
      default:
        SPDLOG_ERROR("Not supported aggregation type");
        assert(false);
        break;
    }
  }

  auto postion{0};
  localPayload->setItem(postion, valueRet);

  return *(localPayload.get());
}

void streamInstance::constructOutputPayload(const std::list<field>& fields) {
  auto i{0};
  for (auto program : fields) {
    expressionEvaluator expression;
    rdb::descFldVT retVal = expression.eval(program.lProgram, inputPayload.get());

    std::any result = std::visit([](auto&& arg) -> std::any { return arg; }, retVal);  // God forgive me ... i did it.

    assert(result.has_value());

    assert(std::get<rdb::rtype>(program.field_) == std::get<rdb::rtype>(outputPayload->getDescriptor()[i]));

    cast<std::any> castAny;
    std::any value = castAny(result, std::get<rdb::rtype>(outputPayload->getDescriptor()[i]));

    outputPayload->getPayload()->setItem(i, value);

    i++;
  }
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
  for (auto const& [key, val] : qSet) val->outputPayload->setRemoveOnExit(false);

  pProc = this;
}

dataModel::~dataModel() {}

std::unique_ptr<rdb::payload>::pointer dataModel::getPayload(std::string instance,  //
                                                             const int revOffset) {
  if (!qSet[instance]->outputPayload->isDeclared()) {
    auto revOffsetMutable(revOffset);
    auto success = qSet[instance]->outputPayload->readReverse(revOffsetMutable);
    assert(success);
  }
  // else
  // This data should be in outputPayload after dataModel::fetchDeclaredPayload call already
  return qSet[instance]->outputPayload->getPayload();
}

bool dataModel::fetchPayload(std::string instance,                            //
                             std::unique_ptr<rdb::payload>::pointer payload,  //
                             const int revOffset) {
  return qSet[instance]->outputPayload->readReverse(revOffset, payload->get());
}

// TODO: work area
void dataModel::processRows(std::set<std::string> inSet) {
  for (auto q : coreInstance) {
    if (inSet.find(q.id) == inSet.end()) continue;                       // Drop off rows that not computed now
    if (!q.isDeclaration()) continue;                                    // Skip non declarations.
    fetchDeclaredPayload(q.id);                                          // Declarations need to process in separate&first
    qSet[q.id]->outputPayload->bufferPolicy = rdb::policyState::freeze;  // freeze data sources
  }
  SPDLOG_INFO("fetch decl. stop");

  for (auto q : coreInstance) {
    if (inSet.find(q.id) == inSet.end()) continue;  // Drop off rows that not computed now
    if (q.isDeclaration()) continue;                // Skip declarations.
    constructInputPayload(q.id);                    // That will create 'from' clause data set
    qSet[q.id]->constructOutputPayload(q.lSchema);  // That will create all fields from 'select' clause/list
    qSet[q.id]->outputPayload->write();             // That will store data from 'select' clause/list
  }

  for (auto q : coreInstance) {
    if (inSet.find(q.id) == inSet.end()) continue;                     // Drop off rows that not computed now
    if (!q.isDeclaration()) continue;                                  // Skip non declarations.
    qSet[q.id]->outputPayload->bufferPolicy = rdb::policyState::flux;  // Unfreeze data sources
  }
}

void dataModel::fetchDeclaredPayload(std::string instance) {
  auto qry = coreInstance[instance];

  assert(qry.isDeclaration());  // lProgram is empty()

  auto success = qSet[instance]->outputPayload->read(0);
  SPDLOG_INFO("fetch decl. {}", instance);
  assert(success);

  *(qSet[instance]->inputPayload) = *(qSet[instance]->outputPayload->getPayload());
}

void dataModel::constructInputPayload(std::string instance) {
  auto qry = coreInstance[instance];

  assert(qry.lProgram.size() < 4 && "all stream programs must be after optimization");
  assert(qry.lProgram.size() > 0 && "constructInputPayload does not process declarations");

  std::vector<token> arg;
  for (auto tk : qry.lProgram) arg.push_back(tk);

  auto operation = qry.lProgram.back();  // Operation is always last element on stack

  const command_id cmd = operation.getCommandID();
  switch (cmd) {
    case PUSH_STREAM: {
      // 	:- PUSH_STREAM(core0)
      //
      assert(arg.size() == 1);

      const auto nameSrc = operation.getStr_();

      *(qSet[instance]->inputPayload) = *getPayload(nameSrc);
    } break;
    case STREAM_TIMEMOVE: {
      // 	:- PUSH_STREAM(core0)
      //  :- STREAM_TIMEMOVE(1)
      //
      assert(arg.size() == 2);

      const auto nameSrc = arg[0].getStr_();
      const auto timeOffset = std::get<int>(operation.getVT());

      *(qSet[instance]->inputPayload) = *getPayload(nameSrc, timeOffset);
    } break;
    case STREAM_DEHASH_MOD:
    case STREAM_DEHASH_DIV: {
      //  :- PUSH_STREAM(core0)
      //  :- PUSH_VAL(2/1)
      //  :- STREAM_DEHASH_MOD
      //
      assert(arg.size() == 3);

      const auto nameSrc = arg[0].getStr_();
      const auto rationalArgument = arg[1].getRI();
      const auto lengthOfSrc = qSet[nameSrc]->outputPayload->getRecordsCount();

      assert(rationalArgument > 0);

      int timeOffset = -1;  // catch on assert(false);
      if (cmd == STREAM_DEHASH_DIV) timeOffset = Div(qry.rInterval, rationalArgument, lengthOfSrc);
      if (cmd == STREAM_DEHASH_MOD) timeOffset = Mod(rationalArgument, qry.rInterval, lengthOfSrc);
      assert(timeOffset >= 0);
      *(qSet[instance]->inputPayload) = *getPayload(nameSrc, timeOffset);
    } break;
    case STREAM_SUM:
    case STREAM_AVG:
    case STREAM_MIN:
    case STREAM_MAX: {
      const auto nameSrc = arg[0].getStr_();

      *(qSet[instance]->inputPayload) = qSet[nameSrc]->constructAggregate(cmd, instance + "_0");
    } break;
    case STREAM_SUBTRACT: {
      //  :- PUSH_STREAM(core0)
      //  :- STREAM_SUBTRACT(1/2)
      //
      assert(arg.size() == 2);

      const auto nameSrc = arg[0].getStr_();
      const auto rationalArgument = arg[1].getRI();
      const auto lengthOfSrc = qSet[nameSrc]->outputPayload->getRecordsCount();
      const auto timeOffset = Subtract(getQuery(nameSrc).rInterval, rationalArgument, lengthOfSrc);

      *(qSet[instance]->inputPayload) = *getPayload(nameSrc, timeOffset);
    } break;
    case STREAM_ADD: {
      // 	:- PUSH_STREAM(core0)
      //  :- PUSH_STREAM(core1)
      //  :- STREAM_ADD
      //
      assert(arg.size() == 3);

      const auto nameSrc1 = arg[0].getStr_();
      const auto nameSrc2 = arg[1].getStr_();

      // operator + from payload payload::operator+(payload &other) step into action here
      // TODO support renaming of double-same fields after merge?

      *(qSet[instance]->inputPayload) = *getPayload(nameSrc1) + *getPayload(nameSrc2);
    } break;
    case STREAM_AGSE: {
      // 	:- PUSH_STREAM core -> delta_source (arg[0]) - operation
      //  :- STREAM_AGSE 2,3 -> window_step, window_length  (arg[1])
      //
      assert(arg.size() == 2);

      const auto nameSrc = arg[0].getStr_();  // * INFO Sync with QStruct.cpp
      auto [step, length] = get<std::pair<int, int>>(operation.getVT());
      assert(step >= 0);
      length = abs(length);
      *(qSet[instance]->inputPayload) = qSet[nameSrc]->constructAgsePayload(length, step, instance);
    } break;
    case STREAM_HASH: {
      // 	:- PUSH_STREAM(core0)
      //  :- PUSH_STREAM(core1)
      //  :- STREAM_HASH
      //
      assert(arg.size() == 3);

      const auto nameSrc1 = arg[0].getStr_();
      const auto nameSrc2 = arg[1].getStr_();
      const auto intervalSrc1 = getQuery(nameSrc1).rInterval;
      const auto intervalSrc2 = getQuery(nameSrc2).rInterval;

      const auto recordOffset = qSet[instance]->outputPayload->getRecordsCount();

      int retPos;
      if (Hash(intervalSrc1, intervalSrc2, recordOffset, retPos)) {
        *(qSet[instance]->inputPayload) = *getPayload(nameSrc2, retPos);
      } else {
        *(qSet[instance]->inputPayload) = *getPayload(nameSrc1, retPos);
      }
    } break;
    default:
      SPDLOG_ERROR("Undefined command_id:{}", cmd);
      abort();
  }
}

// TODO - use this func in printRowValue/executorsm.cpp 281
// TODO - and use std::ostream &operator<<(std::ostream &os, const rdb::descFldVT &rhs) from QStruct.cpp
// TODO - Workarea - Please cover with test
std::vector<rdb::descFldVT> dataModel::getRow(std::string instance, const int timeOffset) {
  std::vector<rdb::descFldVT> retVal;

  auto payload = std::make_unique<rdb::payload>(qSet[instance]->outputPayload->getDescriptor());

  if (!qSet[instance]->outputPayload->isDeclared()) {
    auto success = fetchPayload(instance, payload.get(), timeOffset);
    assert(success);
  } else {
    *payload = *(qSet[instance]->outputPayload->getPayload());
  }
  auto i{0};
  for (auto f : payload->getDescriptor()) {
    if (std::get<rdb::rlen>(f) == 0) continue;
    retVal.push_back(any_to_variant_cast(payload->getItem(i++)));
  }
  return retVal;
}

size_t dataModel::streamStoredSize(const std::string& instance) {
  return qSet[instance]->outputPayload->getDescriptor().getSizeInBytes() * getStreamCount(instance);
}

size_t dataModel::getStreamCount(const std::string& instance) { return qSet[instance]->outputPayload->getRecordsCount(); }
