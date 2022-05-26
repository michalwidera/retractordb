#include "rdb/payloadacc.h"

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
  } else if (desc.Type(fieldName) == "Bytearray") {
    for (auto i = 0; i < desc.Len(fieldName); i++) {
      int data;
      is >> data;
      unsigned char data8 = static_cast<unsigned char>(data);
      memcpy(rhs.getPayloadPtr() + desc.Offset(fieldName) +
                 i * sizeof(unsigned char),
             &data8, sizeof(unsigned char));
    }
  } else if (desc.Type(fieldName) == "Intarray") {
    for (auto i = 0; i < desc.Len(fieldName) / sizeof(int); i++) {
      int data;
      is >> data;
      memcpy(rhs.getPayloadPtr() + desc.Offset(fieldName) + i * sizeof(int),
             &data, sizeof(int));
    }
  } else if (desc.Type(fieldName) == "Byte") {
    int data;
    is >> data;
    unsigned char data8 = static_cast<unsigned char>(data);
    memcpy(rhs.getPayloadPtr() + desc.Offset(fieldName), &data8,
           sizeof(unsigned char));
  } else if (desc.Type(fieldName) == "Uint")
    copyToMemory<uint, payLoadAccessor<K>>(is, rhs, fieldName.c_str());
  else if (desc.Type(fieldName) == "Int")
    copyToMemory<int, payLoadAccessor<K>>(is, rhs, fieldName.c_str());
  else if (desc.Type(fieldName) == "Float")
    copyToMemory<float, payLoadAccessor<K>>(is, rhs, fieldName.c_str());
  else if (desc.Type(fieldName) == "Double")
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
      auto len_ = desc.Len(std::get<rname>(r));
      os << std::string(reinterpret_cast<char *>(rhs.getPayloadPtr() + offset_),
                        len_);
    } else if (std::get<rtype>(r) == Bytearray) {
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
    } else if (std::get<rtype>(r) == Intarray) {
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
    } else if (std::get<rtype>(r) == Byte) {
      unsigned char data;
      memcpy(&data, rhs.getPayloadPtr() + offset_, sizeof(unsigned char));
      if (rhs.hexFormat) {
        os << std::setfill('0');
        os << std::setw(2);
      }
      os << (int)data;
    } else if (std::get<rtype>(r) == Int) {
      int data;
      memcpy(&data, rhs.getPayloadPtr() + offset_, sizeof(int));
      if (rhs.hexFormat) {
        os << std::setfill('0');
        os << std::setw(8);
      }
      os << data;
    } else if (std::get<rtype>(r) == Uint) {
      unsigned int data;
      memcpy(&data, rhs.getPayloadPtr() + offset_, sizeof(unsigned int));
      if (rhs.hexFormat) {
        os << std::setfill('0');
        os << std::setw(8);
      }
      os << data;
    } else if (std::get<rtype>(r) == Float) {
      float data;
      memcpy(&data, rhs.getPayloadPtr() + offset_, sizeof(float));
      os << data;
    } else if (std::get<rtype>(r) == Double) {
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
