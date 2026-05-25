#include "token.hpp"

#include <spdlog/spdlog.h>

#include <cassert>
#include <type_traits>

#include "rdb/convertTypes.hpp"

static_assert(std::is_copy_constructible_v<rdb::descFldVT> == true);

command_id token::getCommandID() { return command_; }

std::string token::getStrCommandID() { return std::string(GetStringcommand_id(command_)); }

boost::rational<int> token::getRI() {
  cast<rdb::descFldVT> castRI;
  auto ret = castRI(getVT(), rdb::RATIONAL);
  return std::get<boost::rational<int>>(ret);
}

std::string token::getStr_() {
  return std::visit(
      [](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return "null";
        else if constexpr (std::is_same_v<T, std::string>)
          return v;
        else if constexpr (std::is_same_v<T, uint8_t>)
          return std::to_string(unsigned(v));
        else if constexpr (std::is_same_v<T, boost::rational<int>>)
          return std::to_string(v.numerator()) + "/" + std::to_string(v.denominator());
        else if constexpr (std::is_same_v<T, std::pair<int, int>>)
          return std::to_string(v.first) + "," + std::to_string(v.second);
        else if constexpr (std::is_same_v<T, std::pair<std::string, int>>)
          return v.first;
        else
          return std::to_string(v);
      },
      getVT());
}

/** Construktor set */

token::token(command_id id, rdb::descFldVT value)
    :  //
      command_(id),
      valueVT_(std::move(value)) {}

std::ostream &operator<<(std::ostream &os, const rdb::descFldVT &rhs) {
  std::visit(
      [&os](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          os << "null";
        else if constexpr (std::is_same_v<T, uint8_t>)
          os << unsigned(v);
        else if constexpr (std::is_same_v<T, std::pair<int, int>>)
          os << v.first << "," << v.second;
        else if constexpr (std::is_same_v<T, std::pair<std::string, int>>)
          os << v.first << "[" << v.second << "]";
        else
          os << v;
      },
      rhs);
  return os;
}

std::ostream &operator<<(std::ostream &os, const token &rhs) {
  os << GetStringcommand_id(rhs.command_) << "(";
  os << rhs.getVT();
  os << ")";
  return os;
}
