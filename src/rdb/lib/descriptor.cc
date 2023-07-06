#include "rdb/descriptor.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <limits>
#include <locale>
#include <typeinfo>
#include <utility>

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

bool flatOutput = false;

rdb::descFld GetFieldType(std::string name) {
  ltrim(name);
  rtrim(name);
  std::map<std::string, rdb::descFld> typeDictionary  //
      = {{"STRING", rdb::STRING},                     //
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
         {rdb::UINT, "UINT"},                         //
         {rdb::BYTE, "BYTE"},                         //
         {rdb::INTEGER, "INTEGER"},                   //
         {rdb::FLOAT, "FLOAT"},                       //
         {rdb::REF, "REF"},                           //
         {rdb::TYPE, "TYPE"},                         //
         {rdb::DOUBLE, "DOUBLE"}};
  return typeDictionary[e];
}

int GetFieldLenFromType(rdb::descFld ft) {
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
      return 1;
    case rdb::REF:
    case rdb::TYPE:
      return 0;
    default:
      SPDLOG_ERROR("Undefined type rdb->int:{}", (int)ft);
      assert(false && "Undefined type");
  }
  return 0;
}

Descriptor::Descriptor(std::initializer_list<rField> l) : std::vector<rField>(l) {}

Descriptor::Descriptor(std::string n, int l, int a, rdb::descFld t) {  //
  push_back(rField(n, l, a, t));                                       //
}

void Descriptor::updateConvMaps() {
  if (!dirtyMap) return;

  convMap.clear();
  convReMap.clear();
  offsetMap.clear();

  clen = 0;
  for (auto it : *this) clen += (std::get<rtype>(it) == rdb::STRING) ? 1 : std::get<rarray>(it);

  std::vector<rField>::iterator it = this->begin();
  int fieldCounter{0};
  int backCounterArray{0};
  int counterArray{std::get<rarray>(*it)};
  int offset{0};
  for (int i = 0; i < clen; i++) {
    if (std::get<rtype>(*it) == rdb::TYPE || std::get<rtype>(*it) == rdb::REF) {
      it++;
      continue;
    }

    if (std::get<rtype>(*it) == rdb::STRING) {
      counterArray = 1;
      backCounterArray = 0;
    }

    if (counterArray > 0) {
      convMap.push_back(std::make_pair(fieldCounter, backCounterArray));
      convReMap[std::pair<int, int>(fieldCounter, backCounterArray)] = i;

      offsetMap.push_back(offset);
      offset += len(*it);
    }

    counterArray--;
    backCounterArray++;

    if (counterArray == 0) {
      it++;
      if (it == this->end()) break;
      backCounterArray = 0;
      fieldCounter++;
      counterArray = std::get<rarray>(*it);
    }
  }
  dirtyMap = false;
}

std::optional<std::pair<int, int>> Descriptor::convert(int position) {
  updateConvMaps();
  if (position < clen) {
    return convMap[position];
  } else
    return {};
}

std::optional<int> Descriptor::convert(std::pair<int, int> position) {
  updateConvMaps();
  if (convReMap.find(position) != convReMap.end())
    return convReMap[position];
  else
    return {};
}

bool Descriptor::isEmpty() const { return this->size() == 0; }

int Descriptor::sizeRel() {
  updateConvMaps();
  return clen;
};

void Descriptor::append(std::initializer_list<rField> l) { insert(end(), l.begin(), l.end()); }

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

  dirtyMap = true;
  return *this;
}

Descriptor &Descriptor::operator=(const Descriptor &rhs) {
  clear();
  insert(end(), rhs.begin(), rhs.end());

  dirtyMap = true;
  return *this;
}

// this      rhs
// 4,INT  == 1,BYTE   1
// 1,BYTE == 4,INT    0
// 4,INT  == 4,INT    1
bool Descriptor::operator==(const Descriptor &rhs) {
  auto refCountRhs = std::count_if(rhs.begin(), rhs.end(),                          //
                                   [](const rField &i) {                            //
                                     return std::get<rdb::rtype>(i) == rdb::REF ||  //
                                            std::get<rdb::rtype>(i) == rdb::TYPE;
                                   });
  auto refCountThis = std::count_if(begin(), end(),                                  //
                                    [](const rField &i) {                            //
                                      return std::get<rdb::rtype>(i) == rdb::REF ||  //
                                             std::get<rdb::rtype>(i) == rdb::TYPE;
                                    });

  auto i{0};
  for (rField &f : *this) {
    if (std::get<rdb::rtype>(f) == rdb::REF ||       //
        std::get<rdb::rtype>(f) == rdb::TYPE ||      //
        std::get<rdb::rtype>(rhs[i]) == rdb::REF ||  //
        std::get<rdb::rtype>(rhs[i]) == rdb::TYPE) {
      i++;
      continue;
    }
    if (len(f) < len(rhs[i]) ||  //
        std::get<rdb::rtype>(f) < std::get<rdb::rtype>(rhs[i]))
      return false;

    i++;
  }
  return this->size() - refCountThis == rhs.size() - refCountRhs;
}

Descriptor &Descriptor::cleanRef() {
  Descriptor rhs(*this);
  clear();
  std::copy_if(rhs.begin(), rhs.end(),                          //
               std::back_inserter(*this),                       //
               [](const rField &i) {                            //
                 return std::get<rdb::rtype>(i) != rdb::REF &&  //
                        std::get<rdb::rtype>(i) != rdb::TYPE;
               });

  dirtyMap = true;
  return *this;
}

Descriptor &Descriptor::createHash(const std::string name, Descriptor lhs, Descriptor rhs) {
  lhs.cleanRef();
  rhs.cleanRef();
  assert(lhs.size() == rhs.size());

  clear();
  auto i{0};
  for (auto const &looper : lhs) {
    auto maxRtype = std::max(std::get<rdb::rtype>(lhs[i]), std::get<rdb::rtype>(rhs[i]));
    auto maxRlen = std::max(std::get<rdb::rlen>(lhs[i]), std::get<rdb::rlen>(rhs[i]));
    push_back(rField(name + "_" + std::to_string(i), maxRlen, 1, maxRtype));
    i++;
  }

  dirtyMap = true;
  return *this;
}

Descriptor::Descriptor(const Descriptor &init) { *this | init; }

constexpr int Descriptor::len(const rdb::rField &field) const {  //
  return std::get<rlen>(field) * std::get<rarray>(field);        //
}

int Descriptor::getSizeInBytes() const {
  auto size{0};
  for (auto const i : *this) size += len(i);
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

int Descriptor::len(const std::string name) { return len((*this)[position(name)]); }

int Descriptor::arraySize(const std::string name) {  //
  return std::get<rarray>((*this)[position(name)]);  //
}

int Descriptor::offsetBegArr(const std::string name) {
  auto offset{0};
  for (auto const field : *this) {
    if (name == std::get<rname>(field)) return offset;
    offset += len(field);
  }
  assert(false && "field not found with that name");
  return error_desc_location;
}

int Descriptor::offset(const int position) {
  auto offset{0};
  updateConvMaps();
  return offsetMap[position];
}

std::string Descriptor::type(const std::string name) {            //
  return GetFieldType(std::get<rtype>((*this)[position(name)]));  //
}

std::pair<rdb::descFld, int> Descriptor::getMaxType() {
  rdb::descFld retVal{rdb::BYTE};
  auto size{1};
  for (auto const field : *this) {
    if (std::get<rtype>(field) == rdb::REF) continue;
    if (std::get<rtype>(field) == rdb::TYPE) continue;
    if (retVal <= std::get<rtype>(field)) {
      retVal = std::get<rtype>(field);
      if (size < len(field)) size = len(field);
    }
  }
  return std::make_pair(retVal, size);
}

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
    if (std::get<rarray>(r) > 1)
      os << "[" << std::get<rarray>(r) << "]";
    else if (std::get<rtype>(r) == rdb::STRING)
      os << "[" << std::get<rlen>(r) << "]";
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

    auto arrayLen = 1;
    char c = is.peek();  // peek character
    if (c == '[') {
      while (is.get(c) && c != '[')
        ;
      is >> arrayLen;
      while (is.get(c) && c != ']')
        ;
    }

    rhs | Descriptor(name, GetFieldLenFromType(ft), arrayLen, ft);
  } while (!is.eof());
  is.imbue(origLocale);

  return is;
}

}  // namespace rdb