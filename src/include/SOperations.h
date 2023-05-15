#pragma once

#include <boost/rational.hpp>

boost::rational<int> deltaHash(const boost::rational<int> &arg1, const boost::rational<int> &arg2);
boost::rational<int> deltaDivMod(const boost::rational<int> &arg1, const boost::rational<int> &arg2);
boost::rational<int> deltaSubtract(const boost::rational<int> &arg2);
boost::rational<int> deltaAdd(const boost::rational<int> &arg1, const boost::rational<int> &arg2);
boost::rational<int> deltaTimemove(const boost::rational<int> &arg1, const boost::rational<int> &arg2);

bool Hash(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i, int &retPos);
int Div(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i);
int Mod(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i);
int Subtract(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i);
int agse(int offset, int step);
