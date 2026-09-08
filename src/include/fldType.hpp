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

using descFldVT = std::variant<uint8_t, int, unsigned int, boost::rational<int>, float, double, std::pair<int, int>,
                               std::pair<std::string, int>, std::string, std::monostate>;

enum descFld : std::uint8_t {
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

// Liczba SLOTOW PLASKICH zajmowanych przez pole w rekordzie. Jedna definicja dla calego
// drzewa: mapowan deskryptora (Descriptor::rebuildFieldMappings), serializacji wiersza do
// klienta (executorsm::printRowValue) i krotnosci pola w odpowiedzi 'detail'. Regula byla
// przepisana recznie w kazdym z tych miejsc, a rozjazd miedzy nimi przesuwa indeksy plaskie
// wzgledem wartosci — czyli po cichu podmienia wartosci pod nazwami pol.
//
// STRING[N] to JEDNA wartosc: N jest dlugoscia tekstu, nie krotnoscia pola.
constexpr int flatElementCount(const rField &field) { return field.rtype == STRING ? 1 : field.rarray; }

}  // namespace rdb
// Support for std::visit over std::variant
template <typename... Ts>
struct Overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;
