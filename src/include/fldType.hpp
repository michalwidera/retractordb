#pragma once

#include <boost/rational.hpp>         // boost::rational
#include <cstdint>                    // uint8_t - C++20
#include <magic_enum/magic_enum.hpp>  // magic_enum::enum_name
#include <string>                     // std::string
#include <type_traits>                // std::is_same_v
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

// Indeks alternatywy descFldVT JEST wartoscia descFld: ewaluator i kompilator zamieniaja
// `value.index()` wprost na descFld, a promocja typow (normalize(), normalizedOperandType)
// i bramki typu liczbowego (`> DOUBLE`, `> STRING`) stoja na porzadku obu list. Przestawienie
// albo dopisanie alternatywy bez zmiany wyliczenia przemapowaloby po cichu wszystkie wartosci.
//
// Zgodnosc obowiazuje tylko na prefiksie BYTE..NULLTYPE. TYPE, REF, RETENTION i RETMEMORY sa
// polami metadeskryptora (.desc), nie typami wartosci - nie maja i nie moga miec alternatywy
// w wariancie, wiec asercji rozmiaru nie "naprawia" sie dopisaniem ich do descFldVT.
static_assert(std::variant_size_v<descFldVT> == NULLTYPE + 1);
static_assert(std::is_same_v<std::variant_alternative_t<BYTE, descFldVT>, uint8_t>);
static_assert(std::is_same_v<std::variant_alternative_t<INTEGER, descFldVT>, int>);
static_assert(std::is_same_v<std::variant_alternative_t<UINT, descFldVT>, unsigned int>);
static_assert(std::is_same_v<std::variant_alternative_t<RATIONAL, descFldVT>, boost::rational<int>>);
static_assert(std::is_same_v<std::variant_alternative_t<FLOAT, descFldVT>, float>);
static_assert(std::is_same_v<std::variant_alternative_t<DOUBLE, descFldVT>, double>);
static_assert(std::is_same_v<std::variant_alternative_t<INTPAIR, descFldVT>, std::pair<int, int>>);
static_assert(std::is_same_v<std::variant_alternative_t<IDXPAIR, descFldVT>, std::pair<std::string, int>>);
static_assert(std::is_same_v<std::variant_alternative_t<STRING, descFldVT>, std::string>);
static_assert(std::is_same_v<std::variant_alternative_t<NULLTYPE, descFldVT>, std::monostate>);

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
// drzewa: wolaja ja deskryptor, kompilator, parser zrodla tekstowego, wypisywanie payloadu
// i odpowiedzi executorsm. Regula byla przepisana recznie w kazdym z tych miejsc, a rozjazd
// miedzy nimi przesuwa indeksy plaskie wzgledem wartosci - czyli po cichu podmienia wartosci
// pod nazwami pol.
//
// STRING[N] to JEDNA wartosc: N jest dlugoscia tekstu, nie krotnoscia pola.
// NULLTYPE[N] zajmuje N slotow jak pole liczbowe, choc nie zajmuje bajtow rekordu. Dzis nic
// w drzewie nie buduje NULLTYPE o rarray != 1 (typu NULL nie ma ani w DESC.g4, ani w DECLARE);
// regula mowi, jak liczyc, gdyby takie pole powstalo.
constexpr int flatElementCount(const rField &field) { return field.rtype == STRING ? 1 : field.rarray; }

// Pole JEDNEGO slotu plaskiego o szerokosci `bytes`. Napis ma zapis `rlen = 1`, `rarray = N` - ten
// sam, ktory daja DECLARE i parser `.desc`; liczba - `rlen = bytes`, `rarray = 1`. Porownania
// ksztaltu biora rlen i rarray osobno, wiec drugi zapis tego samego napisu (`rlen = N`), ktory
// do 2026-09-27 dawalo okno AGSE, bylby dla nich innym polem.
inline rField flatSlotField(std::string name, descFld type, int bytes) {
  return type == STRING ? rField(std::move(name), 1, bytes, type) : rField(std::move(name), bytes, 1, type);
}

}  // namespace rdb
// Support for std::visit over std::variant
template <typename... Ts>
struct Overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;
