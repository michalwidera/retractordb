#include "rdb/payload.h"

#define BOOST_STACKTRACE_USE_BACKTRACE

#include <spdlog/spdlog.h>

#include <algorithm>  // std::min, std::copy, std::fill
#include <boost/rational.hpp>
#include <boost/stacktrace.hpp>
#include <cassert>
#include <cstring>  // std::memcpy (for C-interop)
#include <iomanip>
#include <iostream>
#include <sstream>

#include "rdb/convertTypes.h"

namespace rdb {

// default constructor

payload::payload(const Descriptor &descriptor)
    : descriptor(descriptor),  //
      hexFormat_(false) {
  payloadData_ = std::make_unique<uint8_t[]>(descriptor.getSizeInBytes());
  std::fill(span().begin(), span().end(), 0);

  nullBitset.resize(descriptor.size(), false);
}

// copy constructor

payload::payload(const payload &other) {
  descriptor   = other.descriptor;
  payloadData_ = std::make_unique<uint8_t[]>(other.descriptor.getSizeInBytes());
  std::copy(other.span().begin(), other.span().end(), span().begin());
  nullBitset = other.nullBitset;
}

// Copy & assignment operator

payload &payload::operator=(const payload &other) {
  if (this == &other) return *this;  // assure not a self-assignment

  *this = other.descriptor;  // call operator=(const Descriptor
  std::copy(other.span().begin(), other.span().end(), span().begin());
  nullBitset = other.nullBitset;
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
    descriptor   = other;
    payloadData_ = std::make_unique<uint8_t[]>(other.getSizeInBytes());
    std::fill(span().begin(), span().end(), 0);
    nullBitset.assign(descriptor.size(), false);
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
  descSum += other.descriptor;                    // ! moving this into constructor fails
  descSum.removeConfigurationFields();            //
  payload result(descSum);                        //
  SPDLOG_INFO("operator+ {} {} {}",               //
              descriptor.getSizeInBytes(),        //
              other.descriptor.getSizeInBytes(),  //
              result.descriptor.getSizeInBytes());
  std::copy(span().begin(), span().end(), result.span().begin());
  std::copy(other.span().begin(), other.span().end(), result.span().subspan(descriptor.getSizeInBytes()).begin());

  result.nullBitset.clear();
  result.nullBitset.reserve(result.descriptor.size());

  auto appendDataFieldNullFlags = [&result](const payload &src) {
    for (size_t i = 0; i < src.descriptor.size(); ++i) {
      auto type = src.descriptor[i].rtype;
      if (type == rdb::TYPE || type == rdb::REF || type == rdb::RETENTION || type == rdb::RETMEMORY) continue;
      result.nullBitset.push_back(i < src.nullBitset.size() ? src.nullBitset[i] : false);
    }
  };

  appendDataFieldNullFlags(*this);
  appendDataFieldNullFlags(other);

  return result;
}

// Member Functions

template <typename T, typename K>
void copyToMemory(std::istream &is, const K &rhs, const char *fieldName, int arroffset) {
  T data;
  is >> data;
  Descriptor desc(rhs.descriptor);
  auto dest = rhs.span().subspan(desc.offsetBegArr(fieldName) + arroffset, sizeof(T));
  std::memcpy(dest.data(), &data, sizeof(T));
}

void payload::setHex(bool hexFormatVal) { hexFormat_ = hexFormatVal; }

std::span<uint8_t> payload::span() const { return {payloadData_.get(), descriptor.getSizeInBytes()}; }

template <typename T>
void payload::setItemBy(const int positionFlat, std::optional<std::any> value) {
  T data          = std::any_cast<T>(value.value());
  auto position   = descriptor.convert(positionFlat).value().first;
  auto offsetFlat = descriptor.offset(positionFlat);
  auto dest       = span().subspan(offsetFlat, descriptor[position].rlen);
  std::memcpy(dest.data(), &data, descriptor[position].rlen);
}

void payload::setItem(const int positionFlat, std::optional<std::any> valueParam) {
  if (positionFlat < 0 || positionFlat > descriptor.flatElementCount() - 1) {
    SPDLOG_ERROR("Write out of descriptor - req:{} available len: {}", positionFlat, descriptor.flatElementCount());
    assert(false && "setItem - Write out of descriptor");
    abort();
  }

  auto positionOpt = descriptor.convert(positionFlat);
  if (!positionOpt.has_value()) {
    SPDLOG_ERROR("Write conversion failed for flat position {}", positionFlat);
    assert(false && "setItem - Flat conversion failed");
    abort();
  }

  auto position = positionOpt->first;
  if (position < 0 || position >= static_cast<int>(descriptor.size())) {
    SPDLOG_ERROR("Write converted index out of descriptor bounds: {}", position);
    assert(false && "setItem - Converted index out of range");
    abort();
  }

  auto requestedType = descriptor[position].rtype;

  if (!valueParam.has_value()) {
    nullBitset[position] = true;
    return;
  }

  nullBitset[position] = false;

  cast<std::any> castAny;
  std::any value = castAny(valueParam.value(), requestedType);

  try {
    switch (requestedType) {
      case rdb::STRING: {
        const auto len = descriptor[position].rlen * descriptor[position].rarray;
        std::string data(std::any_cast<std::string>(value));
        auto lenr       = std::min(len, static_cast<int>(data.length()));
        auto destOffset = descriptor.offset(positionFlat);
        auto dest       = span().subspan(destOffset, len);
        assert(position + len <= descriptor.getSizeInBytes());
        std::copy_n(data.c_str(), lenr, dest.begin());
        if (lenr != len) dest[lenr] = '\0';
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
      case rdb::RETENTION: {
        SPDLOG_INFO("Skip RETENTION");
      } break;
      case rdb::RETMEMORY: {
        SPDLOG_INFO("Skip RETMEMORY");
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
T getVal(std::span<uint8_t> s, int offset) {
  T val;
  std::memcpy(&val, s.subspan(offset, sizeof(T)).data(), sizeof(T));
  return val;
}

std::optional<std::any> payload::getItem(const int positionFlat) {
  if (positionFlat < 0 || positionFlat > descriptor.flatElementCount() - 1) {
    SPDLOG_ERROR("Read out of descriptor req:{} available len: {}", positionFlat, descriptor.flatElementCount());
    std::stringstream message;
    message << boost::stacktrace::stacktrace();
    SPDLOG_ERROR("Stack: {}", message.str());
    std::cerr << message.str() << std::endl;
    assert(false && "getItem - Read out of descriptor");
    abort();
  }

  auto positionOpt = descriptor.convert(positionFlat);
  if (!positionOpt.has_value()) {
    SPDLOG_ERROR("Read conversion failed for flat position {}", positionFlat);
    assert(false && "getItem - Flat conversion failed");
    abort();
  }

  auto position = positionOpt->first;
  if (position < 0 || position >= static_cast<int>(descriptor.size())) {
    SPDLOG_ERROR("Read converted index out of descriptor bounds: {}", position);
    assert(false && "getItem - Converted index out of range");
    abort();
  }

  if (nullBitset[position]) return std::nullopt;

  // The aim of this procedure is : get raw data from descriptor and return as std::any

  switch (descriptor[position].rtype) {
    case rdb::STRING: {
      auto len       = descriptor[position].rlen * descriptor[position].rarray;
      auto fieldSpan = span().subspan(descriptor.offset(positionFlat), len);

      auto descLen  = descriptor.getSizeInBytes();
      auto startPos = descriptor.offset(positionFlat);
      assert(startPos + len <= descLen);
      for (auto i = 0; i < len; i++)
        if (fieldSpan[i] == 0) {
          len = i;
          break;
        }
      std::string s(fieldSpan.begin(), fieldSpan.begin() + len);

      return s;
    }
    case rdb::BYTE: {
      uint8_t data = getVal<uint8_t>(span(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::INTEGER: {
      int data = getVal<int>(span(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::UINT: {
      uint data = getVal<uint>(span(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::DOUBLE: {
      double data = getVal<double>(span(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::FLOAT: {
      float data = getVal<float>(span(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::RATIONAL: {
      boost::rational<int> data = getVal<boost::rational<int>>(span(), descriptor.offset(positionFlat));
      return data;
    }
    case rdb::REF: {
      SPDLOG_ERROR("REF not supported.");
      return std::nullopt;
    }
    case rdb::TYPE: {
      SPDLOG_ERROR("TYPE not supported.");
      return std::nullopt;
    }
    case rdb::RETENTION: {
      SPDLOG_ERROR("RETENTION not supported.");
      return std::nullopt;
    }
    case rdb::RETMEMORY: {
      SPDLOG_ERROR("RETENTION MEMORY not supported.");
      return std::nullopt;
    }
  };
  SPDLOG_ERROR("Type not supported. {}", int(descriptor[position].rtype));
  assert(false && "type not supported on getter.");
  return std::nullopt;
}

// Friend operators

std::istream &operator>>(std::istream &is, const payload &rhs) {
  std::string fieldName;
  is >> fieldName;
  if (is.eof()) return is;
  if (rhs.hexFormat_)
    is >> std::hex;
  else
    is >> std::dec;
  Descriptor desc(rhs.descriptor);

  if (desc.type(fieldName) == "STRING") {
    std::string record;
    // std::getline(is >> std::ws, record);
    is >> record;
    auto fieldLen  = desc.fieldSize(fieldName);
    auto fieldSpan = rhs.span().subspan(desc.offsetBegArr(fieldName), fieldLen);
    std::fill(fieldSpan.begin(), fieldSpan.end(), 0);
    std::copy_n(record.c_str(), std::min((size_t)fieldLen, record.size()), fieldSpan.begin());
  } else
    for (auto i = 0; i < desc[desc.position(fieldName)].rarray; i++) {
      if (desc.type(fieldName) == "BYTE") {
        int data;
        is >> data;
        uint8_t data8 = static_cast<uint8_t>(data);
        auto dest     = rhs.span().subspan(desc.offsetBegArr(fieldName) + i * sizeof(uint8_t), sizeof(uint8_t));
        std::memcpy(dest.data(), &data8, sizeof(uint8_t));
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
      else if (desc.type(fieldName) == "RETENTION")
        SPDLOG_ERROR("RETENTION store not supported by this operator.");
      else
        SPDLOG_ERROR("field {} not found", fieldName);
    }
  return is;
}

std::ostream &operator<<(std::ostream &os, const payload &rhs) {
  if (rhs.hexFormat_)
    os << std::hex;
  else
    os << std::dec;
  os << "{";

  Descriptor desc(rhs.descriptor);
  for (auto const &r : rhs.descriptor) {
    if ((r.rtype == rdb::TYPE) ||       //
        (r.rtype == rdb::REF) ||        //
        (r.rtype == rdb::RETENTION) ||  //
        (r.rtype == rdb::RETMEMORY))    // skip these types
      continue;
    if (!Descriptor::getFlat())
      os << "\t";
    else
      os << " ";
    os << r.rname;
    os << ":";
    auto offset_ = desc.offsetBegArr(r.rname);
    if (r.rtype == STRING) {
      auto fieldSpan = rhs.span().subspan(offset_, desc.fieldSize(r.rname));
      auto len       = desc.fieldSize(r.rname);
      for (auto i = 0; i < len; i++)
        if (fieldSpan[i] == 0) {
          len = i;
          break;
        }

      os << std::string(fieldSpan.begin(), fieldSpan.begin() + len);
    } else
      for (auto i = 0; i < r.rarray; i++) {
        if (r.rtype == rdb::BYTE) {
          uint8_t data{0};
          auto src = rhs.span().subspan(offset_ + i * sizeof(uint8_t), sizeof(uint8_t));
          std::memcpy(&data, src.data(), sizeof(uint8_t));
          if (rhs.hexFormat_) {
            os << std::setfill('0');
            os << std::setw(2);
          }
          os << (int)data;
        } else if (r.rtype == rdb::INTEGER) {
          int data{0};
          auto src = rhs.span().subspan(offset_ + i * sizeof(int), sizeof(int));
          std::memcpy(&data, src.data(), sizeof(int));
          if (rhs.hexFormat_) {
            os << std::setfill('0');
            os << std::setw(8);
          }
          os << data;
        } else if (r.rtype == rdb::UINT) {
          unsigned int data{0};
          auto src = rhs.span().subspan(offset_ + i * sizeof(unsigned), sizeof(unsigned int));
          std::memcpy(&data, src.data(), sizeof(unsigned int));
          if (rhs.hexFormat_) {
            os << std::setfill('0');
            os << std::setw(8);
          }
          os << data;
        } else if (r.rtype == rdb::FLOAT) {
          float data{0};
          auto src = rhs.span().subspan(offset_ + i * sizeof(float), sizeof(float));
          std::memcpy(&data, src.data(), sizeof(float));
          os << data;
        } else if (r.rtype == rdb::DOUBLE) {
          double data{0};
          auto src = rhs.span().subspan(offset_ + i * sizeof(double), sizeof(double));
          std::memcpy(&data, src.data(), sizeof(double));
          os << data;
        } else if (r.rtype == rdb::RETENTION) {
          ;
        } else if (r.rtype == rdb::RETMEMORY) {
          ;
        } else
          assert(false && "Unrecognized type");

        if (i < r.rarray - 1) os << " ";
      }
    if (!Descriptor::getFlat()) os << std::endl;
  }
  if (rhs.descriptor.empty()) {
    os << "Empty";
    SPDLOG_ERROR("Empty descriptor on payload.");
  }
  if (Descriptor::getFlat()) os << " ";
  os << "}";
  if (!Descriptor::getFlat()) os << std::endl;
  Descriptor::setFlat(false);
  return os;
}

}  // namespace rdb
