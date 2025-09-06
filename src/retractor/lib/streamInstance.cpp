#include "streamInstance.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>  // std::div
#include <iostream>
#include <memory>  // unique_ptr
#include <regex>

#include "QStruct.h"  // coreInstance
#include "SOperations.hpp"
#include "expressionEvaluator.h"
#include "rdb/convertTypes.h"

streamInstance::streamInstance(qTree &coreInstance, query &qry, std::string storagePathParam) : coreInstance(coreInstance) {
  // only objects with REF has storageNameParam filled.
  assert(!qry.id.empty());

  const auto storageName{qry.filename == "" ? qry.id : qry.filename};

  inputPayload  = std::make_unique<rdb::payload>(qry.descriptorFrom(coreInstance));
  outputPayload = std::make_unique<rdb::storageAccessor>(qry.id, storageName, storagePathParam);

  auto desc = qry.descriptorStorage();
  outputPayload->attachDescriptor(&desc);

  auto requestedCapacity = coreInstance.maxCapacity[qry.id];
  outputPayload->setCapacity(requestedCapacity);
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
    if (it.rtype == rdb::RETMEMORY) continue;

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

void streamInstance::constructRulesAndUpdate(query &qry) {
  // construct if rule is fired
  for (auto &r : qry.lRules) {
    auto left  = r.leftConition;
    auto right = r.rightConition;
    auto type  = r.type;
    expressionEvaluator expression;
    bool result = expression.compare(left, right, outputPayload->getPayload(), type);
    if (result) {
      if (r.action == rule::DUMP) {
        dumpMgr.registerTask(qry.id, dumpManager::dumpTask{r.name, r.dumpRange, r.dump_retention});
      } else if (r.action == rule::SYSTEM) {
        system(r.systemCommand.c_str());
      }
    }
  }

  // update rules on rule list
  for (auto &r : qry.lRules) {
    if (r.action == rule::DUMP) {
      // Update dump rule
      dumpMgr.processStreamChunk(qry.filename);
    }
  }
}