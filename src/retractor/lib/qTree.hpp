#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/rational.hpp>

#include "query.hpp"

class qTree : public std::vector<query> {
  /* Topological sort vars */
  std::map<std::string, bool> visited_;
  std::map<std::string, std::vector<std::string>> adj_;  // adjacency list of graph
  std::vector<std::string> ans_;

  void dfs(const std::string &v);  // Depth First Traversal
  int getSeqNr(const std::string &query_name);

 public:
  query &operator[](const std::string &query_name) { return getQuery(query_name); };

  query &getQuery(const std::string &query_name);

  void sort() { std::sort(begin(), end()); };
  void topologicalSort();
  bool exists(const std::string &query_name);

  boost::rational<int> getDelta(const std::string &query_name);
  void dumpCore();
  std::set<boost::rational<int>> getAvailableTimeIntervals();

  std::map<std::string, int> maxCapacity;
};
