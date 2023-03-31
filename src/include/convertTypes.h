#include <boost/rational.hpp>

#include "fldType.h"

void cast(const rdb::descFldVT& inVar, rdb::descFldVT& retVal, rdb::descFld reqType);

boost::rational<int> Rationalize(double inValue, double DIFF = 1E-6, int ttl = 11);
