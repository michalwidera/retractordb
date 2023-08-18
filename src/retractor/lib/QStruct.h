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

#include "cmdID.h"
#include "rdb/descriptor.h"

typedef boost::rational<int> number;

#include "fldType.h"

class token {
  command_id command;
  rdb::descFldVT valueVT;

 public:
  std::string getStr_();
  boost::rational<int> getRI();
  rdb::descFldVT getVT();

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

class query {
  void fillDescriptor(const std::list<field> &lSchemaVar, rdb::Descriptor &val, const std::string &id);

 public:
  explicit query(boost::rational<int> rInterval, const std::string &id);
  query();

  std::list<std::string> getFieldNamesList();

  std::string id = "";
  std::string filename;
  boost::rational<int> rInterval = 0;
  std::list<field> lSchema;
  std::list<token> lProgram;

  bool isDeclaration() const;
  bool isReductionRequired();
  bool isGenerated();
  bool is(command_id command);

  std::vector<std::string> getDepStream();

  int getFieldIndex(field f);

  void reset();

  rdb::Descriptor descriptorStorage();
  rdb::Descriptor descriptorFrom();

  friend std::ostream &operator<<(std::ostream &os, const query &s);
};

bool operator<(const query &lhs, const query &rhs);

query &getQuery(const std::string &query_name);
int getSeqNr(const std::string &query_name);
bool isDeclared(const std::string &query_name);

std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog);

class qTree : public std::vector<query> {
  /* Topological sort vars */
  std::map<std::string, bool> visited;
  std::map<std::string, vector<std::string>> adj;  // adjacency list of graph
  vector<std::string> ans;

  void dfs(const std::string v);  // Depth First Traversal
  int getSeqNr(const std::string &query_name);

 public:
  query &operator[](const std::string &query_name) { return getQuery(query_name); };

  void sort() { std::sort(begin(), end()); };
  void topologicalSort();

  boost::rational<int> getDelta(const std::string &query_name);
  void dumpCore();
  std::set<boost::rational<int>> getAvailableTimeIntervals();

  // This code removes :STORAGE from coreInstance
  void removeNonStreamItems(const char leadingChar);

  std::map<std::string, int> maxCapacity;
};
