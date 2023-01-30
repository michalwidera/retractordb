#include "rdb/payloadacc.h"

#include <algorithm>  // std::min
#include <cassert>
#include <cstddef>  // std::byte
#include <cstring>  // std:memcpy
#include <iomanip>
#include <iostream>

#include "spdlog/spdlog.h"

namespace rdb {
payloadAccessor::payloadAccessor(Descriptor descriptor, std::byte *ptr, bool hexFormat)
    : descriptor(descriptor),  //
      ptr(ptr),                //
      hexFormat(hexFormat) {}

template <typename T, typename K>
void copyToMemory(std::istream &is, const K &rhs, const char *fieldName) {
  T data;
  is >> data;
  Descriptor desc(rhs.getDescriptor());
  memcpy(rhs.getPayloadPtr() + desc.offset(fieldName), &data, sizeof(T));
}

Descriptor payloadAccessor::getDescriptor() const { return descriptor; }

std::byte *payloadAccessor::getPayloadPtr() const { return ptr; }

void payloadAccessor::setItem(int position, std::any value) {
  auto fieldName = descriptor.fieldName(position);
  auto len = descriptor.len(fieldName);
  if (descriptor.type(fieldName) == "STRING") {
    std::string data(std::any_cast<std::string>(value));
    memcpy(ptr + descriptor.offset(position), data.c_str(), std::min(len, static_cast<uint>(data.length())));
    SPDLOG_INFO("setItem {} string:{}", position, data);
  } else if (descriptor.type(fieldName) == "BYTE") {
    unsigned char data(std::any_cast<unsigned char>(value));
    memcpy(ptr + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} char:{}", position, data);
  } else if (descriptor.type(fieldName) == "BYTEARRAY") {
    std::vector<unsigned char> data(std::any_cast<std::vector<unsigned char>>(value));
    memcpy(ptr + descriptor.offset(position), &data, len);  //- check & todo
    SPDLOG_INFO("setItem {} bytearray", position);
  } else if (descriptor.type(fieldName) == "INTEGER") {
    int data(std::any_cast<int>(value));
    memcpy(ptr + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} int:{}", position, data);
  } else if (descriptor.type(fieldName) == "INTARRAY") {
    std::vector<int> data(std::any_cast<std::vector<int>>(value));
    memcpy(ptr + descriptor.offset(position), &data, len);  //- check & todo
    SPDLOG_INFO("setItem {} intarray", position);
  } else if (descriptor.type(fieldName) == "UINT") {
    uint data(std::any_cast<uint>(value));
    memcpy(ptr + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} uint:{}", position, data);
  } else if (descriptor.type(fieldName) == "DOUBLE") {
    double data(std::any_cast<double>(value));
    memcpy(ptr + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} double:{}", position, data);
  } else if (descriptor.type(fieldName) == "FLOAT") {
    float data(std::any_cast<float>(value));
    memcpy(ptr + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} float:{}", position, data);
  } else {
    SPDLOG_ERROR("Type not supported.");
    assert(false && "type not supported on setter");
  }
}

std::any payloadAccessor::getItem(int position) {
  auto fieldName = descriptor.fieldName(position);
  auto len = descriptor.len(fieldName);
  std::cerr << "set:" << fieldName << std::endl;
  if (descriptor.type(fieldName) == "STRING") {
    std::string retval;
    retval.assign(reinterpret_cast<char *>(ptr) + descriptor.offset(position), len);
    SPDLOG_INFO("getItem {} string:{}", position, retval);
    return retval;
  } else if (descriptor.type(fieldName) == "BYTEARRAY") {
    std::vector<unsigned char> data;
    for (auto i = 0; i < descriptor.len(fieldName); i++) {
      unsigned char data8 = *reinterpret_cast<unsigned char *>(reinterpret_cast<char *>(ptr) + descriptor.offset(position) + i);
      data.push_back(data8);
    }
    SPDLOG_INFO("getItem {} bytearray", position);
    return data;
  } else if (descriptor.type(fieldName) == "BYTE") {
    unsigned char data;
    data = *reinterpret_cast<unsigned char *>(reinterpret_cast<char *>(ptr) + descriptor.offset(position));
    SPDLOG_INFO("getItem {} byte:{}", position, data);
    return data;
  } else if (descriptor.type(fieldName) == "INTARRAY") {
    std::vector<int> data;
    for (auto i = 0; i < descriptor.len(fieldName); i++) {
      int dataInt = *reinterpret_cast<int *>(reinterpret_cast<int *>(ptr) + descriptor.offset(position) + i);
      data.push_back(dataInt);
    }
    SPDLOG_INFO("getItem {} intarray", position);
    return data;
  } else if (descriptor.type(fieldName) == "INTEGER") {
    int data;
    data = *reinterpret_cast<int *>(reinterpret_cast<int *>(ptr) + descriptor.offset(position));
    SPDLOG_INFO("getItem {} int:{}", position, data);
    return data;
  } else if (descriptor.type(fieldName) == "UINT") {
    uint data;
    data = *reinterpret_cast<uint *>(reinterpret_cast<uint *>(ptr) + descriptor.offset(position));
    SPDLOG_INFO("getItem {} uint:{}", position, data);
    return data;
  } else if (descriptor.type(fieldName) == "DOUBLE") {
    double data;
    data = *reinterpret_cast<double *>(reinterpret_cast<double *>(ptr) + descriptor.offset(position));
    SPDLOG_INFO("getItem {} double:{}", position, data);
    return data;
  } else if (descriptor.type(fieldName) == "FLOAT") {
    float data;
    SPDLOG_INFO("getItem {} float:{}", position, data);
    data = *reinterpret_cast<double *>(reinterpret_cast<float *>(ptr) + descriptor.offset(position));
    return data;
  }
  SPDLOG_ERROR("Type not supported.");
  assert(false && "type not supporte on getter.");
  return 0xdead;
}

std::istream &operator>>(std::istream &is, const payloadAccessor &rhs) {
  std::string fieldName;
  is >> fieldName;
  if (is.eof()) return is;
  if (rhs.hexFormat)
    is >> std::hex;
  else
    is >> std::dec;
  Descriptor desc(rhs.getDescriptor());
  if (desc.type(fieldName) == "STRING") {
    std::string record;
    std::getline(is >> std::ws, record);
    memset(rhs.getPayloadPtr() + desc.offset(fieldName), 0, desc.len(fieldName));
    memcpy(rhs.getPayloadPtr() + desc.offset(fieldName), record.c_str(), std::min((size_t)desc.len(fieldName), record.size()));
  } else if (desc.type(fieldName) == "BYTEARRAY") {
    for (auto i = 0; i < desc.len(fieldName); i++) {
      int data;
      is >> data;
      unsigned char data8 = static_cast<unsigned char>(data);
      memcpy(rhs.getPayloadPtr() + desc.offset(fieldName) + i * sizeof(unsigned char), &data8, sizeof(unsigned char));
    }
  } else if (desc.type(fieldName) == "INTARRAY") {
    for (auto i = 0; i < desc.len(fieldName) / sizeof(int); i++) {
      int data;
      is >> data;
      memcpy(rhs.getPayloadPtr() + desc.offset(fieldName) + i * sizeof(int), &data, sizeof(int));
    }
  } else if (desc.type(fieldName) == "BYTE") {
    int data;
    is >> data;
    unsigned char data8 = static_cast<unsigned char>(data);
    memcpy(rhs.getPayloadPtr() + desc.offset(fieldName), &data8, sizeof(unsigned char));
  } else if (desc.type(fieldName) == "UINT")
    copyToMemory<uint, payloadAccessor>(is, rhs, fieldName.c_str());
  else if (desc.type(fieldName) == "INTEGER")
    copyToMemory<int, payloadAccessor>(is, rhs, fieldName.c_str());
  else if (desc.type(fieldName) == "FLOAT")
    copyToMemory<float, payloadAccessor>(is, rhs, fieldName.c_str());
  else if (desc.type(fieldName) == "DOUBLE")
    copyToMemory<double, payloadAccessor>(is, rhs, fieldName.c_str());
  else
    std::cerr << "field not found\n";
  return is;
}

std::ostream &operator<<(std::ostream &os, const payloadAccessor &rhs) {
  os << "{";
  if (rhs.hexFormat)
    os << std::hex;
  else
    os << std::dec;
  for (auto const &r : rhs.getDescriptor()) {
    os << "\t" << std::get<rname>(r) << ":";
    auto desc = rhs.getDescriptor();
    auto offset_ = desc.offset(std::get<rname>(r));
    if (std::get<rtype>(r) == STRING) {
      os << std::string(reinterpret_cast<char *>(rhs.getPayloadPtr() + offset_), desc.len(std::get<rname>(r)));
    } else if (std::get<rtype>(r) == rdb::BYTEARRAY) {
      for (auto i = 0; i < std::get<rlen>(r); i++) {
        unsigned char data;
        memcpy(&data, rhs.getPayloadPtr() + offset_ + i * sizeof(unsigned char), sizeof(unsigned char));
        if (rhs.hexFormat) {
          os << std::setfill('0');
          os << std::setw(2);
        }
        os << (int)data;
        if (i != std::get<rlen>(r)) os << " ";
      }
    } else if (std::get<rtype>(r) == rdb::INTARRAY) {
      for (auto i = 0; i < std::get<rlen>(r) / sizeof(int); i++) {
        int data;
        memcpy(&data, rhs.getPayloadPtr() + offset_ + i * sizeof(int), sizeof(int));
        if (rhs.hexFormat) {
          os << std::setfill('0');
          os << std::setw(8);
        }
        os << (int)data;
        if (i != std::get<rlen>(r)) os << " ";
      }
    } else if (std::get<rtype>(r) == rdb::BYTE) {
      unsigned char data;
      memcpy(&data, rhs.getPayloadPtr() + offset_, sizeof(unsigned char));
      if (rhs.hexFormat) {
        os << std::setfill('0');
        os << std::setw(2);
      }
      os << (int)data;
    } else if (std::get<rtype>(r) == rdb::INTEGER) {
      int data;
      memcpy(&data, rhs.getPayloadPtr() + offset_, sizeof(int));
      if (rhs.hexFormat) {
        os << std::setfill('0');
        os << std::setw(8);
      }
      os << data;
    } else if (std::get<rtype>(r) == rdb::UINT) {
      unsigned int data;
      memcpy(&data, rhs.getPayloadPtr() + offset_, sizeof(unsigned int));
      if (rhs.hexFormat) {
        os << std::setfill('0');
        os << std::setw(8);
      }
      os << data;
    } else if (std::get<rtype>(r) == rdb::FLOAT) {
      float data;
      memcpy(&data, rhs.getPayloadPtr() + offset_, sizeof(float));
      os << data;
    } else if (std::get<rtype>(r) == rdb::DOUBLE) {
      double data;
      memcpy(&data, rhs.getPayloadPtr() + offset_, sizeof(double));
      os << data;
    } else
      assert(false && "Unrecognized type");
    os << std::endl;
  }
  os << "}";
  if (rhs.getDescriptor().isDirty()) os << "[dirty]";
  return os;
}

}  // namespace rdb
