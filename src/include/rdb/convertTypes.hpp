#pragma once

#include <any>
#include <boost/rational.hpp>

#include "fldType.hpp"

inline constexpr double kDefaultRationalizeDiff    = 1E-6;
inline constexpr int kDefaultRationalizeIterations = 11;

template <typename T>
struct cast {
  /// @brief Convert input value to requested descriptor type.
  /// @param inVar source value (std::variant or std::any specializations are used in this project)
  /// @param reqType target rdb field type
  /// @return converted value represented as type T, or std::monostate (NULL) when the value has
  ///         no representation in the requested type: a floating-point value outside an integer
  ///         type's range (NaN and infinity included), an integer or rational value outside the
  ///         range of a narrower integer type (a negative value to UINT, UINT above INT_MAX to
  ///         INTEGER or RATIONAL, a value outside 0..255 to BYTE), any scalar to INTPAIR (a pair is
  ///         two independent numbers, not a fraction), or a string that does not parse
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

/// @brief Approximate floating-point value as rational number by its continued-fraction expansion.
/// @param inValue source floating-point value
/// @param DIFF precision threshold used to stop approximation
/// @param ttl_const maximum number of continued-fraction iterations
/// @return the last convergent whose numerator and denominator both fit in `int` - the expansion stops
///         early when the next one would not - negated for a negative input; {0,1} for NaN, infinity
///         and any value whose magnitude truncates to 2^31 or more (the function is total; NULL for
///         such input is decided on the conversion path, not here)
boost::rational<int> Rationalize(double inValue, double DIFF = kDefaultRationalizeDiff,
                                 int ttl_const = kDefaultRationalizeIterations);
