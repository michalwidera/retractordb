#pragma once

#include <boost/rational.hpp>

bool Hash(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i, int &retPos);
int Div(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i);
int Mod(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i);
int Subtract(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i);
int agse(int offset, int step);
