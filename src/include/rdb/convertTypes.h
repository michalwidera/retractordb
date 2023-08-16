#pragma once

#include <any>
#include <boost/rational.hpp>

#include "fldType.h"

template <typename T>
struct cast {
  T operator()(const T &inVar, rdb::descFld reqType);
};

rdb::descFldVT any_to_variant_cast(std::any a);

boost::rational<int> Rationalize(const double inValue, const double DIFF = 1E-6, const int ttl_const = 11);
