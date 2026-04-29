#include "rdb/descriptor.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>

#include <magic_enum/magic_enum.hpp>

extern std::string parserDESCString(rdb::Descriptor &desc, const std::string_view inlet);

namespace rdb {

bool Descriptor::singleLineOutput_ = false;

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

Descriptor::Descriptor(std::initializer_list<rField> fields) : std::vector<rField>(fields) {}

Descriptor::Descriptor(const std::string &fieldName, int length, int elementCount, rdb::descFld type) {  //
  emplace_back(std::move(fieldName), length, elementCount, type);                                        //
}

void Descriptor::rebuildFieldMappings() {
  if (!fieldMappingsDirty_) return;

  flatToDescriptorIndexMap_.clear();
  fieldByteOffsets_.clear();

  flattenedFieldCount_ = 0;
  int offset{0};
  for (size_t descriptorFieldIdx = 0; descriptorFieldIdx < size(); ++descriptorFieldIdx) {
    const auto &field = (*this)[descriptorFieldIdx];
    if (isConfigurationField(field.rtype)) continue;

    const int flatCount = (field.rtype == rdb::STRING) ? 1 : field.rarray;
    for (int arrayIndex = 0; arrayIndex < flatCount; ++arrayIndex) {
      flatToDescriptorIndexMap_.push_back(std::make_pair(static_cast<int>(descriptorFieldIdx), arrayIndex));
      fieldByteOffsets_.push_back(offset);
      offset += (field.rtype == rdb::STRING) ? fieldSize(field) : field.rlen;
      ++flattenedFieldCount_;
    }
  }
  fieldMappingsDirty_ = false;
}

std::optional<std::pair<int, int>> Descriptor::flatIndexToDescriptorPosition(int flatIndex) {
  rebuildFieldMappings();
  if (flatIndex < 0 || flatIndex >= flattenedFieldCount_) return {};
  return flatToDescriptorIndexMap_[flatIndex];
}

int Descriptor::flatElementCount() {
  rebuildFieldMappings();
  return flattenedFieldCount_;
};

std::vector<rField> Descriptor::dataFields() {
  rebuildFieldMappings();
  std::vector<rField> ret;
  ret.reserve(flattenedFieldCount_);
  for (const auto &i : (*this)) {
    if (isConfigurationField(i.rtype)) continue;
    ret.push_back(i);
  }
  return ret;
}

void Descriptor::append(std::initializer_list<rField> fields) {
  insert(end(), fields.begin(), fields.end());
  fieldMappingsDirty_ = true;
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

  fieldMappingsDirty_ = true;
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

  fieldMappingsDirty_ = true;
}

void Descriptor::composeHashDescriptorFrom(const std::string &fieldNamePrefix, Descriptor lhs, Descriptor rhs) {
  lhs.removeConfigurationFields();
  rhs.removeConfigurationFields();
  assert(lhs.size() == rhs.size());

  clear();
  auto i{0};
  for (auto const &looper : lhs) {
    auto maxRtype = std::max(lhs[i].rtype, rhs[i].rtype);
    auto maxRlen  = std::max(lhs[i].rlen, rhs[i].rlen);
    push_back(rField(fieldNamePrefix + "_" + std::to_string(i), maxRlen, 1, maxRtype));
    ++i;
  }

  fieldMappingsDirty_ = true;
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

std::pair<std::string, size_t> Descriptor::storagePolicy() {
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

size_t Descriptor::fieldIndex(const std::string_view fieldName) {
  auto it = std::find_if(begin(), end(),                                                    //
                         [fieldName](const auto &item) { return item.rname == fieldName; });  //

  if (it != end())
    return std::distance(begin(), it);
  else
    assert(false && "did not find that record id Descriptor:{}");

  return 0;  // ProForma Error
}

int Descriptor::fieldSize(const std::string_view fieldName) { return fieldSize((*this)[fieldIndex(fieldName)]); }

size_t Descriptor::fieldByteOffset(const std::string_view fieldName) {
  auto offset{0};
  for (auto const field : *this) {
    if (fieldName == field.rname) return offset;
    offset += fieldSize(field);
  }
  assert(false && "field not found with that name");
  return 0;  // ProForma Error
}

int Descriptor::byteOffsetAtFlatIndex(const int flatIndex) {
  rebuildFieldMappings();
  assert(flatIndex >= 0 && flatIndex < flattenedFieldCount_ && "flat position out of range");
  if (flatIndex < 0 || flatIndex >= flattenedFieldCount_) return 0;
  return fieldByteOffsets_[flatIndex];
}

std::string_view Descriptor::fieldTypeName(const std::string_view fieldName) {  //
  return GetFieldType(((*this)[fieldIndex(fieldName)]).rtype);                   //
}

std::pair<rdb::descFld, int> Descriptor::widestFieldType() {
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

std::ostream &singleLineFormat(std::ostream &os) {
  Descriptor::setSingleLineOutput(true);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Descriptor &rhs) {
  os << "{";
  for (auto const &r : rhs) {
    if (r.rtype == rdb::RETENTION)
      if (r.rlen == 0 && r.rarray == 0) continue;  // skip retention 0,0
    if (r.rtype == rdb::RETMEMORY)
      if (r.rlen == 0) continue;  // skip retention memory 0
    if (!Descriptor::singleLineOutput_)
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
    if (!Descriptor::singleLineOutput_) os << std::endl;
  }
  if (rhs.empty())
    os << "Empty";
  else if (Descriptor::singleLineOutput_)
    os << " ";
  os << "}";
  Descriptor::singleLineOutput_ = false;

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
