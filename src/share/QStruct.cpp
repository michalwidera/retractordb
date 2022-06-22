#include "QStruct.h"

#include <spdlog/spdlog.h>

#include <boost/core/enable_if.hpp>  // for enable_if_c<>::type
#include <boost/function.hpp>        // IWYU pragma: keep
#include <boost/lambda/bind.hpp>     // IWYU pragma: keep
#include <boost/lambda/lambda.hpp>   // IWYU pragma: keep
#include <cassert>                   // for assert
#include <ext/alloc_traits.h>        // for __alloc_traits<>::value_type
#include <iostream>                  // for cerr
#include <sstream>                   // for operator<<, basic_ostream, endl
#include <stack>                     // for stack
#include <stdexcept>                 // for logic_error

using namespace boost;
using namespace boost::lambda;

#define ENUMDECL_H_CREATE_DEFINITION
#include "enumDecl.h"

extern "C" {
qTree coreInstance;
}

// int _floor( boost::rational<int> & value ) {
//    return static_cast<int> (
//            floor ( rational_cast<double>( value ) )
//        ) ;
//}

bool operator<(const query &lhs, const query &rhs) {
  return lhs.rInterval < rhs.rInterval;
}

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

boost::rational<int> Rationalize(double inValue, double DIFF /*=1E-6*/,
                                 int ttl /*=11*/) {
  std::stack<int> st;
  double startx(inValue), diff, err1, err2;
  unsigned int val;
  for (;;) {
    val = static_cast<unsigned int>(startx);
    st.push(static_cast<int>(val));
    if ((ttl--) == 0) break;
    diff = startx - val;
    if (diff < DIFF)
      break;
    else
      startx = 1 / diff;
    if (startx > (1 / DIFF)) break;
  }
  if (st.empty()) return boost::rational<int>(0, 1);
  boost::rational<int> result1(0, 1), result2(0, 1);
  while (!st.empty()) {
    if (result1.numerator() != 0)
      result2 = st.top() + (1 / result1);
    else
      result2 = st.top();
    st.pop();
    result1 = result2;
  }
  err1 = abs(rational_cast<double>(result1) - inValue);
  err2 = abs(rational_cast<double>(result2) - inValue);
  return err1 > err2 ? result2 : result1;
}

field &query::getField(std::string sField) {
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

bool isThere(std::vector<query> v, std::string query_name) {
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

boost::rational<int> qTree::getDelta(std::string query_name) {
  return getQuery(query_name).rInterval;
}

query &getQuery(std::string query_name) {
  assert(query_name != "");
  for (auto &q : coreInstance) {
    if (q.id == query_name) return q;
  }
  SPDLOG_ERROR("Missing - {}", query_name);
  throw std::logic_error("No such stream in set - getQuery");
  static query void_query;
  return (void_query);  // proforma
}

int getSeqNr(std::string query_name) {
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

bool isDeclared(std::string query_name) {
  for (auto &q : coreInstance) {
    if (query_name == q.id) return q.isDeclaration();
  }
  return false;
}

bool isExist(std::string query_name) {
  for (auto &q : coreInstance) {
    if (query_name == q.id) return true;
  }
  return false;
}

boost::rational<int> token::get() { return numericValue; }

std::string token::getStr() { return textValue; }

bool query::isDeclaration() { return lProgram.empty(); }

bool query::isGenerated() { return !id.compare(0, 7, "STREAM_"); }

/** Construktor set */

field::field() {}

field::field(std::string sFieldName, std::list<token> &lProgram,
             rdb::eType dFieldType, std::string sFieldText)
    : lProgram(lProgram),
      sFieldText(sFieldText),
      fieldType(dFieldType),
      fieldName(sFieldName) {}

std::string field::getFieldText() { return sFieldText; }

token field::getFirstFieldToken() {
  assert(lProgram.size() >
         0);  // If this fails that means in field no program (decl!)
  return *lProgram.begin();
}

/** Construktor set */

token::token(command_id id, std::string sValue, number value)
    : command(id), textValue(sValue) {
  boost::rational<int> *pValRI = std::get_if<boost::rational<int>>(&value);
  if (pValRI)
    numericValue = *pValRI;
  else {
    int *pValI = std::get_if<int>(&value);
    if (pValI)
      numericValue = boost::rational<int>(*pValI, 1);
    else {
      double *pValD = std::get_if<double>(&value);
      if (pValD)
        numericValue = Rationalize(*pValD);
      else
        numericValue = boost::rational<int>(-999, 1);  // Unidentified value
    }
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

/** Construktor set */

query::query(boost::rational<int> rInterval, std::string id)
    : rInterval(rInterval), id(id) {}

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
      if (t.getCommandID() == PUSH_STREAM) lRetVal.push_back(t.getStr());
    } else
      ++reqDep;
  }
  return lRetVal;
}

std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog) {
  auto eIt = prog.begin();
  std::string sArg1;
  std::string sArg2;

  assert(prog.size() < 4);

  if (prog.size() == 1) sArg1 = (*eIt).getStr();   // 1
  if (prog.size() > 1) sArg1 = (*eIt++).getStr();  // 2,3
  if (prog.size() > 2) sArg2 = (*eIt++).getStr();  // 3

  token cmd = (*eIt++);
  return std::make_tuple(sArg1, sArg2, cmd);
}

std::ostream &operator<<(std::ostream &os, rdb::eType s) {
  return os << rdb::GetStringeType(s);
}
