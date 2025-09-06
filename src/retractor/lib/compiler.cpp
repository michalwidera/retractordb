#include "compiler.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/rational.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>
#include <fstream>
#include <iostream>

#include "QStruct.h"
#include "SOperations.hpp"
#include "cmdID.hpp"
#include "fldType.hpp"

using boost::lexical_cast;

boost::regex xprFieldId5("(\\w*)\\[(\\d*)\\]\\[(\\d*)\\]");  // something[1][1]
boost::regex xprFieldId4("(\\w*)\\[(\\d*)\\,(\\d*)\\]");     // something[1,1]
boost::regex xprFieldId2("(\\w*)\\[(\\d*)\\]");              // something[1]
boost::regex xprFieldIdX("(\\w*)\\[_]");                     // something[_]
boost::regex xprFieldId1("(\\w*).(\\w*)");                   // something.in_schema
boost::regex xprFieldId3("(\\w*)");                          // field_of_corn

/** This procedure computes time delays (delta) for generated streams */
std::string compiler::intervalCounter() {
  while (true) {
    bool bOnceAgain(false);
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
      assert(q.lProgram.size() == 3 || q.lProgram.size() == 2);
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
            continue;
          } else
            delta = (delta1 * delta2) / (delta1 + delta2);  // deltaHash(delta1, delta2);
        } break;
        case STREAM_DEHASH_DIV: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = t2.getRI();  // There is no second stream
          // - just fraction argument
          assert(delta2 != 0);
          if (delta1 == 0) {
            bOnceAgain = true;
            continue;
          } else {
            //           D_c * D_b
            //   D_a = --------------
            //         abs(D_c - D_b)
            delta = (delta1 * delta2) / abs(delta1 - delta2);  // deltaDivMod(delta1, delta2);
          }
          if (delta1 > delta) {
            SPDLOG_ERROR("Faster div from slower src q.id={},D1={}, D2={}", q.id, delta1, delta2);
            throw std::out_of_range("You cannot make faster div from slower source");
          }
        } break;
        case STREAM_DEHASH_MOD: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = t2.getRI();
          assert(delta2 != 0);
          if (delta1 == 0) {
            bOnceAgain = true;
            continue;
          } else {
            //           D_c * D_a
            //   D_b = --------------
            //         abs(D_c - D_a)
            delta = (delta2 * delta1) / abs(delta2 - delta1);  // deltaDivMod(delta2, delta1);  (NOTICE DIFF SEQ!)
          }
          if (delta1 > delta) {
            SPDLOG_ERROR("Faster div from slower src q.id={},D1={}, D2={}", q.id, delta1, delta2);
            throw std::out_of_range("You cannot make faster mod from slower source");
          }
        } break;
        case STREAM_SUBTRACT: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          if (delta1 == 0) {
            bOnceAgain = true;
            continue;
          } else
            delta = delta1;  // deltaSubtract(delta1);
        } break;
        case STREAM_ADD: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = coreInstance.getDelta(t2.getStr_());
          if (delta1 == 0 || delta2 == 0) {
            bOnceAgain = true;
            continue;
          } else
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
          int coreWindow                 = coreInstance.getQuery(t1.getStr_()).lSchema.size();
          auto [step, windowSize]        = std::get<std::pair<int, int>>(op.getVT());
          assert(step > 0);
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
          SPDLOG_ERROR("Undefined token: command={}, var={}, txt={}", op.getStrCommandID(), op.getRI(), op.getStr_());
          throw std::out_of_range("Undefined token/command on list");
      }  // switch ( op.getCommandID() )
      assert(delta != -1);
      q.rInterval = delta;  // There is established delta value - return value
    }  // BOOST_FOREACH ( query & q , coreInstance )
    if (!bOnceAgain)
      break;
    else
      coreInstance.sort();
  }  // while(true)
  coreInstance.sort();
  return std::string("OK");
}

std::string compiler::generateStreamName(const std::string &sName1, const std::string &sName2, command_id cmd) {
  if (sName2 == "")
    return std::string(GetStringcommand_id(cmd)) + std::string("_") + sName1;
  else
    return std::string(GetStringcommand_id(cmd)) + std::string("_") + sName2 + std::string("_") + sName1;
}

/* Goal of this procedure is to provide stream to canonical form
TODO: Stream_MAX,MIN,AVG...
*/
std::string compiler::simplifyLProgram() {
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
          std::string arg1 = "";
          std::string arg2 = "";

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
          lTempProgram.push_back(token(PUSH_TSCAN));
          newQuery.lSchema.push_back(field(rdb::rField("*", 1, 1, rdb::BYTE), lTempProgram));
          newQuery.substratPolicy = std::make_pair(substratType_C, 1);
          newQuery.id             = generateStreamName(arg1, arg2, cmd);
          (*it).lProgram.insert(it2, token(PUSH_STREAM, newQuery.id));
          coreInstance.push_back(newQuery);
          it = coreInstance.begin();

          break;

        }  // Endif PUSH_STREAM, PUSH_VAL
      }  // Endfor
    }  // Endif
  }  // Endfor
  return std::string("OK");
}

// Goal of this procedure is to unroll schema based of given command
std::list<field> compiler::combine(const std::string &sName1, const std::string &sName2, token &cmd_token) {
  std::list<field> lRetVal;
  const command_id cmd = cmd_token.getCommandID();
  // Merge of schemas for junction of hash type
  if (cmd == STREAM_HASH) {
    if (coreInstance.getQuery(sName1).descriptorStorage().sizeFlat() !=
        coreInstance.getQuery(sName2).descriptorStorage().sizeFlat())
      throw std::invalid_argument("Hash operation needs same schemas on arguments stream");
    lRetVal = coreInstance.getQuery(sName1).lSchema;
  } else if (cmd == STREAM_DEHASH_DIV)
    lRetVal = coreInstance.getQuery(sName1).lSchema;
  else if (cmd == STREAM_DEHASH_MOD)
    lRetVal = coreInstance.getQuery(sName1).lSchema;
  else if (cmd == STREAM_ADD) {
    int fieldCountSh = 0;
    int i            = 0;
    for (auto f : coreInstance.getQuery(sName1).lSchema) {
      field intf(rdb::rField(sName1 + "_" + boost::lexical_cast<std::string>(fieldCountSh++), f.field_.rlen, f.field_.rarray,
                             f.field_.rtype),
                 token(PUSH_ID, std::make_pair(sName1, i++)));
      lRetVal.push_back(intf);
    }
    i = 0;
    for (auto f : coreInstance.getQuery(sName2).lSchema) {
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
    auto [maxType, maxLen]  = coreInstance[sName1].descriptorStorage().getMaxType();
    std::list<field> schema;
    for (int i = 0; i < abs(windowSize); i++) {
      field intf(rdb::rField(sName1 + "_" + lexical_cast<std::string>(i), maxLen, 1, maxType),
                 token(PUSH_ID, std::make_pair(sName1, 0)));
      schema.push_back(intf);
    }

    lRetVal = schema;
  } else {
    SPDLOG_ERROR("Undefined: str:{} cmd:{}", cmd_token.getStr_(), cmd_token.getStrCommandID());
    // throw std::invalid_argument("Command stack hits undefined operation");
    assert(false && "Undefined stream token command in combine function.");
    abort();
  }
  // Here are added to fields execution methods
  // by reference to schema position
  int offset(0);
  for (auto &f : lRetVal) {
    std::stringstream s;
    if (cmd == STREAM_HASH)
      s << sName1;
    else if (cmd == STREAM_TIMEMOVE)
      s << sName1;
    else if (cmd == STREAM_AGSE)
      s << sName1;
    else {
      s << sName1;  // generateStreamName( sName2, sName1, cmd ) ;
    }
    s << "[";
    s << offset++;
    s << "]";
    if (f.lProgram.size() > 0) f.lProgram.pop_front();
    f.lProgram.push_front(token(PUSH_ID2, std::make_pair(s.str(), 0)));
  }
  return lRetVal;
}

// goal of this procedure is setup of all possible fields name and unroll *
// unfortunately algorithm if broken - because does not search backward but next
// by next and some * can be process which have arguments appear as two asterisk
// In such case unroll does not appear and algorithm gets shitin-shitout
std::string compiler::prepareFields() {
  int fieldCountSh = 0;
  coreInstance.topologicalSort();
  for (auto &q : coreInstance) {
    for (auto &t : q.lProgram) {
      assert(q.lProgram.size() < 4);
      // fail of this assertion means that all streams are
      // after optimization already
      std::vector<std::list<field>::iterator> eraseList;
      auto it = q.lSchema.begin();
      for (auto &f : q.lSchema) {
        if (f.getFirstFieldToken().getCommandID() == PUSH_TSCAN) {
          // found! - and now unroll
          if (q.lProgram.size() == 1) {
            // we assure that on and only token is push_stream
            assert((*q.lProgram.begin()).getCommandID() == PUSH_STREAM);
            auto nameOfscanningTable = (*q.lProgram.begin()).getStr_();
            // Remove of TSCAN
            eraseList.push_back(it);
            // q.lSchema =  getQuery(t.getStr()).lSchema;
            // copy list of fields from one to another
            int filedPosition = 0;
            for (auto s : coreInstance.getQuery(t.getStr_()).lSchema) {
              std::list<token> lTempProgram;
              lTempProgram.push_back(token(PUSH_ID, std::make_pair(nameOfscanningTable, filedPosition++)));
              std::string name = /*"Field_"*/ t.getStr_() + "_" + boost::lexical_cast<std::string>(fieldCountSh++);
              q.lSchema.push_back(field(rdb::rField(name, 4, 1, rdb::INTEGER), lTempProgram));
            }
            break;
          }
          if (q.lProgram.size() == 3 || q.lProgram.size() == 2) {
            auto [sName1, sName2, cmd]{GetArgs(q.lProgram)};
            q.lSchema = combine(sName1, sName2, cmd);
            break;
          }
        }
        ++it;
      }
      for (auto eraseIt : eraseList) q.lSchema.erase(eraseIt);
    }
  }
  coreInstance.sort();
  return std::string("OK");
}

/* If in query plan is PUSH_IDX it means that we need to duplicate [_] */
std::string compiler::replicateIDX() {
  for (auto &q : coreInstance) {             // for each query
    for (auto &f : q.lSchema) {              // for each field in query
      std::vector<std::string> usedSchemaX;  //
      for (auto &t : f.lProgram)             // for each token in query field
        if (t.getCommandID() == PUSH_IDX)
          usedSchemaX.push_back(get<std::pair<std::string, int>>(t.getVT()).first);  // .second arg is always 0
      if (usedSchemaX.size() != 0) {
        int minSizeFlat{std::numeric_limits<int>::max()};
        for (auto schema : usedSchemaX) {
          auto size = coreInstance.getQuery(schema).descriptorStorage().sizeFlat();
          if (size < minSizeFlat) minSizeFlat = size;
        }

        assert(minSizeFlat != std::numeric_limits<int>::max());
        assert(minSizeFlat > 0);
        assert(q.lSchema.size() == 1);

        field oldField = *q.lSchema.begin();
        q.lSchema.clear();
        for (int i = 0; i < minSizeFlat; i++) {
          std::list<token> lTempProgram;
          for (auto &t : oldField.lProgram) {
            if (t.getCommandID() == PUSH_IDX)
              lTempProgram.push_back(token(PUSH_ID, std::make_pair(t.getStr_(), i)));
            else
              lTempProgram.push_back(token(t.getCommandID(), t.getVT()));
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
  return std::string("OK");
}

void compiler::ftokenfix(std::list<token> &lProgram, query &q) {
  for (auto &t : lProgram) {  // for each token in query field
    const command_id cmd(t.getCommandID());
    const std::string text(t.getStr_());
    boost::cmatch what;
    switch (cmd) {
      case PUSH_ID1:
        if (regex_search(text.c_str(), what, xprFieldId1)) {
          assert(what.size() == 3);
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
                } else
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
          assert(what.size() == 2);
          const std::string schema(what[1]);
          t = token(PUSH_IDX, std::make_pair(schema, 0));
        } else
          throw std::out_of_range("No mach on type conversion IDX");
        break;
      case PUSH_ID2:
        if (regex_search(text.c_str(), what, xprFieldId2)) {
          assert(what.size() == 3);
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
          assert(what.size() == 2);
          const std::string field(what[1]);
          query *pQ1(nullptr), *pQ2(nullptr);
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
          assert(what.size() == 4);
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
    }
  }
}
/* Purpose of this function is to translate all references to fields
to form schema_name[postion, time_offset]
Command method of presentation aims simple data processing
Aim of this procedure is change all of push_idXXX to push_id
note that push_id is closest to push_id4
push_idXXX is searched in all stream program after reduction */
std::string compiler::convertReferences() {
  for (auto &q : coreInstance) {  // for each query
    assert(!q.isReductionRequired());
    for (auto &f : q.lSchema) {  // for each field in query
      ftokenfix(f.lProgram, q);  // for each token in query field
    }  // end for each field in query
    for (auto &r : q.lRules) {         // for each rule in query
      ftokenfix(r.leftCondition, q);   // for each token in rule
      ftokenfix(r.rightCondition, q);  // for each token in rule
    }  // end for each rule in query
  }
  return std::string("OK");
}

/* This function will convert fields list where stream a from b#c
clause from b[x1],c[x2] int a[y1],a[y2] according to offset of from operation */
std::string compiler::convertRemotes() {
  std::map<std::string, std::map<std::string, int>> offsetMap;

  // This loop fill&create OffsetMap structure.
  for (auto &q : coreInstance) {            // for each query
    assert(!q.isReductionRequired());       // that has at least two arguments
    auto offset{0};                         //
    std::map<std::string, int> offsetItem;  //
    for (auto &f : q.lProgram) {            // for each token in stream program
      if (f.getCommandID() == PUSH_STREAM) {
        offsetItem[f.getStr_()] = offset;
        offset += coreInstance[f.getStr_()].descriptorStorage().sizeFlat();
      }
      if (f.getCommandID() == STREAM_HASH) {
        for (auto &i : offsetItem) i.second = 0;
      }
    }
    offsetMap[q.id] = offsetItem;
  }

  // This loop converts with help of offsetMap
  for (auto &q : coreInstance) {            // for each query
    assert(!q.isReductionRequired());       // that has at least two arguments and
    for (auto &f : q.lSchema) {             // for each field in query and
      for (auto &t : f.lProgram) {          // for each token in query field - do:
        if (t.getCommandID() == PUSH_ID) {  // fix only PUSH_ID tokens
          auto [schema, offset] = std::get<std::pair<std::string, int>>(t.getVT());
          if (schema != q.id) t = token(PUSH_ID, std::make_pair(q.id, offsetMap[q.id][schema] + offset));
        }
      }
    }
  }
  return std::string("OK");
}

std::string compiler::applyConstraints() {
  for (auto &q : coreInstance) {       // for each query
    if (q.isDeclaration()) continue;   // do not check declaration in constraints.
    assert(!q.isReductionRequired());  // process data only with two or less arguments
    auto [arg1, arg2, cmd]{GetArgs(q.lProgram)};
    auto i{0};
    switch (cmd.getCommandID()) {
      case STREAM_HASH: {
        SPDLOG_ERROR("Hash operations need to work on two schemas with the same size.");
        if (coreInstance.getQuery(arg1).descriptorStorage().sizeFlat() !=
            coreInstance.getQuery(arg2).descriptorStorage().sizeFlat())
          return std::string("HASH operation constraint failed on " + q.id);
      } break;
      default:
        break;
    }
  }
  return std::string("OK");
}

std::string compiler::fillSubstractsMemSize(const std::map<std::string, int> &capMap) {
  for (const auto &q : capMap) {                                     // for each query
    if (coreInstance[q.first].substratPolicy.second == 0) continue;  // do not check declaration in constraints.
    assert(!coreInstance[q.first].isReductionRequired());            // process data only with two or less arguments
    coreInstance[q.first].substratPolicy.second = q.second;          // set memory size
  }
  return std::string("OK");
}

std::map<std::string, int> compiler::countBuffersCapacity() {
  std::map<std::string, int> capMap;  // <- This var goes to qTree class instance

  for (auto &q : coreInstance) {       // for each declaration
    if (!q.isDeclaration()) continue;  // that is declaration
    capMap[q.id] = 1;
  }

  for (auto &q : coreInstance) {       // for each query
    if (q.isDeclaration()) continue;   // that is not declaration
    assert(!q.isReductionRequired());  // process data only with two or less arguments
    auto [arg1, arg2, cmd]{GetArgs(q.lProgram)};
    switch (cmd.getCommandID()) {
      case STREAM_TIMEMOVE: {
        // 	:- PUSH_STREAM(core0)
        //  :- STREAM_TIMEMOVE(1)
        //
        assert(q.lProgram.size() == 2);

        const auto nameSrc    = arg1;
        const auto timeOffset = std::get<int>(cmd.getVT());
        capMap[nameSrc]       = std::max(capMap[nameSrc], timeOffset);
      } break;
      case STREAM_AGSE: {
        // 	:- PUSH_STREAM core -> delta_source (arg[0]) - operation
        //  :- STREAM_AGSE 2,3 -> window_length, window_step (arg[1])
        //
        assert(q.lProgram.size() == 2);

        const auto nameSrc  = arg1;
        auto [step, length] = get<std::pair<int, int>>(cmd.getVT());
        assert(step > 0);
        length                    = abs(length);
        const auto lengthOfSrc    = coreInstance[nameSrc].descriptorStorage().sizeFlat();
        const auto timeBufferSize = int(ceil((length + step) / lengthOfSrc)) + 2;

        capMap[nameSrc] = std::max(capMap[nameSrc], timeBufferSize);
      } break;
      default:
        break;
    }

    // Bump capMap with dumpRange from rules (if they are negative and attached to query declaration)
    for (const auto &rule : q.lRules) {
      auto [l, r] = rule.dumpRange;
      assert(l < r);
      if (l < 0) {
        auto [arg1, arg2, cmd]{GetArgs(q.lProgram)};
        const auto nameSrc = arg1;
        capMap[nameSrc]    = std::max(capMap[nameSrc], abs(l));
      }
    }
  }
  return capMap;
}
