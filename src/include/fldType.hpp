#pragma once

#include <boost/rational.hpp>         // boost::rational
#include <cstdint>                    // uint8_t - C++20
#include <magic_enum/magic_enum.hpp>  // magic_enum::enum_name
#include <string>                     // std::string
#include <utility>                    // std::pair
#include <variant>                    // std::variant

// Based on
// https://www.codeproject.com/Articles/10500/Converting-C-enums-to-strings

namespace rdb {

typedef std::variant<uint8_t,                      // BYTE
                     int,                          // INTEGER
                     unsigned,                     // UINT
                     boost::rational<int>,         // RATIONAL
                     float,                        // FLOAT
                     double,                       // DOUBLE
                     std::pair<int, int>,          // INTPAIR
                     std::pair<std::string, int>,  // IDXPAIR
                     std::string,                  // STRING
                     std::monostate>               // NULL
    descFldVT;

enum descFld {
  BYTE,       //
  INTEGER,    //
  UINT,       //
  RATIONAL,   //
  FLOAT,      //
  DOUBLE,     //
  INTPAIR,    //
  IDXPAIR,    //
  STRING,     //
  NULLTYPE,   // NULL value type
  TYPE,       //
  REF,        //
  RETENTION,  //
  RETMEMORY   // Retention memory
};

constexpr auto GetStringdescFld(const enum descFld index) -> std::string_view {
  return index == NULLTYPE ? std::string_view("NULL") : magic_enum::enum_name(index);
}

struct rField {
  std::string rname;
  int rlen;
  int rarray;
  descFld rtype;
  rField(std::string name, int length, int arrayCount, descFld type)
      : rname(std::move(name)),
        rlen(length),
        rarray(arrayCount),
        rtype(type) {}
};

}  // namespace rdb
// Support for std::visit over std::variant
template <typename... Ts>
struct Overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;
