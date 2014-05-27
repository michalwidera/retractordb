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

#include <boost/assign.hpp>

//sudo apt-get install libncurses5-dev libncursesw5-dev libcdk5 libcdk5-dev
#include <ncurses.h>
#include <panel.h>

//zeby sie dalo: BOOST_FOREACH( cmd_e i, commands | map_keys )
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

//dla potrzeb transmisji danych
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/xml_parser.hpp>

const short multicast_port = 30001;

enum cmd_e { Quit , Dir , Cls , Connect , Help , Detail , Show , Add , Del ,  NoCmd }  ;

using namespace std ;
using namespace boost::assign;
using boost::property_tree::ptree;

//deklaracja
boost::property_tree::ptree netClient( string netCommand , string netArgument );

std::map < cmd_e, boost::regex > commandsMap = map_list_of
    ( Quit    , boost::regex("\\s*quit.*") )
    ( Dir     , boost::regex("\\s*dir\\s*(\\w*)\\s*(\\d*).*") )
    ( Cls     , boost::regex("\\s*cls.*") )
    ( Connect , boost::regex("\\s*connect.*") )
    ( Help    , boost::regex("\\s*help.*") )
    ( Detail  , boost::regex("\\s*detail\\s*(\\w*).*") )
    ( Show    , boost::regex("\\s*show\\s*(\\w*).*") )
    ( Add     , boost::regex("\\s*add.*") )
    ( Del     , boost::regex("\\s*del.*") )
    ;

boost::cmatch what;

cmd_e processCommand( string commandsMaptr )
{
  assert( commandsMap.size() != 0 );

  using namespace boost::adaptors;

  BOOST_FOREACH( cmd_e i, commandsMap | map_keys )
  {
    if ( regex_match(commandsMaptr.c_str(),what,commandsMap[i] ) )
      {
        return i ;
      }
  }

  return NoCmd ;
}
//http://www.tldp.org/HOWTO/NCURSES-Programming-HOWTO/windows.html#WINDOWBASICS
//http://stackoverflow.com/questions/5749634/printing-inside-window-not-on-borders
//http://invisible-island.net/ncurses/ncurses-intro.html

boost::lockfree::spsc_queue< ptree , boost::lockfree::capacity< 1024 > > spsc_queue;

int producer_count = 0 ;

boost::atomic<bool> done (false);

vector < WINDOW * > my_win ;

//
// Proces konsumenta asynchronicznie pobiera dane z kolejki i umieszcza na ekranie
//
void consumer()
{
  ptree e_value ;
  while (!done)
    {
      while (spsc_queue.pop(e_value))
        {
          if ( my_win.size() > 0 )
            {
              //int wnd = rand()% my_win.size() ;

              int stream = boost::lexical_cast<int>( e_value.get("stream" , "" )  ) ;

              if ( stream < my_win.size() )
              {
                  wprintw( my_win[stream], "%d %s\n" , stream , e_value.get("field_a" , "" ).c_str() );
                  wrefresh(my_win[stream]);
              }
            }
        }
    }

  while (spsc_queue.pop(e_value))
    ;
}


class receiver
{
public:
  receiver(boost::asio::io_service& io_service,
      const boost::asio::ip::address& listen_address,
      const boost::asio::ip::address& multicast_address)
    : socket_(io_service) ,
      timer_(io_service) ,
      listen_address( listen_address ),
      multicast_address( multicast_address )
  {
    startRcv();
  }

  void startRcv()
  {
    // Create the socket so that multiple may be bound to the same address.
    boost::asio::ip::udp::endpoint listen_endpoint( listen_address, multicast_port);
    socket_.open(listen_endpoint.protocol());
    socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    socket_.set_option(boost::asio::ip::multicast::enable_loopback(true));
    socket_.set_option(boost::asio::ip::multicast::hops(1));
    socket_.bind(boost::asio::ip::udp::endpoint( multicast_address, multicast_port ));

    // Join the multicast group.
    socket_.set_option(
        boost::asio::ip::multicast::join_group(multicast_address));

    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&receiver::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));

    timer_.expires_from_now(boost::posix_time::seconds(3)); //co trzy sekundy umiera proces zeby sie odrodzic
    timer_.async_wait(boost::bind(&receiver::close, this));
  }

  void close()
  {
    socket_.close();
    if ( !done )
      {
        startRcv();
      }
  }

  void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
  {
    if ( done )
      {
        socket_.close();
        return ;
      }

    if (!error)
      {
        /*
            std::cout.write(data_, bytes_recvd);
            std::cout << std::endl;
            */

        ptree pt;

        {
            std::stringstream strstream;
            strstream << data_ ;
            read_xml( strstream , pt );
        }

        while (!spsc_queue.push( pt ))
          ;

        socket_.async_receive_from(
              boost::asio::buffer(data_, max_length), sender_endpoint_,
              boost::bind(&receiver::handle_receive_from, this,
                          boost::asio::placeholders::error,
                          boost::asio::placeholders::bytes_transferred));
      }
  }

private:
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint sender_endpoint_;
  boost::asio::deadline_timer timer_;
  enum { max_length = 1024 };
  char data_[max_length];

  boost::asio::ip::address listen_address ;
  boost::asio::ip::address multicast_address ;
};

//
// Proces producenta pobiera dane z sieciowego strumienia danych i umieszcza w kolejce
//
void producer(void)
{
  /*
  for (int i = 0; i != 100 ; ++i)
    {
      sleep( 1 );
      int value = ++producer_count;
      while (!spsc_queue.push( pair<int,int>( 1 , value ) ))
        ;
      if ( done ) break ;
    }
  */

  boost::asio::io_service io_service;
  //boost::asio::deadline_timer timer_(io_service);
  receiver r(io_service,
             boost::asio::ip::address::from_string("0.0.0.0"),
             boost::asio::ip::address::from_string("239.255.0.1"));
  io_service.run();
}


MxTerm::MxTerm(bool stopLoop){

                             //http://linux.die.net/man/3/cbreak
  initscr();                 //
  start_color();             //
  scrollok( stdscr , TRUE ); //The scrollok controls cursor of a window is moved off the edge of the window
  keypad(stdscr, TRUE);      //if enabled (bf is TRUE), the user can press a function key
  cbreak();                  //The cbreak routine disables line buffering and erase/kill character-processing
  echo();                    //characters typed by the user are echoed by getch as they are typed.

  //
  // initialize color - kolorki poszczegolnych okien
  //
  init_pair( 1 , COLOR_BLACK, COLOR_RED  );
  init_pair( 2,  COLOR_BLACK, COLOR_GREEN );
  init_pair( 3,  COLOR_BLACK, COLOR_YELLOW );

  init_pair( 4 , COLOR_BLACK, COLOR_MAGENTA );
  init_pair( 5,  COLOR_BLACK, COLOR_CYAN );
  init_pair( 6,  COLOR_BLACK, COLOR_BLUE );

  // szerokosc, wysokosc okiernka ze strumieniem
  int width = 15 ;
  int height = 10 ;

  // poczatek lewego gornego, pierwszego okienka
  int x_offset = 30 ;
  int y_offset = 2 ;

  //
  // Rozklad okien ze strumieniami na ekranie
  //
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
    printw("Ready.\n");
    refresh();

    // Deklaraca
    cmd_e cmdId = Quit ;
    const int MAX_COMMAND_STRING_SIZE = 80 ;

    char str[MAX_COMMAND_STRING_SIZE];

    do
      {
        getnstr( str , MAX_COMMAND_STRING_SIZE );
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
          case Help:
            {
                printw( "DONE: add,del,show,connect,help,quit\n" );
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
              refresh();
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

  printw("Done.\n");
  refresh();

  done = true;
  producer_thread.join();
  consumer_thread.join();

  endwin();

  return ;
}
