// This file is not a regular header file.
// It is goes 3x times in fldType as it is - and this is intentional
// 3 times these macros generate 3 different code in fldType.h
// * Be careful here - there is something non c++ classic
// ! Pragma once or #indef barrier should not appear here!

// This declaration goes into ::rdb namespace
namespace rdb {
BEGIN_E_GEN_T(descFld)
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
END_E_GEN_T(descFld)
}  // namespace rdb
#undef BEGIN_E_GEN_T
#undef END_E_GEN_T
#undef DECL_T
#undef DECL_E
#undef DECL_F