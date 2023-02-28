// Based on
// https://www.codeproject.com/Articles/10500/Converting-C-enums-to-strings

// Checking redefinded namespace sanity
#if defined(DECL) || defined(BEGIN_E_GEN) || defined(END_E_GEN) || defined(LAST)
#error DECL, LAST, BEGIN_E_GEN, END_E_GEN conficts with inner cmdID.h declaration.
#endif

#ifdef CMDID_H_CREATE_DEFINITION_CMDI
// Part responsible for Definition & Initialization of map structure
#include <map>
#include <string>
#define DECL(element) {element, #element},
#define LAST(element) \
  { element, #element }
#define BEGIN_E_GEN(ENUM_NAME) std::map<ENUM_NAME, std::string> tg_##ENUM_NAME = {
#define END_E_GEN(ENUM_NAME) \
  }                          \
  ;                          \
  std::string GetString##ENUM_NAME(enum ENUM_NAME index) { return tg_##ENUM_NAME[index]; };
// This undef will force to BEGIN_E_GEN(...) - END_E_GEN(...) will appear once
// again with new set of BEGIN_E_GEN/END_E_GEN definitions and will goto to
// BEGIN_E_GEN sections
#undef CMDID_H_DECLARATION_DONE_CMDI
#endif

#ifndef CMDID_H_DECLARATION_DONE_CMDI
// Part resposible for declaration
#ifndef BEGIN_E_GEN
#include <string>
#define BEGIN_E_GEN(ENUM_NAME) enum ENUM_NAME {
#define DECL(element) element,
#define LAST(element) element
#define END_E_GEN(ENUM_NAME) \
  }                          \
  ;                          \
  std::string GetString##ENUM_NAME(enum ENUM_NAME index);
#endif

//
// Clarification: This structure in default mode will create: enum XXX {
// void_command, void_value, ... } and GetStringXXX function will be decared as
// existing somewhere. When, before including this header macro
// CMDID_H_CREATE_DEFINITION will be defined this structure will
// create&initialize following map: std::map<XXX, std::string> = { {
// VOID_COMMAND, "VOID_COMMAND
// "} , ...} and function GetStingXXX with body. AT LEAST ONE PLACE WHEN #define
// CMDID_H_CREATE_DEFINITION with #include this file must appear to generate
// defs Benefit of this solution: One place in code that materialize ENUM in
// scope of Runtime.
//
BEGIN_E_GEN(command_id)
DECL(VOID_COMMAND)
DECL(VOID_VALUE)
DECL(PUSH_ID)
DECL(PUSH_ID1)
DECL(PUSH_ID2)
DECL(PUSH_ID3)
DECL(PUSH_ID4)
DECL(PUSH_ID5)
DECL(PUSH_IDX)
DECL(PUSH_VAL)
DECL(PUSH_TSCAN)
DECL(TYPE)
DECL(ADD)
DECL(SUBTRACT)
DECL(MULTIPLY)
DECL(DIVIDE)
DECL(NEGATE)
DECL(AND)
DECL(OR)
DECL(NOT)
DECL(CMP_EQUAL)
DECL(CMP_LT)
DECL(CMP_GT)
DECL(CMP_LE)
DECL(CMP_GE)
DECL(CMP_NOT_EQUAL)
DECL(STREAM_AVG)
DECL(STREAM_MIN)
DECL(STREAM_MAX)
DECL(STREAM_SUM)
DECL(CALL)
DECL(PUSH_STREAM)
DECL(STREAM_HASH)
DECL(STREAM_DEHASH_DIV)
DECL(STREAM_DEHASH_MOD)
DECL(STREAM_ADD)
DECL(STREAM_SUBSTRACT)
DECL(STREAM_TIMEMOVE)
DECL(STREAM_AGSE)
DECL(COUNT)
LAST(COUNT_RANGE)
END_E_GEN(command_id)
#undef BEGIN_E_GEN
#undef END_E_GEN
#undef DECL
#undef LAST
#define CMDID_H_DECLARATION_DONE_CMDI

#endif  // CMDID_H_DECLARATION_DONE_CMDI
