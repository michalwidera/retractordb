#pragma once

#include <cstdlib>
#include <iostream>

#include <boost/rational.hpp>

using namespace boost;

// https://stackoverflow.com/questions/35971827/c-boost-rational-class-floor-function

constexpr int floorR(boost::rational<int> const &num) { return static_cast<int>(num.numerator() / num.denominator()); }

constexpr int ceilR(boost::rational<int> const &num) {
  auto inum = static_cast<int>(num.numerator() / num.denominator());
  if (num == inum) return inum;
  return (num.numerator() > 0) ? ++inum : --inum;
}

inline bool Hash(const rational<int> &deltaA, const rational<int> &deltaB, const int i, int &retPos) {
  if (deltaA <= 0) {
    std::cerr << "\nFATAL: Hash: deltaA must be > 0\n";
    std::exit(EXIT_FAILURE);
  }
  if (deltaB <= 0) {
    std::cerr << "\nFATAL: Hash: deltaB must be > 0\n";
    std::exit(EXIT_FAILURE);
  }
  const rational<int> delta = (deltaA * deltaB) / (deltaA + deltaB);
  const rational<int> zet   = deltaB / (deltaA + deltaB);
  bool ret                  = floorR(zet * i) == floorR(zet * (i + 1));

  if (ret) {
    retPos = i - (floorR((i + 1) * delta));  // B
  } else {
    retPos = floorR(i * delta);  // A
  }
  return ret;
}

constexpr int Div(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
  return i + ceilR((i + 1) * deltaA / deltaB);
}

constexpr int Mod(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
  return i + floorR(i * deltaB / deltaA);
}

constexpr int Subtract(const rational<int> &deltaA, const rational<int> &deltaB, const int i) {
  return ceilR(i * deltaA / deltaB);
}

constexpr int agse(int offset, int step) { return floorR(boost::rational<int>(offset) / boost::rational<int>(step)); }
