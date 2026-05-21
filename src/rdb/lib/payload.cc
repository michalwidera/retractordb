#include "rdb/payload.hpp"

#define BOOST_STACKTRACE_USE_BACKTRACE

#include <spdlog/spdlog.h>

#include <algorithm>  // std::min, std::copy, std::fill
#include <boost/rational.hpp>
#include <boost/stacktrace.hpp>

#include "fatalError.hpp"
#include <cstring>  // std::memcpy (for C-interop)
#include <iomanip>
#include <iostream>
#include <sstream>

#include "rdb/convertTypes.hpp"

namespace rdb {

namespace {

int resolveFieldIndexOrAbort(Descriptor &descriptor, const int positionFlat, const char *context) {
  const auto flatCount = descriptor.flatElementCount();
  if (positionFlat < 0 || positionFlat > flatCount - 1) {
    SPDLOG_ERROR("{} out of descriptor req:{} available len: {}", context, positionFlat, flatCount);
    if (std::string_view(context) == "Read") {
      std::stringstream message;
      message << boost::stacktrace::stacktrace();
      SPDLOG_ERROR("Stack: {}", message.str());
      std::cerr << message.str() << std::endl;
    }
    FATAL_ERROR("payload: flat position out of range");
  }

  auto positionOpt = descriptor.flatIndexToDescriptorPosition(positionFlat);
  if (!positionOpt.has_value()) {
    SPDLOG_ERROR("{} conversion failed for flat position {}", context, positionFlat);
    FATAL_ERROR("payload: flat index to descriptor position conversion failed");
  }

  const auto position = positionOpt->first;
  if (position < 0 || position >= static_cast<int>(descriptor.size())) {
    SPDLOG_ERROR("{} converted index out of descriptor bounds: {}", context, position);
    FATAL_ERROR("payload: converted index out of descriptor bounds");
  }

  return position;
}

void writeValue(std::ostream &os, const std::any &value, const descFld type, const bool hexFormat) {
  switch (type) {
    case rdb::NULLTYPE:
      os << "null";
      break;
    case rdb::STRING:
      os << std::any_cast<std::string>(value);
      break;
    case rdb::BYTE: {
      if (hexFormat) {
        os << std::setfill('0') << std::setw(2);
      }
      os << static_cast<int>(std::any_cast<uint8_t>(value));
      break;
    }
    case rdb::INTEGER: {
      if (hexFormat) {
        os << std::setfill('0') << std::setw(8);
      }
      os << std::any_cast<int>(value);
      break;
    }
    case rdb::UINT: {
      if (hexFormat) {
        os << std::setfill('0') << std::setw(8);
      }
      os << std::any_cast<unsigned>(value);
      break;
    }
    case rdb::FLOAT:
      os << std::any_cast<float>(value);
      break;
    case rdb::DOUBLE:
      os << std::any_cast<double>(value);
      break;
    case rdb::RATIONAL:
      os << std::any_cast<boost::rational<int>>(value);
      break;
    case rdb::REF:
    case rdb::TYPE:
    case rdb::RETENTION:
    case rdb::RETMEMORY:
      FATAL_ERROR("payload: configuration fields (REF/TYPE/RETENTION) cannot be formatted");
      break;
  }
}

template <typename T>
void copyToMemory(std::istream &is, payload &rhs, const std::string_view fieldName, const int arrayOffset) {
  T data;
  is >> data;
  Descriptor desc(rhs.descriptor);
  auto dest = rhs.span().subspan(desc.fieldByteOffset(fieldName) + arrayOffset, sizeof(T));
  std::memcpy(dest.data(), &data, sizeof(T));
}

}  // namespace

// default constructor

payload::payload(const Descriptor &descriptor)
    : descriptor(descriptor),  //
      hexFormat_(false) {
  payloadData_ = std::make_unique<uint8_t[]>(descriptor.getSizeInBytes());
  std::fill(span().begin(), span().end(), 0);

  nullBitset_.resize(descriptor.size(), false);
}

// copy constructor

payload::payload(const payload &other) {
  descriptor   = other.descriptor;
  payloadData_ = std::make_unique<uint8_t[]>(other.descriptor.getSizeInBytes());
  std::copy(other.span().begin(), other.span().end(), span().begin());
  nullBitset_ = other.nullBitset_;
}

// Copy & assignment operator

payload &payload::operator=(const payload &other) {
  if (this == &other) return *this;  // assure not a self-assignment

  *this = other.descriptor;  // call operator=(const Descriptor
  std::copy(other.span().begin(), other.span().end(), span().begin());
  nullBitset_ = other.nullBitset_;
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
    nullBitset_.assign(descriptor.size(), false);
  } else {
    if (descriptor == other) {  // compare rlen and rtype only here
      // descriptor = other; <- Just change field names - descriptor remains the same, payload remains the same
      // pass
    } else
      FATAL_ERROR("payload: descriptor not empty before assign — schema mismatch");
  }
  return *this;
}

// Math operation operators

payload payload::operator+(const payload &other) {
  rdb::Descriptor descSum(descriptor);
  descSum += other.descriptor;                    // ! moving this into constructor fails
  descSum.removeConfigurationFields();            //
  payload result(descSum);                        //
  std::copy(span().begin(), span().end(), result.span().begin());
  std::copy(other.span().begin(), other.span().end(), result.span().subspan(descriptor.getSizeInBytes()).begin());

  result.nullBitset_.clear();
  result.nullBitset_.reserve(result.descriptor.size());

  auto appendDataFieldNullFlags = [&result](const payload &src) {
    for (size_t i = 0; i < src.descriptor.size(); ++i) {
      auto type = src.descriptor[i].rtype;
      if (type == rdb::TYPE || type == rdb::REF || type == rdb::RETENTION || type == rdb::RETMEMORY) continue;
      result.nullBitset_.push_back(i < src.nullBitset_.size() ? src.nullBitset_[i] : false);
    }
  };

  appendDataFieldNullFlags(*this);
  appendDataFieldNullFlags(other);

  return result;
}

// Member Functions

void payload::setHex(bool hexFormatVal) { hexFormat_ = hexFormatVal; }

const std::vector<bool> &payload::getNullBitset() const { return nullBitset_; }

void payload::setNullBitset(const std::vector<bool> &nullBitset) {
  if (nullBitset.size() != descriptor.size()) {
    SPDLOG_ERROR("payload::setNullBitset: size mismatch: nullBitset={} descriptor={}", nullBitset.size(), descriptor.size());
    FATAL_ERROR("payload::setNullBitset: nullBitset size does not match descriptor size");
  }
  nullBitset_ = nullBitset;
}

std::span<uint8_t> payload::span() { return {payloadData_.get(), descriptor.getSizeInBytes()}; }

std::span<const uint8_t> payload::span() const { return {payloadData_.get(), descriptor.getSizeInBytes()}; }

template <typename T>
void payload::setItemBy(const int positionFlat, std::optional<std::any> value) {
  T data          = std::any_cast<T>(value.value());
  auto position   = resolveFieldIndexOrAbort(descriptor, positionFlat, "Write");
  auto offsetFlat = descriptor.byteOffsetAtFlatIndex(positionFlat);
  auto dest       = span().subspan(offsetFlat, descriptor[position].rlen);
  std::memcpy(dest.data(), &data, descriptor[position].rlen);
}

void payload::setItem(const int positionFlat, std::optional<std::any> valueParam) {
  auto position = resolveFieldIndexOrAbort(descriptor, positionFlat, "Write");

  auto requestedType = descriptor[position].rtype;
  std::any value;
  if (!valueParam.has_value()) {
    nullBitset_[position] = true;
    auto fallbackValue    = nullFallbackValue(requestedType);
    std::visit([&value](const auto &v) { value = std::any(v); }, fallbackValue);
  } else {
    nullBitset_[position] = false;
    cast<std::any> castAny;
    value = castAny(valueParam.value(), requestedType);
  }

  auto writeStringField = [&]() {
    const auto len = descriptor[position].rlen * descriptor[position].rarray;
    std::string data(std::any_cast<std::string>(value));
    auto lenr       = std::min(len, static_cast<int>(data.length()));
    auto destOffset = descriptor.byteOffsetAtFlatIndex(positionFlat);
    auto dest       = span().subspan(destOffset, len);
    if (destOffset + len > descriptor.getSizeInBytes()) {
      SPDLOG_ERROR("payload::writeStringField: destOffset {} + len {} exceeds descriptor size {}", destOffset, len, descriptor.getSizeInBytes());
      FATAL_ERROR("payload::writeStringField: string field write out of descriptor bounds");
    }
    std::fill(dest.begin(), dest.end(), 0);
    std::copy_n(data.c_str(), lenr, dest.begin());
  };

  try {
    switch (requestedType) {
      case rdb::NULLTYPE:
        break;
      case rdb::STRING:
        writeStringField();
        break;
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
      case rdb::REF:
      case rdb::TYPE:
      case rdb::RETENTION:
      case rdb::RETMEMORY:
        break;
      default:
        SPDLOG_ERROR("Type not supported: {}", (int)requestedType);
        FATAL_ERROR("payload::setItem: unsupported field type");
    }
  } catch (const std::bad_any_cast &) {
    SPDLOG_ERROR("Error on payload::setItem");
  }
}

template <typename T>
T getVal(std::span<const uint8_t> s, int offset) {
  T val;
  std::memcpy(&val, s.subspan(offset, sizeof(T)).data(), sizeof(T));
  return val;
}

std::optional<std::any> payload::getItem(const int positionFlat) const {
  Descriptor descriptorCopy(descriptor);
  auto position = resolveFieldIndexOrAbort(descriptorCopy, positionFlat, "Read");

  if (nullBitset_[position]) return std::nullopt;

  const auto requestedType = descriptorCopy[position].rtype;
  const auto offsetFlat    = descriptorCopy.byteOffsetAtFlatIndex(positionFlat);
  auto memory              = span();

  auto readStringField = [&]() -> std::string {
    auto len       = descriptorCopy[position].rlen * descriptorCopy[position].rarray;
    auto fieldSpan = memory.subspan(offsetFlat, len);
    auto descLen   = descriptorCopy.getSizeInBytes();
    if (offsetFlat + len > descLen) {
      SPDLOG_ERROR("payload::readStringField: field offset {} + len {} exceeds descriptor size {}", offsetFlat, len, descLen);
      FATAL_ERROR("payload::readStringField: field access out of descriptor bounds");
    }

    for (auto i = 0; i < len; i++) {
      if (fieldSpan[i] == 0) {
        len = i;
        break;
      }
    }
    return std::string(fieldSpan.begin(), fieldSpan.begin() + len);
  };

  switch (requestedType) {
    case rdb::NULLTYPE:
      return std::any(std::monostate{});
    case rdb::STRING:
      return readStringField();
    case rdb::BYTE:
      return getVal<uint8_t>(memory, offsetFlat);
    case rdb::INTEGER:
      return getVal<int>(memory, offsetFlat);
    case rdb::UINT:
      return getVal<uint>(memory, offsetFlat);
    case rdb::DOUBLE:
      return getVal<double>(memory, offsetFlat);
    case rdb::FLOAT:
      return getVal<float>(memory, offsetFlat);
    case rdb::RATIONAL:
      return getVal<boost::rational<int>>(memory, offsetFlat);
    case rdb::REF:
    case rdb::TYPE:
    case rdb::RETENTION:
    case rdb::RETMEMORY:
      SPDLOG_ERROR("Configuration field type not supported in getItem: {}", static_cast<int>(requestedType));
      return std::nullopt;
  }

  SPDLOG_ERROR("Type not supported. {}", int(requestedType));
  FATAL_ERROR("payload::getItem: unsupported field type");
}

// Friend operators

std::istream &operator>>(std::istream &is, payload &rhs) {
  std::string fieldName;
  is >> fieldName;
  if (is.eof()) return is;
  if (rhs.hexFormat_)
    is >> std::hex;
  else
    is >> std::dec;
  Descriptor desc(rhs.descriptor);
  if (!desc.hasField(fieldName)) {
    SPDLOG_ERROR("field {} not found", fieldName);
    return is;
  }

  const auto fieldIndex = desc.fieldIndex(fieldName);

  if (desc.fieldTypeName(fieldName) == "NULL") {
    rhs.setItem(static_cast<int>(fieldIndex), std::any(std::monostate{}));
    return is;
  }

  if (desc.fieldTypeName(fieldName) == "STRING") {
    std::string record;
    is >> record;
    auto fieldLen  = desc.fieldSize(fieldName);
    auto fieldSpan = rhs.span().subspan(desc.fieldByteOffset(fieldName), fieldLen);
    std::fill(fieldSpan.begin(), fieldSpan.end(), 0);
    std::copy_n(record.c_str(), std::min((size_t)fieldLen, record.size()), fieldSpan.begin());
    rhs.nullBitset_[fieldIndex] = false;
  } else
    for (auto i = 0; i < desc[desc.fieldIndex(fieldName)].rarray; i++) {
      if (desc.fieldTypeName(fieldName) == "BYTE") {
        int data;
        is >> data;
        uint8_t data8 = static_cast<uint8_t>(data);
        auto dest     = rhs.span().subspan(desc.fieldByteOffset(fieldName) + i * sizeof(uint8_t), sizeof(uint8_t));
        std::memcpy(dest.data(), &data8, sizeof(uint8_t));
        rhs.nullBitset_[fieldIndex] = false;
      } else if (desc.fieldTypeName(fieldName) == "UINT")
        copyToMemory<uint>(is, rhs, fieldName, i * sizeof(unsigned)), rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "INTEGER")
        copyToMemory<int>(is, rhs, fieldName, i * sizeof(int)), rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "FLOAT")
        copyToMemory<float>(is, rhs, fieldName, i * sizeof(float)), rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "DOUBLE")
        copyToMemory<double>(is, rhs, fieldName, i * sizeof(double)), rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "RATIONAL")
        copyToMemory<boost::rational<int>>(is, rhs, fieldName, i * sizeof(boost::rational<int>)), rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "REF")
        SPDLOG_ERROR("REF store not supported by this operator.");
      else if (desc.fieldTypeName(fieldName) == "TYPE")
        SPDLOG_ERROR("TYPE store not supported by this operator.");
      else if (desc.fieldTypeName(fieldName) == "RETENTION")
        SPDLOG_ERROR("RETENTION store not supported by this operator.");
      else if (desc.fieldTypeName(fieldName) == "RETMEMORY")
        SPDLOG_ERROR("RETMEMORY store not supported by this operator.");
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
  int flatIndex = 0;
  for (size_t idx = 0; idx < rhs.descriptor.size(); ++idx) {
    const auto &r = rhs.descriptor[idx];
    if ((r.rtype == rdb::TYPE) ||       //
        (r.rtype == rdb::REF) ||        //
        (r.rtype == rdb::RETENTION) ||  //
        (r.rtype == rdb::RETMEMORY))    // skip these types
      continue;
    if (!Descriptor::isSingleLineOutput())
      os << "\t";
    else
      os << " ";
    os << r.rname;
    os << ":";
    const int flatCountForField = (r.rtype == rdb::STRING || r.rtype == rdb::NULLTYPE) ? 1 : r.rarray;
    const auto firstValue       = rhs.getItem(flatIndex);
    if (!firstValue.has_value()) {
      os << "null";
      flatIndex += flatCountForField;
    } else if (r.rtype == rdb::STRING || r.rtype == rdb::NULLTYPE) {
      writeValue(os, firstValue.value(), r.rtype, rhs.hexFormat_);
      ++flatIndex;
    } else {
      for (int i = 0; i < flatCountForField; ++i) {
        const auto value = rhs.getItem(flatIndex + i);
        if (!value.has_value()) FATAL_ERROR("payload: non-null array field returned no value for flat element");
        writeValue(os, value.value(), r.rtype, rhs.hexFormat_);
        if (i < flatCountForField - 1) os << " ";
      }
      flatIndex += flatCountForField;
    }
    if (!Descriptor::isSingleLineOutput()) os << std::endl;
  }
  if (rhs.descriptor.empty()) {
    os << "Empty";
    SPDLOG_ERROR("Empty descriptor on payload.");
  }
  if (Descriptor::isSingleLineOutput()) os << " ";
  os << "}";
  if (!Descriptor::isSingleLineOutput()) os << std::endl;
  Descriptor::setSingleLineOutput(false);
  return os;
}

}  // namespace rdb
