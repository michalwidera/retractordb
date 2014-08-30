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

/*
 * Zalozenia projektowe programu konsoli uzytkownika
 * Konsola:
 * 1. jest najprostszym interfejsem uzytkownika dla systemu zarzadzania danymi
 * 2. udostepnima dane na biezaco, zgodnie ze zmianami danych w bazie
 * 3. dane raz wyswietlone ulegaja pozniejszym zmianom (biezacej aktualizacji)
 * 4. komunikuje sie z baza uzgodnionym protokolem sieciowym
 * 5. jest przenosna pomiedzy systemami (Lin/Win)
 * 6. dziala w srodowisku tekstowym
 * TODO: Wez sie w garsc to posprzataj ten bajzel tutaj
 */

#include "stdafx.h"

#include <ctime>

#include <cstdio>
#include <../share/rlutil.h>

#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/thread.hpp>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <istream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/find.hpp>

//dla potrzeb transmisji danych
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic.hpp>

//zeby sie dalo: BOOST_FOREACH( cmd_e i, commands | map_keys )
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

#include <boost/assign.hpp>

using namespace std ;
using namespace boost ;
using namespace boost::assign;

const short multicast_port = 30001;

using boost::asio::ip::tcp ;
using boost::property_tree::ptree;

boost::lockfree::spsc_queue< ptree , boost::lockfree::capacity< 1024 > > spsc_queue;

int producer_count = 0 ;

boost::atomic<bool> done (false);

std::map <string,ptree> streamTable ;
std::list <string> streamList ;

//
// Zestawienie polaczenia
//
namespace dbNet {
  string sHostName ;

  boost::asio::io_service io_service;
  tcp::resolver resolver(io_service);
  tcp::socket socket(io_service);
}


enum cmd_e { Quit , Cls , Connect , Help , Show , Add , Del ,  NoCmd }  ;

std::map < cmd_e, boost::regex > commandsMap = map_list_of
    ( Quit    , boost::regex("\\s*quit.*") )
    ( Cls     , boost::regex("\\s*cls.*") )
    ( Connect , boost::regex("\\s*connect.*") )
    ( Help    , boost::regex("\\s*help.*") )
    ( Show    , boost::regex("\\s*show\\s*(\\w*).*") )
    ( Add     , boost::regex("\\s*add\\s*(\\w*).*") )
    ( Del     , boost::regex("\\s*del\\s*(\\w*).*") )
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
              string stream = e_value.get("stream" , "" )  ;

              using namespace boost::adaptors;

              BOOST_FOREACH( string w , streamTable | map_keys )
                  if ( w == stream )
              {
                  int count = boost::lexical_cast<int> ( e_value.get("count" , "" ) );

                  //printf("%s\n\r|",stream.c_str());
                  printf("|");
                  for ( int i = 0 ; i < count ; i ++ )
                    printf( "%s|" , e_value.get( boost::lexical_cast<string>(i) , "" ).c_str() );

                  printf("\n\r");
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
            memset( data_ , 0 , max_length );
            read_xml( strstream , pt );
        }

        string stream = pt.get("stream" , "" )  ;

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


ptree netClient( string netCommand , string netArgument )
{
  const int max_length = 1024;

  ptree pt_response ; //Zwracana wartosc/odpowiedz przez funkcje
  ptree pt_request;   //Tutaj konstruowane jest pytanie

  pt_request.put("db.query", "hello" ); //potem to wywalic

  try
  {
    boost::asio::io_service io_service;
    tcp::socket socket(io_service);
    boost::asio::connect(socket,
                         tcp::resolver(io_service).resolve(
                           tcp::resolver::query(
                             tcp::v4(), "localhost", "1971" )));

    pt_request.put("db.message", netCommand );

    if ( netArgument != "" )
      pt_request.put("db.argument", netArgument );

    //
    //request part
    //

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);

    write_xml( request_stream , pt_request );

    // Send the request.
    boost::asio::write(socket, request);

    //
    //response part
    //

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;

    // Check that response is OK.
    std::istream response_stream(&response);

    if (response_stream)
      {
        // Read until EOF, writing data to output as we go.
        boost::system::error_code error;
        while (boost::asio::read(socket, response,
                                 boost::asio::transfer_at_least(1), error))
          {
            if ( error == boost::asio::error::eof )
              {
                break ;
              }
          }

        if (error != boost::asio::error::eof)
          {
            throw boost::system::system_error(error);
          }

        read_xml( response_stream , pt_response );
      }
    else
      {
        pt_response.put("error.response" , "invalid" );
      }
  }
  catch (std::exception& e)
  {
    std::cerr << "exception catch:" << e.what() << std::endl ;
    pt_response.put("error.response" , e.what() );
  }

  return pt_response ;
}

int main(int argc, char* argv[])
{
  //Intro
  cerr << argv[0] << " - terminal systemu.\n";
  cerr << "Tag kompilatora:" + string(COMPILER) +
          ", CTS:" << string( __TIMESTAMP__) <<	//http://msdn2.microsoft.com/en-us/library/b0084kay(VS.80).aspx
          std::endl ;
  cerr << "Boost wersja:" << BOOST_LIB_VERSION << std::endl ;

  try {
    namespace po = boost::program_options;

    string sInputFile ;

    po::options_description desc("Dozwolone opcja");
    desc.add_options()
        ("server,s", po::value<string>(&dbNet::sHostName)->default_value("localhost") , "nazwa serwera")
        ("help,h", "program options")
        ;

    po::positional_options_description p;       //Assume that infile is the first option
    p.add("infile", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
        cerr << desc << endl ;
        return 1;
      }

    //
    // funkcja w tym watku wystartuje nasluch udp ze serwera
    //
    boost::thread producer_thread(producer);

    //
    // funkcja w tym watku wystartuje zasilanie konsoli danymi z kolejki
    //
    boost::thread consumer_thread(consumer);

    //Tutaj jest wlasciwe wywolanie terminala w trybie tekstowym
    char polecenie[1024];
    memset( polecenie , 0 ,  1024 );
    int cnt = 0;
    char kreska[] = "-/|\\";
    int key;
    int i=0;
    setbuf(stdout, NULL);

    cmd_e cmdId = NoCmd ;

    do
      { 

        if ( kbhit() )
          {
            polecenie[ cnt ++ ] = getch();
            printf( "%c" , polecenie[ cnt - 1 ] );

            if ( polecenie[ cnt - 1 ] == 13 )
              {
                cmdId = processCommand( polecenie );
                cnt = 0 ;
                memset( polecenie , 0 , 1024 );
              }
          }

        if ( cmdId == Connect )
          {
            cmdId = NoCmd ;

            ptree pt = netClient( "get" , "" ) ;

            BOOST_FOREACH(ptree::value_type &v,
                          pt.get_child("db.stream"))
            {
              string stream = v.second.get<std::string>("") ;
              printf( "%s\n" , stream.c_str() );
              streamList.push_back( stream );
              //streamTable[ stream ] = netClient( "show" , stream ) ;
            }
          }

        if ( cmdId == Add )
          {
            list<string>::iterator i = boost::find(streamList, what[1].str());
            if (i != boost::end(streamList))
              {
                printf(":%s:\n",(*i).c_str());
                streamTable[ (*i) ] =  netClient( "show" , (*i) )  ;
              }
          }

        /*
        else
          {
            printf("%c\b",kreska[(i++)%4]);
          }
          */
      }
    while ( cmdId != Quit );

    done = true;
    producer_thread.join();
    consumer_thread.join();

    printf("\nEND\n");

    return true ; //instance.postMortemRetCode ;
  }
  catch(std::exception& e)
  {
    cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
