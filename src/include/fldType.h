// Based on
// https://www.codeproject.com/Articles/10500/Converting-C-enums-to-strings

// Checking redefined namespace sanity
#if defined(DECL_T) || defined(DECL_F) || defined(DECL_E) || defined(BEGIN_E_GEN_T) || defined(END_E_GEN_T)
#error DECL_T, DECL_F, DECL_E, BEGIN_E_GEN_T,END_E_GEN_T conflicts with inner fldType.h declaration.
#endif

#include <boost/rational.hpp>
#include <string>
#include <vector>

#ifndef FLDTYPE_H_CREATE_VARIANT_T
#define FLDTYPE_H_CREATE_VARIANT_T
#include <variant>
#define BEGIN_E_GEN_T(ENUM_NAME) typedef std::variant <
#define DECL_T(elementName, elementType) elementType,
#define DECL_E(elementName, elementType) elementType
#define DECL_F(elementName)
#define END_E_GEN_T(ENUM_NAME) > ENUM_NAME##VT;
#include "internal/fldList.h"
#endif

#ifdef FLDTYPE_H_CREATE_DEFINITION_FLDT
#undef FLDTYPE_H_CREATE_DEFINITION_FLDT
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
#include "internal/fldList.h"
#endif

#ifndef FLDTYPE_H_DECLARATION_DONE_FLDT
#define FLDTYPE_H_DECLARATION_DONE_FLDT
#define BEGIN_E_GEN_T(ENUM_NAME) enum ENUM_NAME {
#define DECL_T(elementName, elementType) elementName,
#define DECL_E(elementName, elementType) elementName
#define DECL_F(elementName) , elementName
#define END_E_GEN_T(ENUM_NAME) \
  }                            \
  ;                            \
  std::string GetString##ENUM_NAME(enum ENUM_NAME index);
#include "internal/fldList.h"
#endif

// Support for std::visit over std::variant
#ifndef FLDTYPE_H_DECLARATION_OVERLOAD
#define FLDTYPE_H_DECLARATION_OVERLOAD
template <typename... Ts>
struct Overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;
#endif
