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
/*
    std::string GetFieldType(FieldType e)
    {
        std::map<FieldType, std::string> typeDictionary = {
            {STRING, "STRING"}, {BYTEARRAY, "BYTEARRAY"},  {INTARRAY,
   "INTARRAY"}, {UINT, "UINT"}, {BYTE, "BYTE"}, {INT, "INT"}, {Float, "Float"},
   {Double, "Double"}}; return typeDictionary[e];
    }
*/
FieldType GetFieldType(std::string name) {
  ltrim(name);
  rtrim(name);
  std::map<std::string, FieldType> typeDictionary = {
      {"STRING", STRING}, {"BYTEARRAY", BYTEARRAY}, {"INTARRAY", INTARRAY},
      {"UINT", UINT},     {"BYTE", BYTE},           {"INT", INT},
      {"Float", Float},   {"Double", Double}};
  return typeDictionary[name];
}

constexpr const char *GetFieldType(FieldType e) noexcept {
  switch (e) {
    case STRING:
      return "STRING";
    case BYTEARRAY:
      return "BYTEARRAY";
    case INTARRAY:
      return "INTARRAY";
    case UINT:
      return "UINT";
    case BYTE:
      return "BYTE";
    case INT:
      return "INT";
    case Float:
      return "Float";
    case Double:
      return "Double";
    default:
      assert("Undefined type");
      return "error";
  }
}

constexpr uint GetFieldLenFromType(FieldType ft) {
  switch (ft) {
    case UINT:
      return sizeof(UINT);
    case INT:
      return sizeof(INT);
    case Float:
      return sizeof(float);
    case Double:
      return sizeof(double);
    case BYTE:
      return 1;
    case STRING:
    case BYTEARRAY:
    case INTARRAY:
      assert("This type has other method of len-type id");
      break;
    default:
      assert("Undefined type");
  }
  return 0;
}

Descriptor::Descriptor(std::initializer_list<field> l)
    : std::vector<field>(l) {}

Descriptor::Descriptor(fieldName n, fieldLen l, FieldType t) {
  push_back(field(n, l, t));
  UpdateNames();
}

Descriptor::Descriptor(fieldName n, FieldType t) {
  assert((t != STRING || t != BYTEARRAY || t != INTARRAY) &&
         "This method does not work for Stings and Bytearrays.");
  push_back(field(n, GetFieldLenFromType(t), t));
  UpdateNames();
}

bool Descriptor::isDirty() const { return fieldNames.size() == 0; }

void Descriptor::Append(std::initializer_list<field> l) {
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

uint Descriptor::Len(const std::string name) {
  auto pos = Position(name);
  return std::get<rlen>((*this)[pos]);
}

uint Descriptor::Offset(const std::string name) {
  auto offset = 0;
  for (auto const i : *this) {
    if (name == std::get<fieldName>(i)) return offset;
    offset += std::get<rlen>(i);
  }
  assert(false && "record not found with that name");
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
    if (std::get<rtype>(r) == STRING || std::get<rtype>(r) == BYTEARRAY)
      os << "[" << std::to_string(std::get<rlen>(r)) << "]";
    else if (std::get<rtype>(r) == INTARRAY)
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
    if (ft == STRING || ft == BYTEARRAY)
      is >> len;
    else if (ft == INTARRAY) {
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