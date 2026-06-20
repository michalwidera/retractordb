#include "qTree.hpp"

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "fatalError.hpp"

using namespace boost;

void qTree::dfs(const std::string &v) {
  visited_[v] = true;
  for (const auto &u : adj_[v]) {
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
  for (const auto &q : coreInstance)
    visited_[q.id] = false;
  for (auto q : coreInstance)
    adj_[q.id] = q.getDepStream();
  for (const auto &q : coreInstance)
    if (!visited_[q.id]) dfs(q.id);

  qTree tempInstance;
  // for (auto qname : ans_) tempInstance.push_back(coreInstance[qname]); -> same:
  std::ranges::for_each(ans_, [&tempInstance, &coreInstance](const std::string &qname)  //
                        { tempInstance.push_back(coreInstance[qname]); });
  coreInstance = tempInstance;
}

bool qTree::exists(const std::string &query_name) {
  return std::ranges::any_of(*this, [&query_name](const auto &q) { return q.id == query_name; });
}

boost::rational<int> qTree::getDelta(const std::string &query_name) { return getQuery(query_name).rInterval; }

void qTree::dumpCore() {
  std::vector<std::string> vcols = {"Idx", "Delta", "Cap", "Name"};
  std::stringstream ss;
  std::stringstream sp;

  for (const auto &nName : vcols) {
    int maxSize = static_cast<int>(nName.length());
    int size{0};
    for (const auto &it : *this) {
      if (nName == vcols[0])
        size = static_cast<int>(std::to_string(getSeqNr(it.id)).length());
      else if (nName == vcols[1])
        size = static_cast<int>(
            (std::to_string(it.rInterval.numerator()) + "/" + std::to_string(it.rInterval.denominator())).length());
      else if (nName == vcols[2])
        size = static_cast<int>(std::to_string(maxCapacity[it.id]).length());
      else if (nName == vcols[3])
        size = static_cast<int>(it.id.length());
      else {
        FatalError("qTree::dumpCore: unknown column name");
      }
      maxSize = std::max(maxSize, size);
    }
    ss << "|{:>";
    ss << maxSize;
    ss << "}";

    sp << "|";
    for (auto i = 0; i < maxSize; ++i)
      sp << "_";
  }
  ss << "|\n";
  sp << "|\n";

  fmt::vprint(ss.str(), fmt::make_format_args(vcols[0], vcols[1], vcols[2], vcols[3]));
  fmt::print("{}", sp.str());

  for (const auto &it : *this) {
    std::string col0        = std::to_string(getSeqNr(it.id));
    std::string col1        = std::to_string(it.rInterval.numerator()) + "/" + std::to_string(it.rInterval.denominator());
    std::string col2        = std::to_string(maxCapacity[it.id]);
    const std::string &col3 = it.id;
    fmt::vprint(ss.str(), fmt::make_format_args(col0, col1, col2, col3));
  }
}

std::set<boost::rational<int>> qTree::getAvailableTimeIntervals() {
  std::set<boost::rational<int>> lstTimeIntervals;
  for (const auto &it : *this) {
    if (it.rInterval == 0) {
      FatalError("qTree: query '{}' rInterval is zero — check :STORAGE directive", it.id);
    }
    if (it.isCompilerDirective()) continue;
    lstTimeIntervals.insert(it.rInterval);
  }
  return lstTimeIntervals;
}

query &qTree::getQuery(const std::string &query_name) {
  if (query_name.empty()) FatalError("qTree::getQuery: query name is empty");

  auto it = std::ranges::find_if(*this, [query_name](const auto &node) { return node.id == query_name; });
  if (it == std::end(*this)) {
    SPDLOG_ERROR("Missing - {}", query_name);
    throw std::logic_error("Referenced Stream in QUERY _not found_ in CORE TREE. (check log)");
  }
  return (*it);
}

int qTree::getSeqNr(const std::string &query_name) {
  int cnt(0);
  for (auto &q : *this) {
    if (query_name == q.id) return cnt;
    ++cnt;
  }
  SPDLOG_ERROR("No such stream in set - {}", query_name);
  throw std::logic_error("No such stream in set.");
  return -1;  // INVALID QUERY_NR
}
