#include "rdb/descriptor.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <limits>
#include <locale>

namespace rdb {
// https://belaycpp.com/2021/08/24/best-ways-to-convert-an-enum-to-a-string/

static inline void ltrim(std::string &s) {
  s.erase(                                      //
      s.begin(),                                //
      std::find_if(s.begin(), s.end(),          //
                   [](auto ch) {                //
                     return !std::isspace(ch);  //
                   }                            //
                   ));
}

static inline void rtrim(std::string &s) {
  s.erase(                                //
      std::find_if(s.rbegin(), s.rend(),  //
                   [](auto ch) {          //
                     return !std::isspace(ch);
                   }  //
                   )
          .base(),
      s.end());
}

static bool flatOutput = false;

rdb::descFld GetFieldType(std::string name) {
  ltrim(name);
  rtrim(name);
  std::map<std::string, rdb::descFld> typeDictionary  //
      = {{"STRING", rdb::STRING},                     //
         {"BYTEARRAY", rdb::BYTEARRAY},               //
         {"INTARRAY", rdb::INTARRAY},                 //
         {"UINT", rdb::UINT},                         //
         {"BYTE", rdb::BYTE},                         //
         {"INTEGER", rdb::INTEGER},                   //
         {"FLOAT", rdb::FLOAT},                       //
         {"REF", rdb::REF},                           //
         {"TYPE", rdb::TYPE},                         //
         {"DOUBLE", rdb::DOUBLE}};
  return typeDictionary[name];
}

std::string GetFieldType(rdb::descFld e) {
  std::map<rdb::descFld, std::string> typeDictionary  //
      = {{rdb::STRING, "STRING"},                     //
         {rdb::BYTEARRAY, "BYTEARRAY"},               //
         {rdb::INTARRAY, "INTARRAY"},                 //
         {rdb::UINT, "UINT"},                         //
         {rdb::BYTE, "BYTE"},                         //
         {rdb::INTEGER, "INTEGER"},                   //
         {rdb::FLOAT, "FLOAT"},                       //
         {rdb::REF, "REF"},                           //
         {rdb::TYPE, "TYPE"},                         //
         {rdb::DOUBLE, "DOUBLE"}};
  return typeDictionary[e];
}

constexpr int GetFieldLenFromType(rdb::descFld ft) {
  switch (ft) {
    case rdb::BAD:
      SPDLOG_ERROR("rdb::BAD in type");
      abort();
    case rdb::UINT:
      return sizeof(unsigned);
    case rdb::INTEGER:
      return sizeof(int);
    case rdb::FLOAT:
      return sizeof(float);
    case rdb::DOUBLE:
      return sizeof(double);
    case rdb::BYTE:
      return 1;
    case rdb::REF:
    case rdb::TYPE:
      return 0;
    case rdb::STRING:
    case rdb::BYTEARRAY:
    case rdb::INTARRAY:
      assert(false && "This type has other method of len-type id");
      break;
    default:
      SPDLOG_ERROR("Undefined type rdb->int:{}", (int)ft);
      assert(false && "Undefined type");
  }
  return 0;
}

Descriptor::Descriptor(std::initializer_list<rfield> l) : std::vector<rfield>(l) {}

Descriptor::Descriptor(std::string n, int l, rdb::descFld t) {  //
  push_back(rfield(n, l, t));                                   //
}

Descriptor::Descriptor(std::string n, rdb::descFld t) {
  assert((t != rdb::STRING || t != rdb::BYTEARRAY || t != rdb::INTARRAY) &&
         "This method does not work for Stings and Bytearrays.");
  push_back(rfield(n, GetFieldLenFromType(t), t));
}

bool Descriptor::isEmpty() const { return this->size() == 0; }

void Descriptor::append(std::initializer_list<rfield> l) { insert(end(), l.begin(), l.end()); }

Descriptor &Descriptor::operator|(const Descriptor &rhs) {
  if (this != &rhs)
    insert(end(), rhs.begin(), rhs.end());  // TODO: add rename of duplicates here.
  else {
    // Descriptor b(*this);
    // insert(end(), b.begin(), b.end());
    assert(false && "can not merge same to same");
    // can't do safe: data | data
    // due one name policy
  }
  return *this;
}

Descriptor &Descriptor::operator=(const Descriptor &rhs) {
  clear();
  insert(end(), rhs.begin(), rhs.end());
  return *this;
}

Descriptor::Descriptor(const Descriptor &init) { *this | init; }

int Descriptor::getSizeInBytes() const {
  int size = 0;
  for (auto const i : *this) size += std::get<rlen>(i);
  return size;
}

int Descriptor::position(std::string name) {
  auto it = std::find_if(begin(), end(),                          //
                         [name](const auto &item) {               //
                           return std::get<rname>(item) == name;  //
                         }                                        //
  );

  if (it != end())
    return std::distance(begin(), it);
  else
    assert(false && "did not find that record id Descriptor:{}");

  return error_desc_location;
}

std::string Descriptor::fieldName(int fieldPosition) {  //
  return std::get<rname>((*this)[fieldPosition]);       //
}

int Descriptor::len(const std::string name) {      //
  return std::get<rlen>((*this)[position(name)]);  //
}

int Descriptor::offset(const std::string name) {
  auto offset = 0;
  for (auto const field : *this) {
    if (name == std::get<rname>(field)) return offset;
    offset += std::get<rlen>(field);
  }
  assert(false && "field not found with that name");
  return error_desc_location;
}

int Descriptor::offset(const int position) {
  auto offset = 0;
  int i = 0;
  for (auto const field : *this) {
    if (position == i) return offset;
    i++;
    offset += std::get<rlen>(field);
  }
  assert(false && "field not found with that postion");
  return error_desc_location;
}

std::string Descriptor::type(const std::string name) { return GetFieldType(std::get<rtype>((*this)[position(name)])); }

std::ostream &flat(std::ostream &os) {
  flatOutput = true;
  return os;
}

std::ostream &operator<<(std::ostream &os, const Descriptor &rhs) {
  os << "{";
  for (auto const &r : rhs) {
    if (!flatOutput)
      os << "\t";
    else
      os << " ";
    os << GetFieldType(std::get<rtype>(r)) << " ";
    if (std::get<rtype>(r) == rdb::REF)
      os << "\"" << std::get<rname>(r) << "\"";
    else
      os << std::get<rname>(r);
    if (std::get<rtype>(r) == rdb::STRING || std::get<rtype>(r) == rdb::BYTEARRAY)
      os << "[" << std::to_string(std::get<rlen>(r)) << "]";
    else if (std::get<rtype>(r) == rdb::INTARRAY)
      os << "[" << std::to_string(std::get<rlen>(r) / sizeof(int)) << "]";
    if (!flatOutput) os << std::endl;
  }
  if (rhs.isEmpty())
    os << "Empty";
  else if (flatOutput)
    os << " ";
  os << "}";
  flatOutput = false;
  return os;
}

// Look here to explain:
// https://stackoverflow.com/questions/7302996/changing-the-delimiter-for-cin-c
struct synsugar_is_space : std::ctype<char> {
  synsugar_is_space() : ctype<char>(get_table()) {}
  static mask const *get_table() {
    static mask rc[table_size];
    rc['['] = rc[']'] = rc['{'] = rc['}'] = rc[' '] = rc['\n'] = std::ctype_base::space;
    return &rc[0];
  }
};

std::istream &operator>>(std::istream &is, Descriptor &rhs) {
  auto origLocale = is.getloc();
  is.imbue(std::locale(origLocale, std::unique_ptr<synsugar_is_space>(new synsugar_is_space).release()));
  do {
    std::string type;
    std::string name;
    int len = 0;
    is >> type;
    if (is.eof()) break;
    auto ft = GetFieldType(type);

    if (ft == rdb::REF) {
      char c;
      while (is.get(c) && c != '"')
        ;
      while (is.get(c) && c != '"') name += c;
      name.erase(remove(name.begin(), name.end(), '"'), name.end());
    } else {
      is >> name;
      ltrim(name);
      rtrim(name);
    }

    if (ft == rdb::STRING || ft == rdb::BYTEARRAY)
      is >> len;
    else if (ft == rdb::INTARRAY) {
      is >> len;
      len *= sizeof(int);
    } else
      len = GetFieldLenFromType(ft);

    rhs | Descriptor(name, len, ft);
  } while (!is.eof());
  is.imbue(origLocale);
  return is;
}

}  // namespace rdb