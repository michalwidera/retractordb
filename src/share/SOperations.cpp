#include <SOperations.h>
#include <assert.h>                  // for assert
#include <algorithm>                 // for min
#include <boost/core/enable_if.hpp>  // for enable_if_c<>::type
#include <stdexcept>                 // for out_of_range

using namespace boost ;


//https://stackoverflow.com/questions/35971827/c-boost-rational-class-floor-function
namespace boost {
    template <typename IntType>
    constexpr IntType floor(rational<IntType> const &num) {
        return static_cast<IntType>(num.numerator() / num.denominator());
    }
    template <typename IntType>
    constexpr IntType ceil(rational<IntType> const &num) {
        auto inum = static_cast<IntType>(num.numerator() / num.denominator());
        return (num == inum) ? inum : ((num.numerator() > 0) ? ++ inum : --inum) ;
    }
}

/*
for i in range(0,10):
     if floor(i*delta)==floor((i+1)*delta):
         print B[i-int(floor((i+1)*delta))],
     else:
         print A[int(floor(i*delta))],

deltaC = (deltaA*deltaB)/(deltaA+deltaB)
*/

rational<int> deltaHash(const rational<int> &arg1, const rational<int> &arg2) {
    assert(arg1 + arg2 != 0);
    return (arg1 * arg2) / (arg1 + arg2);
}

bool Hash(const rational<int> &deltaA, const rational<int> &deltaB, const int i, int &retPos) {
    assert(deltaA > 0);
    assert(deltaB > 0);
    const rational<int> delta = deltaB / (deltaA + deltaB) ;
    bool ret = floor(delta * i) == floor(delta * (i + 1));

    if (ret) {
        retPos = i - (floor((i + 1) * delta));        //B
    } else {
        retPos = floor(i * delta);    //A
    }

    return ret ;
}

/*
#hash - begin
delta=deltaB/(deltaA+deltaB)

for i in range(0,24):
    if int(i*delta)==int((i+1)*delta):
        C.append( B[i-int((i+1)*delta)] )
    else:
        C.append( A[int(i*delta)] )
#hash - end

#div - begin
deltaC=(deltaA*deltaB)/(deltaA+deltaB)

deltaA_ = deltaB*deltaC/(deltaB-deltaC)
assert(deltaA_ == deltaA)
from math import ceil
for i in range(0,5) :
    print C[i+int(ceil((i+1)*deltaA/deltaB))],    <--- tu jest to div
#Output: 1 2 3 4 5

#div- end
*/

int Div(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
    return i + ceil((i + 1) * deltaA / deltaB);
}

/*
#hash - begin
delta=deltaB/(deltaA+deltaB)

for i in range(0,24):
    if int(i*delta)==int((i+1)*delta):
        C.append( B[i-int((i+1)*delta)] )
    else:
        C.append( A[int(i*delta)] )

#hash - end

#mod - begin
deltaC=(deltaA*deltaB)/(deltaA+deltaB)

deltaB_ = deltaA*deltaC/(deltaA-deltaC)
assert(deltaB_ == deltaB)
for i in range(0,10) :
    print C[i+int(i*deltaB/deltaA)],                <--- tu jest mod
#Output: a b c d e f g h i j#

#mod - end
*/


int Mod(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
    return i + floor(i * deltaB / deltaA);
}

/* Ta funkcja jest taka sama dla obu operacji */

rational<int> deltaDivMod(const rational<int> &arg1, const rational<int> &arg2) {
    assert(arg1 != arg2);

    if (arg1 == arg2) {
        throw std::out_of_range("Delta are equal in DehashDiv - undefinied.");
    }

    return (arg1 * arg2) / abs(arg1 - arg2);
}

/*
from math import ceil
for i in range(0,10):
    if deltaA > deltaB :
        print C[int(ceil(i*deltaA/deltaB))][0],
    else:
        print C[i][0],
*/

rational<int> deltaSubstract(const rational<int> &arg1) {
    return arg1 ;
}
//todo
int Substract(const rational<int> &deltaA, const rational<int> &deltaB, const int i) {
    return ceil(i * deltaA / deltaB);
}

rational<int> deltaAdd(const rational<int> &arg1, const rational<int> &arg2) {
    return std::min(arg1, arg2) ;
}

/*
deltaC = min( deltaA,deltaB )
for i in range(0,10):
     if deltaC == deltaA:
         print str(A[i])+B[int(i*deltaA/deltaB)],
     else:
         print str(A[int(i*deltaB/deltaA)])+B[i],
*/


rational<int> deltaTimemove(const rational<int> &arg1, const rational<int> &arg2) {
    return arg1 ;
}

int agse(int offset, int step) {
    return floor(boost::rational<int> (offset) / boost::rational<int> (step));
}

void SOperations_regtest() {
    boost::rational<int> a(1, 2) ;
    boost::rational<int> b(1, 3) ;
    boost::rational<int> c(2, 3) ;
    boost::rational<int> d(5, 4) ;
    boost::rational<int> e(1) ;
    boost::rational<int> f(0) ;
    boost::rational<int> g(-2, 3);
    boost::rational<int> h(-1);
    boost::rational<int> j(-5, 4);
    assert(floor(a) == 0);
    assert(floor(b) == 0);
    assert(floor(c) == 0);
    assert(floor(d) == 1);
    assert(floor(e) == 1);
    assert(floor(f) == 0);
    assert(floor(g) == 0);
    assert(floor(h) == -1);
    assert(floor(j) == -1);
    assert(ceil(a) == 1);
    assert(ceil(b) == 1);
    assert(ceil(c) == 1);
    assert(ceil(d) == 2);
    assert(ceil(e) == 1);
    assert(ceil(f) == 0);
    assert(ceil(g) == -1);
    assert(ceil(h) == -1);
    assert(ceil(j) == -2);
}