#include "QStruct.h"

#include <rdb/convertTypes.h>
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

bool isThere(const std::vector<query> &v, const std::string query_name) {
  for (auto &q : v) {
    if (q.id == "") continue;
    if (q.id == query_name) return true;
  }
  return false;
}

void qTree::dfs(std::string v) {
  visited[v] = true;
  for (auto u : adj[v]) {
    if (!visited[u]) dfs(u);
  }
  ans.push_back(v);
}

// https://en.wikipedia.org/wiki/Topological_sorting
//
void qTree::tsort() {
  // https://cp-algorithms.com/graph/topological-sort.html#implementation

  ans.clear();
  for (auto q : coreInstance) visited[q.id] = false;
  for (auto q : coreInstance) adj[q.id] = q.getDepStream();
  for (auto q : coreInstance)
    if (!visited[q.id]) dfs(q.id);

  // reverse(ans.begin(), ans.end());

  qTree tempInstance;
  for (auto qname : ans) tempInstance.push_back(coreInstance[qname]);
  coreInstance.clear();
  coreInstance = tempInstance;
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

// boost::rational<int> token::getRI() { return numericValue; }

cast<rdb::descFldVT> castRI;

boost::rational<int> token::getRI() {
  auto ret = castRI(valueVT, rdb::RATIONAL);
  return std::get<boost::rational<int>>(ret);
}

rdb::descFldVT token::getVT() { return valueVT; };

std::string token::getStr_() {
  if (valueVT.index() == rdb::STRING)
    return std::get<std::string>(valueVT);
  else if (valueVT.index() == rdb::IDXPAIR) {
    auto r = std::get<std::pair<std::string, int>>(valueVT);
    return r.first;
  } else
    return "Error";
}

bool query::isDeclaration() { return lProgram.empty(); }

bool query::isGenerated() { return !id.compare(0, 7, "STREAM_"); }

/** Construktor set */

field::field() {}

field::field(std::string sFieldName,     //
             std::list<token> lProgram,  //
             rdb::descFld dFieldType,    //
             int fieldLen)
    : lProgram(lProgram), fieldType(dFieldType), fieldName(sFieldName), fieldLen(fieldLen) {}

token field::getFirstFieldToken() {
  assert(lProgram.size() > 0);  // If this fails that means in field no program (decl!)
  return *lProgram.begin();
}

/** Construktor set */

token::token(command_id id, rdb::descFldVT value)
    :  //
      command(id),
      valueVT(value) {}

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
      auto r = std::get<std::pair<int, int>>(rhs.valueVT);
      os << r.first << "," << r.second;
    } break;
    case rdb::IDXPAIR: {
      auto r = std::get<std::pair<std::string, int>>(rhs.valueVT);
      os << r.first << "[" << r.second << "]";
    } break;
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
      case STREAM_SUBTRACT:
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

std::vector<std::string> query::getDepStream() {
  std::vector<std::string> lRetVal;
  for (auto &t : lProgram)
    if (t.getCommandID() == PUSH_STREAM) lRetVal.push_back(std::get<std::string>(t.getVT()));
  return lRetVal;
}

rdb::Descriptor query::descriptorStorage() {
  rdb::Descriptor retVal{};
  for (auto &f : lSchema) retVal | rdb::Descriptor(f.fieldName, f.fieldLen, f.fieldType);

  if (isDeclaration()) {
    retVal | rdb::Descriptor(filename, 0, rdb::REF);

    auto filenameShdw{filename};
    std::transform(filenameShdw.begin(), filenameShdw.end(), filenameShdw.begin(), ::tolower);
    if (filenameShdw.find(".txt") != std::string::npos)
      retVal | rdb::Descriptor("TEXTSOURCE", 0, rdb::TYPE);
    else
      retVal | rdb::Descriptor("DEVICE", 0, rdb::TYPE);
  }
  return retVal;
}

// TODO: remove Descriptor(a,b) and use Descriptor(a,b,c) here - strings are broken if not fix
rdb::Descriptor query::descriptorFrom() {
  SPDLOG_INFO("call query::descriptorFrom()");
  rdb::Descriptor retVal{};
  if (isDeclaration()) {
    retVal | descriptorStorage();
    return retVal;
  }
  auto [arg1, arg2, cmd]{GetArgs(lProgram)};
  auto i{0};
  switch (cmd.getCommandID()) {
    case STREAM_AVG:
    case STREAM_MAX:
    case STREAM_MIN:
    case STREAM_SUM: {
      auto [maxType, maxLen] = getQuery(arg1).descriptorStorage().getMaxType();
      retVal | rdb::Descriptor(id + "_0", maxLen, maxType);
    } break;
    case STREAM_HASH: {
      retVal.createHash(id, getQuery(arg1).descriptorStorage(), getQuery(arg2).descriptorStorage());
      retVal.cleanRef();
    } break;
    case STREAM_DEHASH_DIV:
    case STREAM_DEHASH_MOD:
    case STREAM_SUBTRACT:
    case STREAM_TIMEMOVE: {
      for (auto &f : getQuery(arg1).lSchema) {
        if (f.fieldLen == 0) continue;
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), f.fieldLen, f.fieldType);
      };
    } break;
    case PUSH_STREAM: {
      for (auto &f : getQuery(cmd.getStr_()).lSchema) {
        if (f.fieldLen == 0) continue;
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), f.fieldLen, f.fieldType);
      };
    } break;
    case STREAM_ADD: {
      for (auto &f : getQuery(arg1).lSchema) {
        if (f.fieldLen == 0) continue;
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), f.fieldLen, f.fieldType);
      };
      for (auto &f : getQuery(arg2).lSchema) {
        if (f.fieldLen == 0) continue;
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), f.fieldLen, f.fieldType);
      };
    } break;
    case STREAM_AGSE: {
      // * INFO - sync with dataModel.cpp

      // 	:- PUSH_STREAM core -> delta_source (arg[0]) - operation
      //  :- STREAM_AGSE 2,3 -> window_length, window_step (arg[1])

      auto [step, length] = std::get<std::pair<int, int>>(cmd.getVT());
      assert(step >= 0);
      auto [maxType, maxLen] = getQuery(arg1).descriptorStorage().getMaxType();
      for (int i = 0; i < abs(length); i++) {
        retVal | rdb::Descriptor(id + "_" + std::to_string(i++), maxLen, maxType);
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
  return std::make_tuple(sArg1, sArg2, cmd);
}

std::ostream &operator<<(std::ostream &os, rdb::descFld &s) { return os << rdb::GetStringdescFld(s); }

std::ostream &operator<<(std::ostream &os, const field &s) {
  os << "FLD/";
  os << "name:" << s.fieldName << ",";
  os << "type:" << s.fieldType << ",";
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
