#include "compiler.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <utility>  // std::pair
#include <vector>

#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>
#include <boost/rational.hpp>
#include <boost/regex.hpp>

#include "exprSimplify.hpp"  // simplifyExpression
#include "fatalError.hpp"
#include "rdb/probe.hpp"    // sonda E3: rozmiar planu, czas kompilacji
#include "SOperations.hpp"  // ceilR

using boost::lexical_cast;

void requireResolvedForEveryNode(const qTree &plan, const std::map<std::string, int> &resolved, std::string_view pass,
                                 std::string_view quantity) {
  // Reguła: kompilator nie wypuszcza planu z węzłem, dla którego nie policzył origin albo ogona.
  //
  // Do 2026-08-07 stało tu SPDLOG_WARN i `continue`. Ponieważ query::logicalOrigin
  // i query::startupLatency mają wartość domyślną 0, węzeł pominięty dostawał ZERO — czyli
  // najgroźniejszą z możliwych wartości: nie ma w niej ani niedefiniowalności, ani oczekiwania.
  // To jest dokładnie reżim ZANIŻAJĄCY, który tabela dokładności ogona wyklucza dla wszystkich
  // dziewięciu klas operatorów. Ostrzeżenie szło przy tym do logu, którego ctest nie czyta,
  // więc plan degradował po cichu.
  //
  // Że to błąd, a nie stan dopuszczalny, wynika z kształtu programu klauzuli FROM: ma on 1, 2
  // albo 3 tokeny i ZAWSZE zaczyna się od PUSH_STREAM — każdy inny kształt zatrzymuje wcześniej
  // compiler::resolveStreamIntervals(). Deklaracje i dyrektywy kompilatora są zaszczepiane
  // zerem przed pętlą. Nie ma więc legalnego planu, który zostawia węzeł nierozwiązany;
  // jeżeli tak się stanie, jest to defekt kompilatora — awaria aparatury, nie wynik.
  for (const auto &q : plan)
    if (!resolved.contains(q.id)) FatalError("{}: unresolved {} for '{}'", pass, quantity, q.id);
}

namespace {
/// Wyliczanie interwałów w szerszym typie, z kontrolą zakresu.
///
/// Wzory operatorów mnożą licznik przez licznik i mianownik przez mianownik
/// ((D_a*D_b)/(D_a+D_b) dla przeplotu, (D_a*D_b)/|D_a-D_b| dla rozplotu), więc
/// dla interwałów o licznikach rzędu 10^4 iloczyn wychodzi poza int.
/// boost::rational<int> nie wykrywa przepełnienia — wynik jest wtedy cichym
/// śmieciem, który ujawnia się dopiero jako niezwiązany błąd walidacji planu
/// ("You cannot make faster div from slower source"). Liczymy więc w 64 bitach
/// i sprawdzamy, czy wynik daje się w ogóle zapisać w typie interwału.
using wideRational = boost::rational<std::int64_t>;

wideRational widen(const boost::rational<int> &value) { return wideRational{value.numerator(), value.denominator()}; }

wideRational widen(int value) { return wideRational{value, 1}; }

boost::rational<int> narrowInterval(const wideRational &value, const std::string &id, const char *formula) {
  constexpr std::int64_t limit = std::numeric_limits<int>::max();
  if (value.numerator() > limit || value.numerator() < std::numeric_limits<int>::min() || value.denominator() > limit) {
    SPDLOG_ERROR("compiler: interval {}/{} of stream '{}' ({}) is out of representable range", value.numerator(),
                 value.denominator(), id, formula);
    throw std::out_of_range("Stream interval out of representable range — simplify the plan or use coarser intervals");
  }
  return boost::rational<int>{static_cast<int>(value.numerator()), static_cast<int>(value.denominator())};
}
}  // namespace

namespace localContext {
boost::regex xprFieldId5(R"((\w*)\[(\d*)\]\[(\d*)\])");  // something[1][1]
boost::regex xprFieldId4(R"((\w*)\[(\d*)\,(\d*)\])");    // something[1,1]
boost::regex xprFieldId2(R"((\w*)\[(\d*)\])");           // something[1]
boost::regex xprFieldIdX("(\\w*)\\[_]");                 // something[_]
boost::regex xprFieldId1("(\\w*).(\\w*)");               // something.in_schema
boost::regex xprFieldId3("(\\w*)");                      // field_of_corn
}  // namespace localContext

using namespace localContext;

/** This procedure computes time delays (delta) for generated streams */
std::string compiler::resolveStreamIntervals() {
  while (true) {
    bool bOnceAgain(false);
    size_t unresolvedCount  = 0;
    size_t resolvedThisPass = 0;
    coreInstance.sort();
    for (auto &q : coreInstance) {
      if (q.lProgram.empty()) {
        continue; /* Declaration */
      }
      if (q.lProgram.size() == 1) {
        token tInstance(*(q.lProgram.begin()));
        // Źródło nierozwiązane w tym przebiegu daje deltę 0. Bez tej kontroli
        // strumień dostawał interwał 0 na stałe i NIKT nie prosił o kolejny
        // przebieg — o wyniku decydowała kolejność po coreInstance.sort().
        const boost::rational<int> sourceDelta = coreInstance.getDelta(tInstance.getStr_());
        if (sourceDelta == 0) {
          bOnceAgain = true;
          unresolvedCount++;
          continue;
        }
        if (q.rInterval == 0) resolvedThisPass++;
        q.rInterval = sourceDelta;
        continue;  // Just one stream
      }
      if (q.lProgram.size() != 3 && q.lProgram.size() != 2) {
        FatalError("compiler::prepareFields: unexpected program size {} for query '{}'", q.lProgram.size(), q.id);
      }
      // This is shit coded (these size2 i size3) and fast fixed
      bool size3           = (q.lProgram.size() == 3);
      std::list<token> loc = q.lProgram;
      token t1(*loc.begin());
      if (size3) loc.pop_front();
      token t2(*loc.begin());
      loc.pop_front();
      token op(*loc.begin());
      loc.pop_front();
      boost::rational<int> delta(-1);
      switch (op.getCommandID()) {
        case STREAM_HASH: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = coreInstance.getDelta(t2.getStr_());
          if (delta1 == 0 || delta2 == 0) {
            bOnceAgain = true;
            unresolvedCount++;
            continue;
          }
          delta = narrowInterval((widen(delta1) * widen(delta2)) / (widen(delta1) + widen(delta2)), q.id,
                                 "(D_a*D_b)/(D_a+D_b)");  // deltaHash(delta1, delta2);
        } break;
        case STREAM_DEHASH_DIV: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = t2.getRI();  // There is no second stream
          // - just fraction argument
          if (delta2 == 0) {
            FatalError("compiler: DEHASH rational argument must not be zero for '{}'", q.id);
          }
          if (delta1 == 0) {
            bOnceAgain = true;
            unresolvedCount++;
            continue;
          }  //           D_c * D_b
          //   D_a = --------------
          //         abs(D_c - D_b)
          delta = narrowInterval((widen(delta1) * widen(delta2)) / abs(widen(delta1) - widen(delta2)), q.id,
                                 "(D_c*D_b)/|D_c-D_b|");  // deltaDivMod(delta1, delta2);

          if (delta1 > delta) {
            SPDLOG_ERROR("Faster div from slower src q.id={}", q.id);
            throw std::out_of_range("You cannot make faster div from slower source");
          }
        } break;
        case STREAM_DEHASH_MOD: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = t2.getRI();
          if (delta2 == 0) {
            FatalError("compiler: DEHASH rational argument must not be zero for '{}'", q.id);
          }
          if (delta1 == 0) {
            bOnceAgain = true;
            unresolvedCount++;
            continue;
          }  //           D_c * D_a
          //   D_b = --------------
          //         abs(D_c - D_a)
          delta = narrowInterval((widen(delta2) * widen(delta1)) / abs(widen(delta2) - widen(delta1)), q.id,
                                 "(D_c*D_a)/|D_c-D_a|");  // deltaDivMod(delta2, delta1);  (NOTICE DIFF SEQ!)

          if (delta1 > delta) {
            SPDLOG_ERROR("Faster div from slower src q.id={}", q.id);
            throw std::out_of_range("You cannot make faster mod from slower source");
          }
        } break;
        case STREAM_SUBTRACT: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = op.getRI();
          if (delta1 == 0) {
            bOnceAgain = true;
            unresolvedCount++;
            continue;
          }
          if (delta2 <= 0) {
            FatalError("compiler: SUBTRACT target interval must be positive for '{}'", q.id);
          }
          delta = delta2;
        } break;
        case STREAM_ADD: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = coreInstance.getDelta(t2.getStr_());
          if (delta1 == 0 || delta2 == 0) {
            bOnceAgain = true;
            unresolvedCount++;
            continue;
          }
          delta = std::min(delta1, delta2);  // deltaAdd(delta1, delta2);
        } break;
        case STREAM_AVG:
        case STREAM_MIN:
        case STREAM_MAX:
        case STREAM_SUM:
        // Delta UNCHANGED ! (like time move)
        case STREAM_TIMEMOVE: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          if (delta1 == 0) {
            bOnceAgain = true;
            unresolvedCount++;
            continue;
          }
          delta = delta1;
        } break;
        case STREAM_AGSE: {
          // ->>> check need
          // core1@(5,3) ->
          // push_stream core0 -> deltaSrc
          // stream agse <5,3> -> step_of_window,size_of_window
          boost::rational<int> coreDelta = coreInstance.getDelta(t1.getStr_());
          if (coreDelta == 0) {
            bOnceAgain = true;
            unresolvedCount++;
            continue;
          }
          int coreWindow          = static_cast<int>(coreInstance.getQuery(t1.getStr_()).lSchema.size());
          auto [step, windowSize] = std::get<std::pair<int, int>>(op.getVT());
          if (step <= 0) {
            FatalError("compiler::prepareFields: AGSE step must be > 0, got {} for query '{}'", step, q.id);
          }
          windowSize = abs(windowSize);
          // if (windowSize < 0) {  // windowSize < 0  (need to double-check and UT cover)
          //   delta = deltaSrc / windowSizeSrc;
          //   delta *= abs(windowSize);
          //   delta /= step;
          // } else
          // delta = (deltaSrc / windowSizeSrc) * step;

          delta = narrowInterval((widen(coreDelta) * widen(step)) / widen(coreWindow), q.id, "(D_src*k)/F");
        } break;
        default:
          SPDLOG_ERROR("Undefined token: command={}", op.getStrCommandID());
          throw std::out_of_range("Undefined token/command on list");
      }  // switch ( op.getCommandID() )
      if (delta == -1) {
        FatalError("compiler::prepareFields: stream interval (delta) not resolved for query '{}'", q.id);
      }
      if (q.rInterval == 0) resolvedThisPass++;
      q.rInterval = delta;  // There is established delta value - return value
    }  // BOOST_FOREACH ( query & q , coreInstance )
    if (!bOnceAgain) break;
    // Cyklem jest dopiero BRAK POSTĘPU: przebieg, w którym nie rozwiązano ani
    // jednego strumienia. Dawny warunek (unresolvedCount >= prevUnresolved)
    // wymagał ŚCISŁEGO spadku licznika w każdym przebiegu, więc był heurystyką
    // postępu, a nie detektorem cykli — i odrzucał plany bezcykliczne zależnie
    // od tego, jak coreInstance.sort() ustawił kolejność oceny.
    if (resolvedThisPass == 0) {
      SPDLOG_ERROR("Circular dependency: stream interval resolution stalled with {} unresolved streams", unresolvedCount);
      return {"Circular dependency in stream definitions"};
    }
    coreInstance.sort();
  }  // while(true)
  coreInstance.sort();
  return {"OK"};
}

std::string compiler::composeStreamName(const std::string &sName1, const std::string &sName2, command_id cmd) {
  if (sName2.empty()) return std::string(GetStringcommand_id(cmd)) + std::string("_") + sName1;
  return std::string(GetStringcommand_id(cmd)) + std::string("_") + sName2 + std::string("_") + sName1;
}

namespace {
/// Czy operator klauzuli FROM zjada DWA poprzedzające tokeny, czy jeden.
///
/// Rozstrzyga o tym miejsce parametru, nie „dwuargumentowość" w potocznym sensie:
///   * `#`, `+` — dwa strumienie, więc dwa tokeny PUSH_STREAM;
///   * `&`, `%` — strumień i liczba wymierna, więc PUSH_STREAM + PUSH_VAL;
///   * `>N`, `-r`, `@(k,L)` — parametr siedzi W SAMYM TOKENIE operatora
///     (patrz RQLParser: `recpToken(STREAM_TIMEMOVE, int)`, `recpToken(STREAM_SUBTRACT, rational)`,
///     `program.emplace_back(STREAM_AGSE, pair)`), więc poprzedza je JEDEN token.
///
/// Lista jest POZYTYWNA celowo. Do 2026-08-09 stała tu czarna lista
/// (`cmd != STREAM_TIMEMOVE && cmd != STREAM_SUBTRACT`), przez którą `@` uchodziło za
/// dwutokenowe. Dla `(A@(1,4))>1` po zdjęciu jedynego argumentu `@` lista programu miała
/// już tylko `[STREAM_TIMEMOVE]`, a kod sięgał po drugi argument: dereferencjonował
/// WARTOWNIKA listy i go kasował. Skutkiem było uszkodzenie sterty ujawniane dopiero
/// w qTree::topologicalSort() jako odczyt zwolnionej pamięci. Czarna lista milczy przy
/// każdym nowym operatorze; pozytywna zawodzi w stronę bezpieczną — nowy operator jest
/// domyślnie jednotokenowy i nie sięga poza początek listy.
bool consumesTwoPrecedingTokens(command_id cmd) {
  switch (cmd) {
    case STREAM_HASH:
    case STREAM_ADD:
    case STREAM_DEHASH_DIV:
    case STREAM_DEHASH_MOD:
      return true;
    default:
      return false;
  }
}
}  // namespace

/* Goal of this procedure is to provide stream to canonical form
TODO: Stream_MAX,MIN,AVG...
*/
std::string compiler::extractIntermediateStreams() {
  coreInstance.sort();

  auto substratType_C = std::string("DEFAULT");
  auto substratTypeIt = std::ranges::find_if(coreInstance,  //
                                             [](const auto &qry) { return qry.id == ":SUBSTRAT"; });
  if (substratTypeIt != std::end(coreInstance)) substratType_C = substratTypeIt->filename;
  std::ranges::transform(substratType_C, substratType_C.begin(), ::toupper);

  for (size_t queryIndex = 0; queryIndex < coreInstance.size(); ++queryIndex) {
    // Optimization phase 2. Redukuj jedno zapytanie do punktu stałego;
    // push_back() może unieważnić iteratory qTree, dlatego zapytanie jest
    // pobierane ponownie po indeksie w każdej rundzie.
    while (coreInstance.at(queryIndex).isReductionRequired()) {
      bool extracted     = false;
      auto &currentQuery = coreInstance.at(queryIndex);
      for (auto it2 = currentQuery.lProgram.begin(); it2 != currentQuery.lProgram.end(); ++it2) {
        if (                                              //
            (*it2).getStrCommandID() != "PUSH_STREAM" &&  //
            (*it2).getStrCommandID() != "PUSH_VAL") {
          query newQuery;
          std::string arg1;
          std::string arg2;

          const command_id cmd = (*it2).getCommandID();
          const int argCount   = consumesTwoPrecedingTokens(cmd) ? 2 : 1;

          // Operator poprzedza w programie DOKŁADNIE argCount tokenów; cofamy się na pierwszy
          // z nich i wycinamy cały podciąg [argumenty..., operator] jednym erase().
          //
          // Dawniej robiło to erase() przeplatane z `--it2`. Gdy kasowany token stał na początku
          // listy, dekrementacja schodziła PRZED begin() — formalnie zachowanie niezdefiniowane,
          // działające wyłącznie dlatego, że std::list w libstdc++ jest cyklem z wartownikiem
          // i `++` wracało na begin(). Ta sama konstrukcja o jeden krok dalej (sięgnięcie po
          // nieistniejący drugi argument `@`) kasowała wartownika i psuła stertę.
          if (std::distance(currentQuery.lProgram.begin(), it2) < argCount) {
            FatalError("compiler::extractIntermediateStreams: operator '{}' in query '{}' has {} preceding tokens, needs {}",
                       GetStringcommand_id(cmd), currentQuery.id, std::distance(currentQuery.lProgram.begin(), it2), argCount);
          }
          auto firstArg = it2;
          std::advance(firstArg, -argCount);
          const auto afterOperator = std::next(it2);

          // Kolejność w substracie zostaje taka jak w programie źródłowym.
          newQuery.lProgram.assign(firstArg, afterOperator);

          // arg1 to token STOJĄCY BEZPOŚREDNIO PRZED operatorem, arg2 — ten przed nim.
          auto argIt = firstArg;
          if (argCount == 2) {
            arg2 = (*argIt).getStr_();
            ++argIt;
          }
          arg1 = (*argIt).getStr_();

          std::list<token> lTempProgram;
          lTempProgram.emplace_back(PUSH_TSCAN);
          newQuery.lSchema.emplace_back(rdb::rField("*", 1, 1, rdb::BYTE), lTempProgram);
          newQuery.policy     = std::make_pair(substratType_C, 1);
          newQuery.id         = composeStreamName(arg1, arg2, cmd);
          newQuery.isSubstrat = true;
          it2                 = currentQuery.lProgram.erase(firstArg, afterOperator);
          currentQuery.lProgram.insert(it2, token(PUSH_STREAM, newQuery.id));
          coreInstance.push_back(newQuery);
          extracted = true;
          break;
        }  // Endif PUSH_STREAM, PUSH_VAL
      }  // Endfor
      if (!extracted) {
        FatalError("compiler::extractIntermediateStreams: query '{}' requires reduction but no operator was extracted",
                   coreInstance.at(queryIndex).id);
      }
    }  // Endwhile
  }  // Endfor
  return {"OK"};
}

// Goal of this procedure is to unroll schema based of given command
std::list<field> compiler::buildOutputSchema(const std::string &sName1, const std::string &sName2, token &cmd_token) {
  std::list<field> lRetVal;
  const command_id cmd = cmd_token.getCommandID();
  // Merge of schemas for junction of hash type
  if (cmd == STREAM_HASH) {
    if (coreInstance.getQuery(sName1).descriptorStorage().flatElementCount() !=
        coreInstance.getQuery(sName2).descriptorStorage().flatElementCount())
      throw std::invalid_argument("Hash operation needs same schemas on arguments stream");
    lRetVal = coreInstance.getQuery(sName1).lSchema;
  } else if (cmd == STREAM_DEHASH_DIV || cmd == STREAM_DEHASH_MOD)
    lRetVal = coreInstance.getQuery(sName1).lSchema;  // NOLINT(bugprone-branch-clone)
  else if (cmd == STREAM_ADD) {
    int fieldCountSh = 0;
    int i            = 0;
    for (const auto &f : coreInstance.getQuery(sName1).lSchema) {
      field intf(rdb::rField(sName1 + "_" + boost::lexical_cast<std::string>(fieldCountSh++), f.field_.rlen, f.field_.rarray,
                             f.field_.rtype),
                 token(PUSH_ID, std::make_pair(sName1, i++)));
      lRetVal.push_back(intf);
    }
    i = 0;
    for (const auto &f : coreInstance.getQuery(sName2).lSchema) {
      field intf(rdb::rField(sName2 + "_" + boost::lexical_cast<std::string>(fieldCountSh++), f.field_.rlen, f.field_.rarray,
                             f.field_.rtype),
                 token(PUSH_ID, std::make_pair(sName2, i++)));
      lRetVal.push_back(intf);
    }
    return lRetVal;
  } else if (cmd == STREAM_SUBTRACT)
    lRetVal = coreInstance.getQuery(sName1).lSchema;
  else if (cmd == STREAM_TIMEMOVE)
    lRetVal = coreInstance.getQuery(sName1).lSchema;
  else if (cmd == STREAM_AVG) {
    field intf(rdb::rField("avg", sizeof(boost::rational<int>), 1, rdb::RATIONAL), token(PUSH_ID, std::make_pair(sName1, 0)));
    lRetVal.push_back(intf);
    return lRetVal;
  } else if (cmd == STREAM_MIN) {
    field intf(rdb::rField("min", sizeof(boost::rational<int>), 1, rdb::RATIONAL), token(PUSH_ID, std::make_pair(sName1, 0)));
    lRetVal.push_back(intf);
    return lRetVal;
  } else if (cmd == STREAM_MAX) {
    field intf(rdb::rField("max", sizeof(boost::rational<int>), 1, rdb::RATIONAL), token(PUSH_ID, std::make_pair(sName1, 0)));
    lRetVal.push_back(intf);
    return lRetVal;
  } else if (cmd == STREAM_SUM) {
    field intf(rdb::rField("sum", sizeof(boost::rational<int>), 1, rdb::RATIONAL), token(PUSH_ID, std::make_pair(sName1, 0)));
    lRetVal.push_back(intf);
    return lRetVal;
  } else if (cmd == STREAM_AGSE) {
    // Unrolling schema for agse - discussion needed if we need do that this way
    auto [step, windowSize] = std::get<std::pair<int, int>>(cmd_token.getVT());
    auto [maxType, maxLen]  = coreInstance[sName1].descriptorStorage().widestFieldType();
    std::list<field> schema;
    for (int i = 0; i < abs(windowSize); i++) {
      field intf(rdb::rField(sName1 + "_" + lexical_cast<std::string>(i), maxLen, 1, maxType),
                 token(PUSH_ID, std::make_pair(sName1, 0)));
      schema.push_back(intf);
    }

    lRetVal = schema;
  } else {
    FatalError("compiler: undefined stream token command in combine function: str={} cmd={}", cmd_token.getStr_(),
               cmd_token.getStrCommandID());
  }
  // Here are added to fields execution methods
  // by reference to schema position
  int offset(0);
  for (auto &f : lRetVal) {
    std::stringstream s;
    s << sName1;  // generateStreamName( sName2, sName1, cmd )
    s << "[";
    s << offset++;
    s << "]";
    if (!f.lProgram.empty()) f.lProgram.pop_front();
    f.lProgram.emplace_front(PUSH_ID2, std::make_pair(s.str(), 0));
  }
  return lRetVal;
}

// goal of this procedure is setup of all possible fields name and unroll *
// unfortunately algorithm if broken - because does not search backward but next
// by next and some * can be process which have arguments appear as two asterisk
// In such case unroll does not appear and algorithm gets shitin-shitout
std::string compiler::expandSchemaWildcards() {
  int fieldCountSh = 0;
  coreInstance.topologicalSort();
  for (auto &q : coreInstance) {
    for (auto &t : q.lProgram) {
      if (q.lProgram.size() >= 4) {
        FatalError("compiler::expandSchemaWildcards: program not optimized — {} tokens for query '{}', expected < 4",
                   q.lProgram.size(), q.id);
      }
      // fail of above check means that all streams are
      // after optimization already
      std::vector<std::list<field>::iterator> eraseList;
      auto it = q.lSchema.begin();
      for (auto &f : q.lSchema) {
        if (f.getFirstFieldToken().getCommandID() == PUSH_TSCAN) {
          // found! - and now unroll
          if (q.lProgram.size() == 1) {
            // we assure that on and only token is push_stream
            if ((*q.lProgram.begin()).getCommandID() != PUSH_STREAM) {
              FatalError(
                  "compiler::expandSchemaWildcards: first token must be PUSH_STREAM for single-token program, got cmd={} for "
                  "query '{}'",
                  (*q.lProgram.begin()).getStrCommandID(), q.id);
            }
            auto nameOfscanningTable = (*q.lProgram.begin()).getStr_();
            // Remove of TSCAN
            eraseList.push_back(it);
            // q.lSchema =  getQuery(t.getStr()).lSchema;
            // copy list of fields from one to another
            int filedPosition = 0;
            for (auto s : coreInstance.getQuery(t.getStr_()).lSchema) {
              std::list<token> lTempProgram;
              lTempProgram.emplace_back(PUSH_ID, std::make_pair(nameOfscanningTable, filedPosition++));
              std::string name = /*"Field_"*/ t.getStr_() + "_" + boost::lexical_cast<std::string>(fieldCountSh++);
              q.lSchema.emplace_back(rdb::rField(name, 4, 1, rdb::INTEGER), lTempProgram);
            }
            break;
          }
          if (q.lProgram.size() == 3 || q.lProgram.size() == 2) {
            auto [sName1, sName2, cmd]{GetArgs(q.lProgram)};
            q.lSchema = buildOutputSchema(sName1, sName2, cmd);
            break;
          }
        }
        ++it;
      }
      for (auto eraseIt : eraseList)
        q.lSchema.erase(eraseIt);
    }
  }
  coreInstance.sort();
  return {"OK"};
}

/* If in query plan is PUSH_IDX it means that we need to duplicate [_] */
std::string compiler::expandIndexWildcards() {
  for (auto &q : coreInstance) {             // for each query
    for (auto &f : q.lSchema) {              // for each field in query
      std::vector<std::string> usedSchemaX;  //
      for (auto &t : f.lProgram)             // for each token in query field
        if (t.getCommandID() == PUSH_IDX)
          usedSchemaX.push_back(get<std::pair<std::string, int>>(t.getVT()).first);  // .second arg is always 0
      if (!usedSchemaX.empty()) {
        int minSizeFlat{std::numeric_limits<int>::max()};
        for (const auto &schema : usedSchemaX) {
          auto size   = coreInstance.getQuery(schema).descriptorStorage().flatElementCount();
          minSizeFlat = std::min(minSizeFlat, size);
        }

        if (minSizeFlat == std::numeric_limits<int>::max()) {
          FatalError("compiler::expandIndexWildcards: flat size not resolved for query '{}'", q.id);
        }
        if (minSizeFlat <= 0) {
          FatalError("compiler::expandIndexWildcards: flat size must be positive, got {} for query '{}'", minSizeFlat, q.id);
        }
        if (q.lSchema.size() != 1) {
          FatalError(
              "compiler::expandIndexWildcards: PUSH_IDX expansion requires exactly one schema field, got {} for query '{}'",
              q.lSchema.size(), q.id);
        }

        field oldField = *q.lSchema.begin();
        q.lSchema.clear();
        for (int i = 0; i < minSizeFlat; i++) {
          std::list<token> lTempProgram;
          for (auto &t : oldField.lProgram) {
            if (t.getCommandID() == PUSH_IDX)
              lTempProgram.emplace_back(PUSH_ID, std::make_pair(t.getStr_(), i));
            else
              lTempProgram.emplace_back(t.getCommandID(), t.getVT());
          }
          field newField(rdb::rField(q.id + "_" + lexical_cast<std::string>(i),  //
                                     oldField.field_.rlen,                       //
                                     1,                                          // (expanded)
                                     oldField.field_.rtype),
                         lTempProgram);
          q.lSchema.push_back(newField);
        }
        break;
      }
    }
  }
  return {"OK"};
}

void compiler::resolveTokenReferences(std::list<token> &lProgram, query &q) {
  for (auto &t : lProgram) {  // for each token in query field
    const command_id cmd(t.getCommandID());
    const std::string text(t.getStr_());
    boost::cmatch what;
    switch (cmd) {
      case PUSH_ID1:
        if (regex_search(text.c_str(), what, xprFieldId1)) {
          if (what.size() != 3) FatalError("compiler: PUSH_ID1 regex match has unexpected capture count");
          const std::string schema(what[1]);
          const std::string field(what[2]);
          // aim of this procedure is found schema, next field in schema
          // and then insert
          for (auto &q1 : coreInstance) {
            if (q1.id == schema) {
              int offset1(0);
              for (auto &f1 : q1.lSchema) {
                if (f1.field_.rname == field) {
                  t = token(PUSH_ID, std::make_pair(schema, offset1));
                  break;
                }
                ++offset1;
              }
              if (offset1 == q1.lSchema.size())
                throw std::out_of_range(
                    "Failure during reference conversation - schema exist, "
                    "no "
                    "fields");
              break;
            }
          }
        } else
          throw std::out_of_range("No mach on type conversion ID1");
        break;
      case PUSH_IDX:
        if (regex_search(text.c_str(), what, xprFieldIdX)) {
          if (what.size() != 2) FatalError("compiler: PUSH_IDX regex match has unexpected capture count");
          const std::string schema(what[1]);
          t = token(PUSH_IDX, std::make_pair(schema, 0));
        } else
          throw std::out_of_range("No mach on type conversion IDX");
        break;
      case PUSH_ID2:
        if (regex_search(text.c_str(), what, xprFieldId2)) {
          if (what.size() != 3) FatalError("compiler: PUSH_ID2 regex match has unexpected capture count");
          const std::string schema(what[1]);
          const std::string sOffset1(what[2]);
          const int offset1(atoi(sOffset1.c_str()));
          t = token(PUSH_ID, std::make_pair(schema, offset1));
        } else {
          SPDLOG_ERROR("No mach on type conversion ID2 text:{}", text.c_str());
          throw std::out_of_range("No mach on type conversion");
        }
        break;
      case PUSH_ID3:
        if (regex_search(text.c_str(), what, xprFieldId3)) {
          if (what.size() != 2) FatalError("compiler: PUSH_ID3 regex match has unexpected capture count");
          const std::string field(what[1]);
          query *pQ1(nullptr);
          query *pQ2(nullptr);
          auto [schema1, schema2, cmd]{GetArgs(q.lProgram)};
          pQ1 = &coreInstance.getQuery(schema1);
          if (q.lProgram.size() == 3) pQ2 = &coreInstance.getQuery(schema2);
          bool bFieldFound(false);
          int offset1(0);
          if (pQ1 != nullptr) {
            offset1 = 0;
            for (auto &f1 : (*pQ1).lSchema) {
              if ((f1.field_).rname == field) {
                t = token(PUSH_ID, std::make_pair(schema1, offset1));
                // Goła nazwa pola też wskazuje składową — `v-w` nad `A#B` znosi tożsamość
                // dokładnie tak jak `A[0]-B[0]`. PUSH_ID3 wystawia wyłącznie parser, więc
                // zapis tutaj nie łapie tokenów kompilatora; migawka wejściowa nie da rady,
                // bo nazwa strumienia powstaje dopiero z tego wyszukiwania.
                namedSourceRefs_[q.id].insert(schema1);
                bFieldFound = true;
              }
              ++offset1;
            }
          }
          if (pQ2 != nullptr && !bFieldFound) {
            offset1 = 0;
            for (auto &f2 : (*pQ2).lSchema) {
              if (f2.field_.rname == field) {
                t = token(PUSH_ID, std::make_pair(schema2, offset1));
                namedSourceRefs_[q.id].insert(schema2);
                bFieldFound = true;
              }
              ++offset1;
            }
          }
          if (!bFieldFound) throw std::logic_error("No field of given name in stream schema ID3");
        } else
          throw std::out_of_range("No mach on type conversion ID3");
        break;
      case PUSH_ID4:
      case PUSH_ID5: {
        if (regex_search(text.c_str(), what, xprFieldId4) || regex_search(text.c_str(), what, xprFieldId5)) {
          if (what.size() != 4) FatalError("compiler: PUSH_ID4/5 regex match has unexpected capture count");
          const std::string schema(what[1]);
          const std::string sOffset1(what[2]);
          const std::string sOffset2(what[3]);
          const int offset1(atoi(sOffset1.c_str()));
          const int offset2(atoi(sOffset2.c_str()));

          namespace ranges = std::ranges;
          const bool foundSchema =
              ranges::find_if(coreInstance, [schema](const auto &qry) { return qry.id == schema; }) != coreInstance.end();

          if (!foundSchema) throw std::logic_error("Field calls non-exist schema - config.log (-g)");
          t = token(PUSH_ID, std::make_pair(schema, offset1 + (offset2 * static_cast<int>(q.lSchema.size()))));
        } else
          throw std::out_of_range("No mach on type conversion ID4");
        break;
      }
      default:
        break;
    }
  }
}
/* Purpose of this function is to translate all references to fields
to form schema_name[postion, time_offset]
Command method of presentation aims simple data processing
Aim of this procedure is change all of push_idXXX to push_id
note that push_id is closest to push_id4
push_idXXX is searched in all stream program after reduction */
std::string compiler::resolveFieldReferences() {
  for (auto &q : coreInstance) {  // for each query
    if (q.isReductionRequired()) {
      FatalError("compiler: query '{}' requires reduction at this stage — pipeline invariant violated", q.id);
    }
    for (auto &f : q.lSchema) {               // for each field in query
      resolveTokenReferences(f.lProgram, q);  // for each token in query field
    }  // end for each field in query
    for (auto &r : q.lRules) {                 // for each rule in query
      resolveTokenReferences(r.condition, q);  // for each token in rule
    }  // end for each rule in query
  }
  return {"OK"};
}

/// Migawka odwołań, które NAPISAŁ użytkownik — `A[0]`, `A.pole`, `A[_]` i `A.*`.
///
/// Zdejmowana z planu prosto po parsowaniu, przed jakimkolwiek przebiegiem, bo później takiej
/// informacji już nie ma: buildOutputSchema() sam syntetyzuje PUSH_ID2 o tekście `lewy[offset]`
/// (rozwinięcie `SELECT *` i schematy substratów), więc typ tokenu przestaje odróżniać
/// użytkownika od kompilatora. Nazwy w migawce są stabilne: przemianowania planu
/// (retargetSchemaReferences) dotykają wyłącznie nazw substratów, nigdy strumieni z DECLARE.
void compiler::snapshotNamedSourceRefs() {
  namedSourceRefs_.clear();
  auto record = [this](const std::list<token> &program, const std::string &id) {
    for (const auto &t : program) {
      const std::string text(t.getStr_());
      boost::cmatch what;
      const boost::regex *pattern = nullptr;
      switch (t.getCommandID()) {
        case PUSH_ID1:
          pattern = &xprFieldId1;
          break;
        case PUSH_ID2:
          pattern = &xprFieldId2;
          break;
        case PUSH_IDX:
          pattern = &xprFieldIdX;
          break;
        case PUSH_TSCAN:
          if (text.size() > 2 && text.ends_with(".*")) namedSourceRefs_[id].insert(text.substr(0, text.size() - 2));
          continue;
        default:
          continue;
      }
      if (regex_search(text.c_str(), what, *pattern)) namedSourceRefs_[id].insert(std::string(what[1]));
    }
  };
  for (const auto &q : coreInstance) {
    for (const auto &f : q.lSchema)
      record(f.lProgram, q.id);
    for (const auto &r : q.lRules)
      record(r.condition, q.id);
  }
}

/* This function will convert fields list where stream a from b#c
clause from b[x1],c[x2] int a[y1],a[y2] according to offset of from operation */
void compiler::collectTransitiveOffsets(const std::string &srcId, int baseOffset, bool viaHash,
                                        std::map<std::string, int> &result, std::set<std::string> &viaInterleave) {
  auto &srcQuery = coreInstance.getQuery(srcId);
  if (!srcQuery.isSubstrat) return;
  bool isHash = std::ranges::any_of(srcQuery.lProgram, [](token &t) { return t.getCommandID() == STREAM_HASH; });
  int offset  = 0;
  for (auto &t : srcQuery.lProgram) {
    if (t.getCommandID() == PUSH_STREAM) {
      const std::string &sub = t.getStr_();
      const int globalOffset = isHash ? baseOffset : (baseOffset + offset);
      // Raz wejdziemy pod przeplot i tożsamość nie wraca: niżej wszystko dzieli tę samą pozycję.
      const bool subViaHash = viaHash || isHash;
      result[sub]           = globalOffset;
      if (subViaHash) viaInterleave.insert(sub);
      collectTransitiveOffsets(sub, globalOffset, subViaHash, result, viaInterleave);
      if (!isHash) offset += coreInstance[sub].descriptorStorage().flatElementCount();
    }
  }
}

std::string compiler::localizeFieldOffsets() {
  std::map<std::string, std::map<std::string, int>> offsetMap;
  std::map<std::string, std::set<std::string>> interleavedSources;

  // This loop fill&create OffsetMap structure.
  for (auto &q : coreInstance) {  // for each query
    if (q.isReductionRequired()) {
      FatalError("compiler: query '{}' requires reduction at this stage — pipeline invariant violated", q.id);
    }  // that has at least two arguments
    auto offset{0};                         //
    std::map<std::string, int> offsetItem;  //
    std::set<std::string> viaInterleave;    // składowe, których tożsamość zniosło `#`
    for (auto &f : q.lProgram) {            // for each token in stream program
      if (f.getCommandID() == PUSH_STREAM) {
        offsetItem[f.getStr_()] = offset;
        offset += coreInstance[f.getStr_()].descriptorStorage().flatElementCount();
      }
      if (f.getCommandID() == STREAM_HASH) {
        for (auto &i : offsetItem) {
          i.second = 0;
          viaInterleave.insert(i.first);
        }
      }
    }
    // Extend with transitive sources from system-generated substrats.
    std::vector<std::pair<std::string, int>> directSources(offsetItem.begin(), offsetItem.end());
    for (const auto &[srcName, srcBase] : directSources)
      collectTransitiveOffsets(srcName, srcBase, viaInterleave.contains(srcName), offsetItem, viaInterleave);
    offsetMap[q.id]          = offsetItem;
    interleavedSources[q.id] = viaInterleave;
  }

  // Bramka F9 (D-F1 = S3, 2026-08-09). `A[0]` na liście pól nie znaczy „bieżąca wartość
  // strumienia A" — znaczy pozycję w schemacie strumienia z FROM, liczoną od miejsca wejścia
  // A do złączenia. Przeplot wymaga IDENTYCZNYCH schematów obu argumentów i wydaje jeden
  // strumień o tym samym schemacie, więc pozycja k lewej składowej i pozycja k prawej to TA
  // SAMA pozycja — pętla wyżej zeruje offsety obu. Skutkiem było, że `A[0]-B[0]` nad `A#B`
  // kompilowało się po cichu do `roznica[0]-roznica[0]`, czyli tożsamościowego zera: dwa
  // syntaktycznie różne odwołania dawały tożsamy wynik, a kompilator o tym nie mówił.
  //
  // Odzyskanie składowej ma w algebrze własny operator — rozplot Theta / ~Theta (`&`, `%`).
  // Sięgania po składową nazwą PRZEZ węzeł `#` algebra nie przewiduje wcale, więc nie ma tu
  // poprawnej wartości do wyliczenia i jedynym uczciwym wyjściem jest odmowa planu.
  //
  // Sprawdzamy WYŁĄCZNIE odwołania napisane przez użytkownika (namedSourceRefs_). PUSH_ID
  // wygenerowane przez buildOutputSchema() dla `SELECT *` i dla substratów wskazują ten jedyny
  // schemat, jaki po `#` istnieje, i dwuznaczne nie są.
  for (const auto &q : coreInstance) {
    const auto refs = namedSourceRefs_.find(q.id);
    if (refs == namedSourceRefs_.end()) continue;
    for (const auto &name : refs->second) {
      if (name == q.id) continue;
      if (!interleavedSources[q.id].contains(name)) continue;
      SPDLOG_ERROR("Reference to '{}' in stream '{}' reaches a constituent of an interleave (#). q.id={}", name, q.id, q.id);
      return std::string("Stream '" + q.id + "' refers to '" + name +
                         "', which is a constituent of an interleave (#). After # the constituents share one schema, so "
                         "this reference cannot be told apart from the other constituent's. Refer to '" +
                         q.id + "' by its own name, or recover the constituent with de-interleave (& / %).");
    }
  }

  // This loop converts with help of offsetMap
  for (auto &q : coreInstance) {  // for each query
    if (q.isReductionRequired()) {
      FatalError("compiler: query '{}' requires reduction at this stage — pipeline invariant violated", q.id);
    }  // that has at least two arguments and
    for (auto &f : q.lSchema) {             // for each field in query and
      for (auto &t : f.lProgram) {          // for each token in query field - do:
        if (t.getCommandID() == PUSH_ID) {  // fix only PUSH_ID tokens
          auto [schema, offset] = std::get<std::pair<std::string, int>>(t.getVT());
          if (schema != q.id) t = token(PUSH_ID, std::make_pair(q.id, offsetMap[q.id][schema] + offset));
        }
      }
    }
  }
  return {"OK"};
}

std::string compiler::validateConstraints() {
  for (auto &q : coreInstance) {      // for each query
    if (q.isDeclaration()) continue;  // do not check declaration in constraints.
    if (q.isReductionRequired()) {
      FatalError("compiler: query '{}' requires reduction at this stage — pipeline invariant violated", q.id);
    }  // process data only with two or less arguments
    auto [arg1, arg2, cmd]{GetArgs(q.lProgram)};
    switch (cmd.getCommandID()) {
      case STREAM_HASH: {
        if (coreInstance.getQuery(arg1).descriptorStorage().flatElementCount() !=
            coreInstance.getQuery(arg2).descriptorStorage().flatElementCount()) {
          SPDLOG_ERROR("Hash operations need to work on two schemas with the same size. q.id={}", q.id);
          return std::string("HASH operation constraint failed on " + q.id);
        }
      } break;
      case STREAM_SUBTRACT: {
        const auto deltaSource = coreInstance.getQuery(arg1).rInterval;
        const auto deltaTarget = cmd.getRI();
        if (deltaTarget < deltaSource) {
          SPDLOG_ERROR("SUBTRACT target interval must not be faster than its source. q.id={} source={} target={}", q.id,
                       boost::lexical_cast<std::string>(deltaSource), boost::lexical_cast<std::string>(deltaTarget));
          return std::string("SUBTRACT interval constraint failed on " + q.id);
        }
      } break;
      case PUSH_STREAM:
      case STREAM_DEHASH_DIV:
      case STREAM_DEHASH_MOD:
      case STREAM_ADD:
      case STREAM_TIMEMOVE:
      case STREAM_AGSE:
      case STREAM_AVG:
      case STREAM_MIN:
      case STREAM_MAX:
      case STREAM_SUM:
        // No additional constraints for these commands in this phase.
        break;
      default:
        FatalError("compiler::validateConstraints: unsupported command '{}' for query '{}'",
                   GetStringcommand_id(cmd.getCommandID()), q.id);
    }
  }
  return {"OK"};
}

std::string compiler::applyCapacitiesToStreams(const std::map<std::string, int> &capMap) {
  for (const auto &q : capMap) {                             // for each query
    if (coreInstance[q.first].policy.second == 0) continue;  // do not check declaration in constraints.
    if (coreInstance[q.first].isReductionRequired()) {
      FatalError("compiler: query '{}' requires reduction at applyCapacities stage", q.first);
    }
    coreInstance[q.first].policy.second = q.second;  // set memory size
  }
  return {"OK"};
}

std::map<std::string, int> compiler::computeRequiredCapacities() {
  // Głębokość historii dla źródeł przeplotu (#) i rozplotu (&, %) — stała
  // w jednostkach rekordów, patrz komentarz przy STREAM_HASH poniżej.
  constexpr int kJunctionHistory = 4;
  // Deklaracja wyprzedza konsumenta o rekord uzbrojony przy otwarciu storage
  // i o zerowy prefetch, wiec jej czolo jest dalej, niz wynika z czasu.
  constexpr int kDeclarationPrefetch = 2;

  std::map<std::string, int> capMap;  // <- This var goes to qTree class instance

  for (auto &q : coreInstance) {       // for each declaration
    if (!q.isDeclaration()) continue;  // that is declaration
    capMap[q.id] = 1;
  }

  for (auto &q : coreInstance) {      // for each query
    if (q.isDeclaration()) continue;  // that is not declaration
    if (q.isReductionRequired()) {
      FatalError("compiler: query '{}' requires reduction at this stage — pipeline invariant violated", q.id);
    }  // process data only with two or less arguments
    auto [arg1, arg2, cmd]{GetArgs(q.lProgram)};
    switch (cmd.getCommandID()) {
      case PUSH_STREAM:
      case STREAM_TIMEMOVE: {
        // 	:- PUSH_STREAM(core0)
        //  :- STREAM_TIMEMOVE(1)
        //
        if (cmd.getCommandID() == STREAM_TIMEMOVE && q.lProgram.size() != 2) {
          FatalError("compiler: unexpected program size in computeRequiredCapacities: {} tokens for query '{}', expected 2",
                     q.lProgram.size(), q.id);
        }

        if (cmd.getCommandID() == PUSH_STREAM) {
          // Pass-through stream does not increase source history requirement.
          break;
        }

        const auto nameSrc    = arg1;
        const auto timeOffset = std::get<int>(cmd.getVT());
        // Rekord n czyta INDEKS LOGICZNY n-N. W chwili odczytu źródło ma wyemitowanych
        //   count = (n + W_out - W_src) - O_src + 1   rekordów fizycznych,
        // a żądany rekord leży na pozycji (n - N) - O_src, więc
        //   rev = count - 1 - pozycja = W_out - W_src + N = N - min(N, W_src)  <=  N.
        // Origin skraca się: przesuwa i czoło, i żądaną pozycję o tyle samo. Pojemność musi
        // pomieścić oba końce zakresu, stąd +1. Ogon producenta ODEJMUJE się od żądania:
        // im dłużej producent każe czekać, tym bliżej czoła leży rekord sprzed N slotów.
        //
        // Człon prefetchu jest tu NOWY i konieczny dla deklaracji. Dopóki przesunięcie szło
        // przez fetchBack z offsetem WZGLĘDNYM, wyprzedzenie czoła deklaracji skracało się samo
        // i N+1 wystarczało. Adresowanie bezwzględne tej własności nie ma: czoło deklaracji
        // biegnie o dwa rekordy przed rachunkiem czasowym (rekord uzbrojony przy otwarciu
        // storage oraz zerowy prefetch), więc bez tego członu odczyt wypadałby poza historię.
        const auto &source = coreInstance[nameSrc];
        const int rev      = q.startupLatency - source.startupLatency + timeOffset;
        const int required = rev + 1 + (source.isDeclaration() ? kDeclarationPrefetch : 0);
        capMap[nameSrc]    = std::max(capMap[nameSrc], std::max(required, 1));
      } break;
      case STREAM_AGSE: {
        // 	:- PUSH_STREAM core -> delta_source (arg[0]) - operation
        //  :- STREAM_AGSE 2,3 -> window_length, window_step (arg[1])
        //
        if (q.lProgram.size() != 2) {
          FatalError("compiler: unexpected program size in computeRequiredCapacities: {} tokens for query '{}', expected 2",
                     q.lProgram.size(), q.id);
        }

        const auto nameSrc = arg1;
        const auto step    = get<std::pair<int, int>>(cmd.getVT()).first;
        if (step <= 0) {
          FatalError("compiler: AGSE step must be > 0, got {} for query '{}' in computeRequiredCapacities", step, q.id);
        }
        auto &source          = coreInstance[nameSrc];
        const int sourceWidth = source.descriptorStorage().flatElementCount();
        const int length      = get<std::pair<int, int>>(cmd.getVT()).second;
        // Okno stemplowane końcem przedziału (rekord n obejmuje pozycje n*step-(|L|-1) ... n*step)
        // czyta wstecz o całą rozpiętość okna, więc pojemność źródła musi ją pomieścić.
        //
        // Odległość wsteczna w chwili emisji rekordu n:
        //   j_max(n) = floor((n+1+Wout)*step/F) - Wsrc - 1   — najnowszy rekord źródła,
        //   r_old(n) = floor((n*step-|L|+1)/F)               — najstarsze pole okna,
        //   dystans  = j_max(n) - r_old(n).
        // Obie części zmieniają się o step/gcd(step,F) przy wzroście n o F/gcd(step,F), więc
        // dystans jest okresowy i maksimum liczymy DOKŁADNIE, przeglądając jeden pełny okres
        // od origin. Postać zamknięta byłaby tu domysłem — a to jest wzór, którego zaniżenie
        // oznacza odczyt poza historią (defekt D1 z K24), nie tylko slot opóźnienia.
        const int period     = sourceWidth / std::gcd(sourceWidth, step);
        int maxDistance      = 0;
        const int firstIndex = q.logicalOrigin;
        for (int n = firstIndex; n < firstIndex + period; ++n) {
          const int newest = floorDiv((n + 1 + q.startupLatency) * step, sourceWidth) - source.startupLatency - 1;
          const int oldest = floorDiv(n * step - std::abs(length) + 1, sourceWidth);
          maxDistance      = std::max(maxDistance, newest - oldest);
        }
        // Bufor musi pomieścić oba końce zakresu, więc pojemność to odległość + 1.
        // Deklaracja ma dodatkowo dwa rekordy przed pierwszym wykonaniem konsumenta:
        // rekord uzbrojony przy otwarciu storage oraz zerowy prefetch.
        const int required = maxDistance + (source.isDeclaration() ? kDeclarationPrefetch : 1);
        capMap[nameSrc]    = std::max(capMap[nameSrc], std::max(required, 1));
      } break;
      case STREAM_HASH:
        // Przeplot/rozplot czytają elementy składowych po indeksie
        // postępującym (fetchForward), konsumując je w tempie produkcji
        // źródła — offset wsteczny nie zależy od proporcji delt (inaczej niż
        // w AGSE, gdzie lookback rośnie z długością okna): najstarszy
        // potrzebny rekord to bieżący element składowej, cofnięty najwyżej
        // o jeden okres źródła (<=1) + prefetch źródła deklarowanego (+1);
        // kJunctionHistory = bound 2 + margines 2.
        for (const auto &nameSrc : {arg1, arg2}) {
          const auto &source = coreInstance[nameSrc];
          const int delayed =
              ceilR(boost::rational<int>(q.startupLatency) * q.rInterval / source.rInterval) - source.startupLatency + 2;
          capMap[nameSrc] = std::max(capMap[nameSrc], std::max(kJunctionHistory, delayed));
        }
        break;
      case STREAM_DEHASH_DIV:
      case STREAM_DEHASH_MOD:
        // Rozplot: jak wyżej, historia tylko dla strumienia rozplatanego
        // (arg2 to argument wymierny, nie strumień).
        {
          const auto &source = coreInstance[arg1];
          const int delayed =
              ceilR(boost::rational<int>(q.startupLatency) * q.rInterval / source.rInterval) - source.startupLatency + 2;
          capMap[arg1] = std::max(capMap[arg1], std::max(kJunctionHistory, delayed));
        }
        break;
      case STREAM_ADD: {
        // K24/P2 wariant A: suma strumieni czyta składową po indeksie postępującym
        // ⌊n·Δout/Δsrc⌋ (Definicja sumy strumieni), więc — inaczej niż przed poprawką,
        // gdy brała bieżący payload — wchodzi do modelu pojemności.
        // Odległość wsteczna w chwili slotu n wynosi
        //   count_src(t_n) - 1 - ⌊n·ratio⌋,  ratio = Δout/Δsrc <= 1,
        // a potrzebna pojemność to maksimum po n z count_src(t_n) - ⌊n·ratio⌋, czyli
        //   max_n [ ⌊(n+1+Wout)·ratio⌋ - ⌊n·ratio⌋ ] - Wsrc = ⌈(1+Wout)·ratio⌉ - Wsrc.
        // Dla deklaracji dochodzi wyprzedzenie czoła (uzbrojenie storage i zerowy
        // prefetch) — ten sam człon co w STREAM_SUBTRACT i STREAM_AGSE.
        for (const auto &nameSrc : {arg1, arg2}) {
          const auto &source = coreInstance[nameSrc];
          const auto ratio   = q.rInterval / source.rInterval;
          int required       = ceilR(boost::rational<int>(1 + q.startupLatency) * ratio) - source.startupLatency;
          if (source.isDeclaration()) required += kDeclarationPrefetch;
          capMap[nameSrc] = std::max(capMap[nameSrc], std::max(required, 1));
        }
      } break;
      case STREAM_AVG:
      case STREAM_MIN:
      case STREAM_MAX:
      case STREAM_SUM:
        // These commands do not increase source history requirement here.
        break;
      case STREAM_SUBTRACT: {
        const auto &source = coreInstance[arg1];
        const auto ratio   = q.rInterval / source.rInterval;
        // Dla deklaracji maksimum odległości od c_{ceil(n*ratio)}
        // występuje w fazie całkowitej. Dwa rekordy startowe mają tę samą
        // genezę co w AGSE (uzbrojenie storage i zerowy prefetch).
        int required = source.isDeclaration() ? floorR(boost::rational<int>(q.startupLatency) * ratio) + 2
                                              : ceilR(boost::rational<int>(q.startupLatency) * ratio - source.startupLatency);
        // K24/P1: formuła dla deklaracji zaniżała pojemność dla 39,1% par
        // (konsument, deklaracja) w korpusie 10 010 planów. Przy ilorazie
        // całkowitym >= 3 odczyt wypadał poza historią i dawał CICHY rekord
        // all-NULL — cały strumień wyjściowy był pusty przy poprawnej liczbie
        // rekordów. Odległość wsteczna z modelu zdarzeniowego wynosi dla
        // deklaracji (Wsrc=0)
        //   max_n [ floor((n+1+Wout)*ratio) - ceil(n*ratio) ] = floor((1+Wout)*ratio),
        // co potwierdzono wyczerpująco na 2070 parach (ratio, Wout).
        // Maksimum obu wartości — patrz komentarz przy STREAM_AGSE.
        if (source.isDeclaration()) {
          required = std::max(required, floorR(boost::rational<int>(1 + q.startupLatency) * ratio) + kDeclarationPrefetch);
        }
        capMap[arg1] = std::max(capMap[arg1], std::max(required, 1));
      } break;
      default:
        FatalError("compiler::computeRequiredCapacities: unsupported command '{}' for query '{}'",
                   GetStringcommand_id(cmd.getCommandID()), q.id);
    }

    // Bump capMap with dumpRange from rules (if they are negative and attached to query declaration)
    for (const auto &rule : q.lRules) {
      if (rule.action != rule::DUMP) continue;
      auto [l, r] = rule.dumpRange;
      if (l >= r) {
        FatalError("compiler: dump range invalid [{}..{}] for query '{}'", l, r, q.id);
      }
      if (l < 0) {
        auto [arg1, arg2, cmd]{GetArgs(q.lProgram)};
        const auto nameSrc = arg1;
        capMap[nameSrc]    = std::max(capMap[nameSrc], static_cast<int>(abs(l)));
      }
    }
  }
  return capMap;
}

void compiler::retargetSchemaReferences(query &q, const std::string &oldName, const std::string &newName) {
  for (auto &f : q.lSchema)
    for (auto &tok : f.lProgram) {
      if (tok.getCommandID() == PUSH_ID) {
        auto [schema, idx] = std::get<std::pair<std::string, int>>(tok.getVT());
        if (schema == oldName) tok = token(PUSH_ID, std::make_pair(newName, idx));
      }
      if (tok.getCommandID() == PUSH_ID2) {
        const std::string str = tok.getStr_();  // copy before tok may be replaced
        if (str.starts_with(oldName + "[")) {
          const std::string updated = newName + str.substr(oldName.size());
          if (std::holds_alternative<std::pair<std::string, int>>(tok.getVT()))
            tok = token(PUSH_ID2, std::make_pair(updated, 0));
          else
            tok = token(PUSH_ID2, updated);
        }
      }
    }
}

void compiler::replaceStreamReferences(const std::string &oldName, const std::string &newName) {
  for (auto &q : coreInstance)
    for (auto &tok : q.lProgram)
      if (tok.getCommandID() == PUSH_STREAM && tok.getStr_() == oldName) tok = token(PUSH_STREAM, newName);

  for (auto &q : coreInstance)
    retargetSchemaReferences(q, oldName, newName);
}

std::map<std::string, std::vector<std::string>> compiler::snapshotUserFieldNames() const {
  // Nazwy pól nazwanych strumieni użytkownika — to one trafiają do pliku .desc, więc są
  // obserwowalne. Substraty i deklaracje pomijamy: substrat nie ma odrębnej tożsamości
  // obserwowalnej (na tym opiera się deduplikacja), a deklaracja nie jest wynikiem planu.
  std::map<std::string, std::vector<std::string>> snapshot;
  for (const auto &q : coreInstance) {
    if (q.isSubstrat || q.isDeclaration() || q.isCompilerDirective()) continue;
    auto &names = snapshot[q.id];
    for (const auto &f : q.lSchema)
      names.push_back(f.field_.rname);
  }
  return snapshot;
}

std::string compiler::verifyUserFieldNamesPreserved(const std::map<std::string, std::vector<std::string>> &before) const {
  // Niezmiennik D3: przepisania planu (faktoryzacja, deduplikacja, współdzielenie SELECT) nie mogą
  // zmienić deskryptora żadnego nazwanego strumienia użytkownika.
  //
  // Predykaty scalania celowo porównują schematy BEZ nazw — scalają węzły wewnętrzne, więc mają do
  // tego prawo, a zawężenie ich o nazwy zmniejszyłoby liczbę scaleń, czyli sam mierzony wynik.
  // Nazwy są jednak obserwowalne (plik .desc), więc zamiast osłabiać scalanie, pilnujemy skutku:
  // to, co widzi użytkownik, ma być takie samo przed optymalizacją i po niej.
  const auto after = snapshotUserFieldNames();

  for (const auto &[id, names] : before) {
    const auto it = after.find(id);
    if (it == after.end()) {
      return "Optimization removed user-named stream '" + id + "'";
    }
    if (it->second != names) {
      const auto asText = [](const std::vector<std::string> &list) {
        std::string out;
        for (const auto &name : list)
          out += (out.empty() ? "" : ", ") + name;
        return out;
      };
      SPDLOG_ERROR("compiler: optimization changed observable field names of '{}': [{}] -> [{}]", id, asText(names),
                   asText(it->second));
      return "Optimization changed observable field names of stream '" + id + "'";
    }
  }
  return {"OK"};
}

namespace {

// Origin jest ograniczony rozpiętością okien w planie, więc realnie jest małą liczbą.
// Limit chroni wyłącznie przed odwzorowaniem, które wbrew założeniu nie rośnie —
// bez niego pętla podwajania byłaby nieskończona.
constexpr int kOriginSearchLimit = 1 << 24;

// Najmniejsze n >= 0, dla którego niemalejące odwzorowanie indeksu osiąga próg.
// Wszystkie odwzorowania rekord->rekord w SOperations.hpp są niemalejące, więc
// zbiór n spełniających warunek jest półprostą i wystarczy znaleźć jej początek.
template <typename Mapping>
int firstIndexReaching(const Mapping &mapping, const int threshold, const std::string &nodeId) {
  if (threshold <= 0) return 0;
  // Podwajanie w poszukiwaniu górnego ograniczenia, potem połowienie. Odwzorowania
  // rozplotu rosną szybciej niż liniowo, więc podwajanie kończy się po kilku krokach.
  int hi = 1;
  while (mapping(hi) < threshold) {
    if (hi > kOriginSearchLimit) {
      FatalError("compiler::computeLogicalOrigin: origin search diverged for '{}' (threshold {})", nodeId, threshold);
    }
    hi *= 2;
  }
  int lo = 0;
  while (lo < hi) {
    const int mid = lo + (hi - lo) / 2;
    if (mapping(mid) < threshold)
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo;
}

}  // namespace

std::string compiler::computeLogicalOrigin() {
  // Początek logiczny (query::logicalOrigin) — indeks pierwszego rekordu, który W OGÓLE istnieje.
  // Różnica wobec ogona: ogon mówi „jeszcze nie teraz", origin mówi „ten rekord nie ma definicji".
  // Źródłem origin jest wyłącznie okno `@` stemplowane końcem przedziału: jego wczesne rekordy
  // sięgałyby przed początek strumienia źródłowego. Reszta planu origin tylko przenosi, przez
  // to samo odwzorowanie indeksu, którym czyta dane w dataModel::constructInputPayload().
  //
  // Wszystkie te odwzorowania są niemalejące, więc „brakujące" rekordy tworzą prefiks, a nie dziury.

  std::map<std::string, int> origin;
  for (const auto &q : coreInstance)
    if (q.isDeclaration() || q.isCompilerDirective()) origin[q.id] = 0;  // źródło istnieje od rekordu 0

  auto deltaOf  = [this](const std::string &id) { return coreInstance.getQuery(id).rInterval; };
  auto originOf = [&origin](const std::string &id, int &out) {
    auto it = origin.find(id);
    if (it == origin.end()) return false;
    out = it->second;
    return true;
  };

  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto &q : coreInstance) {
      if (origin.contains(q.id)) continue;
      if (q.lProgram.empty()) continue;

      auto it = q.lProgram.begin();
      if (it->getCommandID() != PUSH_STREAM) continue;
      const std::string src1 = it->getStr_();
      int o1                 = 0;
      if (!originOf(src1, o1)) continue;  // producent jeszcze nierozwiązany

      const auto op     = q.lProgram.back().getCommandID();
      const auto delta1 = deltaOf(src1);
      int result        = o1;

      if (q.lProgram.size() == 1) {
        // Czysty PUSH_STREAM czyta bieżący payload producenta — ten sam rekord, ten sam origin.
      } else if (op == STREAM_TIMEMOVE) {
        // tau_N jest OPÓŹNIENIEM: rekord n ma treść rekordu n-N producenta. Rekordy o indeksie
        // mniejszym od N nie mają definicji — sięgałyby przed początek producenta — więc
        // niedefiniowalność jest tu origin, a nie ogon.
        //
        // Dotąd `N` siedziało w ogonie, przez co przesunięcie było w złączeniach NIEWIDOCZNE:
        // rekord n brał rekord n i operator zmieniał tylko liczbę rekordów na starcie. To ta sama
        // precesja co w oknie stemplowanym początkiem przedziału.
        //
        // Suma slotów milczenia (origin + ogon) zostaje bez zmian, więc ciąg wydanych rekordów
        // jest identyczny — przesuwa się wyłącznie ich adres w czasie.
        result = o1 + std::get<int>(q.lProgram.back().getVT());
      } else if (op == STREAM_AVG || op == STREAM_MIN || op == STREAM_MAX || op == STREAM_SUM) {
        // Redukcje działają na bieżącej krotce producenta.
      } else if (op == STREAM_AGSE) {
        const auto [step, length] = std::get<std::pair<int, int>>(q.lProgram.back().getVT());
        const int sourceWidth     = coreInstance[src1].descriptorStorage().flatElementCount();
        result                    = AgseLogicalOrigin(sourceWidth, step, length, o1);
      } else if (op == STREAM_SUBTRACT) {
        const auto delta = q.rInterval;
        result           = firstIndexReaching([&](int n) { return Subtract(delta1, delta, n); }, o1, q.id);
      } else if (op == STREAM_DEHASH_DIV) {
        const auto delta = q.rInterval;
        const auto param = std::next(q.lProgram.begin())->getRI();
        result           = firstIndexReaching([&](int n) { return Div(delta, param, n); }, o1, q.id);
      } else if (op == STREAM_DEHASH_MOD) {
        const auto delta = q.rInterval;
        const auto param = std::next(q.lProgram.begin())->getRI();
        result           = firstIndexReaching([&](int n) { return Mod(param, delta, n); }, o1, q.id);
      } else if (op == STREAM_ADD || op == STREAM_HASH) {
        auto second = std::next(q.lProgram.begin());
        int o2      = 0;
        if (second->getCommandID() != PUSH_STREAM || !originOf(second->getStr_(), o2)) continue;
        const auto delta2 = deltaOf(second->getStr_());

        if (op == STREAM_ADD) {
          const auto delta = q.rInterval;
          result           = std::max(firstIndexReaching([&](int n) { return Add(delta, delta1, n); }, o1, q.id),
                                      firstIndexReaching([&](int n) { return Add(delta, delta2, n); }, o2, q.id));
        } else {
          // Przeplot czyta w slocie n tylko JEDNĄ składową, ale obie pozycje — floor(z*n) dla
          // pierwszej i n-floor(z*n) dla drugiej — są niemalejące. Najmniejsze n, od którego
          // KAŻDY dalszy slot trafia w istniejący rekord swojej składowej, to maksimum progów.
          const auto zet = delta2 / (delta1 + delta2);
          result         = std::max(firstIndexReaching([&](int n) { return floorR(zet * n); }, o1, q.id),
                                    firstIndexReaching([&](int n) { return n - floorR(zet * n); }, o2, q.id));
        }
      }

      origin[q.id] = result;
      changed      = true;
    }
  }

  requireResolvedForEveryNode(coreInstance, origin, "compiler::computeLogicalOrigin", "logical origin");
  for (auto &q : coreInstance)
    q.logicalOrigin = origin.at(q.id);
  return {"OK"};
}

std::string compiler::computeStartupLatency() {
  // Ogon strumienia (query::startupLatency) — liczba początkowych slotów własnego interwału, w których
  // wynik nie jest jeszcze zdefiniowany. Zasada brzegu: te sloty nie są rekordami. NULL zostaje wyłącznie
  // wartością pochłaniającą (dane oczekiwane a nieobecne, wynik nieistniejący w zbiorze wartości), nigdy
  // rezerwacją miejsca na dane.
  //
  // Ten przebieg WYLICZA ogon i zapisuje go w query::startupLatency. Emisję doprowadza do zgodności
  // z nim dataModel::processRows() — przez pierwsze startupLatency slotów strumień nie emituje niczego
  // (porównanie z streamInstance::elapsedSlots). Wartość jest raportowana jako 'tail' przez presenter.

  // Ogon źródła przeliczony na sloty konsumenta: w slotów źródła to w*dSrc sekund, czyli ceil(w*dSrc/dDst)
  // slotów konsumenta. Zaokrąglamy w górę — pół slotu opóźnienia to wciąż slot, w którym nie ma czego wydać.
  auto toSlots = [](int w, const boost::rational<int> &dSrc, const boost::rational<int> &dDst) -> int {
    if (w <= 0) return 0;
    return ceilR(boost::rational<int>(w) * dSrc / dDst);
  };

  std::map<std::string, int> latency;
  for (const auto &q : coreInstance)
    if (q.isDeclaration() || q.isCompilerDirective()) latency[q.id] = 0;  // źródło emituje od pierwszego slotu

  auto deltaOf   = [this](const std::string &id) { return coreInstance.getQuery(id).rInterval; };
  auto latencyOf = [&latency](const std::string &id, int &out) {
    auto it = latency.find(id);
    if (it == latency.end()) return false;
    out = it->second;
    return true;
  };

  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto &q : coreInstance) {
      if (latency.contains(q.id)) continue;
      if (q.lProgram.empty()) continue;

      auto it = q.lProgram.begin();
      if (it->getCommandID() != PUSH_STREAM) continue;
      const std::string src1 = it->getStr_();
      int w1                 = 0;
      if (!latencyOf(src1, w1)) continue;  // producent jeszcze nierozwiązany

      const auto op     = q.lProgram.back().getCommandID();
      const auto delta1 = deltaOf(src1);
      int result        = toSlots(w1, delta1, q.rInterval);

      if (q.lProgram.size() == 1) {
        result = w1;  // czysty PUSH_STREAM — ten sam interwał, ten sam ogon
      } else if (op == STREAM_TIMEMOVE) {
        // Rekord n czyta rekord n-N producenta, czyli STARSZY od bieżącego. Przesunięcie o N
        // slotów siedzi w origin (patrz computeLogicalOrigin), bo to niedefiniowalność, nie
        // oczekiwanie. Sam ogon wynika z warunku dostępności: rekord n-N jest określony
        // w chwili (n-N+1+W_src)*Delta, a slot n kończy się w (n+1+W)*Delta, więc
        //   W >= W_src - N,  deficyt jest STAŁY,  stąd  W = max(0, W_src - N).
        //
        // Do 2026-08-07 stało tu W = W_src, wymuszone adresowaniem WZGLĘDNYM w fetchBack:
        // w slocie n+W dostawał rekord (n + W - W_src) - N i tylko przy W = W_src trafiał
        // w żądany. Kampania K24p zmierzyła tamto zawyżenie na 6,6% węzłów `>N` o dokładnie
        // min(W_src, N) slotów; dataModel adresuje teraz indeksem logicznym, więc związek zniknął.
        result = std::max(0, w1 - std::get<int>(q.lProgram.back().getVT()));
      } else if (op == STREAM_HASH) {
        auto second = std::next(q.lProgram.begin());
        int w2      = 0;
        if (second->getCommandID() != PUSH_STREAM || !latencyOf(second->getStr_(), w2)) continue;
        const auto delta2 = deltaOf(second->getStr_());
        // Ogon przeplotu liczy się DOKŁADNIE, przeglądem jednego okresu fazowego —
        // patrz HashStartupLatency() w SOperations.hpp. Do 2026-08-07 stała tu postać O(1)
        // max(conv(W_A), conv(W_B) + ceil((p+q-1)/p)); kampania K24 zmierzyła jej zgodność
        // z granicą zdarzeniową na 92,1% węzłów `#`, z zawyżeniem o slot w pozostałych.
        result = HashStartupLatency(delta1, delta2, q.rInterval, w1, w2);
      } else if (op == STREAM_ADD) {
        auto second = std::next(q.lProgram.begin());
        int w2      = 0;
        if (second->getCommandID() != PUSH_STREAM || !latencyOf(second->getStr_(), w2)) continue;
        // Ogon musi zabezpieczyć dostępność rekordu KAŻDEJ składowej pod indeksem
        // z Definicji sumy strumieni, a nie tylko przeliczyć ogon składowej przez takt —
        // patrz AddStartupLatency() w SOperations.hpp.
        result =
            std::max(AddStartupLatency(delta1, q.rInterval, w1), AddStartupLatency(deltaOf(second->getStr_()), q.rInterval, w2));
      } else if (op == STREAM_DEHASH_DIV) {
        // Θ zawsze wyprzedza swój slot o mniej niż jeden okres wyjścia.
        // Jeden slot jest dokładnym własnym ogonem operatora.
        ++result;
      } else if (op == STREAM_DEHASH_MOD) {
        // ~Theta wybiera pozycję floor(n*DeltaOut/DeltaSource), dostępną
        // najpóźniej w bieżącym slocie — własny ogon wynosi zero.
      } else if (op == STREAM_SUBTRACT) {
        result = SubtractStartupLatency(delta1, q.rInterval, w1, coreInstance[src1].isDeclaration());
      } else if (op == STREAM_AGSE) {
        const auto step       = std::get<std::pair<int, int>>(q.lProgram.back().getVT()).first;
        const int sourceWidth = coreInstance[src1].descriptorStorage().flatElementCount();
        result                = AgseStartupLatency(sourceWidth, step, w1);
      } else if (op == STREAM_AVG || op == STREAM_MIN || op == STREAM_MAX || op == STREAM_SUM) {
        // Redukcje działają wyłącznie na bieżącej krotce producenta.
      }

      latency[q.id] = result;
      changed       = true;
    }
  }

  requireResolvedForEveryNode(coreInstance, latency, "compiler::computeStartupLatency", "startup latency");
  for (auto &q : coreInstance)
    q.startupLatency = latency.at(q.id);
  return {"OK"};
}

std::string compiler::factorMatchedHashTimeMoves() {
  auto findUniqueQueryIndex = [this](const std::string &id) {
    size_t found = coreInstance.size();
    for (size_t i = 0; i < coreInstance.size(); ++i) {
      if (coreInstance.at(i).id != id) continue;
      if (found != coreInstance.size()) return coreInstance.size();
      found = i;
    }
    return found;
  };

  auto matchTimeMove = [](const query &q, std::string &source, int &offset) {
    if (!q.isSubstrat || q.lProgram.size() != 2) return false;
    auto it = q.lProgram.begin();
    if (it->getCommandID() != PUSH_STREAM) return false;
    source = it->getStr_();
    ++it;
    if (it->getCommandID() != STREAM_TIMEMOVE || !std::holds_alternative<int>(it->getVT())) return false;
    offset = std::get<int>(it->getVT());
    return offset >= 0;
  };

  auto matchesHash = [](const query &q, const std::string &left, const std::string &right) {
    if (q.lProgram.size() != 3) return false;
    auto it = q.lProgram.begin();
    if (it->getCommandID() != PUSH_STREAM || it->getStr_() != left) return false;
    ++it;
    if (it->getCommandID() != PUSH_STREAM || it->getStr_() != right) return false;
    ++it;
    return it->getCommandID() == STREAM_HASH;
  };

  // Liczba OBCYCH odwołań do strumienia. Musi obejmować programy pól, bo
  // przekierowanie odwołań jest punktowe (zmienia się wyłącznie dopasowane
  // zapytanie), więc substrat wolno usunąć dopiero, gdy nie używa go już nikt —
  // a odwołanie potrafi siedzieć wyłącznie w PUSH_ID/PUSH_ID2 programu pola.
  // Odwołania własne są pomijane: każdy substrat czyta sam siebie w programie
  // pola i bez tego wyłączenia żaden nie zostałby nigdy uznany za osierocony.
  auto countConsumers = [this](const std::string &name) {
    size_t count = 0;
    for (const auto &q : coreInstance) {
      if (q.id == name) continue;
      for (const auto &tok : q.lProgram)
        if (tok.getCommandID() == PUSH_STREAM && tok.getStr_() == name) ++count;
      for (const auto &f : q.lSchema)
        for (const auto &tok : f.lProgram) {
          if (tok.getCommandID() == PUSH_ID && std::get<std::pair<std::string, int>>(tok.getVT()).first == name) ++count;
          if (tok.getCommandID() == PUSH_ID2 && tok.getStr_().starts_with(name + "[")) ++count;
        }
    }
    return count;
  };

  auto schemasMatch = [](const query &a, const query &b) {
    if (a.lSchema.size() != b.lSchema.size()) return false;
    return std::equal(a.lSchema.begin(), a.lSchema.end(), b.lSchema.begin(), [](const field &left, const field &right) {
      return left.field_.rtype == right.field_.rtype && left.field_.rlen == right.field_.rlen &&
             left.field_.rarray == right.field_.rarray;
    });
  };

  bool optimized = false;
  bool changed   = true;
  while (changed) {
    changed = false;
    for (size_t queryIndex = 0; queryIndex < coreInstance.size(); ++queryIndex) {
      auto &q = coreInstance.at(queryIndex);
      if (q.lProgram.size() != 3) continue;

      auto programIt = q.lProgram.begin();
      if (programIt->getCommandID() != PUSH_STREAM) continue;
      const std::string leftShiftName = programIt->getStr_();
      ++programIt;
      if (programIt->getCommandID() != PUSH_STREAM) continue;
      const std::string rightShiftName = programIt->getStr_();
      ++programIt;
      if (programIt->getCommandID() != STREAM_HASH || leftShiftName == rightShiftName) continue;

      const size_t leftShiftIndex  = findUniqueQueryIndex(leftShiftName);
      const size_t rightShiftIndex = findUniqueQueryIndex(rightShiftName);
      if (leftShiftIndex == coreInstance.size() || rightShiftIndex == coreInstance.size()) continue;

      std::string leftSource;
      std::string rightSource;
      int leftOffset  = 0;
      int rightOffset = 0;
      if (!matchTimeMove(coreInstance.at(leftShiftIndex), leftSource, leftOffset) ||
          !matchTimeMove(coreInstance.at(rightShiftIndex), rightSource, rightOffset))
        continue;

      const auto leftDeltaRaw  = coreInstance.getQuery(leftSource).rInterval;
      const auto rightDeltaRaw = coreInstance.getQuery(rightSource).rInterval;
      if (leftDeltaRaw <= 0 || rightDeltaRaw <= 0) continue;
      const boost::rational<std::int64_t> leftDelta(leftDeltaRaw.numerator(), leftDeltaRaw.denominator());
      const boost::rational<std::int64_t> rightDelta(rightDeltaRaw.numerator(), rightDeltaRaw.denominator());
      if (leftDelta * static_cast<std::int64_t>(leftOffset) != rightDelta * static_cast<std::int64_t>(rightOffset)) continue;

      const std::int64_t combinedOffset = static_cast<std::int64_t>(leftOffset) + rightOffset;
      if (combinedOffset > std::numeric_limits<int>::max()) continue;

      // Kopia przed ewentualnym push_back: dopisanie węzła unieważnia referencję q.
      const auto queryInterval = q.rInterval;

      const std::string hashName = composeStreamName(rightSource, leftSource, STREAM_HASH);
      const bool hashNameExists =
          std::ranges::any_of(coreInstance, [&hashName](const query &candidate) { return candidate.id == hashName; });
      const size_t hashIndex = findUniqueQueryIndex(hashName);
      if (hashNameExists && hashIndex == coreInstance.size()) continue;
      if (hashNameExists) {
        // Ponownie użyć wolno wyłącznie substratu. Konwencja nazewnicza kompilatora
        // nie jest zarezerwowana dla nazw użytkownika, więc zapytanie publiczne może
        // nazywać się jak węzeł przeplotu i mieć program {PUSH X, PUSH Y, STREAM_HASH}.
        // Jego wyjściem jest wtedy projekcja, a nie surowy przeplot — a schemasMatch
        // porównuje tylko typy, długości i liczności pól, więc projekcja zgodna
        // typowo, lecz o innej kolejności pól przechodzi tę kontrolę.
        if (!coreInstance.at(hashIndex).isSubstrat || !matchesHash(coreInstance.at(hashIndex), leftSource, rightSource) ||
            coreInstance.at(hashIndex).rInterval != queryInterval ||
            !schemasMatch(coreInstance.at(hashIndex), coreInstance.at(leftShiftIndex)))
          continue;
      } else {
        // Węzeł przeplotu powstaje jako NOWY element planu. Dawniej reguła
        // przemianowywała substrat A>i w miejscu i przekierowywała wszystkie
        // odwołania globalnie — poprawne wyłącznie dlatego, że wcześniejszy
        // strażnik dopuszczał dokładnie jednego konsumenta. Ten strażnik był
        // zarazem warunkiem "brak współdzielenia", więc wyłączał regułę dokładnie
        // w planach wielozapytaniowych. Mutacja w miejscu bez niego psuje plan,
        // w którym A>i karmi także konsumenta niepasującego do wzorca reguły.
        query hashQuery      = coreInstance.at(leftShiftIndex);
        hashQuery.id         = hashName;
        hashQuery.rInterval  = queryInterval;
        hashQuery.isSubstrat = true;
        hashQuery.lProgram   = {
            token(PUSH_STREAM, leftSource),
            token(PUSH_STREAM, rightSource),
            token(STREAM_HASH),
        };
        // Kopia niesie odwołanie programu pola do starej nazwy — przenieść je na nową.
        retargetSchemaReferences(hashQuery, leftShiftName, hashName);
        coreInstance.push_back(hashQuery);  // unieważnia referencje do elementów qTree
      }

      // Przekierowanie jest punktowe: zmienia się wyłącznie dopasowane zapytanie.
      // Pozostali konsumenci substratów przesunięć zachowują swoje odwołania.
      // Obok drzewa FROM trzeba przenieść także schemat — przy SELECT * pola
      // dopasowanego zapytania odwołują się do substratów przesunięć przez
      // PUSH_ID2, a po przepisaniu ich źródłem jest węzeł przeplotu.
      auto &matched = coreInstance.at(queryIndex);
      if (combinedOffset == 0)
        matched.lProgram = {token(PUSH_STREAM, hashName)};
      else
        matched.lProgram = {
            token(PUSH_STREAM, hashName),
            token(STREAM_TIMEMOVE, static_cast<int>(combinedOffset)),
        };
      retargetSchemaReferences(matched, leftShiftName, hashName);
      retargetSchemaReferences(matched, rightShiftName, hashName);

      // Substrat przesunięcia znika dopiero, gdy stracił ostatniego konsumenta.
      const bool leftOrphaned  = countConsumers(leftShiftName) == 0;
      const bool rightOrphaned = countConsumers(rightShiftName) == 0;
      if (leftOrphaned || rightOrphaned) {
        auto removed = std::ranges::remove_if(coreInstance, [&](const query &candidate) {
          return (leftOrphaned && candidate.id == leftShiftName) || (rightOrphaned && candidate.id == rightShiftName);
        });
        coreInstance.erase(removed.begin(), removed.end());
      }
      rdb::probe::onRewriteR1();
      optimized = true;
      changed   = true;
      break;
    }
  }
  if (optimized) coreInstance.topologicalSort();
  return {"OK"};
}

std::string compiler::deduplicateSubstrats() {
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto it = coreInstance.begin(); it != coreInstance.end(); ++it) {
      if (!it->isSubstrat) continue;

      for (auto it2 = coreInstance.begin(); it2 != coreInstance.end(); ++it2) {
        if (it2 == it) continue;
        if (it2->rInterval != it->rInterval) continue;
        if (it2->lProgram.size() != it->lProgram.size()) continue;
        if (it2->lSchema.size() != it->lSchema.size()) continue;

        bool progMatch = std::equal(it->lProgram.begin(), it->lProgram.end(), it2->lProgram.begin(), [](token &a, token &b) {
          return a.getCommandID() == b.getCommandID() && a.getVT() == b.getVT();
        });
        if (!progMatch) continue;

        bool schemaMatch =
            std::equal(it->lSchema.begin(), it->lSchema.end(), it2->lSchema.begin(), [](const field &a, const field &b) {
              return a.field_.rtype == b.field_.rtype && a.field_.rlen == b.field_.rlen && a.field_.rarray == b.field_.rarray;
            });
        if (!schemaMatch) continue;

        const std::string oldName = it->id;
        const std::string newName = it2->id;
        replaceStreamReferences(oldName, newName);

        coreInstance.erase(it);
        changed = true;
        break;
      }
      if (changed) break;
    }
  }
  return {"OK"};
}

std::string compiler::shareEquivalentSelectComputations() {
  // Współdziel tylko kosztowne programy pól. Publiczne SELECT-y pozostają
  // osobnymi strumieniami, dzięki czemu zachowują storage, reguły i deskryptory.
  auto substratType = std::string("DEFAULT");
  auto directiveIt  = std::ranges::find_if(coreInstance, [](const query &qry) { return qry.id == ":SUBSTRAT"; });
  if (directiveIt != coreInstance.end()) substratType = directiveIt->filename;
  std::ranges::transform(substratType, substratType.begin(), ::toupper);

  std::set<std::string> visiting;
  std::function<std::string(const query &)> programFingerprint;
  std::function<std::string(const std::string &)> sourceFingerprint;

  sourceFingerprint = [&](const std::string &sourceId) {
    auto &source = coreInstance.getQuery(sourceId);
    if (!source.isSubstrat) return std::string("SOURCE{") + sourceId + "}";
    return programFingerprint(source);
  };

  programFingerprint = [&](const query &qry) {
    if (!visiting.insert(qry.id).second) return std::string("CYCLE{") + qry.id + "}";

    std::string result;
    if (qry.lProgram.size() == 3) {
      auto it              = qry.lProgram.begin();
      const token &left    = *it++;
      const token &right   = *it++;
      const token &command = *it;
      if (left.getCommandID() == PUSH_STREAM && right.getCommandID() == PUSH_STREAM && command.getCommandID() == STREAM_ADD) {
        // STREAM_ADD jest przemienny w obrębie jednego węzła. Nie spłaszczamy
        // drzewa, bo różne grupowanie może zmienić harmonogram uruchomienia.
        auto leftFingerprint  = sourceFingerprint(left.getStr_());
        auto rightFingerprint = sourceFingerprint(right.getStr_());
#if RDB_OPT_COMMUTATIVE_ADD
        if (rightFingerprint < leftFingerprint) {
          std::swap(leftFingerprint, rightFingerprint);
          rdb::probe::onRewriteR2(qry.id);
        }
#endif
        result = "ADD{" + leftFingerprint + "}{" + rightFingerprint + "}";
      }
    }

    if (result.empty()) {
      std::ostringstream out;
      out << "PROGRAM{";
      for (const auto &item : qry.lProgram) {
        if (item.getCommandID() == PUSH_STREAM)
          out << "STREAM{" << sourceFingerprint(item.getStr_()) << "}";
        else
          out << "TOKEN{" << item << "}";
      }
      out << "}";
      result = out.str();
    }

    visiting.erase(qry.id);
    return result;
  };

  auto queryFingerprint = [&](query &qry) -> std::optional<std::string> {
    if (qry.isDeclaration() || qry.isCompilerDirective() || qry.isSubstrat || qry.lSchema.empty()) return std::nullopt;
    if (restrictSelectSharing_ && !selectSharingScope_.contains(qry.id)) return std::nullopt;

    const auto fromFingerprint = programFingerprint(qry);
    if (fromFingerprint.find("ADD{") == std::string::npos) return std::nullopt;

    std::ostringstream out;
    out << "INTERVAL{" << qry.rInterval.numerator() << "/" << qry.rInterval.denominator() << "}";
    out << "FROM{" << fromFingerprint << "}";
    out << "FIELDS{";
    for (const auto &item : qry.lSchema) {
      out << "SHAPE{" << static_cast<int>(item.field_.rtype) << ":" << item.field_.rlen << ":" << item.field_.rarray << "}";
      out << "PROGRAM{";
      for (const auto &fieldToken : item.lProgram) {
        switch (fieldToken.getCommandID()) {
          case PUSH_ID1:
          case PUSH_ID2:
          case PUSH_ID3:
          case PUSH_ID4:
          case PUSH_ID5:
          case PUSH_IDX:
          case PUSH_TSCAN:
            return std::nullopt;
          case PUSH_ID: {
            const auto &[sourceId, offset] = std::get<std::pair<std::string, int>>(fieldToken.getVT());
            if (sourceId == qry.id) return std::nullopt;
            out << "FIELD{" << sourceId << ":" << offset << "}";
          } break;
          default:
            out << "TOKEN{" << fieldToken << "}";
            break;
        }
      }
      out << "}";
    }
    out << "}";
    return out.str();
  };

  std::map<std::string, std::vector<std::string>> groups;
  for (auto &qry : coreInstance) {
    auto fingerprint = queryFingerprint(qry);
    if (fingerprint.has_value()) groups[*fingerprint].push_back(qry.id);
  }

  bool changed = false;
  for (auto &group : groups) {
    auto &queryIds = group.second;
    if (queryIds.size() < 2) continue;
    std::ranges::sort(queryIds);

    const query representative = coreInstance.getQuery(queryIds.front());
    std::string sharedId       = "STREAM_SELECT_" + representative.id;
    for (int suffix = 2; coreInstance.exists(sharedId); ++suffix)
      sharedId = "STREAM_SELECT_" + representative.id + "_" + std::to_string(suffix);

    query shared = representative;
    shared.id    = sharedId;
    shared.filename.clear();
    shared.isDisposable = false;
    shared.isOneShot    = false;
    shared.isHold       = false;
    shared.isSubstrat   = true;
    shared.lRules.clear();
    shared.retention      = rdb::retention_t{.segments = 0, .capacity = 0};
    shared.policy         = std::make_pair(substratType, 1);
    shared.storage_policy = "DEFAULT";
    coreInstance.push_back(std::move(shared));

    for (const auto &queryId : queryIds) {
      auto &qry    = coreInstance.getQuery(queryId);
      int position = 0;
      for (auto &item : qry.lSchema)
        item.lProgram = {token(PUSH_ID, std::make_pair(sharedId, position++))};
      qry.lProgram = {token(PUSH_STREAM, sharedId)};
    }
    changed = true;
  }

  if (!changed) return {"OK"};

  bool removed = true;
  while (removed) {
    std::set<std::string> referenced;
    for (const auto &qry : coreInstance)
      for (const auto &item : qry.lProgram)
        if (item.getCommandID() == PUSH_STREAM) referenced.insert(item.getStr_());

    auto newEnd =
        std::ranges::remove_if(coreInstance, [&](const query &qry) { return qry.isSubstrat && !referenced.contains(qry.id); });
    removed = newEnd.begin() != coreInstance.end();
    coreInstance.erase(newEnd.begin(), newEnd.end());
  }

  coreInstance.topologicalSort();
  return {"OK"};
}

/// R3 — uproszczenia algebraiczne w programach pól i w warunkach reguł.
///
/// Reguły i ich uzasadnienie są przy simplifyExpression() (exprSimplify.hpp); tutaj jest
/// tylko dostarczenie typów pól i obejście planu.
///
/// Miejsce w łańcuchu nie jest dowolne — przebieg musi stać MIĘDZY resolveFieldReferences()
/// a localizeFieldOffsets(). Wcześniej odwołania do pól są jeszcze nierozwiązane
/// (PUSH_ID1/PUSH_ID2/PUSH_ID3) i typu nie ma z czego odczytać; później PUSH_ID wskazuje
/// offset w LOKALNYM buforze wejściowym zapytania, więc nazwa strumienia źródłowego
/// przestaje prowadzić do schematu. Typ jest tu konieczny: reasocjacja stałych jest
/// niepoprawna dla FLOAT/DOUBLE i dla podwyrażeń o nieznanym typie.
///
/// Przed shareEquivalentSelectComputations(), bo odciski liczą się z tokenów — kanoniczna
/// postać wyrażenia zwiększa liczbę wykrytych równoważności.
std::string compiler::simplifyFieldExpressions() {
  auto typeOfField = [this](const std::string &streamId, int fieldIndex) -> std::optional<rdb::descFld> {
    auto source = std::ranges::find_if(coreInstance, [&streamId](const query &q) { return q.id == streamId; });
    if (source == coreInstance.end()) return std::nullopt;
    if (fieldIndex < 0 || fieldIndex >= static_cast<int>(source->lSchema.size())) return std::nullopt;
    const auto type = std::next(source->lSchema.begin(), fieldIndex)->field_.rtype;
    // Pola konfiguracyjne deskryptora (TYPE, REF, RETENTION, RETMEMORY) nie są wartościami
    // wyrażeń — ich „typ" nie mówi nic o arytmetyce, więc zgłaszamy je jako nieznane.
    if (type > rdb::STRING) return std::nullopt;
    return type;
  };

  for (auto &q : coreInstance) {
    if (q.isCompilerDirective()) continue;
    for (auto &f : q.lSchema)
      rdb::probe::onRewriteR3(simplifyExpression(f.lProgram, typeOfField));
    for (auto &r : q.lRules)
      rdb::probe::onRewriteR3(simplifyExpression(r.condition, typeOfField));
  }
  return {"OK"};
}

std::string compiler::compile() {
  std::string result;

  // Sonda E3 (rdb/probe.hpp): rozmiar planu na czterech etapach, czas kompilacji i
  // liczba zastosowanych przepisań. Migawki są nieaktywne, dopóki nie ustawiono
  // RDB_BENCH_PLAN; bez wkompilowanej sondy znika cały obiekt razem z migawkami.
  // Kolejność etapów jest istotna dla wyniku: wejście to plan surowy po parsowaniu,
  // a właściwą redukcję strukturalną widać dopiero w parze przed/po deduplikacji.
  rdb::probe::planProbe planBench;
  planBench.capture(rdb::probe::planStage::entry, coreInstance);

  // Musi być PRZED pierwszym przebiegiem — patrz uzasadnienie przy definicji.
  snapshotNamedSourceRefs();

  result = extractIntermediateStreams();
  if (result != "OK") return result;

  result = expandSchemaWildcards();
  if (result != "OK") return result;

  result = resolveStreamIntervals();
  if (result != "OK") return result;

  // Niezmiennik D3 sprawdzany wokół KAŻDEGO przebiegu przepisującego z osobna. Jednego snapshotu
  // "przed optymalizacjami" zrobić się nie da, bo przebiegi przepisujące są przeplecione
  // z przebiegami dopełniającymi schemat (resolveFieldReferences, expandIndexWildcards) — te
  // legalnie zmieniają listę pól, np. rozwijając [_].
  std::map<std::string, std::vector<std::string>> namesBeforeRewrite;

#if RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES
  namesBeforeRewrite = snapshotUserFieldNames();
  result             = factorMatchedHashTimeMoves();
  if (result != "OK") return result;
  result = verifyUserFieldNamesPreserved(namesBeforeRewrite);
  if (result != "OK") return result;
#endif

  planBench.capture(rdb::probe::planStage::preDedup, coreInstance);
#if RDB_OPT_DEDUP_SUBSTRATES
  namesBeforeRewrite = snapshotUserFieldNames();
  result             = deduplicateSubstrats();
  if (result != "OK") return result;
  result = verifyUserFieldNamesPreserved(namesBeforeRewrite);
  if (result != "OK") return result;
#endif
  planBench.capture(rdb::probe::planStage::postDedup, coreInstance);

  result = resolveFieldReferences();
  if (result != "OK") return result;

  result = expandIndexWildcards();
  if (result != "OK") return result;

#if RDB_OPT_SIMPLIFY_EXPRESSIONS
  namesBeforeRewrite = snapshotUserFieldNames();
  result             = simplifyFieldExpressions();
  if (result != "OK") return result;
  result = verifyUserFieldNamesPreserved(namesBeforeRewrite);
  if (result != "OK") return result;
#endif

  // Podpis pól musi używać źródłowych PUSH_ID, zanim ich offsety zostaną
  // przepisane na lokalny bufor wejściowy publicznego zapytania.
#if RDB_OPT_SHARE_EQUIVALENT_SELECTS
  namesBeforeRewrite = snapshotUserFieldNames();
  result             = shareEquivalentSelectComputations();
  if (result != "OK") return result;
  result = verifyUserFieldNamesPreserved(namesBeforeRewrite);
  if (result != "OK") return result;
#endif

  result = localizeFieldOffsets();
  if (result != "OK") return result;

  // Po wszystkich przepisaniach planu — ogon liczymy dla planu, który faktycznie pójdzie do wykonania.
  // Pojemność historii zależy od tej wartości: opóźniony konsument może nadal
  // potrzebować wczesnych rekordów szybszego producenta.
  //
  // Origin przed ogonem: ogon węzła `@` origin nie potrzebuje (minimum reszty n*step mod F
  // jest osiągane niezależnie od tego, od którego n zaczyna się strumień), ale model pojemności
  // potrzebuje obu, a przebiegi mają być czytelnie uporządkowane od definicji do konsekwencji.
  result = computeLogicalOrigin();
  if (result != "OK") return result;

  result = computeStartupLatency();
  if (result != "OK") return result;

  coreInstance.maxCapacity = computeRequiredCapacities();

  result = validateConstraints();
  if (result != "OK") return result;

  result = applyCapacitiesToStreams(coreInstance.maxCapacity);
  if (result != "OK") return result;

  // Kolejność elementów qTree jest kolejnością przetwarzania w takcie
  // (dataModel::processRows). Musi być topologiczna: producent przed
  // konsumentem. resolveStreamIntervals() sortuje qTree po rInterval
  // (qTree::sort, operator< na query), co ten porządek niszczy — a przywracał
  // go dotąd wyłącznie factorMatchedHashTimeMoves(), i tylko gdy reguła
  // faktycznie coś przepisała. Skutkiem była zależność semantyki planu od tego,
  // czy odpaliła niezwiązana optymalizacja. Najdotkliwiej dla przeplotu:
  // delta wyniku # jest mniejsza od delt argumentów, więc sortowanie po
  // interwale stawia konsumenta PRZED jego producentami.
  coreInstance.topologicalSort();

  // Migawka końcowa + raport E3 na stderr (tylko gdy RDB_BENCH_PLAN).
  planBench.report(coreInstance, coreInstance.maxCapacity, RDB_OPT_DEDUP_SUBSTRATES);

  return {"OK"};
}

std::vector<std::string> compiler::importFrom(qTree &source) {
  std::vector<std::string> retVal;
  // Ponowna kompilacja aktywnego planu nie może przepisać już utworzonych
  // streamInstance. W trybie ad-hoc analizujemy wyłącznie właśnie importowane ID.
  restrictSelectSharing_ = true;
  selectSharingScope_.clear();
  for (auto &q : source) {
    if (q.isCompilerDirective()) continue;
    if (coreInstance.exists(q.id)) continue;
    coreInstance.push_back(q);
    retVal.push_back(q.id);
    selectSharingScope_.insert(q.id);
  }
  return retVal;
}
