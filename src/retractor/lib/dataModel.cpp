#include "dataModel.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>  // std::div
#include <iostream>
#include <memory>  // unique_ptr
#include <regex>
#include <thread>

#include "SOperations.hpp"
#include "expressionEvaluator.h"
#include "rdb/convertTypes.h"

// ctest -R '^ut-dataModel' -V

std::mutex core_mutex;

dataModel::dataModel(qTree &coreInstance) : coreInstance_(coreInstance) {
  //
  // Special parameters support in query set
  // fetch all ':*' - and remove them from coreInstance
  //

  assert(!coreInstance_.empty());
  for (const auto &it : coreInstance_) SPDLOG_INFO("query.id {}", it.id);

  for (const auto &it : coreInstance_)
    if (it.isCompilerDirective()) {
      directive_[it.id] = it.filename;
      SPDLOG_INFO("Assert - directive {}", directive_[it.id]);
      assert(!directive_[it.id].empty());
    }

  auto new_end = std::remove_if(coreInstance_.begin(), coreInstance_.end(),  //
                                [](const query &qry) { return qry.isCompilerDirective(); });
  coreInstance_.erase(new_end, coreInstance_.end());

  SPDLOG_INFO("!Storage path set to : {}", directive_[":STORAGE"]);
  SPDLOG_INFO("!Substrat type is set: {}", directive_[":SUBSTRAT"]);
  SPDLOG_INFO("!Rotation file is set: {}", directive_[":ROTATION"]);
  for (auto &qry : coreInstance_) {
    if (directive_[":SUBSTRAT"].empty())
      qry.substratPolicy = std::make_pair(
          "DEFAULT", rdb::memoryFileAccessor::no_retention);  // <- if substratType is empty - set substratPolicy to 0
  }

  SPDLOG_INFO("Create struct on CORE INSTANCE");
  for (auto &qry : coreInstance_)
    qSet.emplace(qry.id, std::make_unique<streamInstance>(coreInstance_, qry, directive_[":STORAGE"]));
  for (auto const &[key, val] : qSet) val->outputPayload->setDisposable(coreInstance_[key].isDisposable);
}

dataModel::~dataModel() {}

bool dataModel::addQueryToModel(std::string id) {
  if (qSet.find(id) != qSet.end()) {
    SPDLOG_ERROR("dataModel::addQuery: Query with id '{}' already exists in dataModel", id);
    return false;
  }

  auto it = std::find_if(coreInstance_.begin(), coreInstance_.end(), [&](const auto &qry) { return qry.id == id; });
  if (it == coreInstance_.end()) {
    SPDLOG_ERROR("dataModel::addQuery: Query with id '{}' not found in coreInstance", id);
    return false;
  }

  qSet.emplace(id, std::make_unique<streamInstance>(coreInstance_, *it, directive_[":STORAGE"]));
  qSet[id]->outputPayload->setDisposable(coreInstance_[id].isDisposable);

  return true;
}

std::unique_ptr<rdb::payload>::pointer dataModel::getPayload(const std::string &instance,  //
                                                             const int revOffset) {
  if (!qSet[instance]->outputPayload->isDeclared()) {
    qSet[instance]->outputPayload->revRead(revOffset, 0);
  }
  return qSet[instance]->outputPayload->getPayload();
}

void dataModel::processZeroStep() {
  std::lock_guard<std::mutex> scoped_lock(core_mutex);
  for (auto q : coreInstance_) {
    if (!q.isDeclaration()) continue;
    if (qSet[q.id]->outputPayload->bufferState == rdb::sourceState::empty) {
      qSet[q.id]->outputPayload->bufferState = rdb::sourceState::flux;  // Unlock data sources - enable physical read from source
      qSet[q.id]->outputPayload->revRead(0);
      qSet[q.id]->outputPayload->fire();  // chamber_ -> outputPayload
      qSet[q.id]->outputPayload->bufferState = rdb::sourceState::lock;
    }
  }
}

void dataModel::processRows(const std::set<std::string> &inSet) {
  std::lock_guard<std::mutex> scoped_lock(core_mutex);
  // Move ALL armed device read to circular buffer. - no inSet dependent.
  for (auto q : coreInstance_) {
    if (!q.isDeclaration()) continue;
    assert(qSet[q.id]->outputPayload->bufferState != rdb::sourceState::empty);
    if (qSet[q.id]->outputPayload->bufferState == rdb::sourceState::armed) {  // move from fetched bucket to circle buffer.
      qSet[q.id]->outputPayload->fire();                                      // chamber_ -> outputPayload
      qSet[q.id]->outputPayload->bufferState = rdb::sourceState::lock;
    }
  }

  for (auto q : coreInstance_) {
    if (inSet.find(q.id) == inSet.end()) continue;  // Drop off rows that not computed now
    if (q.isDeclaration()) {
      assert(qSet[q.id]->outputPayload->bufferState == rdb::sourceState::lock);  //
      qSet[q.id]->outputPayload->bufferState = rdb::sourceState::flux;  // Unlock data sources - enable physical read from source
      qSet[q.id]->outputPayload->revRead(0);                            // Declarations need to process in separate&first
      assert(qSet[q.id]->outputPayload->bufferState == rdb::sourceState::armed);  //
    } else {
      constructInputPayload(q.id);                    // That will create 'from' clause data set
      qSet[q.id]->constructOutputPayload(q.lSchema);  // That will create all fields from 'select' clause/list
      qSet[q.id]->outputPayload->write();             // That will store data from 'select' clause/list
      qSet[q.id]->constructRulesAndUpdate(q);         // That will process all rules for this query
    }
  }
}

void dataModel::constructInputPayload(const std::string &instance) {
  auto qry = coreInstance_[instance];

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
      const auto timeOffset       = Subtract(coreInstance_.getQuery(nameSrc).rInterval, rationalArgument, lengthOfSrc);

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
      const auto intervalSrc1 = coreInstance_.getQuery(nameSrc1).rInterval;
      const auto intervalSrc2 = coreInstance_.getQuery(nameSrc2).rInterval;

      const auto recordOffset = qSet[instance]->outputPayload->getRecordsCount() - 1;  // TODO: techdebt -1 ?

      // This -1 is here becasue math equations are connecting hash operation on inproper order
      // I've tryed to fix this in getRecordsCount() - but it broke to many unit tests
      // looks like techdebt that need to be adressed in future refactor

      int retPos;
      if (Hash(intervalSrc1, intervalSrc2, recordOffset, retPos)) {
        *(qSet[instance]->inputPayload) = *getPayload(nameSrc2, retPos);
      } else {
        *(qSet[instance]->inputPayload) = *getPayload(nameSrc1, retPos);
      }
    } break;
    default:
      SPDLOG_ERROR("Undefined command_id:{}", static_cast<int>(cmd));
      assert(false && "Undefined command_id in constructInputPayload");
      abort();
  }
}

std::vector<rdb::descFldVT> dataModel::getRow(const std::string &instance, const int timeOffset) {
  std::vector<rdb::descFldVT> retVal;

  auto payload = std::make_unique<rdb::payload>(qSet[instance]->outputPayload->getDescriptor());

  if (!qSet[instance]->outputPayload->isDeclared()) {
    auto success = qSet[instance]->outputPayload->revRead(timeOffset, payload->get());
    assert(success);
  } else {
    *payload = *(qSet[instance]->outputPayload->getPayload());
  }
  auto i{0};
  for (auto f : payload->getDescriptor().fieldsFlat()) {
    retVal.push_back(any_to_variant_cast(payload->getItem(i++)));
  }
  return retVal;
}

size_t dataModel::streamStoredSize(const std::string &instance) {
  return qSet[instance]->outputPayload->getDescriptor().getSizeInBytes() * getStreamCount(instance);
}

size_t dataModel::getStreamCount(const std::string &instance) { return qSet[instance]->outputPayload->getRecordsCount(); }
