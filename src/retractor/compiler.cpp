#include "compiler.hpp"

#include <spdlog/spdlog.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>
#include <fstream>
#include <iostream>

#include "QStruct.h"
#include "SOperations.h"
#include "cmdID.h"
#include "fldType.h"

using boost::lexical_cast;

extern int fieldCount;

// Object coreInstance in QStruct.cpp
extern "C" qTree coreInstance;

/** This procedure computes time delays (delata) for generated streams */
std::string intervalCounter() {
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
      bool size3 = (q.lProgram.size() == 3);
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
            delta = deltaHash(delta1, delta2);
        } break;
        case STREAM_DEHASH_DIV: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = t2.get();  // There is no second stream
          // - just fraction argument
          assert(delta2 != 0);
          if (delta1 == 0) {
            bOnceAgain = true;
            continue;
          } else {
            //           D_c * D_b
            //   D_a = --------------
            //         abs(D_c - D_b)
            delta = deltaDivMod(delta1, delta2);
          }
          if (delta1 > delta) {
            SPDLOG_ERROR("Faster div from slower src q.id={},D1={}, D2={}", q.id, delta1, delta2);
            throw std::out_of_range("You cannot make faster div from slower source");
          }
        } break;
        case STREAM_DEHASH_MOD: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = t2.get();
          assert(delta2 != 0);
          if (delta1 == 0) {
            bOnceAgain = true;
            continue;
          } else {
            //           D_c * D_a
            //   D_b = --------------
            //         abs(D_c - D_a)
            delta = deltaDivMod(delta2, delta1);
          }
          if (delta1 > delta) {
            SPDLOG_ERROR("Faster div from slower src q.id={},D1={}, D2={}", q.id, delta1, delta2);
            throw std::out_of_range("You cannot make faster mod from slower source");
          }
        } break;
        case STREAM_SUBSTRACT: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          if (delta1 == 0) {
            bOnceAgain = true;
            continue;
          } else
            delta = deltaSubstract(delta1);
        } break;
        case STREAM_ADD: {
          boost::rational<int> delta1 = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> delta2 = coreInstance.getDelta(t2.getStr_());
          if (delta1 == 0 || delta2 == 0) {
            bOnceAgain = true;
            continue;
          } else
            delta = deltaAdd(delta1, delta2);
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
          delta = deltaTimemove(delta1, 0);
        } break;
        case STREAM_AGSE: {
          // ->>> check need
          // core1@(5,3) ->
          // push_stream core0 -> deltaSrc
          // push_val 5 -> size_of_window
          // stream agse 3 -> step_of_window
          boost::rational<int> deltaSrc = coreInstance.getDelta(t1.getStr_());
          boost::rational<int> schemaSizeSrc = getQuery(t1.getStr_()).lSchema.size();
          boost::rational<int> step = abs(t2.get());
          boost::rational<int> windowSize = abs(op.get());
          assert(windowSize > 0);
          assert(step > 0);
          if (t2.get() < 0) {  // windowSize < 0
            delta = deltaSrc;
            delta /= schemaSizeSrc;
            delta *= windowSize;
            delta /= step;
          } else
            delta = (deltaSrc / schemaSizeSrc) * step;
        } break;
        default:
          SPDLOG_ERROR("Undefined token: command={}, var={}, txt={}", op.getStrCommandID(), op.get(), op.getStr_());
          throw std::out_of_range("Undefined token/command on list");
      }  // switch ( op.getCommandID() )
      assert(delta != -1);
      q.rInterval = delta;  // There is established delta value - return value
    }                       // BOOST_FOREACH ( query & q , coreInstance )
    if (!bOnceAgain)
      break;
    else
      coreInstance.sort();
  }  // while(true)
  coreInstance.sort();
  return std::string("OK");
}

std::string generateStreamName(std::string sName1, std::string sName2, command_id cmd) {
  std::string sOperation = GetStringcommand_id(cmd);
  if (sName2 == "")
    return sOperation + std::string("_") + sName1;
  else
    return sOperation + std::string("_") + sName2 + std::string("_") + sName1;
}

/* Goal of this procedure is to provide stream to canonical form
TODO: Stream_MAX,MIN,AVG...
*/
std::string simplifyLProgram() {
  coreInstance.sort();
  for (auto it = coreInstance.begin(); it != coreInstance.end(); ++it) {
    // Agse phase optimization
    token t0, t1, t2;
    for (auto it2 = (*it).lProgram.begin(); it2 != (*it).lProgram.end(); ++it2) {
      t0 = t1;
      t1 = t2;
      t2 = (*it2);
      if (t2.getStrCommandID() == "STREAM_AGSE" && t1.getStrCommandID() == "PUSH_VAL" && t0.getStrCommandID() == "PUSH_VAL") {
        token newVal(t2.getCommandID(), t1.getStr_(), t1.get());
        it2 = (*it).lProgram.erase(it2);
        --it2;
        it2 = (*it).lProgram.erase(it2);
        (*it).lProgram.insert(it2, newVal);
        it2 = (*it).lProgram.begin();
        break;
      }
    }
    // Otimization phase 2
    if ((*it).isReductionRequired()) {
      for (auto it2 = (*it).lProgram.begin(); it2 != (*it).lProgram.end(); ++it2) {
        if ((*it2).getStrCommandID() == "STREAM_TIMEMOVE" || (*it2).getStrCommandID() == "STREAM_SUBSTRACT") {
          query newQuery;
          std::string arg1;  //< Name of argument operation
          command_id cmd;
          std::string affectedField;
          {
            token newVal(*it2);
            newQuery.lProgram.push_front(newVal);
            cmd = (*it2).getCommandID();
            it2 = (*it).lProgram.erase(it2);
            --it2;
          }
          {
            token newVal(*it2);
            newQuery.lProgram.push_front(newVal);
            std::stringstream s;
            s << (*it2).getStr_();
            arg1 = std::string(s.str());
            affectedField = (*it2).getStr_();
            it2 = (*it).lProgram.erase(it2);
            --it2;
          }
          ++it2;
          std::list<token> lTempProgram;
          lTempProgram.push_back(token(PUSH_TSCAN));
          newQuery.lSchema.push_back(field("*", lTempProgram, rdb::BAD, "*"));
          newQuery.id = generateStreamName(arg1, "", cmd);
          (*it).lProgram.insert(it2, token(PUSH_STREAM, newQuery.id));
          coreInstance.push_back(newQuery);  // After this instruction it loses context
          it = coreInstance.begin();
          break;
        } else if ((*it2).getStrCommandID() != "PUSH_STREAM" && (*it2).getStrCommandID() != "PUSH_VAL") {
          query newQuery;
          std::string arg1,  //< Name of first operation argument
              arg2;          //< Name of second operation argument
          command_id cmd;
          {
            token newVal(*it2);
            newQuery.lProgram.push_front(newVal);
            cmd = (*it2).getCommandID();
            it2 = (*it).lProgram.erase(it2);
            --it2;
          }
          {
            token newVal(*it2);
            newQuery.lProgram.push_front(newVal);
            std::stringstream s;
            s << (*it2).getStr_();
            arg1 = std::string(s.str());
            it2 = (*it).lProgram.erase(it2);
            --it2;
          }
          {
            token newVal(*it2);
            newQuery.lProgram.push_front(newVal);
            std::stringstream s;
            s << (*it2).getStr_();
            arg2 = std::string(s.str());
            it2 = (*it).lProgram.erase(it2);
            --it2;
          }
          ++it2;
          std::list<token> lTempProgram;
          lTempProgram.push_back(token(PUSH_TSCAN));
          newQuery.lSchema.push_back(field("*", lTempProgram, rdb::BAD, "*"));
          newQuery.id = generateStreamName(arg1, arg2, cmd);
          (*it).lProgram.insert(it2, token(PUSH_STREAM, newQuery.id));
          coreInstance.push_back(newQuery);
          it = coreInstance.begin();
          break;
        }  // Endif
      }    // Endfor
    }      // Endif
  }        // Endfor
  return std::string("OK");
}

// Goal of this procedure is to unroll schema based of given command
std::list<field> combine(std::string sName1, std::string sName2, token cmd_token) {
  std::list<field> lRetVal;
  command_id cmd = cmd_token.getCommandID();
  // Merge of schemas for junction of hash type
  if (cmd == STREAM_HASH) {
    if (getQuery(sName1).lSchema.size() != getQuery(sName2).lSchema.size())
      throw std::invalid_argument("Hash operation needs same schemas on arguments stream");
    lRetVal = getQuery(sName1).lSchema;
  } else if (cmd == STREAM_DEHASH_DIV)
    lRetVal = getQuery(sName1).lSchema;
  else if (cmd == STREAM_DEHASH_MOD)
    lRetVal = getQuery(sName1).lSchema;
  else if (cmd == STREAM_ADD) {
    int i = 0;
    for (auto f : getQuery(sName1).lSchema) {
      field intf;
      intf.fieldName = "Field_" + boost::lexical_cast<std::string>(fieldCount++);
      intf.lProgram.push_front(token(PUSH_ID, sName1, boost::rational<int>(i++)));
      lRetVal.push_back(intf);
    }
    i = 0;
    for (auto f : getQuery(sName2).lSchema) {
      field intf;
      intf.fieldName = "Field_" + boost::lexical_cast<std::string>(fieldCount++);
      intf.lProgram.push_front(token(PUSH_ID, sName2, boost::rational<int>(i++)));
      lRetVal.push_back(intf);
    }
    return lRetVal;
  } else if (cmd == STREAM_SUBSTRACT)
    lRetVal = getQuery(sName1).lSchema;
  else if (cmd == STREAM_TIMEMOVE)
    lRetVal = getQuery(sName1).lSchema;
  else if (cmd == STREAM_AVG) {
    field intf;
    intf.fieldName = "avg";
    intf.fieldType = rdb::RATIONAL;
    intf.lProgram.push_front(token(PUSH_ID, sName1));
    lRetVal.push_back(intf);
    return lRetVal;
  } else if (cmd == STREAM_MIN) {
    field intf;
    intf.fieldName = "min";
    intf.fieldType = rdb::RATIONAL;
    intf.lProgram.push_front(token(PUSH_ID, sName1));
    lRetVal.push_back(intf);
    return lRetVal;
  } else if (cmd == STREAM_MAX) {
    field intf;
    intf.fieldName = "max";
    intf.fieldType = rdb::RATIONAL;
    intf.lProgram.push_front(token(PUSH_ID, sName1));
    lRetVal.push_back(intf);
    return lRetVal;
  } else if (cmd == STREAM_SUM) {
    field intf;
    intf.fieldName = "sum";
    intf.fieldType = rdb::RATIONAL;
    intf.lProgram.push_front(token(PUSH_ID, sName1));
    lRetVal.push_back(intf);
    return lRetVal;
  } else if (cmd == STREAM_AGSE) {
    // Unrolling schema for agse - discussion needed if we need do that this way
    std::list<field> schema;
    int windowSize = abs(rational_cast<int>(cmd_token.get()));
    // If winows is negative - reverted schema
    if (rational_cast<int>(cmd_token.get()) > 0) {
      for (int i = 0; i < windowSize; i++) {
        field intf;
        intf.fieldName = sName1 + "_" + lexical_cast<std::string>(i);
        intf.fieldType = rdb::INTEGER;  // TODO - need to inherit BYTE or
        // INTEGER from BYTEARRAY or INTARRAY
        schema.push_back(intf);
      }
    } else {
      for (int i = windowSize - 1; i >= 0; i--) {
        field intf;
        intf.fieldName = sName1 + "_" + lexical_cast<std::string>(i);
        intf.fieldName = rdb::INTEGER;  // TODO - need to inherit BYTE or
        // INTEGER from BYTEARRAY or INTARRAY
        schema.push_back(intf);
      }
    }
    lRetVal = schema;
  } else {
    SPDLOG_ERROR("Undefined: str:{} cmd:{}", cmd_token.getStr_(), cmd_token.getStrCommandID());
    // throw std::invalid_argument("Command stack hits undefined operation");
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
    f.lProgram.push_front(token(PUSH_ID2, s.str()));
  }
  return lRetVal;
}

// goal of this procedure is setup of all possible fields name and unroll *
// unfortunately algorithm if broken - because does not search backward but next
// by next and some * can be process which have arguments appear as two asterisk
// In such case unroll does not appear and algorithm gets shitin-shitout

std::string prepareFields() {
  coreInstance.tsort();
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
            for (auto s : getQuery(t.getStr_()).lSchema) {
              std::list<token> lTempProgram;
              lTempProgram.push_back(token(PUSH_ID, nameOfscanningTable, boost::rational<int>(filedPosition++)));
              std::string name = "Field_" + boost::lexical_cast<std::string>(fieldCount++);
              q.lSchema.push_back(field(name, lTempProgram, rdb::INTEGER, ""));
            }
            break;
          }
          if (q.lProgram.size() == 3 || q.lProgram.size() == 2) {
            auto [sName1, sName2, cmd]{GetArgs(q.lProgram)};
            q.lSchema = combine(sName1, sName2, cmd);
            break;
          }
        }
        it++;
      }
      for (auto eraseIt : eraseList) q.lSchema.erase(eraseIt);
    }
  }
  coreInstance.sort();
  return std::string("OK");
}

void replaceAll(std::string &str, const std::string &from, const std::string &to) {
  if (from.empty()) return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();  // In case 'to' contains 'from', like replacing
                               // 'x' with 'yx'
  }
}

/* If in query plan is PUSH_IDX it means that we need to duplicate [_] */
std::string replicateIDX() {
  for (auto &q : coreInstance) {  // for each query
    for (auto &f : q.lSchema) {   // for each field in query
      std::string schemaName = "";
      bool found(false);
      for (auto &t : f.lProgram) {  // for each token in query field
        if (t.getCommandID() == PUSH_IDX) {
          if (found) {
            if (q.lSchema.size() != 1) {
              // if _ exist then only one filed can be in query
              throw std::out_of_range("Only one _ attribute can be in query");
            }
          }
          found = true;
          schemaName = t.getStr_();
        }
      }
      if (found) {
        const int fieldSize = getQuery(schemaName).lSchema.size();
        assert(fieldSize > 0);
        std::list<field> oldFieldSet = q.lSchema;
        assert(oldFieldSet.size() == 1);
        field oldField = *q.lSchema.begin();
        q.lSchema.clear();
        for (int i = 0; i < fieldSize; i++) {
          std::list<token> lTempProgram;
          for (auto &t : oldField.lProgram) {
            if (t.getCommandID() == PUSH_IDX)
              lTempProgram.push_back(token(PUSH_ID, t.getStr_(), boost::rational<int>(i)));
            else
              lTempProgram.push_back(token(t.getCommandID(), t.getStr_(), t.get()));
          }
          std::string toReplace = oldField.getFieldText();
          replaceAll(toReplace, "_", lexical_cast<std::string>(i));
          auto newFieldType = oldField.fieldType;
          field newField(schemaName + "_" + lexical_cast<std::string>(i), lTempProgram, newFieldType, toReplace);
          q.lSchema.push_back(newField);
        }
        assert(q.lSchema.size() == getQuery(schemaName).lSchema.size());
        break;
      }
    }
  }
  return std::string("OK");
}

/* Purpose of this function is to translate all references to fields
to form schema_name[postion, time_offset]
Command method of presentation aims simple data processing
Aim of this procedure is change all of push_idXXX to push_id
note that push_id is closest to push_id4
push_idXXX is searched in all stream program after reduction
*/
std::string convertReferences() {
  boost::regex xprFieldId5("(\\w*)\\[(\\d*)\\]\\[(\\d*)\\]");  // something[1][1]
  boost::regex xprFieldId4("(\\w*)\\[(\\d*)\\,(\\d*)\\]");     // something[1,1]
  boost::regex xprFieldId2("(\\w*)\\[(\\d*)\\]");              // something[1]
  boost::regex xprFieldIdX("(\\w*)\\[_]");                     // something[_]
  boost::regex xprFieldId1("(\\w*).(\\w*)");                   // something.in_schema
  boost::regex xprFieldId3("(\\w*)");                          // field_of_corn
  for (auto &q : coreInstance) {                               // for each query
    assert(!q.isReductionRequired());
    for (auto &f : q.lSchema) {     // for each field in query
      for (auto &t : f.lProgram) {  // for each token in query field
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
                    if (f1.fieldName == field) {
                      t = token(PUSH_ID, schema, boost::rational<int>(offset1));
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
              t = token(PUSH_IDX, schema, boost::rational<int>(0));
            } else
              throw std::out_of_range("No mach on type conversion IDX");
            break;
          case PUSH_ID2:
            if (regex_search(text.c_str(), what, xprFieldId2)) {
              assert(what.size() == 3);
              const std::string schema(what[1]);
              const std::string sOffset1(what[2]);
              const int offset1(atoi(sOffset1.c_str()));
              t = token(PUSH_ID, schema, boost::rational<int>(offset1));
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
              pQ1 = &getQuery(schema1);
              if (q.lProgram.size() == 3) pQ2 = &getQuery(schema2);
              bool bFieldFound(false);
              int offset1(0);
              if (pQ1 != nullptr) {
                offset1 = 0;
                for (auto &f1 : (*pQ1).lSchema) {
                  if (f1.fieldName == field) {
                    t = token(PUSH_ID, schema1, boost::rational<int>(offset1));
                    bFieldFound = true;
                  }
                  ++offset1;
                }
              }
              if (pQ2 != nullptr && !bFieldFound) {
                offset1 = 0;
                for (auto &f2 : (*pQ2).lSchema) {
                  if (f2.fieldName == field) {
                    t = token(PUSH_ID, schema2, boost::rational<int>(offset1));
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
          case PUSH_ID5:
            if (regex_search(text.c_str(), what, xprFieldId4) || regex_search(text.c_str(), what, xprFieldId5)) {
              assert(what.size() == 4);
              const std::string schema(what[1]);
              const std::string sOffset1(what[2]);
              const std::string sOffset2(what[3]);
              const int offset1(atoi(sOffset1.c_str()));
              const int offset2(atoi(sOffset2.c_str()));
              bool foundSchema(false);
              for (auto &q : coreInstance) {
                if (q.id == schema) {
                  foundSchema = true;
                  break;
                }
              }
              if (!foundSchema) throw std::logic_error("Field calls non-exist schema - config.log (-g)");
              t = token(PUSH_ID, schema, boost::rational<int>(offset1 + offset2 * q.lSchema.size()));
            } else
              throw std::out_of_range("No mach on type conversion ID4");
            break;
        }
      }
    }
  }
  return std::string("OK");
}

std::string dumpInstance(std::string sOutFile) {
  // dumped object must be const
  const qTree coreInstanceCopy(coreInstance);
  std::stringstream retval;
  if (sOutFile == "") {
    boost::archive::text_oarchive oa(retval);
    oa << coreInstanceCopy;
  } else {
    std::ofstream ofs(sOutFile.c_str());
    boost::archive::text_oarchive oa(ofs);
    oa << coreInstanceCopy;
  }
  return retval.str();
}
