#include "rdb/desc.h"

#include <cassert>
#include <cctype>
#include <iostream>
#include <locale>

namespace rdb {
// https://belaycpp.com/2021/08/24/best-ways-to-convert-an-enum-to-a-string/

static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

rdb::eType GetFieldType(std::string name) {
  ltrim(name);
  rtrim(name);
  std::map<std::string, rdb::eType> typeDictionary = {
      {"STRING", rdb::STRING},     {"BYTEARRAY", rdb::BYTEARRAY},
      {"INTARRAY", rdb::INTARRAY}, {"UINT", rdb::UINT},
      {"BYTE", rdb::BYTE},         {"INTEGER", rdb::INTEGER},
      {"FLOAT", rdb::FLOAT},       {"DOUBLE", rdb::DOUBLE}};
  return typeDictionary[name];
}

std::string GetFieldType(rdb::eType e) {
  std::map<rdb::eType, std::string> typeDictionary = {
      {rdb::STRING, "STRING"},     {rdb::BYTEARRAY, "BYTEARRAY"},
      {rdb::INTARRAY, "INTARRAY"}, {rdb::UINT, "UINT"},
      {rdb::BYTE, "BYTE"},         {rdb::INTEGER, "INTEGER"},
      {rdb::FLOAT, "FLOAT"},       {rdb::DOUBLE, "DOUBLE"}};
  return typeDictionary[e];
}

constexpr uint GetFieldLenFromType(rdb::eType ft) {
  switch (ft) {
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
    case rdb::STRING:
    case rdb::BYTEARRAY:
    case rdb::INTARRAY:
      assert("This type has other method of len-type id");
      break;
    default:
      assert("Undefined type");
  }
  return 0;
}

Descriptor::Descriptor(std::initializer_list<rfield> l)
    : std::vector<rfield>(l) {}

Descriptor::Descriptor(fieldName n, fieldLen l, rdb::eType t) {
  push_back(rfield(n, l, t));
  UpdateNames();
}

Descriptor::Descriptor(fieldName n, rdb::eType t) {
  assert((t != rdb::STRING || t != rdb::BYTEARRAY || t != rdb::INTARRAY) &&
         "This method does not work for Stings and Bytearrays.");
  push_back(rfield(n, GetFieldLenFromType(t), t));
  UpdateNames();
}

bool Descriptor::isDirty() const { return fieldNames.size() == 0; }

void Descriptor::Append(std::initializer_list<rfield> l) {
  insert(end(), l.begin(), l.end());
}

Descriptor &Descriptor::operator|(const Descriptor &rhs) {
  if (this != &rhs)
    insert(end(), rhs.begin(), rhs.end());
  else {
    // Descriptor b(*this);
    // insert(end(), b.begin(), b.end());
    assert(false && "can not merge same to same");
    // can't do safe: data | data
    // due one name policy
  }
  UpdateNames();
  return *this;
}

Descriptor &Descriptor::operator=(const Descriptor &rhs) {
  clear();
  insert(end(), rhs.begin(), rhs.end());
  UpdateNames();
  return *this;
}

Descriptor::Descriptor(const Descriptor &init) {
  *this | init;
  UpdateNames();
}

uint Descriptor::GetSize() const {
  uint size = 0;
  for (auto const i : *this) size += std::get<rlen>(i);
  return size;
}

bool Descriptor::UpdateNames() {
  uint position = 0;
  fieldNames.clear();
  for (auto const i : *this) {
    if (fieldNames.find(std::get<fieldName>(i)) == fieldNames.end())
      fieldNames[std::get<fieldName>(i)] = position;
    else {
      fieldNames.clear();
      assert(false && "that name aleardy exist so it will make dirty");
      return false;
    }
    position++;
  }
  return true;
}

uint Descriptor::Position(std::string name) {
  if (fieldNames.find(name) != fieldNames.end()) return fieldNames[name];
  assert(false && "did not find that record id Descriptor:{}");
  return -1;
}

std::string Descriptor::FieldName(uint fieldPosition) {
  return std::get<fieldName>((*this)[fieldPosition]);
}

uint Descriptor::Len(const std::string name) {
  auto pos = Position(name);
  return std::get<rlen>((*this)[pos]);
}

uint Descriptor::Offset(const std::string name) {
  auto offset = 0;
  for (auto const field : *this) {
    if (name == std::get<fieldName>(field)) return offset;
    offset += std::get<rlen>(field);
  }
  assert(false && "field not found with that name");
  return -1;
}

uint Descriptor::Offset(const int position) {
  auto offset = 0;
  int i = 0;
  for (auto const field : *this) {
    if (position == i) return offset;
    i++;
    offset += std::get<rlen>(field);
  }
  assert(false && "field not found with that postion");
  return -1;
}

std::string Descriptor::Type(const std::string name) {
  auto pos = Position(name);
  return GetFieldType(std::get<rtype>((*this)[pos]));
}

std::ostream &operator<<(std::ostream &os, const Descriptor &rhs) {
  os << "{";
  for (auto const &r : rhs) {
    os << "\t" << GetFieldType(std::get<rtype>(r)) << " " << std::get<rname>(r);
    if (std::get<rtype>(r) == rdb::STRING ||
        std::get<rtype>(r) == rdb::BYTEARRAY)
      os << "[" << std::to_string(std::get<rlen>(r)) << "]";
    else if (std::get<rtype>(r) == rdb::INTARRAY)
      os << "[" << std::to_string(std::get<rlen>(r) / sizeof(int)) << "]";
    os << std::endl;
  }
  os << "}";
  if (rhs.fieldNames.size() == 0) os << "[dirty]";
  return os;
}

// Look here to explain:
// https://stackoverflow.com/questions/7302996/changing-the-delimiter-for-cin-c
struct synsugar_is_space : std::ctype<char> {
  synsugar_is_space() : ctype<char>(get_table()) {}
  static mask const *get_table() {
    static mask rc[table_size];
    rc['['] = rc[']'] = rc['{'] = rc['}'] = rc[' '] = rc['\n'] =
        std::ctype_base::space;
    return &rc[0];
  }
};

std::istream &operator>>(std::istream &is, Descriptor &rhs) {
  auto origLocale = is.getloc();
  is.imbue(std::locale(
      origLocale,
      std::unique_ptr<synsugar_is_space>(new synsugar_is_space).release()));
  do {
    std::string type;
    std::string name;
    int len = 0;
    is >> type;
    if (is.eof()) break;
    is >> name;
    ltrim(name);
    rtrim(name);
    auto ft = GetFieldType(type);
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