#include <boost/rational.hpp>

#include "fldType.h"

void cast(const rdb::descFldVT& inVar, rdb::descFldVT& retVal, rdb::descFld reqType);

boost::rational<int> Rationalize(const double inValue, const double DIFF = 1E-6,
                                 const int ttl_const = 11);
