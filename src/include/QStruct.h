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

  token(command_id id = VOID_COMMAND, rdb::descFldVT value = 0);

  std::string getStrCommandID();
  command_id getCommandID();

  friend std::ostream &operator<<(std::ostream &os, const token &rhs);
};

class field {
 private:
  std::string fieldText;

 public:
  std::list<token> lProgram;

  std::string fieldName;
  rdb::descFld fieldType;

  rdb::rfield fieldDesc;  // TODO

  field();
  field(std::string sFieldName,     //
        std::list<token> lProgram,  //
        rdb::descFld fieldType,     //
        std::string fieldText);

  std::string getFieldText();
  token getFirstFieldToken();

  friend std::ostream &operator<<(std::ostream &os, const rdb::descFld &s);
  friend std::ostream &operator<<(std::ostream &os, const field &s);
};

class query {
 public:
  query(boost::rational<int> rInterval, const std::string &id);
  query();

  std::list<std::string> getFieldNamesList();

  std::string id = "";
  std::string filename;
  boost::rational<int> rInterval = 0;
  std::list<field> lSchema;
  std::list<token> lProgram;

  bool isDeclaration();
  bool isReductionRequired();
  bool isGenerated();
  bool is(command_id command);

  field &getField(const std::string &sField);

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
bool isExist(const std::string &query_name);

std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog);

class qTree : public std::vector<query> {
 public:
  query &operator[](const std::string &query_name) { return getQuery(query_name); };

  void sort() { std::sort(begin(), end()); };

  /** Topological sort*/
  std::map<std::string, bool> visited;
  std::map<std::string, vector<std::string>> adj;  // adjacency list of graph
  vector<std::string> ans;

  void tsort();
  void dfs(std::string v);

  boost::rational<int> getDelta(const std::string &query_name);
};
