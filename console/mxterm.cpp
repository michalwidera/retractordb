/*Copyright 2010-2014 Streams Lab Michal Widera

  RESTRICTED RIGHTS LEGEND
  Use, duplication or disclosure by any commercial industry (public or
  private), private individual or by any Government Agency without an
  expressed written consent of release from Streams Lab Michal Widera
  is subject to restriction of the Rights.

  Manufacturer is:
      Streams Lab Michal Widera
      Reymonta str. 56/1
      Poland, Zabrze, 41-800
      (+48) 606 995 918
*/

#include "mxterm.h"

#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <iostream>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <ncurses.h>

//zeby sie dalo: BOOST_FOREACH( cmd_e i, commands | map_keys )
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

enum cmd_e { Quit , Dir , Cls , Connect , Help , Detail , Show ,  NoCmd }  ;

enum { EVERYTHING_OK = 0 } ;

using namespace std ;

//deklaracja
boost::property_tree::ptree netClient( string netCommand , string netArgument );

std::map < cmd_e, boost::regex > commands ;

boost::cmatch what;

void gotoxy(int x,int y)
{
    printf("\033[%d;%df",y,x);
}

cmd_e processCommand( string commandStr )
{
  assert( commands.size() != 0 );

  using namespace boost::adaptors;

  BOOST_FOREACH( cmd_e i, commands | map_keys )
  {
    if ( regex_match(commandStr.c_str(),what,commands[i] ) )
      {
        return i ;
      }
  }

  return NoCmd ;
}

MxTerm::MxTerm(bool stopLoop){

  // ---- inicjacja tablicy polecen
  commands[ Quit ] = boost::regex("quit.*") ;
  commands[ Dir ] = boost::regex("\\s*dir\\s*(\\w*)\\s*(\\d*).*") ;
  commands[ Cls ] = boost::regex("cls.*") ;
  commands[ Connect ] = boost::regex("connect.*") ;
  commands[ Help ] = boost::regex("help.*") ;
  commands[ Detail ] = boost::regex("\\s*detail\\s*(\\w*).*") ;
  commands[ Show ] = boost::regex("\\s*show\\s*(\\w*).*") ;
  // ----

  postMortemRetCode = EVERYTHING_OK ;

  string commandTxt;
  cmd_e cmdId ;

  char str[80];

  initscr();
  printw("Ready.\n");
  refresh();

  try
  {
    do
      {
        getstr( str );
        cmdId = processCommand( str );
        switch ( cmdId )
          {
          case Connect:
            {
              using boost::property_tree::ptree ;
              ptree pt = netClient( "get" , "" ) ;

              BOOST_FOREACH(ptree::value_type &v,
                            pt.get_child("db.stream"))
              {
                printw( "%s\n" , v.second.get<std::string>("").c_str()  );
              }
            }
          }
      }
    while ( cmdId != Quit );
    endwin();
  }
  catch(std::exception& e)
  {
    endwin();
    cerr << e.what() << "\n";
  }

  return ;
}
