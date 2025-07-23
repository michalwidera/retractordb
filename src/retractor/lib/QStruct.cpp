#include "QStruct.h"

#include <rdb/convertTypes.h>
#include <spdlog/spdlog.h>

#include <cassert>             // for assert
#include <cctype>              // tolower
#include <ext/alloc_traits.h>  // for __alloc_traits<>::value_type
#include <iostream>            // for cerr
#include <sstream>             // for operator<<, basic_ostream, endl
#include <stack>               // for stack
#include <stdexcept>           // for logic_error
#include <type_traits>

using namespace boost;

#define FLDTYPE_H_CREATE_DEFINITION_FLDT
#include "fldType.h"
#define CMDID_H_CREATE_DEFINITION_CMDI
#include "cmdID.h"

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

void query::reset() {
  id.clear();
  filename.clear();
  rInterval = 0;
  lSchema.clear();
  lProgram.clear();
  return;
}

bool isThere(const std::vector<query> &v, const std::string &query_name) {
  for (const auto &q : v) {
    if (q.id == "") continue;
    if (q.id == query_name) return true;
  }
  return false;
}

void qTree::dfs(const std::string &v) {
  visited[v] = true;
  for (auto u : adj[v]) {
    if (!visited[u]) dfs(u);
  }
  ans.push_back(v);
}

// https://en.wikipedia.org/wiki/Topological_sorting
//
void qTree::topologicalSort() {
  // https://cp-algorithms.com/graph/topological-sort.html#implementation

  qTree &coreInstance{*this};

  ans.clear();
  for (auto q : coreInstance) visited[q.id] = false;
  for (auto q : coreInstance) adj[q.id] = q.getDepStream();
  for (auto q : coreInstance)
    if (!visited[q.id]) dfs(q.id);

  qTree tempInstance;
  // for (auto qname : ans) tempInstance.push_back(coreInstance[qname]); -> same:
  std::for_each(ans.begin(), ans.end(),
                [&tempInstance, &coreInstance](const std::string &qname)  //
                { tempInstance.push_back(coreInstance[qname]); });
  coreInstance = tempInstance;
}

boost::rational<int> qTree::getDelta(const std::string &query_name) { return getQuery(query_name).rInterval; }

void qTree::dumpCore() {
  std::vector<std::string> vcols = {"Idx", "Delta", "Cap", "Name"};
  std::stringstream ss;
  std::stringstream sp;

  for (const auto nName : vcols) {
    int maxSize = nName.length();
    int size{0};
    for (const auto &it : *this) {
      if (nName == vcols[0])
        size = std::to_string(getSeqNr(it.id)).length();
      else if (nName == vcols[1])
        size = (std::to_string(it.rInterval.numerator()) + "/" + std::to_string(it.rInterval.denominator())).length();
      else if (nName == vcols[2])
        size = std::to_string(maxCapacity[it.id]).length();
      else if (nName == vcols[3])
        size = it.id.length();
      else {
        assert(false && "wrong caption in dumpCore");
        abort();
      }
      if (maxSize < size) maxSize = size;
    }
    ss << "|%";
    ss << maxSize;
    ss << "s";

    sp << "|";
    for (auto i = 0; i < maxSize; ++i) sp << "_";
  }
  ss << "|\n";
  sp << "|\n";

  printf(ss.str().c_str(), vcols[0].c_str(), vcols[1].c_str(), vcols[2].c_str(), vcols[3].c_str());
  printf(sp.str().c_str());

  for (const auto &it : *this)
    printf(ss.str().c_str(),                                                                                       //
           std::to_string(getSeqNr(it.id)).c_str(),                                                                //
           (std::to_string(it.rInterval.numerator()) + "/" + std::to_string(it.rInterval.denominator())).c_str(),  //
           std::to_string(maxCapacity[it.id]).c_str(),                                                             //
           it.id.c_str());                                                                                         //
}

std::set<boost::rational<int>> qTree::getAvailableTimeIntervals() {
  std::set<boost::rational<int>> lstTimeIntervals;
  for (const auto &it : *this) {
    assert(it.rInterval != 0);  // :STORAGE has created ugly error here
    lstTimeIntervals.insert(it.rInterval);
  }
  return lstTimeIntervals;
}

void qTree::removeNonStreamItems(const char leadingSign) {
  auto new_end = std::remove_if(begin(), end(),  //
                                [leadingSign](const query &qry) { return qry.id[0] == leadingSign; });
  erase(new_end, end());
}

query &qTree::getQuery(const std::string &query_name) {
  assert(query_name != "");

  auto it = std::find_if(begin(), end(), [query_name](const auto &node) { return node.id == query_name; });
  if (it == std::end(*this)) {
    SPDLOG_ERROR("Missing - {}", query_name);
    throw std::logic_error("Query not found.");
  }
  return (*it);
}

int qTree::getSeqNr(const std::string &query_name) {
  int cnt(0);
  for (auto &q : *this) {
    if (query_name == q.id)
      return cnt;
    else
      ++cnt;
  }
  SPDLOG_ERROR("No such stream in set - {}", query_name);
  throw std::logic_error("No such stream in set.");
  return -1;  // INVALID QUERY_NR
}

boost::rational<int> token::getRI() {
  cast<rdb::descFldVT> castRI;
  auto ret = castRI(valueVT, rdb::RATIONAL);
  return std::get<boost::rational<int>>(ret);
}

rdb::descFldVT token::getVT() { return valueVT; };

std::string token::getStr_() {
  if (valueVT.index() == rdb::STRING)
    return std::get<std::string>(valueVT);
  else if (valueVT.index() == rdb::IDXPAIR) {
    auto r = std::get<std::pair<STRINT>>(valueVT);
    return r.first;
  } else
    return "Error";
}

bool query::isDeclaration() const { return lProgram.empty(); }

bool query::isGenerated() { return !id.compare(0, 7, "STREAM_"); }

/** Construktor set */

field::field(rdb::rField field_, token lToken)
    :                 //
      field_(field_)  //
{
  lProgram.push_back(lToken);
}

field::field(rdb::rField field_, std::list<token> lProgram)
    :                      //
      lProgram(lProgram),  //
      field_(field_) {}

token field::getFirstFieldToken() {
  assert(lProgram.size() > 0);  // If this fails that means in field no program (decl!)
  return *lProgram.begin();
}

/** Construktor set */

token::token(command_id id, rdb::descFldVT value)
    :  //
      command(id),
      valueVT(value) {}

/** Construktor set */

query::query(boost::rational<int> rInterval, const std::string &id) : rInterval(rInterval), id(id) {}

query::query() {}

int query::getFieldIndex(field f_arg) {
  int idx(0);
  for (auto f : lSchema) {
    if (f.field_.rname == f_arg.field_.rname)  // Todo
      return idx;
    ++idx;
  }
  SPDLOG_ERROR("Field not found in query - {}", f_arg.field_.rname);
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
  for (auto &f : lSchema)
    retVal += rdb::Descriptor(f.field_.rname,   //
                              f.field_.rlen,    //
                              f.field_.rarray,  //
                              f.field_.rtype);

  if (!isDeclaration()) {
    if (retention != std::pair<int, int>{0, 0}) {
      SPDLOG_INFO("descriptorStorage/Retention: {} {}", retention.first, retention.second);
      retVal += rdb::Descriptor("", retention.first, retention.second, rdb::RETENTION);
    } else {
      SPDLOG_INFO("descriptorStorage/Retention: Empty");
    }
    return retVal;
  }
  retVal += rdb::Descriptor(filename, 0, 0, rdb::REF);

  auto filenameShdw{filename};
  std::transform(filenameShdw.begin(), filenameShdw.end(), filenameShdw.begin(), ::tolower);
  if (filenameShdw.find(".txt") != std::string::npos)
    retVal += rdb::Descriptor("TEXTSOURCE", 0, 0, rdb::TYPE);
  else
    retVal += rdb::Descriptor("DEVICE", 0, 0, rdb::TYPE);

  return retVal;
}

void query::fillDescriptor(const std::list<field> &lSchemaVar, rdb::Descriptor &val, const std::string &id) {
  auto i{0};
  for (const auto &f : lSchemaVar) {
    if (f.field_.rlen == 0) continue;
    val += rdb::Descriptor(id + "_" + std::to_string(i++),  //
                           f.field_.rlen,                   //
                           f.field_.rarray,                 //
                           f.field_.rtype);
  };
}

// TODO: remove Descriptor(a,b) and use Descriptor(a,b,c) here - strings are broken if not fix
rdb::Descriptor query::descriptorFrom(qTree &coreInstance) {
  SPDLOG_INFO("call query::descriptorFrom()");
  rdb::Descriptor retVal{};
  if (isDeclaration()) {
    retVal += descriptorStorage();
    return retVal;
  }

  auto [arg1, arg2, cmd]{GetArgs(lProgram)};
  switch (cmd.getCommandID()) {
    case STREAM_AVG:
    case STREAM_MAX:
    case STREAM_MIN:
    case STREAM_SUM: {
      auto [maxType, maxLen] = coreInstance.getQuery(arg1).descriptorStorage().getMaxType();
      retVal += rdb::Descriptor(id + "_0", maxLen, 1, maxType);
    } break;
    case STREAM_HASH: {
      retVal.createHash(id, coreInstance.getQuery(arg1).descriptorStorage(), coreInstance.getQuery(arg2).descriptorStorage());
      retVal.cleanRef();
    } break;
    case STREAM_DEHASH_DIV:
    case STREAM_DEHASH_MOD:
    case STREAM_SUBTRACT:
    case STREAM_TIMEMOVE: {
      fillDescriptor(coreInstance.getQuery(arg1).lSchema, retVal, id);
    } break;
    case PUSH_STREAM: {
      fillDescriptor(coreInstance.getQuery(cmd.getStr_()).lSchema, retVal, id);
    } break;
    case STREAM_ADD: {
      fillDescriptor(coreInstance.getQuery(arg1).lSchema, retVal, id);
      fillDescriptor(coreInstance.getQuery(arg2).lSchema, retVal, id);
    } break;
    case STREAM_AGSE: {
      // * INFO - sync with dataModel.cpp

      // 	:- PUSH_STREAM core -> delta_source (arg[0]) - operation
      //  :- STREAM_AGSE 2,3 -> window_length, window_step (arg[1])

      auto [step, length] = std::get<std::pair<int, int>>(cmd.getVT());
      assert(step > 0);
      auto [maxType, maxLen] = coreInstance.getQuery(arg1).descriptorStorage().getMaxType();
      for (int i = 0; i < abs(length); i++) {
        retVal += rdb::Descriptor(id + "_" + std::to_string(i), maxLen, 1, maxType);
      }
    } break;
    default:
      SPDLOG_ERROR("Undefined cmd: {} str:{}", cmd.getStrCommandID(), cmd.getStr_());
      assert(false);
  }

  if (retention != std::pair<int, int>{0, 0}) {
    SPDLOG_INFO("descriptorFrom/Retention: {} {}", retention.first, retention.second);
    retVal += rdb::Descriptor("", retention.first, retention.second, rdb::RETENTION);
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
  os << "name:" << s.field_.rname << ",";
  os << "type:" << s.field_.rtype << ",";
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

std::ostream &operator<<(std::ostream &os, const rdb::descFldVT &rhs) {
  switch (rhs.index()) {
    case rdb::STRING:
      return os << std::get<std::string>(rhs);
    case rdb::FLOAT:
      return os << std::get<float>(rhs);
    case rdb::DOUBLE:
      return os << std::get<double>(rhs);
    case rdb::INTEGER:
      return os << std::get<int>(rhs);
    case rdb::UINT:
      return os << std::get<unsigned>(rhs);
    case rdb::BYTE:
      return os << unsigned(std::get<uint8_t>(rhs));
    case rdb::RATIONAL:
      return os << std::get<boost::rational<int>>(rhs);
    case rdb::INTPAIR: {
      auto r = get<std::pair<int, int>>(rhs);
      return os << r.first << "," << r.second;
    }
    case rdb::IDXPAIR: {
      auto r = get<std::pair<std::string, int>>(rhs);
      return os << r.first << "[" << r.second << "]";
    }
  }
  return os << "not supported";
}

std::ostream &operator<<(std::ostream &os, const token &rhs) {
  os << GetStringcommand_id(rhs.command) << "(";
  os << rhs.valueVT;
  os << ")";
  return os;
}
