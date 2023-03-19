#include "rdb/payload.h"

#include <spdlog/spdlog.h>

#include <algorithm>  // std::min
#include <cassert>
#include <cstddef>  // std::byte
#include <cstring>  // std:memcpy
#include <iomanip>
#include <iostream>

namespace rdb {

// default constructor

payload::payload(Descriptor descriptor)
    : descriptor(descriptor),  //
      hexFormat(false) {
  payloadData = std::make_unique<std::byte[]>(descriptor.getSizeInBytes());
  std::memset(payloadData.get(), 0, descriptor.getSizeInBytes());
}

// copy constructor

payload::payload(const payload &other) {
  payloadData = std::make_unique<std::byte[]>(other.descriptor.getSizeInBytes());
  descriptor = other.getDescriptor();
  std::memcpy(get(), other.get(), other.descriptor.getSizeInBytes());
}

// Copy & assignment operator

payload &payload::operator=(payload &other) {
  if (this != &other) {                                // assure not a self-assignment
    if (other.descriptor.size() != descriptor.size())  // speed up memory management
      payloadData = std::make_unique<std::byte[]>(other.descriptor.getSizeInBytes());
    descriptor = other.getDescriptor();
    std::memcpy(get(), other.get(), other.descriptor.getSizeInBytes());
  }
  return *this;
}

payload &payload::operator=(const payload &other) {
  if (this != &other) {                                // assure not a self-assignment
    if (other.descriptor.size() != descriptor.size())  // speed up memory management
      payloadData = std::make_unique<std::byte[]>(other.descriptor.getSizeInBytes());
    descriptor = other.getDescriptor();
    std::memcpy(get(), other.get(), other.descriptor.getSizeInBytes());
  }
  return *this;
}

// Math operation operators

payload payload::operator+(payload &other) {
  rdb::Descriptor descSum(descriptor);
  descSum | other.getDescriptor();  // ! moving this into constructor fails
  payload result(descSum);
  SPDLOG_INFO("operator+ {} {} {}",               //
              descriptor.getSizeInBytes(),        //
              other.descriptor.getSizeInBytes(),  //
              result.getDescriptor().getSizeInBytes());
  std::memcpy(result.get(), get(), descriptor.getSizeInBytes());
  std::memcpy(result.get() + descriptor.getSizeInBytes(), other.get(), other.descriptor.getSizeInBytes());
  return result;
}

// Member Functions

template <typename T, typename K>
void copyToMemory(std::istream &is, const K &rhs, const char *fieldName) {
  T data;
  is >> data;
  Descriptor desc(rhs.getDescriptor());
  std::memcpy(rhs.get() + desc.offset(fieldName), &data, sizeof(T));
}

void payload::setHex(bool hexFormatVal) { hexFormat = hexFormatVal; }

Descriptor payload::getDescriptor() const { return descriptor; }

std::byte *payload::get() const { return payloadData.get(); }

void payload::setItem(int position, std::any value) {
  auto fieldName = descriptor.fieldName(position);
  auto len = descriptor.len(fieldName);
  if (position > descriptor.size()) abort();
  if (descriptor.type(fieldName) == "STRING") {
    std::string data(std::any_cast<std::string>(value));
    std::memcpy(payloadData.get() + descriptor.offset(position), data.c_str(), std::min(len, static_cast<int>(data.length())));
    SPDLOG_INFO("setItem {} string:{}", position, data);
  } else if (descriptor.type(fieldName) == "BYTE") {
    std::byte data;
    int dataint(std::any_cast<int>(value));
    data = (std::byte)dataint;
    std::memcpy(payloadData.get() + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} char:{}", position, data);
  } else if (descriptor.type(fieldName) == "BYTEARRAY") {
    std::vector<unsigned char> data(std::any_cast<std::vector<unsigned char>>(value));
    std::memcpy(payloadData.get() + descriptor.offset(position), &data, len);  //- check & todo
    SPDLOG_INFO("setItem {} bytearray", position);
  } else if (descriptor.type(fieldName) == "INTEGER") {
    int data(std::any_cast<int>(value));
    std::memcpy(payloadData.get() + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} int:{}", position, data);
  } else if (descriptor.type(fieldName) == "INTARRAY") {
    std::vector<int> data(std::any_cast<std::vector<int>>(value));
    std::memcpy(payloadData.get() + descriptor.offset(position), &data, len);  //- check & todo
    SPDLOG_INFO("setItem {} intarray", position);
  } else if (descriptor.type(fieldName) == "UINT") {
    uint data(std::any_cast<uint>(value));
    std::memcpy(payloadData.get() + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} uint:{}", position, data);
  } else if (descriptor.type(fieldName) == "DOUBLE") {
    double data(std::any_cast<double>(value));
    std::memcpy(payloadData.get() + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} double:{}", position, data);
  } else if (descriptor.type(fieldName) == "FLOAT") {
    float data(std::any_cast<float>(value));
    std::memcpy(payloadData.get() + descriptor.offset(position), &data, len);
    SPDLOG_INFO("setItem {} float:{}", position, data);
  } else if (descriptor.type(fieldName) == "REF") {
    SPDLOG_INFO("Skip REF");
  } else if (descriptor.type(fieldName) == "TYPE") {
    SPDLOG_INFO("Skip TYPE");
  } else {
    SPDLOG_ERROR("Type not supported.");
    assert(false && "type not supported on setter");
  }
}
template <typename T>
T getVal(void *ptr, int offset) {
  return *(reinterpret_cast<T *>(static_cast<std::byte *>(ptr) + offset));
}

std::any payload::getItem(int position) {
  auto fieldName = descriptor.fieldName(position);
  auto len = descriptor.len(fieldName);
  if (position > descriptor.size()) abort();
  if (descriptor.type(fieldName) == "STRING") {
    std::string retval;
    retval.assign(reinterpret_cast<char *>(payloadData.get()) + descriptor.offset(position), len);
    SPDLOG_INFO("getItem {} string:{}", position, retval);
    return retval;
  } else if (descriptor.type(fieldName) == "BYTEARRAY") {
    std::vector<uint8_t> data;
    for (auto i = 0; i < descriptor.len(fieldName); i++) {
      uint8_t data8 = getVal<uint8_t>(payloadData.get(), descriptor.offset(position) + i);
      data.push_back(data8);
    }
    SPDLOG_INFO("getItem {} bytearray", position);
    return data;
  } else if (descriptor.type(fieldName) == "BYTE") {
    uint8_t data = getVal<uint8_t>(payloadData.get(), descriptor.offset(position));
    SPDLOG_INFO("getItem {} byte:{}", position, data);
    return data;
  } else if (descriptor.type(fieldName) == "INTARRAY") {
    std::vector<int> data;
    for (auto i = 0; i < descriptor.len(fieldName); i++) {
      int dataInt = getVal<int>(payloadData.get(), descriptor.offset(position) + i);
      data.push_back(dataInt);
    }
    SPDLOG_INFO("getItem {} intarray", position);
    return data;
  } else if (descriptor.type(fieldName) == "INTEGER") {
    int data = getVal<int>(payloadData.get(), descriptor.offset(position));
    SPDLOG_INFO("getItem {} int:{} offset:{}", position, data, descriptor.offset(position));
    return data;
  } else if (descriptor.type(fieldName) == "UINT") {
    uint data = getVal<uint>(payloadData.get(), descriptor.offset(position));
    SPDLOG_INFO("getItem {} uint:{}", position, data);
    return data;
  } else if (descriptor.type(fieldName) == "DOUBLE") {
    double data = getVal<double>(payloadData.get(), descriptor.offset(position));
    SPDLOG_INFO("getItem {} double:{}", position, data);
    return data;
  } else if (descriptor.type(fieldName) == "FLOAT") {
    float data = getVal<float>(payloadData.get(), descriptor.offset(position));
    SPDLOG_INFO("getItem {} float:{}", position, data);
    return data;
  } else if (descriptor.type(fieldName) == "REF") {
    SPDLOG_ERROR("REF not supported.");
    return 0xdead;
  } else if (descriptor.type(fieldName) == "TYPE") {
    SPDLOG_ERROR("TYPE not supported.");
    return 0xdead;
  }
  SPDLOG_ERROR("Type not supported.");
  assert(false && "type not supported on getter.");
  return 0xdead;
}

// Friend operators

std::istream &operator>>(std::istream &is, const payload &rhs) {
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
    // std::getline(is >> std::ws, record);
    is >> record;
    std::memset(rhs.get() + desc.offset(fieldName), 0, desc.len(fieldName));
    std::memcpy(rhs.get() + desc.offset(fieldName), record.c_str(), std::min((size_t)desc.len(fieldName), record.size()));
  } else if (desc.type(fieldName) == "BYTEARRAY") {
    for (auto i = 0; i < desc.len(fieldName); i++) {
      int data;
      is >> data;
      unsigned char data8 = static_cast<unsigned char>(data);
      std::memcpy(rhs.get() + desc.offset(fieldName) + i * sizeof(unsigned char), &data8, sizeof(unsigned char));
    }
  } else if (desc.type(fieldName) == "INTARRAY") {
    for (auto i = 0; i < desc.len(fieldName) / sizeof(int); i++) {
      int data;
      is >> data;
      std::memcpy(rhs.get() + desc.offset(fieldName) + i * sizeof(int), &data, sizeof(int));
    }
  } else if (desc.type(fieldName) == "BYTE") {
    int data;
    is >> data;
    unsigned char data8 = static_cast<unsigned char>(data);
    std::memcpy(rhs.get() + desc.offset(fieldName), &data8, sizeof(unsigned char));
  } else if (desc.type(fieldName) == "UINT")
    copyToMemory<uint, payload>(is, rhs, fieldName.c_str());
  else if (desc.type(fieldName) == "INTEGER")
    copyToMemory<int, payload>(is, rhs, fieldName.c_str());
  else if (desc.type(fieldName) == "FLOAT")
    copyToMemory<float, payload>(is, rhs, fieldName.c_str());
  else if (desc.type(fieldName) == "DOUBLE")
    copyToMemory<double, payload>(is, rhs, fieldName.c_str());
  else if (desc.type(fieldName) == "REF")
    SPDLOG_ERROR("REF store not supported by this operator.");
  else if (desc.type(fieldName) == "TYPE")
    SPDLOG_ERROR("TYPE store not supported by this operator.");
  else
    SPDLOG_ERROR("field {} not found", fieldName);
  return is;
}

std::ostream &operator<<(std::ostream &os, const payload &rhs) {
  if (rhs.hexFormat)
    os << std::hex;
  else
    os << std::dec;
  for (auto const &r : rhs.getDescriptor()) {
    if ((std::get<rtype>(r) == rdb::TYPE) || (std::get<rtype>(r) == rdb::REF)) break;
    os << " " << std::get<rname>(r) << " ";
    auto desc = rhs.getDescriptor();
    auto offset_ = desc.offset(std::get<rname>(r));
    if (std::get<rtype>(r) == STRING) {
      os << std::string(reinterpret_cast<char *>(rhs.get() + offset_), desc.len(std::get<rname>(r)));
    } else if (std::get<rtype>(r) == rdb::BYTEARRAY) {
      for (auto i = 0; i < std::get<rlen>(r); i++) {
        unsigned char data;
        std::memcpy(&data, rhs.get() + offset_ + i * sizeof(unsigned char), sizeof(unsigned char));
        if (rhs.hexFormat) {
          os << std::setfill('0');
          os << std::setw(2);
        }
        os << (int)data;
        if (i + 1 != std::get<rlen>(r)) os << " ";
      }
    } else if (std::get<rtype>(r) == rdb::INTARRAY) {
      for (auto i = 0; i < std::get<rlen>(r) / sizeof(int); i++) {
        int data;
        std::memcpy(&data, rhs.get() + offset_ + i * sizeof(int), sizeof(int));
        if (rhs.hexFormat) {
          os << std::setfill('0');
          os << std::setw(8);
        }
        os << (int)data;
        if (i + 1 != std::get<rlen>(r) / sizeof(int)) os << " ";
      }
    } else if (std::get<rtype>(r) == rdb::BYTE) {
      unsigned char data;
      std::memcpy(&data, rhs.get() + offset_, sizeof(unsigned char));
      if (rhs.hexFormat) {
        os << std::setfill('0');
        os << std::setw(2);
      }
      os << (int)data;
    } else if (std::get<rtype>(r) == rdb::INTEGER) {
      int data;
      std::memcpy(&data, rhs.get() + offset_, sizeof(int));
      if (rhs.hexFormat) {
        os << std::setfill('0');
        os << std::setw(8);
      }
      os << data;
    } else if (std::get<rtype>(r) == rdb::UINT) {
      unsigned int data;
      std::memcpy(&data, rhs.get() + offset_, sizeof(unsigned int));
      if (rhs.hexFormat) {
        os << std::setfill('0');
        os << std::setw(8);
      }
      os << data;
    } else if (std::get<rtype>(r) == rdb::FLOAT) {
      float data;
      std::memcpy(&data, rhs.get() + offset_, sizeof(float));
      os << data;
    } else if (std::get<rtype>(r) == rdb::DOUBLE) {
      double data;
      std::memcpy(&data, rhs.get() + offset_, sizeof(double));
      os << data;
    } else
      assert(false && "Unrecognized type");

    os << std::endl;
  }
  if (rhs.getDescriptor().isEmpty()) {
    os << "Empty";
    SPDLOG_ERROR("Empty descriptor on payload.");
  }
  return os;
}

}  // namespace rdb
