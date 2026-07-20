#include "dataModel.hpp"

#include <algorithm>
#include <iostream>
#include <memory>  // unique_ptr
#include <mutex>

#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>

#include "fatalError.hpp"
#include "rdb/convertTypes.hpp"
#include "SOperations.hpp"

// ctest -R '^ut-dataModel' -V

std::mutex core_mutex;

dataModel::dataModel(qTree &coreInstance) : coreInstance_(coreInstance) {
  //
  // Special parameters support in query set
  // fetch all ':*' - and remove them from coreInstance
  //

  if (coreInstance_.empty()) FatalError("dataModel: coreInstance is empty — no queries to process");

  for (const auto &it : coreInstance_)
    if (it.isCompilerDirective()) {
      directive_[it.id] = it.filename;
      if (directive_[it.id].empty()) {
        FatalError("dataModel: compiler directive '{}' has empty value", it.id);
      }
    }

  auto removed = std::ranges::remove_if(coreInstance_,  //
                                        [](const query &qry) { return qry.isCompilerDirective(); });
  coreInstance_.erase(removed.begin(), removed.end());

  for (auto &qry : coreInstance_)
    qSet.emplace(qry.id, std::make_unique<streamInstance>(coreInstance_, qry, directive_[":STORAGE"]));
  for (auto const &[key, val] : qSet)
    val->outputPayload->setDisposable(coreInstance_[key].isDisposable);
}

dataModel::~dataModel() = default;

bool dataModel::addQueryToModel(const std::string &id) {
  if (qSet.contains(id)) {
    SPDLOG_ERROR("dataModel::addQuery: Query with id '{}' already exists in dataModel", id);
    return false;
  }

  auto it = std::ranges::find_if(coreInstance_, [&](const auto &qry) { return qry.id == id; });
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
  // This gePayload is called by constructInputPayload algebraic functions
  // that need to access different streams from qSet
  // this also need to release HOLD state if set for each stream before read
  qSet[instance]->outputPayload->releaseOnHold();

  if (!qSet[instance]->outputPayload->isDeclared()) {
    qSet[instance]->outputPayload->revRead(revOffset);
  }
  return qSet[instance]->outputPayload->getPayload();
}

rdb::payload dataModel::fetchForward(const std::string &instance, const int forwardIndex) {
  auto &out = *(qSet[instance]->outputPayload);
  out.releaseOnHold();

  // Konwersja indeksu postępującego na offset wsteczny względem bieżącej
  // liczby rekordów źródła — uniezależnia odczyt od kadencji prefetch
  // źródeł deklarowanych i od siatki slotów.
  const auto count = static_cast<int>(out.getRecordsCount());
  const int rev    = count - 1 - forwardIndex;

  const bool outOfRange = forwardIndex < 0 || rev < 0 ||  //
                          (out.isDeclared() && rev >= static_cast<int>(out.historySize()));
  if (outOfRange) {
    // Rekord niedostępny (przyszłość na osi czasu źródła albo poza historią
    // bufora) — rekord all-null; o jego losie decyduje ścieżka zapisu.
    SPDLOG_WARN("fetchForward {}: record {} not available (count={})", instance, forwardIndex, count);
    rdb::payload nullRecord(out.descriptor);
    nullRecord.setNullBitset(std::vector<bool>(out.descriptor.size(), true));
    return nullRecord;
  }

  if (out.isDeclared()) return out.history(static_cast<size_t>(rev));

  out.revRead(static_cast<size_t>(rev));
  return *out.getPayload();
}

void dataModel::processZeroStep() {
  std::scoped_lock scoped_lock(core_mutex);
  for (auto q : coreInstance_) {
    if (!q.isDeclaration()) continue;

    if (qSet.at(q.id)->outputPayload->bufferState != rdb::sourceState::empty) {
      FatalError("dataModel::processZeroStep: stream '{}' not in empty state at start of cycle", q.id);
    }
    qSet[q.id]->outputPayload->bufferState = rdb::sourceState::flux;  // Unlock data sources - enable physical read from source
    qSet[q.id]->outputPayload->revRead(0);                            // state -> armed
    qSet[q.id]->outputPayload->fire();                                // chamber_ -> outputPayload
    if (qSet.at(q.id)->outputPayload->bufferState != rdb::sourceState::armed) {
      FatalError("dataModel::processZeroStep: stream '{}' not armed after fire()", q.id);
    }
  }
}

void dataModel::processRows(const std::set<std::string> &inSet) {
  std::scoped_lock scoped_lock(core_mutex);

  // first - process all non-declaration queries
  for (const auto &q : coreInstance_) {
    if (!inSet.contains(q.id)) continue;  // Drop off rows that not computed now
    if (q.isDeclaration()) continue;      // Declarations already processed

    constructInputPayload(q.id);                    // That will create 'from' clause data set
    qSet[q.id]->constructOutputPayload(q.lSchema);  // That will create all fields from 'select' clause/list
    qSet[q.id]->outputPayload->write();             // That will store data from 'select' clause/list
    qSet[q.id]->constructRulesAndUpdate(q);         // That will process all rules for this query
  }

  // Then - process all declarations to unlock them for next step
  for (const auto &q : coreInstance_) {
    if (!inSet.contains(q.id)) continue;  // Drop off rows that not computed now
    if (!q.isDeclaration()) continue;     // first declarations need to be processed

    if (qSet[q.id]->outputPayload->bufferState != rdb::sourceState::armed) continue;  // already processed
    qSet[q.id]->outputPayload->bufferState = rdb::sourceState::flux;  // Unlock data sources - enable physical read from source
    qSet[q.id]->outputPayload->revRead(0);                            // Declarations need to process in separate&first
    qSet[q.id]->outputPayload->fire();                                // chamber_ -> outputPayload
    if (qSet.at(q.id)->outputPayload->bufferState != rdb::sourceState::armed) {
      FatalError("dataModel::processRows: stream '{}' not armed after processing", q.id);
    }
  }
}

void dataModel::constructInputPayload(const std::string &instance) {
  const query &qry = coreInstance_[instance];

  if (qry.lProgram.size() >= 4) {
    FatalError("dataModel::constructInputPayload: program not optimized — {} tokens for query '{}', expected < 4",
               qry.lProgram.size(), instance);
  }

  std::vector<token> arg;
  std::ranges::copy(qry.lProgram, std::back_inserter(arg));
  // same: for (auto tk : qry.lProgram) arg.push_back(tk);

  auto operation = qry.lProgram.back();  // Operation is always last element on stack

  const command_id cmd = operation.getCommandID();
  switch (cmd) {
    case PUSH_STREAM: {
      // 	:- PUSH_STREAM(core0)
      //
      if (arg.size() != 1) FatalError("dataModel::constructInputPayload: PUSH_STREAM expects 1 token");

      const auto nameSrc = operation.getStr_();

      *(qSet[instance]->inputPayload) = *getPayload(nameSrc);
    } break;
    case STREAM_TIMEMOVE: {
      // 	:- PUSH_STREAM(core0)
      //  :- STREAM_TIMEMOVE(1)
      //
      if (arg.size() != 2) FatalError("dataModel::constructInputPayload: STREAM_TIMEMOVE expects 2 tokens");

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
      if (arg.size() != 3) FatalError("dataModel::constructInputPayload: STREAM_DEHASH expects 3 tokens");

      const auto nameSrc          = arg[0].getStr_();
      const auto rationalArgument = arg[1].getRI();

      if (rationalArgument <= 0) {
        FatalError("dataModel::constructInputPayload: DEHASH rational argument must be positive");
      }

      // n — 0-bazowy indeks rekordu wyjściowego; Div/Mod (SOperations.hpp)
      // zwracają indeks POSTĘPUJĄCY elementu w strumieniu przeplecionym.
      const auto n = static_cast<int>(qSet[instance]->outputPayload->getRecordsCount());

      int fwdPos = -1;
      if (cmd == STREAM_DEHASH_DIV) {
        // Θ: a_n = c_{n+⌈(n+1)·Δa/Δb⌉} — element c o tym indeksie powstaje
        // dopiero PO slocie n strumienia wynikowego (definicja jest o jeden
        // slot nieprzyczynowa). Realizacja przyczynowa: opóźnienie o jeden
        // slot — rekord n zawiera a_{n-1}, rekord 0 jest all-null.
        fwdPos = (n == 0) ? -1 : Div(qry.rInterval, rationalArgument, n - 1);
      } else {
        // ~Θ: b_n = c_{n+⌊n·Δb/Δa⌋} — dostępny w swoim slocie.
        fwdPos = Mod(rationalArgument, qry.rInterval, n);
      }
      *(qSet[instance]->inputPayload) = fetchForward(nameSrc, fwdPos);
    } break;
    case STREAM_SUM:
    case STREAM_AVG:
    case STREAM_MIN:
    case STREAM_MAX: {
      const auto nameSrc = arg[0].getStr_();

      *(qSet[instance]->inputPayload) = qSet[nameSrc]->reduceFieldsToPayload(cmd, instance + "_0");
    } break;
    case STREAM_SUBTRACT: {
      //  :- PUSH_STREAM(core0)
      //  :- STREAM_SUBTRACT(1/2)
      //
      if (arg.size() != 2) FatalError("dataModel::constructInputPayload: STREAM_SUBTRACT expects 2 tokens");

      const auto nameSrc          = arg[0].getStr_();
      const auto rationalArgument = arg[1].getRI();
      const auto lengthOfSrc      = qSet[nameSrc]->outputPayload->getRecordsCount();
      const auto timeOffset =
          Subtract(coreInstance_.getQuery(nameSrc).rInterval, rationalArgument, static_cast<int>(lengthOfSrc));

      *(qSet[instance]->inputPayload) = *getPayload(nameSrc, timeOffset);
    } break;
    case STREAM_ADD: {
      // 	:- PUSH_STREAM(core0)
      //  :- PUSH_STREAM(core1)
      //  :- STREAM_ADD
      //
      if (arg.size() != 3) FatalError("dataModel::constructInputPayload: STREAM_ADD expects 3 tokens");

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
      if (arg.size() != 2) FatalError("dataModel::constructInputPayload: STREAM_AGSE expects 2 tokens");

      const auto nameSrc  = arg[0].getStr_();  // * INFO Sync with query.cpp
      auto [step, length] = get<std::pair<int, int>>(operation.getVT());
      if (step <= 0) {
        FatalError("dataModel::constructInputPayload: AGSE step must be > 0, got {} for '{}'", step, instance);
      }
      const int storedRecordsInOutput = static_cast<int>(qSet[instance]->outputPayload->getRecordsCount());
      *(qSet[instance]->inputPayload) = qSet[nameSrc]->constructAgsePayload(length, step, nameSrc, storedRecordsInOutput);
    } break;
    case STREAM_HASH: {
      // 	:- PUSH_STREAM(core0)
      //  :- PUSH_STREAM(core1)
      //  :- STREAM_HASH
      //
      if (arg.size() != 3) FatalError("dataModel::constructInputPayload: STREAM_HASH expects 3 tokens");

      const auto nameSrc1     = arg[0].getStr_();
      const auto nameSrc2     = arg[1].getStr_();
      const auto intervalSrc1 = coreInstance_.getQuery(nameSrc1).rInterval;
      const auto intervalSrc2 = coreInstance_.getQuery(nameSrc2).rInterval;

      // n — 0-bazowy indeks rekordu wyjściowego (indeks c_n z definicji
      // przeplotu); Hash zwraca indeks POSTĘPUJĄCY elementu składowej.
      const auto n = static_cast<int>(qSet[instance]->outputPayload->getRecordsCount());

      int fwdPos                      = 0;
      const bool takeSecond           = Hash(intervalSrc1, intervalSrc2, n, fwdPos);
      *(qSet[instance]->inputPayload) = fetchForward(takeSecond ? nameSrc2 : nameSrc1, fwdPos);

    } break;
    default:
      FatalError("dataModel::constructInputPayload: undefined command_id {}", static_cast<int>(cmd));
  }
}

std::vector<rdb::descFldVT> dataModel::getRow(const std::string &instance, const int timeOffset) {
  std::vector<rdb::descFldVT> retVal;

  auto payload = std::make_unique<rdb::payload>(qSet[instance]->outputPayload->descriptor);

  if (!qSet[instance]->outputPayload->isDeclared()) {
    auto success = qSet[instance]->outputPayload->revRead(timeOffset, payload->span().data());
    if (!success) {
      FatalError("dataModel::getRow: revRead failed for stream '{}' at timeOffset {}", instance, timeOffset);
    }
  } else {
    *payload = *(qSet[instance]->outputPayload->getPayload());
  }
  auto i{0};
  for (const auto &f : payload->descriptor.dataFields()) {
    auto valueOpt = payload->getItem(i++);
    if (valueOpt.has_value()) {
      retVal.push_back(any_to_variant_cast(valueOpt.value()));
      continue;
    }

    retVal.push_back(nullFallbackValue(f.rtype));
  }
  return retVal;
}

size_t dataModel::streamStoredSize(const std::string &instance) {
  return qSet[instance]->outputPayload->descriptor.getSizeInBytes() * getStreamCount(instance);
}

size_t dataModel::getStreamCount(const std::string &instance) { return qSet[instance]->outputPayload->getRecordsCount(); }
