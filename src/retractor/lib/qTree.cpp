#include "qTree.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <sstream>
#include <stdexcept>

using namespace boost;

void qTree::dfs(const std::string &v) {
  visited_[v] = true;
  for (auto u : adj_[v]) {
    if (!visited_[u]) dfs(u);
  }
  ans_.push_back(v);
}

// https://en.wikipedia.org/wiki/Topological_sorting
//
void qTree::topologicalSort() {
  // https://cp-algorithms.com/graph/topological-sort.html#implementation

  qTree &coreInstance{*this};

  ans_.clear();
  for (auto q : coreInstance)
    visited_[q.id] = false;
  for (auto q : coreInstance)
    adj_[q.id] = q.getDepStream();
  for (auto q : coreInstance)
    if (!visited_[q.id]) dfs(q.id);

  qTree tempInstance;
  // for (auto qname : ans_) tempInstance.push_back(coreInstance[qname]); -> same:
  std::for_each(ans_.begin(), ans_.end(),
                [&tempInstance, &coreInstance](const std::string &qname)  //
                { tempInstance.push_back(coreInstance[qname]); });
  coreInstance = tempInstance;
}

bool qTree::exists(const std::string &query_name) {
  return std::any_of(begin(), end(), [&query_name](const auto &q) { return q.id == query_name; });
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
    for (auto i = 0; i < maxSize; ++i)
      sp << "_";
  }
  ss << "|\n";
  sp << "|\n";

  printf(ss.str().c_str(), vcols[0].c_str(), vcols[1].c_str(), vcols[2].c_str(), vcols[3].c_str());
  printf("%s", sp.str().c_str());

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
    if (it.isCompilerDirective()) continue;
    lstTimeIntervals.insert(it.rInterval);
  }
  return lstTimeIntervals;
}

query &qTree::getQuery(const std::string &query_name) {
  assert(query_name != "");

  auto it = std::find_if(begin(), end(), [query_name](const auto &node) { return node.id == query_name; });
  if (it == std::end(*this)) {
    SPDLOG_ERROR("Missing - {}", query_name);
    throw std::logic_error("Referenced Stream in QUERY _not found_ in CORE TREE. (check log)");
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
