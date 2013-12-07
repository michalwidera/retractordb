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
#include <boost/atomic.hpp>

#include <iostream>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>

//sudo apt-get install libncurses5-dev libncursesw5-dev libcdk5
#include <ncurses.h>
#include <panel.h>
#include <cdk/cdk.h>

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
//http://www.tldp.org/HOWTO/NCURSES-Programming-HOWTO/windows.html#WINDOWBASICS
//http://stackoverflow.com/questions/5749634/printing-inside-window-not-on-borders

WINDOW *my_win[2];
PANEL  *my_panels[2];

boost::lockfree::spsc_queue< pair<int,int> , boost::lockfree::capacity<1024> > spsc_queue;

int producer_count = 0 ;

boost::atomic<bool> done (false);

void producer(void)
{
  for (int i = 0; i != 100 ; ++i)
    {
      sleep( 1 );
      int value = ++producer_count;
      while (!spsc_queue.push( pair<int,int>( 1 , value ) ))
        ;
      if ( done ) break ;
    }
}

void consumer(void)
{
    pair<int,int> e_value ;
    while (!done) {
        while (spsc_queue.pop(e_value))
          {
            wprintw(my_win[0], "%d %d\n" , e_value.first , e_value.second );
            wrefresh(my_win[0]);
            //refresh();
          }
    }

    while (spsc_queue.pop(e_value))
      ;
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

  int x = 4 ,y = 2 ;
  int lines = 10, cols = 40 ;

  initscr();
  start_color();
  scrollok( stdscr , TRUE );
  cbreak();
  //noecho();

  // initialize color
  init_pair( 1 , COLOR_RED , COLOR_BLACK );
  init_pair( 2,  COLOR_GREEN, COLOR_BLACK );

  my_win[0] = newwin(lines, cols, y, x);
  my_win[1] = newwin(lines, cols, y + 1, x + 5);

  scrollok( my_win[0] , TRUE );
  scrollok( my_win[1] , TRUE );

  /*
   * Create borders around the windows so that you can see the effect
   * of panels
   */
  box(my_win[0], 0, 0);
  box(my_win[1], 0, 0);

  /* Attach a panel to each window */ 	/* Order is bottom up */
  my_panels[0] = new_panel(my_win[0]); 	/* Push 0, order: stdscr-0 */
  my_panels[1] = new_panel(my_win[1]); 	/* Push 1, order: stdscr-0-1 */

  set_panel_userptr( my_panels[0] , my_panels[1] );
  set_panel_userptr( my_panels[1] , my_panels[0] );

  update_panels();
  doupdate();

  top_panel( my_panels[0] );

  printw("Ready.\n");
  refresh();

  //
  // funkcja w tym watku wystartuje nasluch udp ze serwera
  //
  boost::thread producer_thread(producer);

  //
  // funkcja w tym watku wystartuje zasilanie konsoli danymi z kolejki
  //
  boost::thread consumer_thread(consumer);

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
              break ;
            }
          case Show:
            {
              using boost::property_tree::ptree ;
              ptree pt = netClient( "show" ,  what[1].str() ) ;
              break ;
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

  done = true;
  producer_thread.join();
  consumer_thread.join();

  endwin();

  return ;
}
