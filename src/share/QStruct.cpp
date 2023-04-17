#include "QStruct.h"

#include <spdlog/spdlog.h>

#include <boost/core/enable_if.hpp>  // for enable_if_c<>::type
#include <boost/function.hpp>        // IWYU pragma: keep
#include <boost/lambda/bind.hpp>     // IWYU pragma: keep
#include <boost/lambda/lambda.hpp>   // IWYU pragma: keep
#include <cassert>                   // for assert
#include <cctype>                    // tolower
#include <ext/alloc_traits.h>        // for __alloc_traits<>::value_type
#include <iostream>                  // for cerr
#include <sstream>                   // for operator<<, basic_ostream, endl
#include <stack>                     // for stack
#include <stdexcept>                 // for logic_error
#include <type_traits>

using namespace boost;
using namespace boost::lambda;

#define FLDTYPE_H_CREATE_DEFINITION_FLDT
#include "fldType.h"
#define CMDID_H_CREATE_DEFINITION_CMDI
#include "cmdID.h"
#include "convertTypes.h"

extern "C" {
qTree coreInstance;
}

static_assert(std::is_copy_constructible_v<rdb::descFldVT> == true);

// int _floor( boost::rational<int> & value ) {
//    return static_cast<int> (
//            floor ( rational_cast<double>( value ) )
//        ) ;
//}

bool operator<(const query &lhs, const query &rhs) { return lhs.rInterval < rhs.rInterval; }

//{
//  //
//  //        |  b[n-floor(n*z)] dla floor(n*z)=floor((n+1)z)
//  // c[n] = <
//  //        |  a[floor(n*z)] dla floor(n*z)<>floor((n+1)z)
//  //
//  // z = delta_b / ( delta_a + delta_b )
//  //
//  // delta_c = ( delta_a * delta_b ) / ( delta_a + delta_b )
//  //

//  // Test required
//  // result.delta = delta ;

//  R zet = inB.getDelta() / ( inA.getDelta() + inB.getDelta() ) ;

//  int n = result.position ;

//  if ( _floor( n * zet ) == _floor( ( n + 1 ) * zet ) )
//  {
//      result.position = n - _floor( n * zet ) ;
//      result.tupleRef = inB.tupleRef ;
//  }
//  else
//  {
//      result.position = _floor( n * zet );
//      result.tupleRef = inA.tupleRef ;
//  }
//}

command_id token::getCommandID() { return command; }

std::string token::getStrCommandID() { return GetStringcommand_id(command); }

field &query::getField(const std::string &sField) {
  for (auto &f : lSchema) {
    if (f.fieldName == sField) return f;
  }
  throw std::logic_error("No such field in schema");
  return *new field();
}

void query::reset() {
  id.clear();
  filename.clear();
  rInterval = 0;
  lSchema.clear();
  lProgram.clear();
  return;
}

bool isThere(std::vector<query> v, const std::string &query_name) {
  for (auto &q : v) {
    if (q.id == "") continue;
    if (q.id == query_name) return true;
  }
  return false;
}

// https://en.wikipedia.org/wiki/Topological_sorting
//
void qTree::tsort() {
  vector<query> v = *this;
  vector<query> des;
  while (!v.empty())
    for (auto it = v.begin(); it != v.end(); ++it) {
      if (v.empty()) break;
      bool fullDependent(true);
      for (auto s : (*it).getDepStreamName()) {
        if (!isThere(des, s)) fullDependent = false;
      }
      if (fullDependent) {
        des.push_back(*it);
        v.erase(it);
        it = v.begin();
      }
    }
  assert(des.size() == coreInstance.size());
  clear();
  for (auto &q : des) push_back(q);
}

boost::rational<int> qTree::getDelta(const std::string &query_name) { return getQuery(query_name).rInterval; }

query &getQuery(const std::string &query_name) {
  assert(query_name != "");
  for (auto &q : coreInstance) {
    if (q.id == query_name) return q;
  }
  SPDLOG_ERROR("Missing - {}", query_name);
  throw std::logic_error("No such stream in set - getQuery");
  static query void_query;
  return (void_query);  // proforma
}

int getSeqNr(const std::string &query_name) {
  int cnt(0);
  for (auto &q : coreInstance) {
    if (query_name == q.id)
      return cnt;
    else
      cnt++;
  }
  SPDLOG_ERROR("No such stream in set - {}", query_name);
  throw std::logic_error("No such stream in set - getSeqNr");
  return -1;  // INVALID QUERY_NR
}

bool isDeclared(const std::string &query_name) {
  for (auto &q : coreInstance) {
    if (query_name == q.id) return q.isDeclaration();
  }
  return false;
}

bool isExist(const std::string &query_name) {
  for (auto &q : coreInstance) {
    if (query_name == q.id) return true;
  }
  return false;
}

boost::rational<int> token::getRI() { return numericValue; }
/*
cast<rdb::descFldVT> castRI;

boost::rational<int> token::getRI() {
  auto ret = castRI(valueVT, rdb::RATIONAL);
  return std::get<boost::rational<int>>(ret);
}
*/

rdb::descFldVT token::getVT() { return valueVT; };

std::string token::getStr_() { return textValue; }

bool query::isDeclaration() { return lProgram.empty(); }

bool query::isGenerated() { return !id.compare(0, 7, "STREAM_"); }

/** Construktor set */

field::field() {}

field::field(std::string sFieldName,     //
             std::list<token> lProgram,  //
             rdb::descFld dFieldType,    //
             std::string fieldText)
    : lProgram(lProgram), fieldText(fieldText), fieldType(dFieldType), fieldName(sFieldName) {}

std::string field::getFieldText() { return fieldText; }

token field::getFirstFieldToken() {
  assert(lProgram.size() > 0);  // If this fails that means in field no program (decl!)
  return *lProgram.begin();
}

/** Construktor set */

token::token(command_id id, rdb::descFldVT value, std::string desc)
    :  //
      command(id),
      valueVT(value),
      textValue(desc) {
  switch (value.index()) {
    case rdb::STRING:
      if (desc == "") textValue = std::get<std::string>(value);
      break;
    case rdb::FLOAT:
      numericValue = Rationalize(std::get<float>(value));
      break;
    case rdb::DOUBLE:
      numericValue = Rationalize(std::get<double>(value));
      break;
    case rdb::INTEGER:
      numericValue = boost::rational<int>(std::get<int>(value), 1);
      break;
    case rdb::UINT:
      numericValue = boost::rational<int>(std::get<unsigned>(value), 1);
      break;
    case rdb::BYTE:
      numericValue = boost::rational<int>(std::get<uint8_t>(value), 1);
      break;
    case rdb::RATIONAL:
      numericValue = std::get<number>(value);
      break;
    default:
      numericValue = boost::rational<int>(-999, 1);  // Unidentified value
  }
  if (textValue == "") {
    std::stringstream ss;
    ss << numericValue.numerator();
    if (numericValue.denominator() != 1) {
      ss << "_";
      ss << numericValue.denominator();
    }
    textValue = std::string(ss.str());
  }
}

std::ostream &operator<<(std::ostream &os, const token &rhs) {
  os << GetStringcommand_id(rhs.command) << "(";
  switch (rhs.valueVT.index()) {
    case rdb::STRING:
      os << std::get<std::string>(rhs.valueVT);
      break;
    case rdb::FLOAT:
      os << std::get<float>(rhs.valueVT);
      break;
    case rdb::DOUBLE:
      os << std::get<double>(rhs.valueVT);
      break;
    case rdb::INTEGER:
      os << std::get<int>(rhs.valueVT);
      break;
    case rdb::UINT:
      os << std::get<unsigned>(rhs.valueVT);
      break;
    case rdb::BYTE:
      os << std::get<uint8_t>(rhs.valueVT);
      break;
    case rdb::RATIONAL:
      os << std::get<number>(rhs.valueVT);
      break;
    case rdb::INTPAIR: {
      auto r = std::get<std::pair<int,int>>(rhs.valueVT);
      os << r.first << "," << r.second;
      }
      break;
    default:
      os << "not supported";
  }
  os << ")";
  return os;
}

/** Construktor set */

query::query(boost::rational<int> rInterval, const std::string &id) : rInterval(rInterval), id(id) {}

query::query() {}

int query::getFieldIndex(field f_arg) {
  int idx(0);
  for (auto f : lSchema) {
    if (f.fieldName == f_arg.fieldName)  // Todo
      return idx;
    idx++;
  }
  SPDLOG_ERROR("Field not found in query - {}", f_arg.fieldName);
  throw std::logic_error("Field not found in query");
  return -1;  // not found
}

bool query::isReductionRequired() {
  int streamOperatorCount(0);
  for (auto &t : lProgram) {
    switch (t.getCommandID()) {
      case STREAM_HASH:
      case STREAM_DEHASH_DIV:
      case STREAM_DEHASH_MOD:
      case STREAM_ADD:
      case STREAM_SUBSTRACT:
      case STREAM_TIMEMOVE:
      case STREAM_AGSE:
        ++streamOperatorCount;
      default:;
    }
  }
  return streamOperatorCount > 1;
}

bool query::is(command_id command) {
  for (auto &t : lProgram) {
    if (t.getCommandID() == command) return true;
  }
  return false;
}

std::vector<std::string> query::getDepStreamName(int reqDep) {
  std::vector<std::string> lRetVal;
  for (auto &t : lProgram) {
    if (reqDep == 0) {
      // defult behaviour
      if (t.getCommandID() == PUSH_STREAM) lRetVal.push_back(t.getStr_());
    } else
      ++reqDep;
  }
  return lRetVal;
}

rdb::Descriptor query::descriptorExpression() {
  rdb::Descriptor retVal{};
  for (auto &f : lSchema) {
    bool isTableType = (f.fieldType == rdb::STRING) ||     //
                       (f.fieldType == rdb::BYTEARRAY) ||  //
                       (f.fieldType == rdb::INTARRAY);
    if (isTableType)
      assert(false && "TODO: Add support in QStruct");  // retVal | rdb::Descriptor(f.fieldName, f.XXXX , f.fieldType);
    else
      retVal | rdb::Descriptor(f.fieldName, f.fieldType);
  }
  return retVal;
}

rdb::Descriptor query::descriptorFrom() {
  SPDLOG_INFO("call query::descriptorFrom()");
  rdb::Descriptor retVal{};
  if (isDeclaration()) {
    retVal | descriptorExpression();
    retVal | rdb::Descriptor(filename, rdb::REF);

    auto filenameShdw{filename};
    std::transform(filenameShdw.begin(), filenameShdw.end(), filenameShdw.begin(), ::tolower);
    if (filenameShdw.find(".txt") != std::string::npos)
      retVal | rdb::Descriptor("TEXTSOURCE", rdb::TYPE);
    else
      retVal | rdb::Descriptor("DEVICE", rdb::TYPE);
    return retVal;
  }
  auto [arg1, arg2, cmd]{GetArgs(lProgram)};
  auto i{0};
  switch (cmd.getCommandID()) {
    case STREAM_HASH:
    case STREAM_DEHASH_DIV:
    case STREAM_DEHASH_MOD:
    case STREAM_SUBSTRACT:
    case STREAM_TIMEMOVE: {
      for (auto &f : getQuery(arg1).lSchema) {
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), f.fieldType);
      };
    } break;
    case PUSH_STREAM: {
      for (auto &f : getQuery(cmd.getStr_()).lSchema) {
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), f.fieldType);
      };
    } break;
    case STREAM_ADD: {
      for (auto &f : getQuery(arg1).lSchema) {
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), f.fieldType);
      };
      for (auto &f : getQuery(arg2).lSchema) {
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), f.fieldType);
      };
    } break;
    case STREAM_AGSE: {
      assert(cmd.getCommandID() == STREAM_AGSE);
      assert(cmd.getVT().index() == rdb::INTPAIR);
      auto r = std::get<std::pair<int, int>>(cmd.getVT());
      int windowSize = abs(r.second);
      auto firstFieldType = getQuery(arg1).lSchema.front().fieldType;
      for (int i = 0; i < windowSize; i++) {
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), firstFieldType);
      }
    } break;
    default:
      SPDLOG_ERROR("Undefined cmd: {} str:{}", cmd.getStrCommandID(), cmd.getStr_());
      assert(false);
  }
  return retVal;
}

std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog) {
  auto eIt = prog.begin();
  std::string sArg1;
  std::string sArg2;
  assert(prog.size() < 4);
  if (prog.size() == 1) sArg1 = (*eIt).getStr_();   // 1
  if (prog.size() > 1) sArg1 = (*eIt++).getStr_();  // 2,3
  if (prog.size() > 2) sArg2 = (*eIt++).getStr_();  // 3

  token cmd(*eIt);
  if ( cmd.getCommandID() == STREAM_AGSE) {
    assert(cmd.getVT().index() == rdb::INTPAIR);
  }
  return std::make_tuple(sArg1, sArg2, cmd);
}

std::ostream &operator<<(std::ostream &os, rdb::descFld &s) { return os << rdb::GetStringdescFld(s); }

std::ostream &operator<<(std::ostream &os, const field &s) {
  os << "FLD/";
  os << "name:" << s.fieldName << ",";
  os << "type:" << s.fieldType << ",";
  os << "text:" << s.fieldText << ",";
  os << "prog:";
  for (auto &i : s.lProgram) os << i << ",";
  return os;
}

std::ostream &operator<<(std::ostream &os, const query &s) {
  os << "QRY/";
  os << "id:" << s.id << ",";
  os << "filename:" << s.filename << ",";
  os << "rInterval:" << s.rInterval << ",";
  os << "lSchema:";
  for (auto &i : s.lSchema) os << i << ",";
  os << "lProgram:";
  for (auto &i : s.lProgram) os << i << ",";
  return os;
}
