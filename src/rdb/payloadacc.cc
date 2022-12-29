#include "rdb/payloadacc.h"

#include <algorithm>  // std::min
#include <cassert>
#include <cstddef>  // std::byte
#include <cstring>  // std:memcpy
#include <iomanip>
#include <iostream>
namespace rdb {
template <typename T>
payLoadAccessor<T>::payLoadAccessor(Descriptor descriptor, T *ptr,
                                    bool hexFormat)
    : descriptor(descriptor), ptr(ptr), hexFormat(hexFormat) {}

template <typename T, typename K>
void copyToMemory(std::istream &is, const K &rhs, const char *fieldName) {
  T data;
  is >> data;
  Descriptor desc(rhs.getDescriptor());
  memcpy(rhs.getPayloadPtr() + desc.Offset(fieldName), &data, sizeof(T));
}

template <typename T>
Descriptor payLoadAccessor<T>::getDescriptor() const {
  return descriptor;
}

template <typename T>
T *payLoadAccessor<T>::getPayloadPtr() const {
  return ptr;
}

template <typename T>
void payLoadAccessor<T>::set_item(int position, std::any value) {
  auto fieldName = descriptor.fieldName(position);
  auto len = descriptor.Len(fieldName);
  if (descriptor.Type(fieldName) == "STRING") {
    std::string data(std::any_cast<std::string>(value));
    memcpy(ptr + descriptor.Offset(position), data.c_str(),
           std::min(len, static_cast<uint>(data.length())));
    return;
  } else if (descriptor.Type(fieldName) == "BYTE") {
    unsigned char data(std::any_cast<unsigned char>(value));
    memcpy(ptr + descriptor.Offset(position), &data, len);
    return;
  } else if (descriptor.Type(fieldName) == "BYTEARRAY") {
    std::vector<unsigned char> data(
        std::any_cast<std::vector<unsigned char>>(value));
    memcpy(ptr + descriptor.Offset(position), &data, len);  //- check & todo
    return;
  } else if (descriptor.Type(fieldName) == "INTEGER") {
    int data(std::any_cast<int>(value));
    memcpy(ptr + descriptor.Offset(position), &data, len);
    return;
  } else if (descriptor.Type(fieldName) == "INTARRAY") {
    std::vector<int> data(std::any_cast<std::vector<int>>(value));
    memcpy(ptr + descriptor.Offset(position), &data, len);  //- check & todo
    return;
  } else if (descriptor.Type(fieldName) == "UINT") {
    uint data(std::any_cast<uint>(value));
    memcpy(ptr + descriptor.Offset(position), &data, len);
    return;
  } else if (descriptor.Type(fieldName) == "DOUBLE") {
    double data(std::any_cast<double>(value));
    memcpy(ptr + descriptor.Offset(position), &data, len);
    return;
  } else if (descriptor.Type(fieldName) == "FLOAT") {
    float data(std::any_cast<float>(value));
    memcpy(ptr + descriptor.Offset(position), &data, len);
    return;
  }
  assert(false && "type not supported on setter");
}

template <typename T>
std::any payLoadAccessor<T>::get_item(int position) {
  auto fieldName = descriptor.fieldName(position);
  auto len = descriptor.Len(fieldName);
  std::cerr << "set:" << fieldName << std::endl;
  if (descriptor.Type(fieldName) == "STRING") {
    std::string retval;
    retval.assign(reinterpret_cast<char *>(ptr) + descriptor.Offset(position),
                  len);
    return retval;
  } else if (descriptor.Type(fieldName) == "BYTEARRAY") {
    std::vector<unsigned char> data;
    for (auto i = 0; i < descriptor.Len(fieldName); i++) {
      unsigned char data8 = *reinterpret_cast<unsigned char *>(
          reinterpret_cast<char *>(ptr) + descriptor.Offset(position) + i);
      data.push_back(data8);
    }
    return data;
  } else if (descriptor.Type(fieldName) == "BYTE") {
    unsigned char data;
    data = *reinterpret_cast<unsigned char *>(reinterpret_cast<char *>(ptr) +
                                              descriptor.Offset(position));
    return data;
  } else if (descriptor.Type(fieldName) == "INTARRAY") {
    std::vector<int> data;
    for (auto i = 0; i < descriptor.Len(fieldName); i++) {
      int dataInt = *reinterpret_cast<int *>(reinterpret_cast<int *>(ptr) +
                                             descriptor.Offset(position) + i);
      data.push_back(dataInt);
    }
    return data;
  } else if (descriptor.Type(fieldName) == "INTEGER") {
    int data;
    data = *reinterpret_cast<int *>(reinterpret_cast<int *>(ptr) +
                                    descriptor.Offset(position));
    return data;
  } else if (descriptor.Type(fieldName) == "UINT") {
    uint data;
    data = *reinterpret_cast<uint *>(reinterpret_cast<uint *>(ptr) +
                                     descriptor.Offset(position));
    return data;
  } else if (descriptor.Type(fieldName) == "DOUBLE") {
    double data;
    data = *reinterpret_cast<double *>(reinterpret_cast<double *>(ptr) +
                                       descriptor.Offset(position));
    return data;
  } else if (descriptor.Type(fieldName) == "FLOAT") {
    float data;
    data = *reinterpret_cast<double *>(reinterpret_cast<float *>(ptr) +
                                       descriptor.Offset(position));
    return data;
  }
  assert(false && "type not supporte on getter.");
  return 0xdead;
}

template <typename K>
std::istream &operator>>(std::istream &is, const payLoadAccessor<K> &rhs) {
  std::string fieldName;
  is >> fieldName;
  if (is.eof()) return is;
  if (rhs.hexFormat)
    is >> std::hex;
  else
    is >> std::dec;
  Descriptor desc(rhs.getDescriptor());
  if (desc.Type(fieldName) == "STRING") {
    std::string record;
    std::getline(is >> std::ws, record);
    memset(rhs.getPayloadPtr() + desc.Offset(fieldName), 0,
           desc.Len(fieldName));
    memcpy(rhs.getPayloadPtr() + desc.Offset(fieldName), record.c_str(),
           std::min((size_t)desc.Len(fieldName), record.size()));
  } else if (desc.Type(fieldName) == "BYTEARRAY") {
    for (auto i = 0; i < desc.Len(fieldName); i++) {
      int data;
      is >> data;
      unsigned char data8 = static_cast<unsigned char>(data);
      memcpy(rhs.getPayloadPtr() + desc.Offset(fieldName) +
                 i * sizeof(unsigned char),
             &data8, sizeof(unsigned char));
    }
  } else if (desc.Type(fieldName) == "INTARRAY") {
    for (auto i = 0; i < desc.Len(fieldName) / sizeof(int); i++) {
      int data;
      is >> data;
      memcpy(rhs.getPayloadPtr() + desc.Offset(fieldName) + i * sizeof(int),
             &data, sizeof(int));
    }
  } else if (desc.Type(fieldName) == "BYTE") {
    int data;
    is >> data;
    unsigned char data8 = static_cast<unsigned char>(data);
    memcpy(rhs.getPayloadPtr() + desc.Offset(fieldName), &data8,
           sizeof(unsigned char));
  } else if (desc.Type(fieldName) == "UINT")
    copyToMemory<uint, payLoadAccessor<K>>(is, rhs, fieldName.c_str());
  else if (desc.Type(fieldName) == "INTEGER")
    copyToMemory<int, payLoadAccessor<K>>(is, rhs, fieldName.c_str());
  else if (desc.Type(fieldName) == "FLOAT")
    copyToMemory<float, payLoadAccessor<K>>(is, rhs, fieldName.c_str());
  else if (desc.Type(fieldName) == "DOUBLE")
    copyToMemory<double, payLoadAccessor<K>>(is, rhs, fieldName.c_str());
  else
    std::cerr << "field not found\n";
  return is;
}

template <class K>
std::ostream &operator<<(std::ostream &os, const payLoadAccessor<K> &rhs) {
  os << "{";
  if (rhs.hexFormat)
    os << std::hex;
  else
    os << std::dec;
  for (auto const &r : rhs.getDescriptor()) {
    os << "\t" << std::get<rname>(r) << ":";
    auto desc = rhs.getDescriptor();
    auto offset_ = desc.Offset(std::get<rname>(r));
    if (std::get<rtype>(r) == STRING) {
      os << std::string(reinterpret_cast<char *>(rhs.getPayloadPtr() + offset_),
                        desc.Len(std::get<rname>(r)));
    } else if (std::get<rtype>(r) == rdb::BYTEARRAY) {
      for (auto i = 0; i < std::get<rlen>(r); i++) {
        unsigned char data;
        memcpy(&data, rhs.getPayloadPtr() + offset_ + i * sizeof(unsigned char),
               sizeof(unsigned char));
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
        memcpy(&data, rhs.getPayloadPtr() + offset_ + i * sizeof(int),
               sizeof(int));
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

template class payLoadAccessor<std::byte>;
template class payLoadAccessor<char>;
template class payLoadAccessor<unsigned char>;

template std::istream &operator>>(std::istream &is,
                                  const payLoadAccessor<std::byte> &rhs);
template std::istream &operator>>(std::istream &is,
                                  const payLoadAccessor<char> &rhs);
template std::istream &operator>>(std::istream &is,
                                  const payLoadAccessor<unsigned char> &rhs);

template std::ostream &operator<<(std::ostream &os,
                                  const payLoadAccessor<std::byte> &rhs);
template std::ostream &operator<<(std::ostream &os,
                                  const payLoadAccessor<char> &rhs);
template std::ostream &operator<<(std::ostream &os,
                                  const payLoadAccessor<unsigned char> &rhs);
}  // namespace rdb
