// Based on
// https://www.codeproject.com/Articles/10500/Converting-C-enums-to-strings

// Checking redefinded namespace sanity
#if defined(DECL_T) || defined(DECL_F) || defined(DECL_E) || defined(BEGIN_E_GEN_T) || defined(END_E_GEN_T)
#error DECL_T, DECL_F, DECL_E, BEGIN_E_GEN_T,END_E_GEN_T conficts with inner fldType.h declaration.
#endif

#include <string>

#ifdef ENUMDECL_H_CREATE_VARIANT_T
#include <boost/rational.hpp>
#include <vector>
#define BEGIN_E_GEN_T(ENUM_NAME) typedef std::variant <
#define DECL_T(elementName, elementType) elementType,
#define DECL_E(elementName, elementType) elementType
#define DECL_F(elementName)
#define END_E_GEN_T(ENUM_NAME) > variant_t;
#undef ENUMDECL_H_DECLARATION_DONE_FLDT
#undef ENUMDECL_H_CREATE_VARIANT_T
#endif

#ifdef ENUMDECL_H_CREATE_DEFINITION_FLDT
// Part responsible for Definition & Initialization of map structure
#include <map>
#define DECL_T(elementName, elementType) {elementName, #elementName},
#define DECL_E(elementName, elementType) \
  { elementName, #elementName }
#define DECL_F(elementName) \
  , { elementName, #elementName }
#define BEGIN_E_GEN_T(ENUM_NAME) std::map<ENUM_NAME, std::string> tg_##ENUM_NAME = {
#define END_E_GEN_T(ENUM_NAME) \
  }                            \
  ;                            \
  std::string GetString##ENUM_NAME(enum ENUM_NAME index) { return tg_##ENUM_NAME[index]; };
// This undef will force to BEGIN_E_GEN_T(...) - END_E_GEN_T(...) will appear once
// again with new set of BEGIN_E_GEN_T/END_E_GEN_T definitions and will goto to
// BEGIN_E_GEN_T sections
#undef ENUMDECL_H_DECLARATION_DONE_FLDT
#endif

#ifndef ENUMDECL_H_DECLARATION_DONE_FLDT
#ifndef BEGIN_E_GEN_T
#define BEGIN_E_GEN_T(ENUM_NAME) enum ENUM_NAME {
#define DECL_T(elementName, elementType) elementName,
#define DECL_E(elementName, elementType) elementName
#define DECL_F(elementName) , elementName
#define END_E_GEN_T(ENUM_NAME) \
  }                            \
  ;                            \
  std::string GetString##ENUM_NAME(enum ENUM_NAME index);
#endif

// This declaration goes into ::rdb namespace
namespace rdb {
BEGIN_E_GEN_T(descFldType)
DECL_T(BAD, std::monostate)
DECL_T(BYTE, uint8_t)
DECL_T(INTEGER, int)
DECL_T(UINT, unsigned)
DECL_T(RATIONAL, boost::rational<int>)
DECL_T(FLOAT, float)
DECL_T(DOUBLE, double)
DECL_T(BYTEARRAY, std::vector<uint8_t>)
DECL_T(INTARRAY, std::vector<int>)
DECL_E(STRING, std::string)
DECL_F(TYPE)
DECL_F(REF)
END_E_GEN_T(descFldType)
}  // namespace rdb

#undef BEGIN_E_GEN_T
#undef END_E_GEN_T
#undef DECL_T
#undef DECL_E
#undef DECL_F
#define ENUMDECL_H_DECLARATION_DONE_FLDT

#endif  // ENUMDECL_H_DECLARATION_DONE_FLDT
