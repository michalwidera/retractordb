#include "fldType.h"
#include <boost/rational.hpp>

rdb::descFldVT cast(rdb::descFldVT inVar, rdb::descFld reqType);

boost::rational<int> Rationalize(double inValue, double DIFF =1E-6, int ttl =11);