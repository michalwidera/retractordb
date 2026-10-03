#include "qTree.hpp"

#include <fmt/core.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <sstream>
#include <stdexcept>

#include "rdb/error.hpp"
#include "rdb/rationalFormat.hpp"

using namespace boost;

std::uint64_t qTree::nextPlanRevision() {
  // Zaczyna od 1, zeby zero zostalo wolne dla tego, kto numer zapamietuje: u niego oznacza
  // "jeszcze nic nie obserwowane" i nie moze trafic w zadne zywe drzewo.
  static std::atomic<std::uint64_t> dispenser{1};
  // relaxed wystarcza: numer nie porzadkuje niczego, ma byc wylacznie niepowtarzalny.
  // Sama zawartosc planu jedzie miedzy watkami pod core_mutex, razem z tym polem.
  return dispenser.fetch_add(1, std::memory_order_relaxed);
}

void qTree::push_back(const query &node) {
  std::vector<query>::push_back(node);
  planRevision_ = nextPlanRevision();
}

void qTree::push_back(query &&node) {
  std::vector<query>::push_back(std::move(node));
  planRevision_ = nextPlanRevision();
}

void qTree::pop_back() {
  std::vector<query>::pop_back();
  planRevision_ = nextPlanRevision();
}

std::vector<query>::iterator qTree::erase(std::vector<query>::const_iterator position) {
  auto next     = std::vector<query>::erase(position);
  planRevision_ = nextPlanRevision();
  return next;
}

std::vector<query>::iterator qTree::erase(std::vector<query>::const_iterator first, std::vector<query>::const_iterator last) {
  auto next     = std::vector<query>::erase(first, last);
  planRevision_ = nextPlanRevision();
  return next;
}

void qTree::clear() {
  std::vector<query>::clear();
  planRevision_ = nextPlanRevision();
}

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

  std::vector<query> reordered;
  reordered.reserve(ans_.size());
  // for (auto qname : ans_) reordered.push_back(coreInstance[qname]); -> same:
  std::ranges::for_each(ans_, [&reordered, &coreInstance](const std::string &qname)  //
                        { reordered.push_back(coreInstance[qname]); });

  // Nowy numer rewizji wnosi replaceAll ponizej - to przez nia przechodzi cala zmiana
  // kolejnosci, wiec drugiego numeru tutaj nie ma po co brac.
  coreInstance.replaceAll(std::move(reordered));
}

void qTree::replaceAll(std::vector<query> &&nodes) {
  static_cast<std::vector<query> &>(*this) = std::move(nodes);
  planRevision_                            = nextPlanRevision();
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
        size = static_cast<int>(fmt::format("{}", it.rInterval).length());
      else if (nName == vcols[2])
        size = static_cast<int>(std::to_string(maxCapacity[it.id]).length());
      else if (nName == vcols[3])
        size = static_cast<int>(it.id.length());
      else {
        rdb::fatal("qTree::dumpCore: unknown column name");
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
    std::string col1        = fmt::format("{}", it.rInterval);
    std::string col2        = std::to_string(maxCapacity[it.id]);
    const std::string &col3 = it.id;
    fmt::vprint(ss.str(), fmt::make_format_args(col0, col1, col2, col3));
  }
}

rdb::Result<std::set<boost::rational<int>>> qTree::getAvailableTimeIntervals() {
  std::set<boost::rational<int>> lstTimeIntervals;
  for (const auto &it : *this) {
    if (it.rInterval == 0) {
      // Config, nie Logic: rInterval bierze sie wprost z interwalu w DECLARE (RQLParser.cpp,
      // `qry.rInterval = rationalResult`), a gramatyka dopuszcza tam zero -
      // `DECLARE v INTEGER STREAM src, 0 FILE 'a.txt'`. Wartosc jest uzytkownika, wiec blad
      // tez jest jego, i tak brzmial komunikat na dlugo przed faza 1. To miejsce wola
      // executorsm::run() i Engine::compile() JUZ PO kompilacji - stad wlasny kanal bledu.
      return rdb::fail(
          rdb::Errc::Config,
          fmt::format("qTree: query '{}' has a zero interval - check its DECLARE or the :STORAGE directive", it.id));
    }
    if (it.isCompilerDirective()) continue;
    lstTimeIntervals.insert(it.rInterval);
  }
  return lstTimeIntervals;
}

query &qTree::getQuery(const std::string &query_name) {
  // Warunek wstepny: nazwa istnieje w planie. Odwolania z tresci planu sprawdza kompilator
  // (compiler::checkStreamReferences) zanim ktorykolwiek przebieg tu siegnie, a nazwy od klienta
  // (Engine, polecenia demona) sprawdzaja exists() ich wolajacy. Do 2026-10 `FROM nosuch` dochodzilo
  // tu i konczylo kompilacje std::logic_error, ktorego nie lapal nikt po drodze.
  RDB_ASSERT(!query_name.empty(), "qTree::getQuery: query name is empty");

  auto it = std::ranges::find_if(*this, [&query_name](const auto &node) { return node.id == query_name; });
  RDB_ASSERT(it != std::end(*this), "qTree::getQuery: stream '{}' not found in the plan", query_name);
  return (*it);
}

int qTree::getSeqNr(const std::string &query_name) {
  int cnt(0);
  for (auto &q : *this) {
    if (query_name == q.id) return cnt;
    ++cnt;
  }
  // Warunek wstepny jak w getQuery(): nazwa pochodzi z samego drzewa albo zostala sprawdzona.
  rdb::fatal(fmt::format("qTree::getSeqNr: no such stream in set - {}", query_name));
}
