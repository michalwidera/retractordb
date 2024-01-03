#include "rdb/payload.h"

#include <spdlog/spdlog.h>

#include <algorithm>  // std::min
#include <boost/rational.hpp>
#include <cassert>
#include <cstring>  // std:memcpy
#include <iomanip>
#include <iostream>
#include <numeric>  // itoa

#include "rdb/convertTypes.h"

namespace rdb {

// default constructor

payload::payload(const Descriptor &descriptor)
    : descriptor(descriptor),  //
      hexFormat(false) {
  payloadData = std::make_unique<uint8_t[]>(descriptor.getSizeInBytes());
  std::memset(payloadData.get(), 0, descriptor.getSizeInBytes());
}

// copy constructor

payload::payload(const payload &other) {
  payloadData = std::make_unique<uint8_t[]>(other.descriptor.getSizeInBytes());
  descriptor  = other.getDescriptor();
  std::memcpy(get(), other.get(), other.descriptor.getSizeInBytes());
}

// Copy & assignment operator

payload &payload::operator=(const payload &other) {
  if (this == &other) return *this;  // assure not a self-assignment

  *this = other.getDescriptor();  // call operator=(const Descriptor
  std::memcpy(get(), other.get(), other.descriptor.getSizeInBytes());
  return *this;
}

// Special init operator

payload &payload::operator=(const Descriptor &other) {
  // TODO: - create assignment operator, cover with test
  // * Plan: when descriptor is empty =Descriptor or =payload
  // * will create descriptor or descriptor with payload
  // * if non empty - this goes strange
  if (descriptor.size() == 0) {
    // default descriptor constructor (=default) has been used and descriptor is empty and ready to assign.
    descriptor  = other;
    payloadData = std::make_unique<uint8_t[]>(other.getSizeInBytes());
  } else {
    if (descriptor == other) {  // compare rlen and rtype only here
      // descriptor = other; <- Just change field names - descriptor remains the same, payload remains the same
      // pass
    } else
      assert(false && "Descriptor should be empty before this assign.");
  }
  return *this;
}

// Math operation operators

payload payload::operator+(const payload &other) {
  rdb::Descriptor descSum(descriptor);
  descSum += other.getDescriptor();               // ! moving this into constructor fails
  descSum.cleanRef();                             //
  payload result(descSum);                        //
  SPDLOG_INFO("operator+ {} {} {}",               //
              descriptor.getSizeInBytes(),        //
              other.descriptor.getSizeInBytes(),  //
              result.getDescriptor().getSizeInBytes());
  std::memcpy(result.get(), get(), descriptor.getSizeInBytes());
  std::memcpy(result.get() + descriptor.getSizeInBytes(), other.get(), other.descriptor.getSizeInBytes());
  descSum.dirtyMap = true;
  return result;
}

// Member Functions

template <typename T, typename K>
void copyToMemory(std::istream &is, const K &rhs, const char *fieldName, int arroffset) {
  T data;
  is >> data;
  Descriptor desc(rhs.getDescriptor());
  std::memcpy(rhs.get() + desc.offsetBegArr(fieldName) + arroffset, &data, sizeof(T));
}

void payload::setHex(bool hexFormatVal) { hexFormat = hexFormatVal; }

Descriptor payload::getDescriptor() const { return descriptor; }

uint8_t *payload::get() const { return payloadData.get(); }

template <typename T>
void payload::setItemBy(const int positionFlat, std::any value) {
  T data          = std::any_cast<T>(value);
  auto position   = descriptor.convert(positionFlat).value().first;
  auto offsetFlat = descriptor.offset(positionFlat);
  std::memcpy(payloadData.get() + offsetFlat, &data, std::get<rlen>(descriptor[position]));
}

void payload::setItem(const int positionFlat, std::any valueParam) {
  if (positionFlat > descriptor.sizeFlat() - 1) {
    SPDLOG_ERROR("Write out of descriptor - req:{} available len: {}", positionFlat, descriptor.sizeFlat());
    assert(false && "setItem - Write out of descriptor");
    abort();
  }

  auto position      = descriptor.convert(positionFlat).value().first;
  auto requestedType = std::get<rtype>(descriptor[position]);

  cast<std::any> castAny;
  std::any value = castAny(valueParam, requestedType);

  try {
    switch (requestedType) {
      case rdb::STRING: {
        const auto len = std::get<rlen>(descriptor[position]) * std::get<rarray>(descriptor[position]);
        std::string data(std::any_cast<std::string>(value));
        auto lenr       = std::min(len, static_cast<int>(data.length()));
        auto destOffset = descriptor.offset(positionFlat);
        auto addr       = payloadData.get() + destOffset;
        assert(position + len <= descriptor.getSizeInBytes());
        std::memcpy(addr, data.c_str(), lenr);
        if (lenr != len) addr[lenr] = '\0';
      } break;
      case rdb::BYTE:
        setItemBy<uint8_t>(positionFlat, value);
        break;
      case rdb::INTEGER:
        setItemBy<int>(positionFlat, value);
        break;
      case rdb::UINT:
        setItemBy<unsigned>(positionFlat, value);
        break;
      case rdb::DOUBLE:
        setItemBy<double>(positionFlat, value);
        break;
      case rdb::FLOAT:
        setItemBy<float>(positionFlat, value);
        break;
      case rdb::RATIONAL:
        setItemBy<boost::rational<int>>(positionFlat, value);
        break;
      case rdb::REF: {
        SPDLOG_INFO("Skip REF");
      } break;
      case rdb::TYPE: {
        SPDLOG_INFO("Skip TYPE");
      } break;
      default: {
        SPDLOG_ERROR("Type not supported: {}", (int)requestedType);
        assert(false && "setItem - Type not supported.");
        abort();
      }
    }
  } catch (const std::bad_any_cast &e) {
    SPDLOG_ERROR("Error on payload::setItem");
  }
}

template <typename T>
T getVal(void *ptr, int offset) {
  return *(reinterpret_cast<T *>(reinterpret_cast<std::uint8_t *>(ptr) + offset));
}

std::any payload::getItem(const int positionFlat) {
  if (positionFlat > descriptor.sizeFlat() - 1) {
    SPDLOG_ERROR("Read out of descriptor req:{} available len: {}", positionFlat, descriptor.sizeFlat());
    assert(false && "getItem - Read out of descriptor");
    abort();
  }

  auto position = descriptor.convert(positionFlat).value().first;

  if (position > descriptor.sizeFlat() - 1) {
    SPDLOG_ERROR("Read out of descriptor req:{} available len: {}", position, descriptor.sizeFlat());
    assert(false && "getItem - Read out of descriptor");
    abort();
  }

  // The aim of this procedure is : get raw data from descriptor and return as std::any

  switch (std::get<rtype>(descriptor[position])) {
    case rdb::STRING: {
      auto len       = std::get<rlen>(descriptor[position]) * std::get<rarray>(descriptor[position]);
      char *charData = reinterpret_cast<char *>(payloadData.get()) + descriptor.offset(positionFlat);

      auto descLen  = descriptor.getSizeInBytes();
      auto startPos = descriptor.offset(positionFlat);
      assert(startPos + len <= descLen);
      for (auto i = 0; i < len; i++)
        if (charData[i] == 0) {
          len = i;
          break;
        }
      std::string s(charData, len);

      return s;
    }
    case rdb::BYTE: {
      uint8_t data = getVal<uint8_t>(payloadData.get(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::INTEGER: {
      int data = getVal<int>(payloadData.get(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::UINT: {
      uint data = getVal<uint>(payloadData.get(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::DOUBLE: {
      double data = getVal<double>(payloadData.get(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::FLOAT: {
      float data = getVal<float>(payloadData.get(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::RATIONAL: {
      boost::rational<int> data = getVal<boost::rational<int>>(payloadData.get(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::REF: {
      SPDLOG_ERROR("REF not supported.");
      return 0xdead;
    }
    case rdb::TYPE: {
      SPDLOG_ERROR("TYPE not supported.");
      return 0xdead;
    }
  };
  SPDLOG_ERROR("Type not supported. {}", int(std::get<rtype>(descriptor[position])));
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
    std::memset(rhs.get() + desc.offsetBegArr(fieldName), 0, desc.len(fieldName));
    std::memcpy(rhs.get() + desc.offsetBegArr(fieldName), record.c_str(), std::min((size_t)desc.len(fieldName), record.size()));
  } else
    for (auto i = 0; i < desc.arraySize(fieldName); i++) {
      if (desc.type(fieldName) == "BYTE") {
        int data;
        is >> data;
        uint8_t data8 = static_cast<uint8_t>(data);
        std::memcpy(rhs.get() + desc.offsetBegArr(fieldName) + i * sizeof(uint8_t), &data8, sizeof(uint8_t));
      } else if (desc.type(fieldName) == "UINT")
        copyToMemory<uint, payload>(is, rhs, fieldName.c_str(), i * sizeof(unsigned));
      else if (desc.type(fieldName) == "INTEGER")
        copyToMemory<int, payload>(is, rhs, fieldName.c_str(), i * sizeof(int));
      else if (desc.type(fieldName) == "FLOAT")
        copyToMemory<float, payload>(is, rhs, fieldName.c_str(), i * sizeof(float));
      else if (desc.type(fieldName) == "DOUBLE")
        copyToMemory<double, payload>(is, rhs, fieldName.c_str(), i * sizeof(double));
      else if (desc.type(fieldName) == "REF")
        SPDLOG_ERROR("REF store not supported by this operator.");
      else if (desc.type(fieldName) == "TYPE")
        SPDLOG_ERROR("TYPE store not supported by this operator.");
      else
        SPDLOG_ERROR("field {} not found", fieldName);
    }
  return is;
}

std::ostream &operator<<(std::ostream &os, const payload &rhs) {
  if (rhs.specialDebug) {
    os << "[ ";
    for (auto i = 0; i < rhs.getDescriptor().getSizeInBytes(); i++) {
      os << std::hex;
      os << std::setfill('0');
      os << std::setw(2);
      os << static_cast<int>(*(rhs.get() + i));
      os << " ";
    }
    os << "]";
    return os;
  }

  if (rhs.hexFormat)
    os << std::hex;
  else
    os << std::dec;
  os << "{";

  for (auto const &r : rhs.getDescriptor()) {
    if ((std::get<rtype>(r) == rdb::TYPE) || (std::get<rtype>(r) == rdb::REF)) break;
    if (!getFlat())
      os << "\t";
    else
      os << " ";
    os << std::get<rname>(r);
    os << ":";
    auto desc    = rhs.getDescriptor();
    auto offset_ = desc.offsetBegArr(std::get<rname>(r));
    if (std::get<rtype>(r) == STRING) {
      char *charData = reinterpret_cast<char *>(rhs.get() + offset_);
      auto len       = desc.len(std::get<rname>(r));
      for (auto i = 0; i < len; i++)
        if (charData[i] == 0) {
          len = i;
          break;
        }

      os << std::string(charData, len);
    } else
      for (auto i = 0; i < std::get<rarray>(r); i++) {
        if (std::get<rtype>(r) == rdb::BYTE) {
          uint8_t data{0};
          std::memcpy(&data, rhs.get() + offset_ + i * sizeof(uint8_t), sizeof(uint8_t));
          if (rhs.hexFormat) {
            os << std::setfill('0');
            os << std::setw(2);
          }
          os << (int)data;
        } else if (std::get<rtype>(r) == rdb::INTEGER) {
          int data{0};
          std::memcpy(&data, rhs.get() + offset_ + i * sizeof(int), sizeof(int));
          if (rhs.hexFormat) {
            os << std::setfill('0');
            os << std::setw(8);
          }
          os << data;
        } else if (std::get<rtype>(r) == rdb::UINT) {
          unsigned int data{0};
          std::memcpy(&data, rhs.get() + offset_ + i * sizeof(unsigned), sizeof(unsigned int));
          if (rhs.hexFormat) {
            os << std::setfill('0');
            os << std::setw(8);
          }
          os << data;
        } else if (std::get<rtype>(r) == rdb::FLOAT) {
          float data{0};
          std::memcpy(&data, rhs.get() + offset_ + i * sizeof(float), sizeof(float));
          os << data;
        } else if (std::get<rtype>(r) == rdb::DOUBLE) {
          double data{0};
          std::memcpy(&data, rhs.get() + offset_ + i * sizeof(double), sizeof(double));
          os << data;
        } else
          assert(false && "Unrecognized type");

        if (i < std::get<rarray>(r) - 1) os << " ";
      }
    if (!getFlat()) os << std::endl;
  }
  if (rhs.getDescriptor().isEmpty()) {
    os << "Empty";
    SPDLOG_ERROR("Empty descriptor on payload.");
  }
  if (getFlat()) os << " ";
  os << "}";
  if (!getFlat()) os << std::endl;
  setFlat(false);
  return os;
}

}  // namespace rdb
