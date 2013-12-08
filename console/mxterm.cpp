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

//zeby sie dalo: BOOST_FOREACH( cmd_e i, commands | map_keys )
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

enum cmd_e { Quit , Dir , Cls , Connect , Help , Detail , Show , Add , Del ,  NoCmd }  ;

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
//http://invisible-island.net/ncurses/ncurses-intro.html


boost::lockfree::spsc_queue< pair<int,int> , boost::lockfree::capacity<1024> > spsc_queue;

int producer_count = 0 ;

boost::atomic<bool> done (false);

vector < WINDOW * > my_win ;

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

void consumer()
{
    pair<int,int> e_value ;
    while (!done) {
        while (spsc_queue.pop(e_value))
          {
            if ( my_win.size() > 0 )
              {
                int wnd = rand()% my_win.size() ;
                assert( wnd < my_win.size() );

                wprintw( my_win[wnd], "%d %d\n" , e_value.first , e_value.second );
                wrefresh(my_win[wnd]);
              }
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
  commands[ Add ] = boost::regex("\\s*add.*");
  commands[ Del ] = boost::regex("\\s*del.*");
  // ----

  postMortemRetCode = EVERYTHING_OK ;

  string commandTxt;
  cmd_e cmdId ;
  char str[80];

  //WINDOW *my_win[2];
  PANEL  *my_panel[2];

                             //http://linux.die.net/man/3/cbreak
  initscr();                 //
  start_color();             //
  scrollok( stdscr , TRUE ); //The scrollok controls cursor of a window is moved off the edge of the window
  keypad(stdscr, TRUE);      //if enabled (bf is TRUE), the user can press a function key
  cbreak();                  //The cbreak routine disables line buffering and erase/kill character-processing
  echo();                    //characters typed by the user are echoed by getch as they are typed.

  // initialize color
  init_pair( 1 , COLOR_BLACK, COLOR_RED  );
  init_pair( 2,  COLOR_BLACK, COLOR_GREEN );
  init_pair( 3,  COLOR_BLACK, COLOR_YELLOW );

  init_pair( 4 , COLOR_BLACK, COLOR_MAGENTA );
  init_pair( 5,  COLOR_BLACK, COLOR_CYAN );
  init_pair( 6,  COLOR_BLACK, COLOR_BLUE );

  int width = 15 ;
  int height = 10 ;
  int x_offset = 30 ;
  int y_offset = 2 ;

  struct pos {
    int c,w,h,x,y ;
    pos(){};
    pos( int c , int w, int h, int x, int y ) : c(c),w(w),h(h),x(x),y(y) {} ;
  } m_pos[10] = {
    pos( 1 , width , height , y_offset + 0 * ( height + 1 ) , x_offset + 0 * ( width + 1 ) ) ,
    pos( 2,  width , height , y_offset + 0 * ( height + 1 ) , x_offset + 1 * ( width + 1 ) ) ,
    pos( 3,  width , height , y_offset + 0 * ( height + 1 ) , x_offset + 2 * ( width + 1 ) ) ,
    pos( 4,  width , height , y_offset + 1 * ( height + 1 ) , x_offset + 0 * ( width + 1 ) ) ,
    pos( 5,  width , height , y_offset + 1 * ( height + 1 ) , x_offset + 1 * ( width + 1 ) ) ,
    pos( 6,  width , height , y_offset + 1 * ( height + 1 ) , x_offset + 2 * ( width + 1 ) ) ,
  } ;

  printw("Ready.\n");
  refresh();            //taka kolejnosc - bo inaczej wyczysci sie caly ekran

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
            }
            break ;
          case Show:
            {
              using boost::property_tree::ptree ;
              ptree pt = netClient( "show" ,  what[1].str() ) ;
            }
            break ;
          case Add:
            {
              int i = my_win.size();
              my_win.push_back( newwin( m_pos[i].h, m_pos[i].w , m_pos[i].x , m_pos[i].y ) );
              wbkgd( my_win[i], COLOR_PAIR( m_pos[i].c ) );
              scrollok( my_win[i] , TRUE );
              wrefresh( my_win[i] );
            }
            break ;
          case Del:
            {
              my_win.pop_back();
              refresh();            //taka kolejnosc - bo inaczej wyczysci sie caly ekran
              BOOST_FOREACH( WINDOW * w , my_win ) wrefresh( w );
            }
            break ;
          }
      }
    while ( cmdId != Quit );
  }
  catch(std::exception& e)
  {
    cerr << e.what() << "\n";
  }

  done = true;
  producer_thread.join();
  consumer_thread.join();

  endwin();

  return ;
}
