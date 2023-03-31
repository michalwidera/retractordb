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
    if (q.id == query_name) return true;
  }
  return false;
}

// https://en.wikipedia.org/wiki/Topological_sorting
//
// NOTE: Technical debt!! - if in compiled query set
// there is missing declaration of one stream this tsort
// function will go into infinite loop
// This is bug that need to be resolved
void qTree::tsort() {
  vector<query> v = *this;
  vector<query> des;
  // this function is broken - this watchdog prevents hang
  int watchdog = 100;
  while (!v.empty())
    for (auto it = v.begin(); it != v.end(); ++it) {
      watchdog--;
      if (watchdog == 0) {
        std::cerr << "Tsort failure." << std::endl;
        abort();
      }
      if (v.empty()) break;
      std::vector<std::string> ls = (*it).getDepStreamName();
      bool fullDependent(true);
      for (auto s : ls) {
        if (!isThere(des, s)) fullDependent = false;
      }
      if (fullDependent) {
        des.push_back(*it);
        v.erase(it);
        it = v.begin();
      }
    }
  erase(begin(), end());
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

boost::rational<int> token::get() { return numericValue; }

rdb::descFldVT token::getVT() { return valueVT; };

std::string token::getStr_() { return textValue; }

bool query::isDeclaration() { return lProgram.empty(); }

bool query::isGenerated() { return !id.compare(0, 7, "STREAM_"); }

/** Construktor set */

field::field() {}

field::field(std::string sFieldName, std::list<token> &lProgram, rdb::descFld dFieldType, std::string sFieldText)
    : lProgram(lProgram), sFieldText(sFieldText), fieldType(dFieldType), fieldName(sFieldName) {}

std::string field::getFieldText() { return sFieldText; }

token field::getFirstFieldToken() {
  assert(lProgram.size() > 0);  // If this fails that means in field no program (decl!)
  return *lProgram.begin();
}

/** Construktor set */
template <typename T>
token::token(command_id id, const std::string &sValue, T value) : command(id), textValue(sValue) {
  if constexpr (std::is_same_v<T, number>) {
    numericValue = value;
    valueVT = value;
  } else if constexpr (std::is_same_v<T, int>) {
    numericValue = boost::rational<int>(value, 1);
    valueVT = value;
  } else if constexpr (std::is_same_v<T, double>) {
    numericValue = Rationalize(value);
    valueVT = value;
  } else if constexpr (std::is_same_v<T, float>) {
    numericValue = Rationalize(value);
    valueVT = value;
  } else {
    numericValue = boost::rational<int>(-999, 1);  // Unidentified value
  }
  if (sValue == "") {
    std::stringstream ss;
    ss << numericValue.numerator();
    if (numericValue.denominator() != 1) {
      ss << "_";
      ss << numericValue.denominator();
    }
    textValue = std::string(ss.str());
  }
}

template token::token(command_id id, const std::string &sValue, number);
template token::token(command_id id, const std::string &sValue, int);
template token::token(command_id id, const std::string &sValue, double);
template token::token(command_id id, const std::string &sValue, float);

token::token(command_id id, rdb::descFldVT value, const std::string sValue)
    :  //
      command(id),
      textValue(sValue),
      valueVT(value) {}
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
      int windowSize = abs(rational_cast<int>(cmd.get()));
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
  token cmd = (*eIt++);
  return std::make_tuple(sArg1, sArg2, cmd);
}

std::ostream &operator<<(std::ostream &os, rdb::descFld s) { return os << rdb::GetStringdescFld(s); }
