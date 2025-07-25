#include "dataModel.h"

#include <rdb/convertTypes.h>
#include <spdlog/spdlog.h>

#include <algorithm>
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

enum { noHexFormat = false, HexFormat = true };

/*
std::string removeCRLF(std::string input) { return std::regex_replace(input, std::regex("\\r\\n|\\r|\\n"), ""); }
std::string removeSpc(std::string input) { return std::regex_replace(input, std::regex(R"(\s+)"), " "); }
*/

streamInstance::streamInstance(qTree &coreInstance,                       //
                               const std::string &descriptorName,         // q.id
                               const std::string &storageNameParam,       // filename
                               const rdb::Descriptor &storageDescriptor,  //
                               const rdb::Descriptor &internalDescriptor)
    : coreInstance(coreInstance) {
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
    SPDLOG_INFO("storage/external descriptor: id:{} desc:{}", descriptorName, strStream.str());
  }
  {
    std::stringstream strStream;
    strStream << rdb::flat << inputPayload->getDescriptor();
    SPDLOG_INFO("image/internal descriptor: filename:{} desc:{}", storageNameParam, strStream.str());
  }
};

streamInstance::streamInstance(qTree &coreInstance,                        //
                               const std::string &idAndStorageName,        // q.id = filename
                               const rdb::Descriptor &storageDescriptor,   //
                               const rdb::Descriptor &internalDescriptor)  //
    : streamInstance(coreInstance,                                         //
                     idAndStorageName,                                     // descriptor file
                     idAndStorageName,                                     // storage file
                     storageDescriptor,                                    //
                     internalDescriptor                                    //
      ) {
  SPDLOG_INFO("streamInstance - storage and id are the same");
}

streamInstance::streamInstance(qTree &coreInstance, query &qry)
    : streamInstance(coreInstance,                     //
                     qry.id,                           // descriptor file (q.id)
                     qry.filename,                     // storage file (filename)
                     qry.descriptorStorage(),          //
                     qry.descriptorFrom(coreInstance)  //
      ) {
  SPDLOG_INFO("streamInstance <- qry");
};

// https://en.cppreference.com/w/cpp/numeric/math/div
// RQL.G4: stream_factor AT '(' step=DECIMAL COMMA '-'? window=DECIMAL ')' # SExpAgse
// core@(step == 3, window|length == -2)
// core:  core@(1,1) core@(2,1)  core@(1,2)   core@(2,2)  core@(2,-2)
// 10     10         40          50 40        40 30       10 20
// 20     20         60          60 50        60 50       30 40
// 30     30         80          70 60        80 70       50 60
// 40     40         10          80 70        10 90       70 80
//
// Issue:
// DECLARE a INTEGER STREAM core, 1 FILE 'datafile.txt'
// SELECT * STREAM str1 FROM core@(2,-2)
// SELECT * STREAM str2 FROM str1@(1,1)
// str2  expected
// 10    20
// 30    30
// 30    40
// 50    50
rdb::payload streamInstance::constructAgsePayload(const int length,             //
                                                  const int step,               //
                                                  const std::string &instance,  //
                                                  const int storedRecordCountDst) {
  assert(step > 0);

  // temporary alias for variable - for better understand what is happening here.
  const auto &source = outputPayload;

  // 1. Descriptor construction process
  rdb::Descriptor descriptor;
  bool flip      = (length < 0);
  auto lengthAbs = abs(length);

  auto recordsCountSrc   = source->getRecordsCount();
  auto descriptorSrcSize = source->getDescriptor().sizeFlat();
  auto [maxType, maxLen] = source->getDescriptor().getMaxType();
  for (auto i = 0; i < lengthAbs; ++i) {
    rdb::rField x(instance + "_" + std::to_string(i),  //
                  maxLen,                              //
                  1,                                   //
                  maxType);
    descriptor += rdb::Descriptor{x};
  }

  // 2. Construct payload
  std::unique_ptr<rdb::payload> result = std::make_unique<rdb::payload>(descriptor);

  auto deltaSrc = coreInstance[instance].rInterval;
  auto deltaDst = boost::rational<int>(step) / descriptorSrcSize;

  auto outFasterThanIn = deltaDst < deltaSrc;

  auto storedRecordCountDst_{storedRecordCountDst};

  // This fix look's like heuristic. More investigation need here.
  // There is zerostep with prefetch on declared streams
  // in case of the rest there are latency due missing zero step.
  if (!source->isDeclared()) storedRecordCountDst_ -= descriptorSrcSize;

  storedRecordCountDst_ += 1;
  storedRecordCountDst_ *= descriptorSrcSize * deltaDst.numerator();
  storedRecordCountDst_ /= deltaDst.denominator();
  storedRecordCountDst_ -= 1;

  for (auto i = 0; i < lengthAbs; ++i) {
    auto fp = std::div(storedRecordCountDst_ - i, descriptorSrcSize);
    auto readPosition{recordsCountSrc - fp.quot - 1};
    if (recordsCountSrc > fp.quot)
      source->revRead(readPosition);
    else
      fp.rem = -1;  // skip to undefined(-1) as value

    auto locSrc = fp.rem;
    if (locSrc >= 0) {
      std::any value = source->getPayload()->getItem(locSrc);
      result->setItem(flip ? lengthAbs - i - 1 : i, value);
    } else
      result->setItem(flip ? lengthAbs - i - 1 : i, -1);
  }

  // 3. Cleanup source after processing
  source->revRead(0);  // Reset source

  // 4. Return constructed object.
  return *(result.get());
}

enum opType { maxop, minop, sumop, avgop };

template <typename T>
void fnOp(opType op, std::any value, std::any &valueRet) {
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

rdb::payload streamInstance::constructAggregate(command_id cmd, const std::string &instance) {
  assert(cmd == STREAM_MAX || cmd == STREAM_MIN || cmd == STREAM_SUM || cmd == STREAM_AVG);

  // First construct descriptor
  outputPayload->revRead(0);

  auto [maxType, maxLen] = outputPayload->getDescriptor().getMaxType();
  rdb::rField x{instance, maxLen, 1, maxType};  // TODO - Check 1
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
    if (it.rtype == rdb::REF) continue;
    if (it.rtype == rdb::TYPE) continue;
    if (it.rtype == rdb::RETENTION) continue;

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

void streamInstance::constructOutputPayload(const std::list<field> &fields) {
  auto i{0};
  for (auto program : fields) {
    expressionEvaluator expression;
    rdb::descFldVT retVal = expression.eval(program.lProgram, inputPayload.get());

    std::any result = std::visit([](auto &&arg) -> std::any { return arg; }, retVal);  // God forgive me ... i did it.

    assert(result.has_value());

    assert(program.field_.rtype == (outputPayload->getDescriptor()[i]).rtype);

    cast<std::any> castAny;
    std::any value = castAny(result, (outputPayload->getDescriptor()[i]).rtype);

    outputPayload->getPayload()->setItem(i, value);

    i++;
  }
}

dataModel::dataModel(qTree &coreInstance) : coreInstance(coreInstance) {
  //
  // Special parameters support in query set
  // fetch all ':*' - and remove them from coreInstance
  //
  auto storageIt = std::find_if(coreInstance.begin(), coreInstance.end(),  //
                                [](const auto &qry) { return qry.id == ":STORAGE"; });
  if (storageIt != std::end(coreInstance)) storagePath = storageIt->filename;

  auto new_end = std::remove_if(coreInstance.begin(), coreInstance.end(),  //
                                [](const auto &qry) { return qry.id[0] == ':'; });
  coreInstance.erase(new_end, coreInstance.end());

  SPDLOG_INFO("Create struct on CORE INSTANCE");

  for (auto &qry : coreInstance) qSet.emplace(qry.id, std::make_unique<streamInstance>(coreInstance, qry));
  for (auto const &[key, val] : qSet) val->outputPayload->setRemoveOnExit(false);
}

dataModel::~dataModel() {}

std::unique_ptr<rdb::payload>::pointer dataModel::getPayload(const std::string &instance,  //
                                                             const int revOffset) {
  if (!qSet[instance]->outputPayload->isDeclared()) {
    auto revOffsetMutable(revOffset);
    auto success = qSet[instance]->outputPayload->revRead(revOffsetMutable);
    assert(success);
  }
  // else
  // This data should be in outputPayload after dataModel::fetchDeclaredPayload call already
  return qSet[instance]->outputPayload->getPayload();
}

bool dataModel::fetchPayload(const std::string &instance,                     //
                             std::unique_ptr<rdb::payload>::pointer payload,  //
                             const int revOffset) {
  return qSet[instance]->outputPayload->revRead(revOffset, payload->get());
}

// TODO: work area
void dataModel::processRows(const std::set<std::string> &inSet) {
  bool zeroStep{false};

  // Move ALL armed device read to circular buffer. - no inSet dependent.
  for (auto q : coreInstance) {
    if (!q.isDeclaration()) continue;
    if (qSet[q.id]->outputPayload->bufferState == rdb::sourceState::empty) {
      qSet[q.id]->outputPayload->bufferState = rdb::sourceState::flux;  // Unlock data sources - enable physical read from source
      fetchDeclaredPayload(q.id);                                       // Declarations need to process in separate&first
      assert(qSet[q.id]->outputPayload->bufferState == rdb::sourceState::armed);  //
      zeroStep = true;
    }
    if (qSet[q.id]->outputPayload->bufferState == rdb::sourceState::armed) {  // move from fetched bucket to circle buffer.
      qSet[q.id]->outputPayload->fire();                                      // chamber -> outputPayload
      assert(qSet[q.id]->outputPayload->bufferState == rdb::sourceState::lock);
    }
  }

  // If we find at least one empty state in declarations
  // - we need fill only buffers and do not expect any streams in this moment

  if (zeroStep) return;

  // Report all processed inSet
  {
    std::stringstream s;
    for (auto i : inSet) s << i << ":";
    SPDLOG_INFO("PROCESS:{}", s.str());
  }

  //
  // Process expected declarations - if found - read from device and move to chamber
  //
  std::stringstream s;
  for (auto q : coreInstance) {
    if (inSet.find(q.id) == inSet.end()) continue;                             // Drop off rows that not computed now
    if (!q.isDeclaration()) continue;                                          // Skip non declarations.
    assert(qSet[q.id]->outputPayload->bufferState == rdb::sourceState::lock);  //
    qSet[q.id]->outputPayload->bufferState = rdb::sourceState::flux;  // Unlock data sources - enable physical read from source
    s << " DECL:{" << q.id << "}";                                    //
    fetchDeclaredPayload(q.id);                                       // Declarations need to process in separate&first
    assert(qSet[q.id]->outputPayload->bufferState == rdb::sourceState::armed);  //
  }

  for (auto q : coreInstance) {
    if (inSet.find(q.id) == inSet.end()) continue;  // Drop off rows that not computed now
    if (q.isDeclaration()) continue;                // Skip declarations.
    constructInputPayload(q.id);                    // That will create 'from' clause data set
    s << " QRY:[" << q.id << "]";                   //
    qSet[q.id]->constructOutputPayload(q.lSchema);  // That will create all fields from 'select' clause/list
    qSet[q.id]->outputPayload->write();             // That will store data from 'select' clause/list
  }

  SPDLOG_INFO("END PROCESS: {}", s.str());
}

void dataModel::fetchDeclaredPayload(const std::string &instance) {
  auto qry = coreInstance[instance];

  assert(qry.isDeclaration());  // lProgram is empty()

  assert(qSet[instance]->outputPayload->bufferState == rdb::sourceState::flux);

  auto success = qSet[instance]->outputPayload->revRead(0);
  assert(success);

  assert(qSet[instance]->outputPayload->bufferState == rdb::sourceState::armed);
}

void dataModel::constructInputPayload(const std::string &instance) {
  auto qry = coreInstance[instance];

  assert(qry.lProgram.size() < 4 && "all stream programs must be after optimization");
  assert(qry.lProgram.size() > 0 && "constructInputPayload does not process declarations");

  std::vector<token> arg;
  std::copy(qry.lProgram.begin(), qry.lProgram.end(), std::back_inserter(arg));
  // same: for (auto tk : qry.lProgram) arg.push_back(tk);

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

      const auto nameSrc    = arg[0].getStr_();
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

      const auto nameSrc          = arg[0].getStr_();
      const auto rationalArgument = arg[1].getRI();
      const auto lengthOfSrc      = qSet[nameSrc]->outputPayload->getRecordsCount();

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

      const auto nameSrc          = arg[0].getStr_();
      const auto rationalArgument = arg[1].getRI();
      const auto lengthOfSrc      = qSet[nameSrc]->outputPayload->getRecordsCount();
      const auto timeOffset       = Subtract(coreInstance.getQuery(nameSrc).rInterval, rationalArgument, lengthOfSrc);

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

      const auto nameSrc  = arg[0].getStr_();  // * INFO Sync with QStruct.cpp
      auto [step, length] = get<std::pair<int, int>>(operation.getVT());
      assert(step > 0);
      const int storedRecordsInOutput = qSet[instance]->outputPayload->getRecordsCount();
      *(qSet[instance]->inputPayload) = qSet[nameSrc]->constructAgsePayload(length, step, nameSrc, storedRecordsInOutput);
    } break;
    case STREAM_HASH: {
      // 	:- PUSH_STREAM(core0)
      //  :- PUSH_STREAM(core1)
      //  :- STREAM_HASH
      //
      assert(arg.size() == 3);

      const auto nameSrc1     = arg[0].getStr_();
      const auto nameSrc2     = arg[1].getStr_();
      const auto intervalSrc1 = coreInstance.getQuery(nameSrc1).rInterval;
      const auto intervalSrc2 = coreInstance.getQuery(nameSrc2).rInterval;

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
      assert(false && "Undefined command_id in constructInputPayload");
      abort();
  }
}

std::vector<rdb::descFldVT> dataModel::getRow(const std::string &instance, const int timeOffset) {
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
    if (f.rlen == 0) continue;
    retVal.push_back(any_to_variant_cast(payload->getItem(i++)));
  }
  return retVal;
}

size_t dataModel::streamStoredSize(const std::string &instance) {
  return qSet[instance]->outputPayload->getDescriptor().getSizeInBytes() * getStreamCount(instance);
}

size_t dataModel::getStreamCount(const std::string &instance) { return qSet[instance]->outputPayload->getRecordsCount(); }
