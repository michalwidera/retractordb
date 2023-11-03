#pragma once

#include <boost/property_tree/ptree.hpp>
#include <string>

bool select(bool, int, const std::string &);
void dir();
int hello();
void setmode(std::string const &);
bool detailShow(const std::string &);
boost::property_tree::ptree netClient(std::string, const std::string &);
