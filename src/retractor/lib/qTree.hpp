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
  // Wlasne operator[](nazwa) UKRYWA komplet przeciazen bazy, wiec bez tej deklaracji dostep po
  // pozycji - plan[i] - nie kompiluje sie wcale, a komunikat wskazuje na std::string zamiast na
  // przyczyne (w C++17 gorzej: plan[0] szlo przez 0 -> const char* -> std::string i wywracalo
  // sie w locie; dzis basic_string(nullptr_t) jest = delete). Rozstrzyganie jest jednoznaczne:
  // z typu calkowitego nie ma niejawnej konwersji do std::string, wiec indeks zawsze wybiera
  // wersje pozycyjna, a napis - wersje po nazwie.
  using std::vector<query>::operator[];

  query &operator[](const std::string &query_name) { return getQuery(query_name); };

  query &getQuery(const std::string &query_name);

  // Keep explicit comparator: std::ranges::less is not invocable for query on newer GCC.
  void sort() {
    std::ranges::sort(*this, [](const query &lhs, const query &rhs) { return lhs < rhs; });
  };
  void topologicalSort();
  bool exists(const std::string &query_name);

  boost::rational<int> getDelta(const std::string &query_name);
  void dumpCore();
  std::set<boost::rational<int>> getAvailableTimeIntervals();

  std::map<std::string, int> maxCapacity;
};
