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

//sudo apt-get install libncurses5-dev libncursesw5-dev
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

WINDOW *create_newwin(int height, int width, int starty, int startx)
{	WINDOW *local_win;

	local_win = newwin(height, width, starty, startx);
	box(local_win, 0 , 0);		/* 0, 0 gives default characters
					 * for the vertical and horizontal
					 * lines			*/
	wrefresh(local_win);		/* Show that box 		*/

	return local_win;
}

void destroy_win(WINDOW *local_win)
{
	/* box(local_win, ' ', ' '); : This won't produce the desired
	 * result of erasing the window. It will leave it's four corners
	 * and so an ugly remnant of window.
	 */
	wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	/* The parameters taken are
	 * 1. win: the window on which to operate
	 * 2. ls: character to be used for the left side of the window
	 * 3. rs: character to be used for the right side of the window
	 * 4. ts: character to be used for the top side of the window
	 * 5. bs: character to be used for the bottom side of the window
	 * 6. tl: character to be used for the top left corner of the window
	 * 7. tr: character to be used for the top right corner of the window
	 * 8. bl: character to be used for the bottom left corner of the window
	 * 9. br: character to be used for the bottom right corner of the window
	 */
	wrefresh(local_win);
	delwin(local_win);
}

WINDOW *my_win;

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
            wprintw(my_win, "%d %d\n" , e_value.first , e_value.second );
            wrefresh(my_win);
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

  initscr();
  printw("Ready.\n");
  refresh();

  int startx = 30 , starty = 5 , width = 30 , height = 10 ;
  my_win = create_newwin(height, width, starty, startx);

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
