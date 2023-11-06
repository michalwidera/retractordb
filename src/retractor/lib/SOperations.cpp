#include <SOperations.h>

#include <algorithm>                 // for min
#include <cassert>                   // for assert
#include <stdexcept>                 // for out_of_range

using namespace boost;

// https://stackoverflow.com/questions/35971827/c-boost-rational-class-floor-function

static int floorR(boost::rational<int> const &num) { return static_cast<int>(num.numerator() / num.denominator()); }

static int ceilR(boost::rational<int> const &num) {
  auto inum = static_cast<int>(num.numerator() / num.denominator());
  return (num == inum) ? inum : ((num.numerator() > 0) ? ++inum : --inum);
}

bool Hash(const rational<int> &deltaA, const rational<int> &deltaB, const int i, int &retPos) {
  assert(deltaA > 0);
  assert(deltaB > 0);
  const rational<int> delta = deltaB / (deltaA + deltaB);
  bool ret                  = floorR(delta * i) == floorR(delta * (i + 1));
  if (ret) {
    retPos = i - (floorR((i + 1) * delta));  // B
  } else {
    retPos = floorR(i * delta);  // A
  }
  return ret;
}

int Div(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
  return i + ceilR((i + 1) * deltaA / deltaB);
}

int Mod(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
  return i + floorR(i * deltaB / deltaA);
}

int Subtract(const rational<int> &deltaA, const rational<int> &deltaB, const int i) { return ceilR(i * deltaA / deltaB); }

int agse(int offset, int step) { return floorR(boost::rational<int>(offset) / boost::rational<int>(step)); }
