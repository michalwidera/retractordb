#include "rdb/descriptor.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <iostream>
#include <limits>
#include <locale>
#include <typeinfo>
#include <utility>

extern std::string parserDESCString(rdb::Descriptor &desc, std::string inlet);

namespace rdb {

static bool flatOutput = false;

bool getFlat() { return flatOutput; }
void setFlat(bool var) { flatOutput = var; }

rdb::descFld GetFieldType(std::string name) {
  std::map<std::string, rdb::descFld> typeDictionary  //
      = {{"STRING", rdb::STRING},                     //
         {"UINT", rdb::UINT},                         //
         {"BYTE", rdb::BYTE},                         //
         {"INTEGER", rdb::INTEGER},                   //
         {"FLOAT", rdb::FLOAT},                       //
         {"REF", rdb::REF},                           //
         {"TYPE", rdb::TYPE},                         //
         {"RETENTION", rdb::RETENTION},               //
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
         {rdb::RETENTION, "RETENTION"},               //
         {rdb::DOUBLE, "DOUBLE"}};
  return typeDictionary[e];
}

Descriptor::Descriptor(std::initializer_list<rField> l) : std::vector<rField>(l) {}

Descriptor::Descriptor(const std::string &n, int l, int a, rdb::descFld t) {  //
  push_back(rField(n, l, a, t));                                              //
}

void Descriptor::updateConvMaps() {
  if (!dirtyMap) return;

  convMap.clear();
  convReMap.clear();
  offsetMap.clear();

  clen = 0;
  for (auto it : *this) clen += (it.rtype == rdb::STRING) ? 1 : it.rarray;

  std::vector<rField>::iterator it = this->begin();
  int fieldCounter{0};
  int backCounterArray{0};
  int counterArray{(*it).rarray};
  int offset{0};
  int clen_alignment{0};
  for (int i = 0; i < clen; ++i) {
    if ((*it).rtype == rdb::TYPE ||  //
        (*it).rtype == rdb::REF ||   //
        (*it).rtype == rdb::RETENTION) {
      ++it;
      ++clen_alignment;
      continue;
    }

    if ((*it).rtype == rdb::STRING) {
      counterArray     = 1;
      backCounterArray = 0;
    }

    if (counterArray > 0) {
      convMap.push_back(std::make_pair(fieldCounter, backCounterArray));
      convReMap[std::pair<int, int>(fieldCounter, backCounterArray)] = i;

      offsetMap.push_back(offset);
      if ((*it).rtype == rdb::STRING)
        offset += len(*it);
      else
        offset += (*it).rlen;
    }

    --counterArray;
    ++backCounterArray;

    if (counterArray == 0) {
      ++it;
      if (it == this->end()) break;
      backCounterArray = 0;
      ++fieldCounter;
      counterArray = (*it).rarray;
    }
  }
  clen -= clen_alignment;
  dirtyMap = false;
}

std::optional<std::pair<int, int>> Descriptor::convert(int position) {
  updateConvMaps();
  if (position < clen) {
    return convMap[position];
  } else {
    assert(false);
    return {};
  }
}

std::optional<int> Descriptor::convert(std::pair<int, int> position) {
  updateConvMaps();
  if (convReMap.find(position) != convReMap.end())
    return convReMap[position];
  else {
    assert(false);
    return {};
  }
}

bool Descriptor::isEmpty() const { return this->size() == 0; }

int Descriptor::sizeFlat() {
  updateConvMaps();
  return clen;
};

void Descriptor::append(std::initializer_list<rField> l) { insert(end(), l.begin(), l.end()); }

Descriptor operator+(const Descriptor &lhs, const Descriptor &rhs) {
  Descriptor ret(lhs);
  ret += rhs;
  return ret;
}

Descriptor &Descriptor::operator+=(const Descriptor &rhs) {
  if (this != &rhs) {
    insert(end(), rhs.begin(), rhs.end());  // TODO: add rename of duplicates here.
  } else {
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
  if (this == &rhs) return *this;

  clear();
  insert(end(), rhs.begin(), rhs.end());
  dirtyMap = true;
  return *this;
}

// this      rhs
// 4,INT  == 1,BYTE   1
// 1,BYTE == 4,INT    0
// 4,INT  == 4,INT    1
bool Descriptor::operator==(const Descriptor &rhs) const {
  auto refCountRhs  = std::count_if(rhs.begin(), rhs.end(),              //
                                    [](const rField &i) {                //
                                     return i.rtype == rdb::REF ||      //
                                            i.rtype == rdb::TYPE ||     //
                                            i.rtype == rdb::RETENTION;  //
                                   });
  auto refCountThis = std::count_if(begin(), end(),                      //
                                    [](const rField &i) {                //
                                      return i.rtype == rdb::REF ||      //
                                             i.rtype == rdb::TYPE ||     //
                                             i.rtype == rdb::RETENTION;  //
                                    });

  auto i{0};
  for (const rField &f : *this) {
    if (f.rtype == rdb::REF ||             //
        f.rtype == rdb::TYPE ||            //
        f.rtype == rdb::RETENTION ||       //
        rhs[i].rtype == rdb::RETENTION ||  //
        rhs[i].rtype == rdb::REF ||        //
        rhs[i].rtype == rdb::TYPE) {
      ++i;
      continue;
    }
    if (len(f) < len(rhs[i]) || f.rtype < rhs[i].rtype) return false;

    ++i;
  }
  return this->size() - refCountThis == rhs.size() - refCountRhs;
}

Descriptor &Descriptor::cleanRef() {
  Descriptor rhs(*this);
  clear();
  std::copy_if(rhs.begin(), rhs.end(),             //
               std::back_inserter(*this),          //
               [](const rField &i) {               //
                 return i.rtype != rdb::REF &&     //
                        i.rtype != rdb::TYPE &&    //
                        i.rtype != rdb::RETENTION  //
                     ;
               });

  dirtyMap = true;
  return *this;
}

Descriptor &Descriptor::createHash(const std::string &name, Descriptor lhs, Descriptor rhs) {
  lhs.cleanRef();
  rhs.cleanRef();
  assert(lhs.size() == rhs.size());

  clear();
  auto i{0};
  for (auto const &looper : lhs) {
    auto maxRtype = std::max(lhs[i].rtype, rhs[i].rtype);
    auto maxRlen  = std::max(lhs[i].rlen, rhs[i].rlen);
    push_back(rField(name + "_" + std::to_string(i), maxRlen, 1, maxRtype));
    ++i;
  }

  dirtyMap = true;
  return *this;
}

Descriptor::Descriptor(const Descriptor &init) { *this += init; }

constexpr int Descriptor::len(const rdb::rField &field) const {  //
  if (field.rtype == rdb::RETENTION) return 0;
  return field.rlen * field.rarray;
}

size_t Descriptor::getSizeInBytes() const {
  auto size{0};
  for (auto const i : *this) size += len(i);
  return size;
}

std::pair<size_t, size_t> Descriptor::retention() {
  std::pair<size_t, size_t> retval{0, 0};

  auto it = std::find_if(begin(), end(),                                          //
                         [](auto &item) { return item.rtype == rdb::RETENTION; }  //
  );

  if (it != end()) retval = std::pair<int, int>((*it).rlen, (*it).rarray);

  return retval;
}

size_t Descriptor::position(const std::string &name) {
  auto it = std::find_if(begin(), end(),               //
                         [name](const auto &item) {    //
                           return item.rname == name;  //
                         }  //
  );

  if (it != end())
    return std::distance(begin(), it);
  else
    assert(false && "did not find that record id Descriptor:{}");

  return 0;  // ProForma Error
}

std::string Descriptor::fieldName(int fieldPosition) {  //
  return ((*this)[fieldPosition]).rname;                //
}

int Descriptor::len(const std::string &name) { return len((*this)[position(name)]); }

int Descriptor::arraySize(const std::string &name) {  //
  return ((*this)[position(name)]).rarray;            //
}

size_t Descriptor::offsetBegArr(const std::string &name) {
  auto offset{0};
  for (auto const field : *this) {
    if (name == field.rname) return offset;
    offset += len(field);
  }
  assert(false && "field not found with that name");
  return 0;  // ProForma Error
}

int Descriptor::offset(const int position) {
  updateConvMaps();
  return offsetMap[position];
}

std::string Descriptor::type(const std::string &name) {  //
  return GetFieldType(((*this)[position(name)]).rtype);  //
}

std::pair<rdb::descFld, int> Descriptor::getMaxType() {
  rdb::descFld retVal{rdb::BYTE};
  auto size{1};
  for (auto const field : *this) {
    if (field.rtype == rdb::REF) continue;
    if (field.rtype == rdb::TYPE) continue;
    if (field.rtype == rdb::RETENTION) continue;
    if (retVal <= field.rtype) {
      retVal = field.rtype;
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
    if (r.rtype == rdb::RETENTION)
      if (r.rtype == 0 && r.rarray == 0) continue;  // skip retention 0,0

    if (!flatOutput)
      os << "\t";
    else
      os << " ";
    os << GetFieldType(r.rtype) << " ";

    switch (r.rtype) {
      case rdb::REF:
        os << "\"" << r.rname << "\"";
        break;
      case rdb::TYPE:
        os << r.rname;
        break;
      case rdb::RETENTION:
        // retention {capacity} {segment}
        os << r.rlen << " " << r.rarray;
        break;
      default:
        os << r.rname;
    }

    if (r.rarray > 1 && (r.rtype != rdb::RETENTION))
      os << "[" << r.rarray << "]";
    else if (r.rtype == rdb::STRING)
      os << "[" << r.rlen << "]";
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

std::istream &operator>>(std::istream &is, Descriptor &rhs) {
  std::stringstream strstream;
  std::string str;
  while (is >> str) strstream << " " << str;

  auto result = parserDESCString(rhs, strstream.str().c_str());
  assert(result == "OK");

  return is;
}

}  // namespace rdb