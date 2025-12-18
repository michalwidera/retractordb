#pragma once

// Standard template library
#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <variant>

// Boost libraries
#include <boost/rational.hpp>

#include "cmdID.hpp"
#include "fldType.hpp"
#include "rdb/descriptor.h"
#include "rdb/retention.h"

class qTree;

class token {
  command_id command_;
  rdb::descFldVT valueVT_;

 public:
  std::string getStr_();
  boost::rational<int> getRI();
  constexpr rdb::descFldVT getVT() const { return valueVT_; };

  explicit token(command_id id = VOID_COMMAND, rdb::descFldVT value = 0);

  std::string getStrCommandID();
  command_id getCommandID();

  friend std::ostream &operator<<(std::ostream &os, const token &rhs);
};

struct field {
  std::list<token> lProgram;

  rdb::rField field_;

  explicit field(rdb::rField field_, token lToken);
  explicit field(rdb::rField field_, std::list<token> lProgram);

  token getFirstFieldToken();

  friend std::ostream &operator<<(std::ostream &os, const rdb::descFld &s);
  friend std::ostream &operator<<(std::ostream &os, const field &s);
};

struct rule {
  std::string name;
  std::list<token> condition;

  enum actionType { UNKNOWN_ACTION, DUMP, SYSTEM } action{UNKNOWN_ACTION};

  std::pair<long int, long int> dumpRange{std::make_pair(0, 0)};
  size_t dump_retention{0};

  std::string systemCommand{};

  rule(std::string name, std::list<token> condition) : name(std::move(name)), condition(std::move(condition)) {}
};

class query {
  void fillDescriptor(const std::list<field> &lSchemaVar, rdb::Descriptor &val, const std::string &id);

 public:
  explicit query(boost::rational<int> rInterval, const std::string &id);
  query();

  std::list<std::string> getFieldNamesList();

  std::string id                 = "";
  std::string filename           = "";
  boost::rational<int> rInterval = 0;
  bool isDisposable              = false;
  bool isOneShot                 = false;

  std::list<field> lSchema;
  std::list<token> lProgram;

  std::list<rule> lRules;

  rdb::retention_t retention                    = rdb::retention_t{0, 0};        // Retention segments and capacity
  std::pair<std::string, size_t> substratPolicy = std::make_pair("DEFAULT", 0);  // rdb::memoryFileAccessor::no_retention

  bool isDeclaration() const { return lProgram.empty(); }
  bool isReductionRequired();
  bool isGenerated() const { return !id.compare(0, 7, "STREAM_"); }
  bool isCompilerDirective() const { return id.size() > 0 && id[0] == ':'; }
  bool is(command_id command);

  std::vector<std::string> getDepStream();

  int getFieldIndex(const field &f);

  void reset();

  rdb::Descriptor descriptorStorage();
  rdb::Descriptor descriptorFrom(qTree &coreInstance);

  friend std::ostream &operator<<(std::ostream &os, const query &s);
};

bool operator<(const query &lhs, const query &rhs);

std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog);

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
