#pragma once

#include <any>
#include <boost/rational.hpp>

#include "fldType.hpp"

template <typename T>
struct cast {
  /// @brief Convert input value to requested descriptor type.
  /// @param inVar source value (std::variant or std::any specializations are used in this project)
  /// @param reqType target rdb field type
  /// @return converted value represented as type T
  T operator()(const T &inVar, rdb::descFld reqType);
};

/// @brief Convert std::any holding supported scalar value to descriptor variant.
/// @param a input value; must contain one of supported payload scalar types
/// @return value converted to rdb::descFldVT
/// @throws std::bad_any_cast when input type is unsupported or empty
rdb::descFldVT any_to_variant_cast(std::any a);

/// @brief Build default fallback value for a given descriptor field type.
/// @param type descriptor field type used to choose default value
/// @return zero/empty value encoded as rdb::descFldVT for the given type
rdb::descFldVT nullFallbackValue(rdb::descFld type);

/// @brief Approximate floating-point value as rational number.
/// @param inValue source floating-point value
/// @param DIFF precision threshold used to stop approximation
/// @param ttl_const maximum number of continued-fraction iterations
/// @return rational approximation of input value
boost::rational<int> Rationalize(const double inValue, const double DIFF = 1E-6, const int ttl_const = 11);
