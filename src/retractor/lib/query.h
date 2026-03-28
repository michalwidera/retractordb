#pragma once

#include <iostream>
#include <list>
#include <string>
#include <tuple>
#include <vector>

#include <boost/rational.hpp>

#include "field.h"
#include "rule.h"
#include "token.h"
#include "rdb/descriptor.h"
#include "rdb/faccmemory.h"

class qTree;

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
  bool isHold                    = false;

  std::list<field> lSchema;
  std::list<token> lProgram;

  std::list<rule> lRules;

  rdb::retention_t retention            = rdb::retention_t{0, 0};  // Retention segments and capacity
  std::pair<std::string, size_t> policy = std::make_pair("DEFAULT", rdb::memoryFile::no_retention);

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

bool isThere(const std::vector<query> &v, const std::string &query_name);
