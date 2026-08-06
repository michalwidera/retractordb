#include "dataModel.hpp"

#include <algorithm>
#include <iostream>
#include <memory>  // unique_ptr
#include <mutex>

#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>

#include "fatalError.hpp"
#include "rdb/convertTypes.hpp"
#include "rdb/probe.hpp"
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

  for (auto &qry : coreInstance_) {
    auto runtime              = std::make_unique<streamInstance>(coreInstance_, qry, directive_[":STORAGE"]);
    runtime->logicalIndexBase = qry.logicalOrigin;
    qSet.emplace(qry.id, std::move(runtime));
  }
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
  // SELECT dodany do działającego planu nie zaczyna w historycznym origin całego systemu.
  // Jego bazę wyznaczy dokładny pierwszy slot, w którym runtime zobaczy tę instancję.
  if (!it->isDeclaration()) qSet[id]->logicalIndexBase.reset();

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

rdb::payload dataModel::fetchBack(const std::string &instance, const int revOffset) {
  // Odczyt wsteczny o revOffset rekordów — nośnik konwencji operatora przesunięcia.
  //
  // tau_N jest OPÓŹNIENIEM: wynik ma tę samą treść co źródło i pojawia się N slotów później.
  // Konwencja wybrana świadomie, bo odczyt w przód (s_{n+m}) jest nieprzyczynowy dla źródła
  // pracującego na żywo — nie da się wydać próbki, która jeszcze nie powstała.
  //
  // Wcześniej offset był honorowany wyłącznie dla strumieni obliczanych, więc dla źródeł
  // deklarowanych operator przesunięcia był operacją pustą (dwie różne konwencje w jednym
  // silniku). Historia deklaracji leży w buforze kołowym, a jego pojemność zapewnia
  // compiler::computeRequiredCapacities() (capMap[src] >= offset + 1).
  auto &out = *(qSet[instance]->outputPayload);
  out.releaseOnHold();

  const auto available              = static_cast<int>(out.getRecordsCount());
  const bool outsideRetainedHistory = out.isDeclared() && revOffset >= static_cast<int>(out.historySize());
  if (revOffset < 0 || revOffset >= available || outsideRetainedHistory) {
    // Rekord poza zgromadzoną historią — wartość nieokreślona, czyli all-null (pochłaniająca).
    // Ogon strumienia (query::startupLatency) jest tak dobrany, żeby ta ścieżka nie była
    // wykorzystywana na starcie; pozostaje zabezpieczeniem, nie normalną drogą.
    //
    // Poziom ERROR, choć proces nie ginie: defekt D1 (K24) przeżył niezauważony właśnie
    // dlatego, że ten komunikat był na WARN, a Release kompiluje WARN na wylot.
    SPDLOG_ERROR("fetchBack {}: record {} back not available (count={})", instance, revOffset, available);
    rdb::payload nullRecord(out.descriptor);
    nullRecord.setNullBitset(std::vector<bool>(out.descriptor.size(), true));
    return nullRecord;
  }
  if (!out.isDeclared()) {
    out.revRead(static_cast<size_t>(revOffset));
    return *out.getPayload();
  }
  return out.history(static_cast<size_t>(revOffset));
}

rdb::payload dataModel::fetchForward(const std::string &instance, const int forwardIndex) {
  auto &out = *(qSet[instance]->outputPayload);
  out.releaseOnHold();

  // Konwersja indeksu postępującego na offset wsteczny względem bieżącej
  // liczby rekordów źródła — uniezależnia odczyt od kadencji prefetch
  // źródeł deklarowanych i od siatki slotów.
  //
  // forwardIndex jest indeksem LOGICZNYM (walutą wszystkich odwzorowań z SOperations.hpp).
  // Strumień nie ma rekordów o indeksach mniejszych od swojej runtime'owej bazy logicznej,
  // więc rekord o indeksie równym bazie jest fizycznie rekordem 0 w buforze. To jedyne miejsce,
  // w którym ta różnica jest przeliczana — dzięki temu ADD, SUBTRACT, HASH i rozplot dostają
  // poprawkę raz, a nie każdy z osobna.
  const auto count        = static_cast<int>(out.getRecordsCount());
  const auto &logicalBase = qSet[instance]->logicalIndexBase;
  if (!logicalBase.has_value()) {
    FatalError("dataModel::fetchForward: logical index base not initialized for '{}'", instance);
  }
  const int physical = forwardIndex - *logicalBase;
  const int rev      = count - 1 - physical;

  const bool outOfRange = physical < 0 || rev < 0 ||  //
                          (out.isDeclared() && rev >= static_cast<int>(out.historySize()));
  if (outOfRange) {
    // Rekord niedostępny (przyszłość na osi czasu źródła, przed początkiem logicznym
    // albo poza historią bufora) — rekord all-null; o jego losie decyduje ścieżka zapisu.
    // Poziom ERROR z tego samego powodu co w fetchBack powyżej.
    SPDLOG_ERROR("fetchForward {}: record {} not available (count={}, base={})", instance, forwardIndex, count, *logicalBase);
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

void dataModel::processRows(const std::set<std::string> &inSet, const boost::rational<int> &currentTimeSlot) {
  std::scoped_lock scoped_lock(core_mutex);

  // first - process all non-declaration queries
  for (const auto &q : coreInstance_) {
    if (!inSet.contains(q.id)) continue;  // Drop off rows that not computed now
    if (q.isDeclaration()) continue;      // Declarations already processed

    // Ogon strumienia: w tych slotach wynik nie jest jeszcze zdefiniowany, więc strumień NIE emituje
    // rekordu — ani zerowego, ani all-null. NULL jest wartością pochłaniającą (dane oczekiwane a
    // nieobecne, wynik nieistniejący w zbiorze wartości), nigdy rezerwacją miejsca na dane. Długość
    // ogona jest zadeklarowana w planie (query::startupLatency) i raportowana jako 'tail'.
    //
    // Origin dokłada do bramki własny człon o innej naturze: ogon mówi „ten rekord jeszcze nie jest
    // gotowy", origin mówi „tego rekordu nie ma". Pierwszy wyemitowany rekord nosi indeks logiczny
    // równy origin, więc slotów milczenia jest origin + ogon.
    const auto silentSlots = static_cast<size_t>(std::max(q.startupLatency, 0) + std::max(q.logicalOrigin, 0));

    auto &runtime = *qSet[q.id];
    if (!runtime.logicalIndexBase.has_value()) {
      // Instancja ad hoc dołącza do już biegnącej osi. W jej pierwszym należnym slocie T
      // indeks rekordu wynika z definicji chwili emisji:
      //   T = (n + 1 + W) * Delta  =>  n = T/Delta - 1 - W.
      // Nie wolno zaczynać ponownie od query::logicalOrigin, bo fizyczny rekord 0 niósłby
      // wtedy bieżącą wartość oznaczoną historycznym indeksem.
      const auto slotNumber = currentTimeSlot / q.rInterval;
      if (slotNumber.denominator() != 1) {
        FatalError("dataModel::processRows: current slot {}/{} is not aligned with interval {}/{} for '{}'",
                   currentTimeSlot.numerator(), currentTimeSlot.denominator(), q.rInterval.numerator(),
                   q.rInterval.denominator(), q.id);
      }
      const int firstLogicalIndex = slotNumber.numerator() - 1 - q.startupLatency;
      if (firstLogicalIndex < q.logicalOrigin) continue;

      runtime.logicalIndexBase = firstLogicalIndex;
      // Bieżąca oś przeszła już origin i ogon. Ustawienie licznika na granicy
      // pozwala temu samemu wywołaniu wyemitować pierwszy rekord ad hoc.
      runtime.elapsedSlots = silentSlots;
    }
    if (runtime.elapsedSlots++ < silentSlots) continue;

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

  // Indeks LOGICZNY rekordu, który właśnie powstaje. Liczba rekordów w buforze jest indeksem
  // fizycznym; strumień o niezerowym origin nie ma rekordów przed origin, więc jego rekord
  // fizyczny 0 nosi indeks logiczny origin. Wszystkie odwzorowania z SOperations.hpp są
  // zdefiniowane na indeksach logicznych, więc karmimy je tą wartością.
  const auto logicalIndexBase = [&](const std::string &id) {
    const auto &logicalBase = qSet[id]->logicalIndexBase;
    if (!logicalBase.has_value()) {
      FatalError("dataModel::constructInputPayload: logical index base not initialized for '{}'", id);
    }
    return *logicalBase;
  };
  const auto logicalIndex = [&](const std::string &id) {
    return static_cast<int>(qSet[id]->outputPayload->getRecordsCount()) + logicalIndexBase(id);
  };

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

      *(qSet[instance]->inputPayload) = fetchBack(nameSrc, timeOffset);
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
      const auto n = logicalIndex(instance);

      int fwdPos = -1;
      if (cmd == STREAM_DEHASH_DIV) {
        // Θ: a_n = c_{n+⌈(n+1)·Δa/Δb⌉} — element c o tym indeksie powstaje dopiero PO slocie n
        // strumienia wynikowego (definicja jest o jeden slot nieprzyczynowa). Przyczynowość
        // zapewnia ogon strumienia (query::startupLatency zawiera dla Θ dodatkowy slot): przez ten
        // slot strumień nie emituje niczego. Rekord n jest więc już a_n, bez przesunięcia o jeden
        // i bez rekordu-zastępnika — placeholder byłby użyciem NULL/zera jako rezerwacji miejsca.
        fwdPos = Div(qry.rInterval, rationalArgument, n);
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
      const auto n                = logicalIndex(instance);
      const auto forwardIndex     = Subtract(coreInstance_.getQuery(nameSrc).rInterval, rationalArgument, n);

      *(qSet[instance]->inputPayload) = fetchForward(nameSrc, forwardIndex);
    } break;
    case STREAM_ADD: {
      // 	:- PUSH_STREAM(core0)
      //  :- PUSH_STREAM(core1)
      //  :- STREAM_ADD
      //
      if (arg.size() != 3) FatalError("dataModel::constructInputPayload: STREAM_ADD expects 3 tokens");

      const auto nameSrc1 = arg[0].getStr_();
      const auto nameSrc2 = arg[1].getStr_();

      // K24/P2 wariant A: składowe są czytane po indeksie POSTĘPUJĄCYM z Definicji sumy
      // strumieni (c_n = (a_n, b_{⌊nΔa/Δb⌋})), a nie jako bieżący payload obu składowych.
      // Bieżący payload dawał b_{⌊(n+1)Δa/Δb⌋} — rekord wolniejszej składowej domknięty
      // dopiero na koniec slotu, czyli w slocie n jeszcze nieokreślony.
      //
      // n — 0-bazowy indeks rekordu wyjściowego (indeks c_n z definicji).
      const auto n = logicalIndex(instance);

      const auto fwdPos1 = Add(qry.rInterval, coreInstance_.getQuery(nameSrc1).rInterval, n);
      const auto fwdPos2 = Add(qry.rInterval, coreInstance_.getQuery(nameSrc2).rInterval, n);

      // operator + from payload payload::operator+(payload &other) step into action here
      // TODO support renaming of double-same fields after merge?

      rdb::probe::onAddMerge();
      *(qSet[instance]->inputPayload) = fetchForward(nameSrc1, fwdPos1) + fetchForward(nameSrc2, fwdPos2);
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
      // Okno jest stemplowane końcem przedziału, więc rekord o indeksie logicznym n sięga
      // wstecz od pozycji n*step. Runtime'owa baza źródła przesuwa jego pozycje
      // spłaszczone o base*F. Dla planu startowego jest równa origin kompilatora,
      // ale zapytanie dodane ad hoc dostaje ją z bieżącej osi logicznej.
      const int windowIndex           = logicalIndex(instance);
      const int sourceIndexBase       = logicalIndexBase(nameSrc);
      *(qSet[instance]->inputPayload) = qSet[nameSrc]->constructAgsePayload(length, step, nameSrc, windowIndex, sourceIndexBase);
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
      const auto n = logicalIndex(instance);

      rdb::probe::onHashPick();
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
