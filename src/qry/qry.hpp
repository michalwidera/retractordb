#include <boost/property_tree/ptree.hpp>
#include <string>

int _getch();
void consumer();
void producer();
void select( bool needctrlc );
int hello();
void setmode( std::string mode );
boost::property_tree::ptree netClient( std::string netCommand, std::string netArgument );

extern std::string sInputStream;
extern int iTimeLimitCnt;