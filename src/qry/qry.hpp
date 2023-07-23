#pragma once

#include <boost/property_tree/ptree.hpp>
#include <string>

int _getch();
void consumer();
void producer();
bool select(bool noneedctrlc, int timeLimit, const std::string &input);
void dir();
int hello();
void setmode(std::string const &mode);
bool detailShow(const std::string &input);
boost::property_tree::ptree netClient(std::string netCommand, const std::string &netArgument);
