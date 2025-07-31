// Based on
// https://github.com/Neargye/magic_enum

// NOLINTBEGIN(cert-err58-cpp)

#ifndef CMDID_H_DECLARATION_DONE_CMDI
#define CMDID_H_DECLARATION_DONE_CMDI

#include <magic_enum/magic_enum.hpp>  // magic_enum::enum_name
#include <string>                     // std::string

enum command_id {
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
  PUSH_STREAM,        // 31
  STREAM_HASH,        // 32
  STREAM_DEHASH_DIV,  // 33
  STREAM_DEHASH_MOD,  // 34
  STREAM_ADD,         // 35
  STREAM_SUBTRACT,    // 36
  STREAM_TIMEMOVE,    // 37
  STREAM_AGSE,        // 38
  COUNT,              // 39
  COUNT_RANGE         // 40
};

constexpr auto GetStringcommand_id(enum command_id index) -> std::string_view { return magic_enum::enum_name(index); }

#endif  // CMDID_H_DECLARATION_DONE_CMDI

// NOLINTEND(cert-err58-cpp)
