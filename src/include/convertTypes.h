#pragma once

#include <boost/rational.hpp>

#include "fldType.h"

template <typename T>
struct cast {
  T operator()(const T& inVar, rdb::descFld reqType);
};

boost::rational<int> Rationalize(const double inValue, const double DIFF = 1E-6, const int ttl_const = 11);
