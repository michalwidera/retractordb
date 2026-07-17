#include "streamInstance.hpp"

#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdlib>  // std::div
#include <memory>   // unique_ptr
#include <optional>
#include <utility>

#include "fatalError.hpp"

#include "expressionEvaluator.hpp"
#include "persistentCounter.hpp"
#include "rdb/convertTypes.hpp"

extern std::unique_ptr<PersistentCounter> pCounterPtr;

streamInstance::streamInstance(qTree &coreInstance, query &qry, const std::string &storagePathParam)
    : coreInstance(coreInstance) {
  // only objects with REF has storageNameParam filled.
  if (qry.id.empty()) FatalError("streamInstance: query id must not be empty");

  const auto storageName{qry.filename.empty() ? qry.id : qry.filename};

  int percounter = -1;
  if (pCounterPtr) percounter = pCounterPtr->getCount();

  inputPayload  = std::make_unique<rdb::payload>(qry.descriptorFrom(coreInstance));
  outputPayload = std::make_unique<rdb::storage>(qry.id, storageName, storagePathParam, qry.storage_policy, qry.isOneShot,
                                                 qry.isHold, percounter);

  auto desc = qry.descriptorStorage();
  outputPayload->attachDescriptor(&desc);

  if (qry.rInterval > 0 && qry.isDeclaration()) {
    outputPayload->configureGapDetection(qry.rInterval);
  }

  auto requestedCapacity = coreInstance.maxCapacity[qry.id];
  outputPayload->setCapacity(requestedCapacity);

  dumpMgr.setDumpStorage(storagePathParam);
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
                                                  const int storedRecordCountDst) const {
  if (step <= 0) {
    FatalError("streamInstance::constructAgsePayload: step must be > 0, got {}", step);
  }

  // temporary alias for variable - for better understand what is happening here.
  const auto &source = outputPayload;

  bool flip      = (length < 0);
  auto lengthAbs = abs(length);

  auto recordsCountSrc   = source->getRecordsCount();
  auto descriptorSrcSize = source->descriptor.flatElementCount();

  // 1. Descriptor construction process -- shape depends only on (this stream,
  //    lengthAbs) and is invariant across intervals, so build it once per
  //    distinct lengthAbs and reuse afterwards (see agseDescriptorCache_).
  auto cacheIt = agseDescriptorCache_.find(lengthAbs);
  if (cacheIt == agseDescriptorCache_.end()) {
    rdb::Descriptor descriptor;
    auto [maxType, maxLen] = source->descriptor.widestFieldType();
    for (auto i = 0; i < lengthAbs; ++i) {
      rdb::rField x(instance + "_" + std::to_string(i),  //
                    maxLen,                              //
                    1,                                   //
                    maxType);
      descriptor += rdb::Descriptor{x};
    }
    cacheIt = agseDescriptorCache_.emplace(lengthAbs, std::move(descriptor)).first;
  }
  const rdb::Descriptor &descriptor = cacheIt->second;

  // 2. Construct payload (values are per-interval, so this stays; only the
  //    intermediate heap allocation is gone -- payload(const Descriptor&)
  //    already copies the shape into its own member, and NRVO avoids a copy
  //    on return).
  rdb::payload result(descriptor);

  auto deltaDst = boost::rational<int>(step) / descriptorSrcSize;

  auto storedRecordCountDst_{storedRecordCountDst};

  // This fix look's like heuristic. More investigation need here.
  // There is zerostep with prefetch on declared streams
  // in case of the rest there are latency due missing zero step.
  if (!source->isDeclared()) storedRecordCountDst_ -= descriptorSrcSize;

  storedRecordCountDst_ += 1;
  storedRecordCountDst_ *= descriptorSrcSize * deltaDst.numerator();
  storedRecordCountDst_ /= deltaDst.denominator();
  storedRecordCountDst_ -= 1;

  // Przy descriptorSrcSize > 1 kolejne i trafiaja w ten sam rekord zrodlowy (rozni sie
  // tylko fp.rem) — rekord siedzi juz w storagePayload_, wiec nie czytamy go ponownie.
  std::optional<size_t> lastReadPosition;
  for (auto i = 0; i < lengthAbs; ++i) {
    auto fp = std::div(storedRecordCountDst_ - i, descriptorSrcSize);
    auto readPosition{recordsCountSrc - fp.quot - 1};
    // Guard negative quotient before revRead: out-of-range windows must stay null, not stale 0.
    if (fp.quot >= 0 && std::cmp_less(fp.quot, recordsCountSrc)) {
      if (lastReadPosition != static_cast<size_t>(readPosition)) {
        if (source->revRead(static_cast<size_t>(readPosition)))
          lastReadPosition = static_cast<size_t>(readPosition);
        else
          fp.rem = -1;
      }
    } else
      fp.rem = -1;  // skip to undefined(-1) as value

    auto locSrc = fp.rem;
    if (locSrc >= 0) {
      auto valueOpt = source->getPayload()->getItem(locSrc);
      result.setItem(flip ? lengthAbs - i - 1 : i, valueOpt);
    } else
      result.setItem(flip ? lengthAbs - i - 1 : i, std::nullopt);
  }

  // 3. Cleanup source after processing
  source->revRead(0);  // Reset source

  // 4. Return constructed object.
  return result;
}

enum opType : std::uint8_t { maxop, minop, sumop, avgop };

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
      try {
        valueRet = val1 + val2;
      } catch (...) {
        valueRet = std::numeric_limits<T>::max();
      }
      break;
    case avgop:
      valueRet = val1 / val2;
      break;
    default:
      FatalError("streamInstance::fnOp: unsupported opType");
  }
}

rdb::payload streamInstance::reduceFieldsToPayload(command_id cmd, const std::string &instance) const {
  if (cmd != STREAM_MAX && cmd != STREAM_MIN && cmd != STREAM_SUM && cmd != STREAM_AVG) {
    FatalError("streamInstance::reduceFieldsToPayload: cmd must be STREAM_MAX/MIN/SUM/AVG, got {}", static_cast<int>(cmd));
  }

  // First construct descriptor
  outputPayload->revRead(0);

  auto [maxType_, maxLen] = outputPayload->descriptor.widestFieldType();
  auto const maxType      = maxType_;
  rdb::rField x{instance, maxLen, 1, maxType};  // TODO - Check 1
  rdb::Descriptor descriptor{x};
  // same as core[instance].descriptorFrom()

  // Second construct payload
  std::unique_ptr<rdb::payload> localPayload = std::make_unique<rdb::payload>(descriptor);

  if (maxType > rdb::DOUBLE) {  // fldlist.h -  rdb types are in sequence
    FatalError("streamInstance: aggregation not supported for this field type");
  }

  // choose aggregate operation
  cast<std::any> castAny;
  std::any valueRet;  // ok for sum,avg
  opType op;

  if (cmd == STREAM_MIN) {
    op = minop;
    switch (maxType) {
      case rdb::BYTE:
      case rdb::INTEGER:
      case rdb::UINT:
      case rdb::RATIONAL:
        valueRet = boost::rational<int>(std::numeric_limits<int>::max(), 1);
        maxType_ = rdb::RATIONAL;
        break;
      case rdb::FLOAT:
        valueRet = std::numeric_limits<float>::max();
        break;
      case rdb::DOUBLE:
        valueRet = std::numeric_limits<double>::max();
        break;
      default:
        FatalError("streamInstance: unsupported aggregation type");
    }
  }
  if (cmd == STREAM_MAX) {
    op = maxop;
    switch (maxType) {
      case rdb::BYTE:
      case rdb::INTEGER:
      case rdb::UINT:
      case rdb::RATIONAL:
        valueRet = boost::rational<int>(std::numeric_limits<int>::min(), 1);
        maxType_ = rdb::RATIONAL;
        break;
      case rdb::FLOAT:
        valueRet = std::numeric_limits<float>::min();
        break;
      case rdb::DOUBLE:
        valueRet = std::numeric_limits<double>::min();
        break;
      default:
        FatalError("streamInstance: unsupported aggregation type");
    }
  }
  if (cmd == STREAM_SUM || cmd == STREAM_AVG) {
    op = sumop;  // <- STREAM_AVG: this is not an error
    switch (maxType) {
      case rdb::BYTE:
      case rdb::INTEGER:
      case rdb::UINT:
      case rdb::RATIONAL:
        valueRet = boost::rational<int>(0, 1);
        maxType_ = rdb::RATIONAL;
        break;
      case rdb::FLOAT:
        valueRet = float(0);
        break;
      case rdb::DOUBLE:
        valueRet = double(0);
        break;
      default:
        FatalError("streamInstance: unsupported aggregation type");
    }
  }

  if (!valueRet.has_value()) FatalError("streamInstance::reduceFieldsToPayload: valueRet not initialized after switch");

  auto item{0};
  auto validItemCount{0};
  for (auto const &it : outputPayload->descriptor) {
    if (it.rtype == rdb::REF) continue;
    if (it.rtype == rdb::TYPE) continue;
    if (it.rtype == rdb::RETENTION) continue;
    if (it.rtype == rdb::RETMEMORY) continue;

    auto valueOpt = outputPayload->getPayload()->getItem(item++);
    if (!valueOpt.has_value()) continue;
    validItemCount++;

    std::any value = castAny(valueOpt.value(), maxType_);
    switch (maxType_) {
      case rdb::BYTE:
      case rdb::INTEGER:
      case rdb::UINT:
        FatalError("streamInstance: BYTE/INT/UINT should have been cast to RATIONAL before aggregation");
      case rdb::RATIONAL:
        fnOp<boost::rational<int>>(op, value, valueRet);
        break;
      case rdb::FLOAT:
        fnOp<float>(op, value, valueRet);
        break;
      case rdb::DOUBLE:
        fnOp<double>(op, value, valueRet);
        break;
      default:
        FatalError("streamInstance: unsupported aggregation type");
    }
  }

  if (validItemCount == 0) {
    auto postion{0};
    localPayload->setItem(postion, std::nullopt);
    return *(localPayload);
  }

  if (cmd == STREAM_AVG) {
    std::any value = castAny(static_cast<uint8_t>(validItemCount), maxType_);
    switch (maxType_) {
      case rdb::BYTE:
      case rdb::INTEGER:
      case rdb::UINT:
        FatalError("streamInstance: BYTE/INT/UINT should have been cast to RATIONAL before aggregation");
      case rdb::RATIONAL:
        fnOp<boost::rational<int>>(avgop, value, valueRet);
        break;
      case rdb::FLOAT:
        fnOp<float>(avgop, value, valueRet);
        break;
      case rdb::DOUBLE:
        fnOp<double>(avgop, value, valueRet);
        break;
      default:
        FatalError("streamInstance: unsupported aggregation type");
    }
  }

  switch (maxType) {
    case rdb::BYTE: {
      if (valueRet.type() != typeid(boost::rational<int>))
        FatalError("streamInstance: aggregation result must be rational at finalization");
      auto tobyte = std::any_cast<boost::rational<int>>(valueRet);
      if (tobyte > boost::rational<int>(std::numeric_limits<uint8_t>::max(), 1)) {
        valueRet = uint8_t(std::numeric_limits<uint8_t>::max());
      } else if (tobyte < boost::rational<int>(std::numeric_limits<uint8_t>::min(), 1))
        valueRet = uint8_t(std::numeric_limits<uint8_t>::min());
      else
        valueRet = uint8_t(rational_cast<int>(tobyte));
    } break;

    case rdb::INTEGER: {
      if (valueRet.type() != typeid(boost::rational<int>))
        FatalError("streamInstance: aggregation result must be rational at finalization");
      auto toint = std::any_cast<boost::rational<int>>(valueRet);
      if (toint > boost::rational<int>(std::numeric_limits<int>::max(), 1))
        valueRet = std::numeric_limits<int>::max();
      else if (toint < boost::rational<int>(std::numeric_limits<int>::min(), 1))
        valueRet = std::numeric_limits<int>::min();
      else
        valueRet = int(rational_cast<int>(toint));
    } break;

    case rdb::UINT: {
      if (valueRet.type() != typeid(boost::rational<int>))
        FatalError("streamInstance: aggregation result must be rational at finalization");
      auto touint = std::any_cast<boost::rational<int>>(valueRet);
      if (touint > boost::rational<int>(std::numeric_limits<unsigned>::max(), 1))
        valueRet = std::numeric_limits<unsigned>::max();
      else if (touint < boost::rational<int>(0, 1))
        valueRet = unsigned(0);
      else
        valueRet = rational_cast<unsigned>(std::any_cast<boost::rational<int>>(valueRet));
    } break;

    case rdb::RATIONAL:
    case rdb::FLOAT:
    case rdb::DOUBLE:
      break;
    default:
      FatalError("streamInstance: unsupported type in aggregation finalization");
  }
  auto postion{0};
  localPayload->setItem(postion, valueRet);

  return *(localPayload);
}

void streamInstance::constructOutputPayload(const std::list<field> &fields) const {
  auto i{0};
  for (const auto &program : fields) {
    expressionEvaluator expression;
    rdb::descFldVT retVal = expression.eval(program.lProgram, inputPayload.get());

    if (std::holds_alternative<std::monostate>(retVal)) {
      outputPayload->getPayload()->setItem(i, std::nullopt);
      i++;
      continue;
    }

    std::any result = std::visit([](auto &&arg) -> std::any { return arg; }, retVal);  // God forgive me ... i did it.

    if (!result.has_value()) FatalError("streamInstance::constructOutputPayload: expression result has no value");

    if (program.field_.rtype != (outputPayload->descriptor[i]).rtype) {
      FatalError("streamInstance::constructOutputPayload: program field type does not match descriptor type at index {}", i);
    }

    cast<std::any> castAny;
    std::any value = castAny(result, (outputPayload->descriptor[i]).rtype);

    outputPayload->getPayload()->setItem(i, value);

    i++;
  }
}

bool boolCast(const rdb::descFldVT &inVar) {
  bool retVal(false);

  std::visit(Overload{
                 [&retVal](std::monostate) { retVal = false; },                                                               //
                 [&retVal](uint8_t a) { retVal = (a != 0); },                                                                 //
                 [&retVal](int a) { retVal = (a != 0); },                                                                     //
                 [&retVal](unsigned a) { retVal = (a != 0); },                                                                //
                 [&retVal](boost::rational<int> a) { retVal = (a != 0); },                                                    //
                 [&retVal](float a) { retVal = (a != 0); },                                                                   //
                 [&retVal](double a) { retVal = (a != 0); },                                                                  //
                 [&retVal](std::pair<int, int>) { FatalError("boolCast: pair<int,int> not supported"); },                     //
                 [&retVal](const std::pair<std::string, int> &) { FatalError("boolCast: pair<string,int> not supported"); },  //
                 [&retVal](const std::string &) { FatalError("boolCast: string type not supported"); }                        //
             },
             inVar);

  return retVal;
}

void streamInstance::constructRulesAndUpdate(const query &qry) {
  // Kopia payloadu jest potrzebna tylko do ewaluacji warunkow regul — bez regul nie placimy za nia co interwal.
  if (qry.lRules.empty()) {
    dumpMgr.processStreamChunk(qry.id);
    return;
  }

  rdb::payload payload(*outputPayload->getPayload());

  for (const auto &r : qry.lRules) {
    if (r.condition.empty()) FatalError("streamInstance::constructRulesAndUpdate: rule condition is empty");
    if (r.action != rule::DUMP && r.action != rule::SYSTEM)
      FatalError("streamInstance::constructRulesAndUpdate: unsupported rule action");
    auto condition = r.condition;
    expressionEvaluator expression;
    auto result = expression.eval(condition, &payload);
    if (boolCast(result)) {
      if (r.action == rule::DUMP) {
        dumpMgr.registerTask(qry.id, dumpTask(r.name, r.dumpRange, r.dump_retention));
      } else if (r.action == rule::SYSTEM) {
        auto ret = ::system(r.systemCommand.c_str());
        if (ret == -1) {
          SPDLOG_ERROR("system() call failed");
        } else {
          if (WIFEXITED(ret)) {
            auto exitStatus = WEXITSTATUS(ret);
            if (exitStatus != 0) {
              SPDLOG_ERROR("system() command exited with status: {}", exitStatus);
            }
          } else {
            SPDLOG_ERROR("system() command did not terminate normally");
          }
        }
      }
    }
  }

  dumpMgr.processStreamChunk(qry.id);
}
