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

constexpr auto GetFieldType(const std::string_view name) {
  if (name == "NULL") return std::optional<rdb::descFld>(rdb::NULLTYPE);
  return magic_enum::enum_cast<rdb::descFld>(name);
}

constexpr auto GetFieldType(const rdb::descFld index) {
  return index == rdb::NULLTYPE ? std::string_view("NULL") : magic_enum::enum_name(index);
}

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
  int offset{0};
  for (size_t fieldIndex = 0; fieldIndex < size(); ++fieldIndex) {
    const auto &field = (*this)[fieldIndex];
    if (isConfigurationField(field.rtype)) continue;

    const int flatCount = (field.rtype == rdb::STRING) ? 1 : field.rarray;
    for (int arrayIndex = 0; arrayIndex < flatCount; ++arrayIndex) {
      convMap_.push_back(std::make_pair(static_cast<int>(fieldIndex), arrayIndex));
      offsetMap_.push_back(offset);
      offset += (field.rtype == rdb::STRING) ? fieldSize(field) : field.rlen;
      ++clen_;
    }
  }
  dirtyMap = false;
}

std::optional<std::pair<int, int>> Descriptor::convert(int position) {
  updateConvMaps();
  if (position < 0 || position >= clen_) return {};
  return convMap_[position];
}

int Descriptor::flatElementCount() {
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
  auto lhsIt = begin();
  auto rhsIt = rhs.begin();

  auto skipConfigurationFields = [](auto &it, const auto &container) {
    while (it != container.end() && isConfigurationField(it->rtype)) {
      ++it;
    }
  };

  while (true) {
    skipConfigurationFields(lhsIt, *this);
    skipConfigurationFields(rhsIt, rhs);

    if (lhsIt == end() || rhsIt == rhs.end()) {
      return lhsIt == end() && rhsIt == rhs.end();
    }

    if (fieldSize(*lhsIt) < fieldSize(*rhsIt) || lhsIt->rtype < rhsIt->rtype) return false;

    ++lhsIt;
    ++rhsIt;
  }
}

void Descriptor::removeConfigurationFields() {
  Descriptor rhs(*this);
  clear();
  std::copy_if(rhs.begin(), rhs.end(),     //
               std::back_inserter(*this),  //
               [](const rField &i) {       //
                 return !isConfigurationField(i.rtype);
               });

  dirtyMap = true;
}

void Descriptor::composeHashDescriptorFrom(const std::string &name, Descriptor lhs, Descriptor rhs) {
  lhs.removeConfigurationFields();
  rhs.removeConfigurationFields();
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
}

constexpr int Descriptor::fieldSize(const rdb::rField &field) const {
  if (isConfigurationField(field.rtype)) return 0;
  if (field.rtype == rdb::NULLTYPE) return 0;
  return field.rlen * field.rarray;
}

size_t Descriptor::getSizeInBytes() const {
  auto size{0};
  for (auto const i : *this)
    size += fieldSize(i);
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

int Descriptor::fieldSize(const std::string_view name) { return fieldSize((*this)[position(name)]); }

size_t Descriptor::offsetBegArr(const std::string_view name) {
  auto offset{0};
  for (auto const field : *this) {
    if (name == field.rname) return offset;
    offset += fieldSize(field);
  }
  assert(false && "field not found with that name");
  return 0;  // ProForma Error
}

int Descriptor::offset(const int position) {
  updateConvMaps();
  assert(position >= 0 && position < clen_ && "flat position out of range");
  if (position < 0 || position >= clen_) return 0;
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
      if (size < fieldSize(field)) size = fieldSize(field);
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
