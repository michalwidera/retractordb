#pragma once

// Standard template library
#include <algorithm>
#include <list>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <variant>

// Boost libraries
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/rational.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>

#include "cmdID.h"
#include "rdb/descriptor.h"

typedef boost::rational<int> number;

#include "fldType.h"

namespace boost {
namespace serialization {

template <class Archive, class T>
inline void serialize(Archive &ar, boost::rational<T> &p, unsigned int /* file_version */
) {
  T _num(p.numerator());
  T _den(p.denominator());
  ar &_num;
  ar &_den;
  p.assign(_num, _den);
}

template <class Archive, typename... Ts>
void save(Archive &ar, const std::variant<Ts...> &obj, const unsigned int version) {
  boost::variant<Ts...> v;
  std::visit([&](const auto &arg) { v = arg; }, obj);

  ar &v;
}

template <class Archive, typename... Ts>
void load(Archive &ar, std::variant<Ts...> &obj, const unsigned int version) {
  boost::variant<Ts...> v;
  ar &v;

  boost::apply_visitor([&](auto &arg) { obj = arg; }, v);
}

template <class Archive, typename... Ts>
void serialize(Archive &ar, std::variant<Ts...> &t, const unsigned int file_version) {
  split_free(ar, t, file_version);
}

template <class Archive>
void serialize(Archive &ar, std::monostate &, const unsigned int /*version*/) {}

// https://stackoverflow.com/questions/14744303/does-boost-support-serialization-of-c11s-stdtuple/14928368#14928368
template <typename Archive, typename... Types>
inline void serialize(Archive &ar, std::tuple<Types...> &t, const unsigned int) {
  std::apply([&](auto &...element) { ((ar & element), ...); }, t);
}

}  // namespace serialization
}  // namespace boost

class token {
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, unsigned int version) {
    ar &command;
    ar &numericValue;
    ar &textValue;
    ar &valueVT;
  }

  command_id command;
  boost::rational<int> numericValue;
  std::string textValue;
  rdb::descFldVT valueVT;

 public:
  std::string getStr_();
  boost::rational<int> getRI();
  rdb::descFldVT getVT();

  token(command_id id = VOID_COMMAND, rdb::descFldVT value = 0, std::string desc = "");

  std::string getStrCommandID();
  command_id getCommandID();
};

class field {
 private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, unsigned int version) {
    ar &lProgram;
    ar &sFieldText;
    ar &fieldName;
    ar &fieldType;
  }

  std::string sFieldText;

 public:
  std::list<token> lProgram;

  std::string fieldName;
  rdb::descFld fieldType;

  field();
  field(std::string sFieldName, std::list<token> &lProgram, rdb::descFld fieldType, std::string sFieldText);

  std::string getFieldText();
  token getFirstFieldToken();

  friend std::ostream &operator<<(std::ostream &os, const rdb::descFld s);
};

class query {
 private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, unsigned int version) {
    ar &id;
    ar &filename;
    ar &rInterval;
    ar &lSchema;
    ar &lProgram;
  }

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

  std::vector<std::string> getDepStreamName(int reqDep = 0);

  int getFieldIndex(field f);

  void reset();

  rdb::Descriptor descriptorExpression();
  rdb::Descriptor descriptorFrom();
};

bool operator<(const query &lhs, const query &rhs);

query &getQuery(const std::string &query_name);
int getSeqNr(const std::string &query_name);
bool isDeclared(const std::string &query_name);
bool isExist(const std::string &query_name);

std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog);

class qTree : public std::vector<query> {
 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, unsigned int version) {
    ar &boost::serialization::base_object<vector<query>>(*this);
  }

 public:
  query &operator[](const std::string &query_name) { return getQuery(query_name); };

  void sort() { std::sort(begin(), end()); };

  /** Topological sort*/
  void tsort();

  boost::rational<int> getDelta(const std::string &query_name);
};
