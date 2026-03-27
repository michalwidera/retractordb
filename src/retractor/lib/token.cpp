#include "token.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <type_traits>

#include "rdb/convertTypes.h"

static_assert(std::is_copy_constructible_v<rdb::descFldVT> == true);

command_id token::getCommandID() { return command_; }

std::string token::getStrCommandID() { return std::string(GetStringcommand_id(command_)); }

boost::rational<int> token::getRI() {
  cast<rdb::descFldVT> castRI;
  auto ret = castRI(getVT(), rdb::RATIONAL);
  return std::get<boost::rational<int>>(ret);
}

std::string token::getStr_() {
  if (getVT().index() == rdb::STRING)
    return std::get<std::string>(getVT());
  else if (getVT().index() == rdb::FLOAT)
    return std::to_string(std::get<float>(getVT()));
  else if (getVT().index() == rdb::DOUBLE)
    return std::to_string(std::get<double>(getVT()));
  else if (getVT().index() == rdb::INTEGER)
    return std::to_string(std::get<int>(getVT()));
  else if (getVT().index() == rdb::UINT)
    return std::to_string(std::get<unsigned>(getVT()));
  else if (getVT().index() == rdb::BYTE)
    return std::to_string(unsigned(std::get<uint8_t>(getVT())));
  else if (getVT().index() == rdb::RATIONAL) {
    auto r = std::get<boost::rational<int>>(getVT());
    return std::to_string(r.numerator()) + "/" + std::to_string(r.denominator());
  } else if (getVT().index() == rdb::INTPAIR) {
    auto r = std::get<std::pair<int, int>>(getVT());
    return std::to_string(r.first) + "," + std::to_string(r.second);
  } else if (getVT().index() == rdb::IDXPAIR) {
    auto r = std::get<std::pair<std::string, int>>(getVT());
    return r.first;
  } else
    return "Error";
}

/** Construktor set */

token::token(command_id id, rdb::descFldVT value)
    :  //
      command_(id),
      valueVT_(std::move(value)) {}

std::ostream &operator<<(std::ostream &os, const rdb::descFldVT &rhs) {
  switch (rhs.index()) {
    case rdb::STRING:
      return os << std::get<std::string>(rhs);
    case rdb::FLOAT:
      return os << std::get<float>(rhs);
    case rdb::DOUBLE:
      return os << std::get<double>(rhs);
    case rdb::INTEGER:
      return os << std::get<int>(rhs);
    case rdb::UINT:
      return os << std::get<unsigned>(rhs);
    case rdb::BYTE:
      return os << unsigned(std::get<uint8_t>(rhs));
    case rdb::RATIONAL:
      return os << std::get<boost::rational<int>>(rhs);
    case rdb::INTPAIR: {
      auto r = get<std::pair<int, int>>(rhs);
      return os << r.first << "," << r.second;
    }
    case rdb::IDXPAIR: {
      auto r = get<std::pair<std::string, int>>(rhs);
      return os << r.first << "[" << r.second << "]";
    }
  }
  return os << "not supported";
}

std::ostream &operator<<(std::ostream &os, const token &rhs) {
  os << GetStringcommand_id(rhs.command_) << "(";
  os << rhs.getVT();
  os << ")";
  return os;
}
