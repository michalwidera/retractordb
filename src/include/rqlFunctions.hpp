#pragma once

#include <cctype>

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

/// Jedna lista funkcji skalarnych RQL — wspolna dla kontroli w kompilatorze i dla
/// ewaluatora. Do 2026-08-30 listy byly dwie i rozjezdzaly sie w OBIE strony:
///
/// - `RQL.g4` wymienial `Abs`, `Sign`, `Chr`, `Length`, `ToNumber`, `ToTimeStamp`,
///   `FloatCast`, `IntCast`, `Count`, `Crc`, `Sum`, `IsZero`, `IsNonZero`, ktorych
///   ewaluator nie znal. Program kompilowal sie czysto, a `-c` przechodzilo — proces
///   ginal dopiero w wykonaniu na `Unsupported function call`. `Abs`, `IsZero` i `IsNonZero`
///   zostaly wtedy zaimplementowane, `Length` doszedl 30.08.2026, reszta znikla;
/// - `round`, `trunc`, `sin`, `cos`, `tan`, `log` i `log2` byly zaimplementowane
///   w ewaluatorze, ale nie stalo ich w gramatyce, wiec byly nieosiagalne z RQL.
///
/// Nazwy dopasowuje sie BEZ WZGLEDU na wielkosc liter, bo do 2026-08-29 wielkosc liter
/// byla czescia skladni: `Sqrt(x)` przechodzilo, `sqrt(x)` bylo bledem skladni, a dla
/// `to_integer` i `isnull` bylo odwrotnie.
///
/// `canonical` jest postacia zapisywana do tokena — NIE ta, ktora napisal autor
/// zapytania. To jest istotne w dwoch miejscach naraz:
///
/// - zrzuty planu pokazuja `CALL(Sqrt)` i tak wygladaja wzorce testow integracyjnych
///   oraz zapisy planow pilota H9 w `test/research_gate/h9/pilot/out/`; kanonizacja
///   pozwala dopuscic dowolna pisownie w zrodle bez ruszania ani jednego z tych plikow;
/// - `RQLParser::exitExpression` i `exprSimplify` porownuja `getStr_() == "to_string"`
///   doslownie. Bez kanonizacji `TO_STRING(x:16)` przeszloby te warunki bokiem i dalo
///   zla szerokosc pola w deskryptorze.
namespace rdb {

struct RqlFunction {
  std::string_view canonical;
  int minArgs;
  int maxArgs;
};

/// Arnosc jest DANYMI, nie ksztaltem gramatyki. Gramatyka zna dzis dwa ksztalty
/// wywolania (jednoargumentowy i `to_string(expr : N)`), ale liczbe argumentow
/// sprawdza compiler::checkFunctionCalls() z tej tabeli. Gdy pojawi sie funkcja
/// dwuargumentowa nad wyrazeniami, zmienia sie tabela i jedna alternatywa w regule
/// `function_call` — reszta potoku zostaje bez zmian.
///
/// UWAGA na przyszly `min(a, b)`: `MIN`, `MAX`, `AVG` i `SUMC` sa tokenami leksera
/// stojacymi PRZED `ID` (reduktory strumieniowe), wiec `min` nigdy nie zaleksuje sie
/// jako nazwa funkcji skalarnej. Skalarne minimum bedzie musialo nazywac sie inaczej.
inline constexpr std::array<RqlFunction, 19> kRqlFunctions{{
    {"Sqrt", 1, 1},        //
    {"Ceil", 1, 1},        //
    {"Floor", 1, 1},       //
    {"Abs", 1, 1},         //
    {"round", 1, 1},       //
    {"trunc", 1, 1},       //
    {"sin", 1, 1},         //
    {"cos", 1, 1},         //
    {"tan", 1, 1},         //
    {"log", 1, 1},         //
    {"log2", 1, 1},        //
    {"isnull", 1, 1},      //
    {"IsZero", 1, 1},      //
    {"IsNonZero", 1, 1},   //
    {"Length", 1, 1},      // WYLACZNIE nad napisem — argument liczbowy jest bledem wykonania
    {"to_integer", 1, 1},  //
    {"to_float", 1, 1},    //
    {"to_double", 1, 1},   //
    {"to_string", 1, 2},   // drugi argument to ZADEKLAROWANA SZEROKOSC pola, nie wartosc na stosie
}};

/// @brief Znajdz funkcje po nazwie, ignorujac wielkosc liter.
/// @param name nazwa tak, jak napisal ja autor zapytania
/// @return wpis tabeli albo nullopt, gdy nazwa jest nieznana
inline std::optional<RqlFunction> findRqlFunction(std::string_view name) {
  const auto sameIgnoringCase = [](std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](unsigned char x, unsigned char y) {
             return std::tolower(x) == std::tolower(y);
           });
  };

  const auto it =
      std::ranges::find_if(kRqlFunctions, [&](const RqlFunction &fn) { return sameIgnoringCase(fn.canonical, name); });
  if (it == kRqlFunctions.end()) return std::nullopt;
  return *it;
}

}  // namespace rdb
