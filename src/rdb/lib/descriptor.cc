#include "rdb/descriptor.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>

#include <magic_enum/magic_enum.hpp>

extern std::string parserDESCString(rdb::Descriptor &desc, const std::string_view inlet);

namespace rdb {

bool Descriptor::flatOutput_ = false;

constexpr auto GetFieldType(const std::string_view name) { return magic_enum::enum_cast<rdb::descFld>(name); }

constexpr auto GetFieldType(const rdb::descFld index) { return magic_enum::enum_name(index); }

constexpr auto isConfigurationField(const rdb::descFld index) {
  return index == rdb::TYPE ||       //
         index == rdb::REF ||        //
         index == rdb::RETENTION ||  //
         index == rdb::RETMEMORY;
}

Descriptor::Descriptor(std::initializer_list<rField> l) : std::vector<rField>(l) {}

Descriptor::Descriptor(const std::string &name, int length, int arrayCount, rdb::descFld type) {  //
  emplace_back(std::move(name), length, arrayCount, type);                                        //
}

void Descriptor::updateConvMaps() {
  if (!dirtyMap) return;

  convMap_.clear();
  offsetMap_.clear();

  clen_ = 0;
  for (auto it : *this)
    clen_ += (it.rtype == rdb::STRING) ? 1 : it.rarray;

  std::vector<rField>::iterator it = this->begin();
  int fieldCounter{0};
  int backCounterArray{0};
  int counterArray{(*it).rarray};
  int offset{0};
  int clen_alignment{0};
  for (int i = 0; i < clen_; ++i) {
    if (isConfigurationField((*it).rtype)) {
      ++it;
      ++clen_alignment;
      continue;
    }

    if ((*it).rtype == rdb::STRING) {
      counterArray     = 1;
      backCounterArray = 0;
    }

    if (counterArray > 0) {
      convMap_.push_back(std::make_pair(fieldCounter, backCounterArray));
      offsetMap_.push_back(offset);
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
  clen_ -= clen_alignment;
  dirtyMap = false;
}

std::optional<std::pair<int, int>> Descriptor::convert(int position) {
  updateConvMaps();
  if (position < clen_) {
    return convMap_[position];
  } else {
    assert(false);
    return {};
  }
}

int Descriptor::sizeFlat() {
  updateConvMaps();
  return clen_;
};

std::vector<rField> Descriptor::fieldsFlat() {
  updateConvMaps();
  std::vector<rField> ret;
  ret.reserve(clen_);
  for (const auto &i : (*this)) {
    if (isConfigurationField(i.rtype)) continue;  // skip configuration fields
    ret.push_back(i);
  }
  return ret;
}

void Descriptor::append(std::initializer_list<rField> l) {
  insert(end(), l.begin(), l.end());
  dirtyMap = true;
}

Descriptor operator+(const Descriptor &lhs, const Descriptor &rhs) {
  Descriptor ret(lhs);
  ret += rhs;
  return ret;
}

Descriptor &Descriptor::operator+=(const Descriptor &rhs) {
  if (this != &rhs) {
    insert(end(), rhs.begin(), rhs.end());  // TODO: add rename of duplicates here.
  } else {
    assert(false && "can not merge same to same");
    // can't do safe: data | data
    // due one name policy
  }

  dirtyMap = true;
  return *this;
}

// this      rhs
// 4,INT  == 1,BYTE   1
// 1,BYTE == 4,INT    0
// 4,INT  == 4,INT    1
bool Descriptor::operator==(const Descriptor &rhs) const {
  auto refCountRhs  = std::count_if(rhs.begin(), rhs.end(),                                          //
                                    [](const rField &i) { return isConfigurationField(i.rtype); });  //
  auto refCountThis = std::count_if(begin(), end(),                                                  //
                                    [](const rField &i) { return isConfigurationField(i.rtype); });  //
  auto i{0};
  for (const rField &f : *this) {
    if (isConfigurationField(f.rtype) || isConfigurationField(rhs[i].rtype)) {
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
  std::copy_if(rhs.begin(), rhs.end(),     //
               std::back_inserter(*this),  //
               [](const rField &i) {       //
                 return !isConfigurationField(i.rtype);
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

constexpr int Descriptor::len(const rdb::rField &field) const {
  if (isConfigurationField(field.rtype)) return 0;
  return field.rlen * field.rarray;
}

size_t Descriptor::getSizeInBytes() const {
  auto size{0};
  for (auto const i : *this)
    size += len(i);
  return size;
}

rdb::retention_t Descriptor::retention() {
  rdb::retention_t retval{0, 0};

  auto it = std::find_if(begin(), end(),                                                //
                         [](const auto &item) { return item.rtype == rdb::RETENTION; }  //
  );

  if (it != end()) retval = std::pair<int, int>((*it).rlen, (*it).rarray);

  return retval;
}

std::pair<std::string, size_t> Descriptor::policy() {
  int retval{0};

  auto it1 = std::find_if(begin(), end(),                                                //
                          [](const auto &item) { return item.rtype == rdb::RETMEMORY; }  //
  );

  if (it1 != end()) retval = (*it1).rlen;

  std::string retvalType{""};
  auto it2 = std::find_if(begin(), end(),                                           //
                          [](const auto &item) { return item.rtype == rdb::TYPE; }  //
  );

  if (it2 != end()) retvalType = (*it2).rname;

  return std::make_pair(retvalType, retval);
}

size_t Descriptor::position(const std::string_view name) {
  auto it = std::find_if(begin(), end(),                                            //
                         [name](const auto &item) { return item.rname == name; });  //

  if (it != end())
    return std::distance(begin(), it);
  else
    assert(false && "did not find that record id Descriptor:{}");

  return 0;  // ProForma Error
}

int Descriptor::len(const std::string_view name) { return len((*this)[position(name)]); }

size_t Descriptor::offsetBegArr(const std::string_view name) {
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
  return offsetMap_[position];
}

std::string_view Descriptor::type(const std::string_view name) {  //
  return GetFieldType(((*this)[position(name)]).rtype);           //
}

std::pair<rdb::descFld, int> Descriptor::getMaxType() {
  rdb::descFld retVal{rdb::BYTE};
  auto size{1};
  for (auto const field : *this) {
    if (isConfigurationField(field.rtype)) continue;
    if (retVal <= field.rtype) {
      retVal = field.rtype;
      if (size < len(field)) size = len(field);
    }
  }
  return std::make_pair(retVal, size);
}

std::ostream &flat(std::ostream &os) {
  Descriptor::setFlat(true);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Descriptor &rhs) {
  os << "{";
  for (auto const &r : rhs) {
    if (r.rtype == rdb::RETENTION)
      if (r.rlen == 0 && r.rarray == 0) continue;  // skip retention 0,0
    if (r.rtype == rdb::RETMEMORY)
      if (r.rlen == 0) continue;  // skip retention memory 0
    if (!Descriptor::flatOutput_)
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
        // retention {segment} {capacity}
        os << r.rarray << " " << r.rlen;
        break;
      case rdb::RETMEMORY:
        // retention memory {capacity}
        os << r.rlen;
        break;
      default:
        os << r.rname;
    }

    if (r.rarray > 1 && (r.rtype != rdb::RETENTION))
      os << "[" << r.rarray << "]";
    else if (r.rtype == rdb::STRING)
      os << "[" << r.rlen << "]";
    if (!Descriptor::flatOutput_) os << std::endl;
  }
  if (rhs.empty())
    os << "Empty";
  else if (Descriptor::flatOutput_)
    os << " ";
  os << "}";
  Descriptor::flatOutput_ = false;

  return os;
}

std::istream &operator>>(std::istream &is, Descriptor &rhs) {
  std::stringstream strstream;
  std::string str;
  while (is >> str)
    strstream << " " << str;

  auto result = parserDESCString(rhs, strstream.str());
  assert(result == "OK");

  return is;
}

}  // namespace rdb
