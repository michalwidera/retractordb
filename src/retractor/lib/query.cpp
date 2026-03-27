#include "query.h"

#include "qTree.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <stdexcept>

bool operator<(const query &lhs, const query &rhs) { return lhs.rInterval < rhs.rInterval; }

void query::reset() {
  id.clear();
  filename.clear();
  rInterval = 0;
  lSchema.clear();
  lProgram.clear();
  isDisposable = false;
  isOneShot    = false;
  isHold       = false;
  policy       = std::make_pair("DEFAULT", 0);
  retention    = rdb::retention_t{0, 0};
  return;
}

bool isThere(const std::vector<query> &v, const std::string &query_name) {
  return std::any_of(v.begin(), v.end(),
                     [&query_name](const auto &q) { return !q.id.empty() && q.id == query_name; });
}

/** Construktor set */

query::query(boost::rational<int> rInterval, const std::string &id) : rInterval(rInterval), id(id) {}

query::query() {}

int query::getFieldIndex(const field &f_arg) {
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
    if (!retention.noRetention()) {
      SPDLOG_INFO("descriptorStorage/Retention: {} {}", retention.segments, retention.capacity);
      retVal += rdb::Descriptor("", retention.segments, retention.capacity, rdb::RETENTION);
    } else {
      SPDLOG_INFO("descriptorStorage/Retention: Empty");
    }
    if (policy.second != 0) {
      SPDLOG_INFO("descriptorStorage/Retention memory: {}", policy.second);
      retVal += rdb::Descriptor("", policy.second, 0, rdb::RETMEMORY);
      retVal += rdb::Descriptor(policy.first, 0, 0, rdb::TYPE);
    } else {
      SPDLOG_INFO("descriptorStorage/Retention memory: Empty");
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

  if (!retention.noRetention()) {
    SPDLOG_INFO("descriptorFrom/Retention: {} {}", retention.segments, retention.capacity);
    retVal += rdb::Descriptor("", retention.segments, retention.capacity, rdb::RETENTION);
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

std::ostream &operator<<(std::ostream &os, const query &s) {
  os << "QRY/";
  os << "id:" << s.id << ",";
  os << "filename:" << s.filename << ",";
  os << "rInterval:" << s.rInterval << ",";
  os << "lSchema:";
  for (auto &i : s.lSchema)
    os << i << ",";
  os << "lProgram:";
  for (auto &i : s.lProgram)
    os << i << ",";
  return os;
}
