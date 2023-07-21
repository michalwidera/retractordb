// This file is not a regular header file.
// It is goes 3x times in fldType as it is - and this is intentional
// 3 times these macros generate 3 different code in fldType.h
// * Be careful here - there is something non c++ classic
// ! Pragma once or #indef barrier should not appear here!

// NOLINTBEGIN(cert-err58-cpp)

// This declaration goes into ::rdb namespace
#define INTINT int, int
#define STRINT std::string, int
namespace rdb {
BEGIN_E_GEN_T(descFld)
DECL_T(BYTE, uint8_t)
DECL_T(INTEGER, int)
DECL_T(UINT, unsigned)
DECL_T(RATIONAL, boost::rational<int>)
DECL_T(FLOAT, float)
DECL_T(DOUBLE, double)
DECL_T(INTPAIR, std::pair<INTINT>)
DECL_T(IDXPAIR, std::pair<STRINT>)
DECL_E(STRING, std::string)
DECL_F(TYPE)
DECL_F(REF)
END_E_GEN_T(descFld)
}  // namespace rdb
#undef BEGIN_E_GEN_T
#undef END_E_GEN_T
#undef DECL_T
#undef DECL_E
#undef DECL_F

// NOLINTEND(cert-err58-cpp)
