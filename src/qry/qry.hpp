#pragma once

#include <boost/property_tree/ptree.hpp>
#include <string>

int _getch();
void consumer();
void producer();
bool select( bool noneedctrlc );
void dir();
int hello();
void setmode( std::string mode );
bool detailShow();
boost::property_tree::ptree netClient( std::string netCommand, std::string netArgument );

extern std::string sInputStream;
extern int iTimeLimitCnt;