#pragma once

#include <cstdint>

#include <magic_enum/magic_enum.hpp>  // magic_enum::enum_name
#include <string>                     // std::string

// Based on
// https://github.com/Neargye/magic_enum

enum command_id : std::uint8_t {
  VOID_COMMAND,       // 0
  VOID_VALUE,         // 1
  PUSH_ID,            // 2
  PUSH_ID1,           // 3
  PUSH_ID2,           // 4
  PUSH_ID3,           // 5
  PUSH_ID4,           // 6
  PUSH_ID5,           // 7
  PUSH_IDX,           // 8
  PUSH_VAL,           // 9
  PUSH_TSCAN,         // 10
  TYPE,               // 11
  ADD,                // 12
  SUBTRACT,           // 13
  MULTIPLY,           // 14
  DIVIDE,             // 15
  NEGATE,             // 16
  AND,                // 17
  OR,                 // 18
  NOT,                // 19
  CMP_EQUAL,          // 20
  CMP_LT,             // 21
  CMP_GT,             // 22
  CMP_LE,             // 23
  CMP_GE,             // 24
  CMP_NOT_EQUAL,      // 25
  STREAM_AVG,         // 26
  STREAM_MIN,         // 27
  STREAM_MAX,         // 28
  STREAM_SUM,         // 29
  CALL,               // 30
  CALL2,              // 31
  PUSH_STREAM,        // 32
  STREAM_HASH,        // 33
  STREAM_DEHASH_DIV,  // 34
  STREAM_DEHASH_MOD,  // 35
  STREAM_ADD,         // 36
  STREAM_SUBTRACT,    // 37
  STREAM_TIMEMOVE,    // 38
  STREAM_AGSE,        // 39
  COUNT,              // 40
  COUNT_RANGE,        // 41
  PUSH_GENIDX,        // 42 - numer instancji generatora; zyje tylko do expandStreamGenerators()
  POWER,              // 43 - potegowanie `a^b`; dopisane NA KONCU, zeby nie przenumerowac reszty
  // Agregaty okna REKORDOWEGO w liscie SELECT: `MIN(cells : 10)`. Odrebne od
  // STREAM_MIN/MAX/AVG/SUM, ktore redukuja pola JEDNEGO rekordu w klauzuli FROM.
  //
  // Zycie tokena jest dwuetapowe. Parser wystawia go z szerokoscia okna i poprzedza
  // odwolaniem do pola (PUSH_ID3/PUSH_ID1/PUSH_ID2). compiler::resolveWindowAggregates()
  // zamienia szerokosc na indeks grupy w query::windowGroups i USUWA poprzedzajacy PUSH_ID,
  // wiec od tego przebiegu token jest bezargumentowym lisciem programu. Etap rozpoznaje sie
  // po tabeli grup zapytania, nie po tokenie: obie postaci niosa zwykly `int`.
  WINDOW_MIN,  // 44
  WINDOW_MAX,  // 45
  WINDOW_AVG,  // 46
  WINDOW_SUM   // 47
};

constexpr auto GetStringcommand_id(enum command_id index) -> std::string_view { return magic_enum::enum_name(index); }
