#include "compiler.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>
#include <boost/rational.hpp>
#include <boost/regex.hpp>

#include "fatalError.hpp"

using boost::lexical_cast;

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
  size_t prevUnresolved = std::numeric_limits<size_t>::max();
  while (true) {
    bool bOnceAgain(false);
    size_t unresolvedCount = 0;
    coreInstance.sort();
    for (auto &q : coreInstance) {
      if (q.lProgram.empty()) {
        continue; /* Declaration */
      }
      if (q.lProgram.size() == 1) {
        token tInstance(*(q.lProgram.begin()));
        q.rInterval = coreInstance.getDelta(tInstance.getStr_());
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
          delta = (delta1 * delta2) / (delta1 + delta2);  // deltaHash(delta1, delta2);
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
          delta = (delta1 * delta2) / abs(delta1 - delta2);  // deltaDivMod(delta1, delta2);

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
          delta = (delta2 * delta1) / abs(delta2 - delta1);  // deltaDivMod(delta2, delta1);  (NOTICE DIFF SEQ!)

          if (delta1 > delta) {
            SPDLOG_ERROR("Faster div from slower src q.id={}", q.id);
            throw std::out_of_range("You cannot make faster mod from slower source");
          }
        } break;
        case STREAM_SUBTRACT: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          if (delta1 == 0) {
            bOnceAgain = true;
            unresolvedCount++;
            continue;
          }
          delta = delta1;  // deltaSubtract(delta1);
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
          int coreWindow                 = static_cast<int>(coreInstance.getQuery(t1.getStr_()).lSchema.size());
          auto [step, windowSize]        = std::get<std::pair<int, int>>(op.getVT());
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

          delta = (coreDelta * step) / coreWindow;
        } break;
        default:
          SPDLOG_ERROR("Undefined token: command={}", op.getStrCommandID());
          throw std::out_of_range("Undefined token/command on list");
      }  // switch ( op.getCommandID() )
      if (delta == -1) {
        FatalError("compiler::prepareFields: stream interval (delta) not resolved for query '{}'", q.id);
      }
      q.rInterval = delta;  // There is established delta value - return value
    }  // BOOST_FOREACH ( query & q , coreInstance )
    if (!bOnceAgain) break;
    if (unresolvedCount >= prevUnresolved) {
      SPDLOG_ERROR("Circular dependency: stream interval resolution stalled with {} unresolved streams", unresolvedCount);
      return {"Circular dependency in stream definitions"};
    }
    prevUnresolved = unresolvedCount;
    coreInstance.sort();
  }  // while(true)
  coreInstance.sort();
  return {"OK"};
}

std::string compiler::composeStreamName(const std::string &sName1, const std::string &sName2, command_id cmd) {
  if (sName2.empty()) return std::string(GetStringcommand_id(cmd)) + std::string("_") + sName1;
  return std::string(GetStringcommand_id(cmd)) + std::string("_") + sName2 + std::string("_") + sName1;
}

/* Goal of this procedure is to provide stream to canonical form
TODO: Stream_MAX,MIN,AVG...
*/
std::string compiler::extractIntermediateStreams() {
  coreInstance.sort();

  auto substratType_C = std::string("DEFAULT");
  auto substratTypeIt = std::find_if(coreInstance.begin(), coreInstance.end(),  //
                                     [](const auto &qry) { return qry.id == ":SUBSTRAT"; });
  if (substratTypeIt != std::end(coreInstance)) substratType_C = substratTypeIt->filename;
  std::transform(substratType_C.begin(), substratType_C.end(), substratType_C.begin(), ::toupper);

  for (auto it = coreInstance.begin(); it != coreInstance.end(); ++it) {
    // Optimization phase 2
    if ((*it).isReductionRequired()) {
      for (auto it2 = (*it).lProgram.begin(); it2 != (*it).lProgram.end(); ++it2) {
        if (                                              //
            (*it2).getStrCommandID() != "PUSH_STREAM" &&  //
            (*it2).getStrCommandID() != "PUSH_VAL") {
          query newQuery;
          std::string arg1;
          std::string arg2;

          token newVal(*it2);
          newQuery.lProgram.push_front(newVal);
          command_id cmd = (*it2).getCommandID();

          it2 = (*it).lProgram.erase(it2);
          --it2;

          {
            token newValSh1(*it2);
            newQuery.lProgram.push_front(newValSh1);
            std::stringstream s;
            s << (*it2).getStr_();
            arg1 = std::string(s.str());
            it2  = (*it).lProgram.erase(it2);
            --it2;
          }
          if (cmd != STREAM_TIMEMOVE && cmd != STREAM_SUBTRACT) {
            token newValSh2(*it2);
            newQuery.lProgram.push_front(newValSh2);
            std::stringstream s;
            s << (*it2).getStr_();
            arg2 = std::string(s.str());
            it2  = (*it).lProgram.erase(it2);
            --it2;
          }
          ++it2;

          std::list<token> lTempProgram;
          lTempProgram.emplace_back(PUSH_TSCAN);
          newQuery.lSchema.emplace_back(rdb::rField("*", 1, 1, rdb::BYTE), lTempProgram);
          newQuery.policy     = std::make_pair(substratType_C, 1);
          newQuery.id         = composeStreamName(arg1, arg2, cmd);
          newQuery.isSubstrat = true;
          (*it).lProgram.insert(it2, token(PUSH_STREAM, newQuery.id));
          coreInstance.push_back(newQuery);
          it = coreInstance.begin();

          break;

        }  // Endif PUSH_STREAM, PUSH_VAL
      }  // Endfor
    }  // Endif
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
    lRetVal = coreInstance.getQuery(sName1).lSchema;
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
          auto size = coreInstance.getQuery(schema).descriptorStorage().flatElementCount();
          if (size < minSizeFlat) minSizeFlat = size;
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
                t           = token(PUSH_ID, std::make_pair(schema1, offset1));
                bFieldFound = true;
              }
              ++offset1;
            }
          }
          if (pQ2 != nullptr && !bFieldFound) {
            offset1 = 0;
            for (auto &f2 : (*pQ2).lSchema) {
              if (f2.field_.rname == field) {
                t           = token(PUSH_ID, std::make_pair(schema2, offset1));
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
          t = token(PUSH_ID, std::make_pair(schema, offset1 + offset2 * q.lSchema.size()));
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

/* This function will convert fields list where stream a from b#c
clause from b[x1],c[x2] int a[y1],a[y2] according to offset of from operation */
std::string compiler::localizeFieldOffsets() {
  std::map<std::string, std::map<std::string, int>> offsetMap;

  // This loop fill&create OffsetMap structure.
  for (auto &q : coreInstance) {  // for each query
    if (q.isReductionRequired()) {
      FatalError("compiler: query '{}' requires reduction at this stage — pipeline invariant violated", q.id);
    }  // that has at least two arguments
    auto offset{0};                         //
    std::map<std::string, int> offsetItem;  //
    for (auto &f : q.lProgram) {            // for each token in stream program
      if (f.getCommandID() == PUSH_STREAM) {
        offsetItem[f.getStr_()] = offset;
        offset += coreInstance[f.getStr_()].descriptorStorage().flatElementCount();
      }
      if (f.getCommandID() == STREAM_HASH) {
        for (auto &i : offsetItem)
          i.second = 0;
      }
    }
    offsetMap[q.id] = offsetItem;
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
      case PUSH_STREAM:
      case STREAM_DEHASH_DIV:
      case STREAM_DEHASH_MOD:
      case STREAM_ADD:
      case STREAM_SUBTRACT:
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
        capMap[nameSrc]       = std::max(capMap[nameSrc], timeOffset);
      } break;
      case STREAM_AGSE: {
        // 	:- PUSH_STREAM core -> delta_source (arg[0]) - operation
        //  :- STREAM_AGSE 2,3 -> window_length, window_step (arg[1])
        //
        if (q.lProgram.size() != 2) {
          FatalError("compiler: unexpected program size in computeRequiredCapacities: {} tokens for query '{}', expected 2",
                     q.lProgram.size(), q.id);
        }

        const auto nameSrc  = arg1;
        auto [step, length] = get<std::pair<int, int>>(cmd.getVT());
        if (step <= 0) {
          FatalError("compiler: AGSE step must be > 0, got {} for query '{}' in computeRequiredCapacities", step, q.id);
        }
        length                    = abs(length);
        const auto lengthOfSrc    = coreInstance[nameSrc].descriptorStorage().flatElementCount();
        const auto timeBufferSize = static_cast<int>(ceil(static_cast<double>(length + step) / static_cast<double>(lengthOfSrc))) + 2;

        capMap[nameSrc] = std::max(capMap[nameSrc], timeBufferSize);
      } break;
      case STREAM_HASH:
      case STREAM_DEHASH_DIV:
      case STREAM_DEHASH_MOD:
      case STREAM_ADD:
      case STREAM_SUBTRACT:
      case STREAM_AVG:
      case STREAM_MIN:
      case STREAM_MAX:
      case STREAM_SUM:
        // These commands do not increase source history requirement here.
        break;
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
        for (auto &q : coreInstance)
          for (auto &tok : q.lProgram)
            if (tok.getCommandID() == PUSH_STREAM && tok.getStr_() == oldName) tok = token(PUSH_STREAM, newName);

        coreInstance.erase(it);
        changed = true;
        break;
      }
      if (changed) break;
    }
  }
  return {"OK"};
}

std::string compiler::compile() {
  std::string result;

  result = extractIntermediateStreams();
  if (result != "OK") return result;

  result = expandSchemaWildcards();
  if (result != "OK") return result;

  result = resolveStreamIntervals();
  if (result != "OK") return result;

  result = deduplicateSubstrats();
  if (result != "OK") return result;

  result = resolveFieldReferences();
  if (result != "OK") return result;

  result = expandIndexWildcards();
  if (result != "OK") return result;

  result = localizeFieldOffsets();
  if (result != "OK") return result;

  coreInstance.maxCapacity = computeRequiredCapacities();

  result = validateConstraints();
  if (result != "OK") return result;

  result = applyCapacitiesToStreams(coreInstance.maxCapacity);
  if (result != "OK") return result;

  return {"OK"};
}

std::vector<std::string> compiler::importFrom(qTree &source) {
  std::vector<std::string> retVal;
  for (auto &q : source) {
    if (q.isCompilerDirective()) continue;
    if (coreInstance.exists(q.id)) continue;
    coreInstance.push_back(q);
    retVal.push_back(q.id);
  }
  return retVal;
}
