#pragma once

// Standard template library
#include <algorithm>
#include <list>
#include <set>
#include <sstream>
#include <stack>
#include <string>

// Boost libraries
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/rational.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>
#include <variant>

#include "enumDecl.h"
#include "rdb/desc.h"

typedef std::variant<boost::rational<int>, int, double, unsigned char> number;

namespace boost {
namespace serialization {

template <class Archive, class T>
inline void serialize(Archive &ar, boost::rational<T> &p,
                      unsigned int /* file_version */
) {
  T _num(p.numerator());
  T _den(p.denominator());
  ar &_num;
  ar &_den;
  p.assign(_num, _den);
}

// https://stackoverflow.com/questions/14744303/does-boost-support-serialization-of-c11s-stdtuple/14928368#14928368
template <typename Archive, typename... Types>
inline void serialize(Archive &ar, std::tuple<Types...> &t,
                      const unsigned int) {
  std::apply([&](auto &...element) { ((ar & element), ...); }, t);
}

}  // namespace serialization
}  // namespace boost

boost::rational<int> Rationalize(double inValue, double DIFF = 1E-6,
                                 int ttl = 11);

class token {
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, unsigned int version) {
    ar &command;
    ar &numericValue;
    ar &textValue;
  }

  command_id command;
  boost::rational<int> numericValue;
  std::string textValue;

 public:
  std::string getStr();
  boost::rational<int> get();

  token(command_id id = VOID_COMMAND, std::string sValue = "",
        number value = 0);

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
  rdb::eType fieldType;

  field();
  field(std::string sFieldName, std::list<token> &lProgram,
        rdb::eType fieldType, std::string sFieldText);

  std::string getFieldText();
  token getFirstFieldToken();

  friend std::ostream &operator<<(std::ostream &os, const rdb::eType s);
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
  query(boost::rational<int> rInterval, std::string id);
  query();

  std::list<std::string> getFieldNamesList();

  std::string id;
  std::string filename;
  boost::rational<int> rInterval;
  std::list<field> lSchema;
  std::list<token> lProgram;

  bool isDeclaration();
  bool isReductionRequired();
  bool isGenerated();
  bool is(command_id command);

  field &getField(std::string sField);

  std::vector<std::string> getDepStreamName(int reqDep = 0);

  int getFieldIndex(field f);

  void reset();
};

bool operator<(const query &lhs, const query &rhs);

query &getQuery(std::string query_name);
int getSeqNr(std::string query_name);
bool isDeclared(std::string query_name);
bool isExist(std::string query_name);

std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog);

class qTree : public std::vector<query> {
 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, unsigned int version) {
    ar &boost::serialization::base_object<vector<query>>(*this);
  }

 public:
  query &operator[](std::string query_name) { return getQuery(query_name); };

  void sort() { std::sort(begin(), end()); };

  /** Topological sort*/
  void tsort();

  boost::rational<int> getDelta(std::string query_name);
};
