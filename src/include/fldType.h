// Based on
// https://www.codeproject.com/Articles/10500/Converting-C-enums-to-strings


#ifndef FLDTYPE_H
#define FLDTYPE_H

#include <boost/rational.hpp>         // boost::rational
#include <cstdint>                    // uint8_t - C++20
#include <magic_enum/magic_enum.hpp>  // magic_enum::enum_name
#include <string>                     // std::string
#include <utility>                    // std::pair
#include <vector>  // std::vector
#include <variant>

namespace rdb {

typedef std::variant<uint8_t,                      //
                     int,                          //
                     unsigned,                     //
                     boost::rational<int>,         //
                     float,                        //
                     double,                       //
                     std::pair<int, int>,          //
                     std::pair<std::string, int>,  //
                     std::string>
    descFldVT;

enum descFld {
  BYTE,      //
  INTEGER,   //
  UINT,      //
  RATIONAL,  //
  FLOAT,     //
  DOUBLE,    //
  INTPAIR,   //
  IDXPAIR,   //
  STRING,    //
  TYPE,      //
  REF,       //
  RETENTION
};

constexpr auto GetStringdescFld(const enum descFld index) -> std::string_view { return magic_enum::enum_name(index); }

struct rField {
  std::string rname;
  int rlen;
  int rarray;
  descFld rtype;
  rField(std::string n, int s, int c, descFld t) : rname(std::move(n)), rlen(s), rarray(c), rtype(t) {}
};

}  // namespace rdb
// Support for std::visit over std::variant
template <typename... Ts>
struct Overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;
#endif

