/*Copyright 2010-2012 Streams Lab Michal Widera

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

#include "stdafx.h"

#include <boost/static_assert.hpp>

#include <fsmtemplate.h>
#include <inputFileInstance.h>
#include <QStruct.h>
#include <ctime>

using namespace std ;
using namespace boost ;
using namespace QS ;
using namespace fsm ;

//namespace mpl = boost::mpl;
using namespace boost::mpl::placeholders;

#include <cassert>

extern "C" {
    qTree coreInstance ;
    config configInstance ;
}

extern int main_rlutil() ;
extern int main_bpt_server() ;
extern int main_bpt_client() ;

struct play {};
struct open_close {};
struct cd_detected {
    cd_detected(char const*, std::vector<std::clock_t> const&) {}
};
struct pauze {};
struct stop {};

// konkretna implementacja automatu
class player : public state_machine<player>
{
    // lista stanów
    enum states {
        Empty, Open, Stopped, Playing, Paused, initial_state = Empty
    };

    void start_playback(play const&);
    void open_drawer(open_close const&);
    void close_drawer(open_close const&);
    void store_cd_info(cd_detected const&);
    void stop_playback(stop const&);
    void pause_playback(pauze const&);
    void resume_playback(play const&);
    void stop_and_open(open_close const&);

    friend class state_machine<player>;

    typedef player p; // upraszcza zapis tabeli przejœæ

    // tabela przejœæ
    struct transition_table : mpl::vector11<

    //    Aktualny   Zdarzenie    Nastêpny      Akcja
    //  +---------+-------------+---------+---------------------+
    row < Stopped , play        , Playing , &p::start_playback  >,
    row < Stopped , open_close  , Open    , &p::open_drawer     >,
    //  +---------+-------------+---------+---------------------+
    row < Open    , open_close  , Empty   , &p::close_drawer    >,
    //  +---------+-------------+---------+---------------------+
    row < Empty   , open_close  , Open    , &p::open_drawer     >,
    row < Empty   , cd_detected , Stopped , &p::store_cd_info   >,
    //  +---------+-------------+---------+---------------------+
    row < Playing , stop        , Stopped , &p::stop_playback   >,
    row < Playing , pauze       , Paused  , &p::pause_playback  >,
    row < Playing , open_close  , Open    , &p::stop_and_open   >,
    //  +---------+-------------+---------+---------------------+
    row < Paused  , play        , Playing , &p::resume_playback >,
    row < Paused  , stop        , Stopped , &p::stop_playback   >,
    row < Paused  , open_close  , Open    , &p::stop_and_open   >
    //  +---------+-------------+---------+---------------------+

    > {};
};

void player::start_playback(play const&){ std::cerr << "start "; }
void player::open_drawer(open_close const&){ std::cerr << "open "; }
void player::close_drawer(open_close const&){ std::cerr << "close "; }
void player::store_cd_info(cd_detected const&){ std::cerr << "store "; }
void player::stop_playback(stop const&){ std::cerr << "stop ";}
void player::pause_playback(const pauze &){ std::cerr << "pause ";}
void player::resume_playback(play const&){ std::cerr << "resume "; }
void player::stop_and_open(open_close const&){ std::cerr << "stop-open ";}

int fsmTest(){
    std::cerr << "*Program main body - start*" << std::endl ;

    player pl;                      // egzemplarz automatu

    pl.process_event(open_close()); // u¿ytkownik otwiera odtwarzacz CD
    pl.process_event(open_close()); // wk³ada CD i zamyka tackê
    pl.process_event(               // nastêpuje wykrywanie p³yty CD
        cd_detected(
             "louie, louie"
           , std::vector<std::clock_t>( /* d³ugoœci œcie¿ek */ )
        )
    );
    pl.process_event(play());       // itd.
    pl.process_event(pauze());
    pl.process_event(play());
    pl.process_event(stop());

    std::cerr << std::endl ;
    std::cerr << "*Program main body - stop*" << std::endl ;

    return 0;
}

map < string , inputFileInstance > gFileMap ;

int fileTest()
{
    gFileMap[ "core0" ] = inputFileInstance("datafile1.dat") ;
    gFileMap[ "core1" ] = inputFileInstance("datafile2.dat") ;
    gFileMap[ "core2" ] = inputFileInstance("datafile2.dat") ;
    gFileMap[ "core3" ] = inputFileInstance("datafile2.dat") ;

   int b(60);
   while ( b -- )
        cout << " " << gFileMap[ "core0" ].get<char>() ;
   b = 60;
   cout << "\n";
   while ( b -- )
        cout << " " << gFileMap[ "core1" ].get<int>() ;
   b = 60;
   cout << "\n";
   while ( b -- )
        cout << " " << gFileMap[ "core2" ].get<R>() ;
   b = 60;
   cout << "\n";
   while ( b -- )
        cout << " " << gFileMap[ "core3" ].get<complex<R> >() ;
   cout << "\n";

   int iRetVal (0);
   char bRetVal (0);
   R rRetVal (0);
   complex<R> crRetVal (0);

   {
       list < token > lProgram ;
       list < field > lSchema ;

       list < boost::any > lResult ;

       field a( "ala" , lProgram , field::INTEGER );
       field b( "ela" , lProgram , field::INTEGER );

       lSchema.push_back( a );
       lSchema.push_back( b );
       BOOST_FOREACH( field &f , lSchema )
       {
           switch( f.dFieldType )
           {
               case field::BYTE:
                   lResult.push_back( gFileMap[ "core1" ].get<char>() );
                   break ;
               case field::INTEGER:
                   lResult.push_back( gFileMap[ "core1" ].get<int>() ) ;
                   break ;
               case field::RATIONAL:
                   lResult.push_back( gFileMap[ "core1" ].get<R>() );
                   break ;
               case field::CRATIONAL:
                   lResult.push_back( gFileMap[ "core1" ].get<complex<R> >() ) ;
                   break ;
               case field::BAD:
               default:
                   throw std::out_of_range( "niezdefiniowany typ" );
                   break ; //proforma
           }
       }

       BOOST_FOREACH( boost::any &r , lResult )
       {
           if ( r.type() == typeid(int) ) cout << any_cast<int>(r) << endl ;
           if ( r.type() == typeid(char) ) cout << any_cast<char>(r) << endl ;
           if ( r.type() == typeid(R) ) cout << any_cast<R>(r) << endl ;
           if ( r.type() == typeid(complex<R>) ) cout << any_cast<complex<R> >(r) << endl ;
       }
   }
   return 0;
}
int client()
{
    return 0 ;
}
int server()
{
    return 0 ;
}

struct A {
    virtual int ala( float a ) { printf("a"); return 1 ; } ;
};

struct B : public A {
    virtual int ala( int a , int b ) { printf("b");  return 1 ; } ;
};

int main_class()
{
    B instance ;
    instance.A::ala(1);
    instance.ala( 1 , 1 );
    return 0 ;
}

//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

int client(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " -c <host>" << std::endl;
      return 1;
    }

    boost::asio::io_service io_service;

    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), argv[2], "daytime" );
    udp::endpoint receiver_endpoint = *resolver.resolve(query);

    udp::socket socket(io_service);
    socket.open(udp::v4());

    boost::array<char, 1> send_buf  = { 0 };
    socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);

    boost::array<char, 128> recv_buf;
    udp::endpoint sender_endpoint;
    size_t len = socket.receive_from(
        boost::asio::buffer(recv_buf), sender_endpoint);

    std::cout.write(recv_buf.data(), len);
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}

int main(int argc, char* argv[])
{
    //Intro
    cerr << "Stage - staging area of database algorithms.\n";
    string revision(  "$Rev: 38 $" );  //This string is svn keyword
    std::replace( revision.begin() , revision.end() , '$' , ' ' );
    revision.erase( std::remove( revision.begin() , revision.end() , ' ' ) , revision.end()) ; //erase-remove idiom
    cerr <<
            "used compiler tag:" + string(COMPILER) +
            "," + revision <<
            ", CTS:" << string( __TIMESTAMP__) <<	//http://msdn2.microsoft.com/en-us/library/b0084kay(VS.80).aspx
            std::endl ;

    try {
        namespace po = boost::program_options;

        string sInputFile ;

        po::options_description desc("Obslugiwane opcje programu");
        desc.add_options()
                ("infile,i", po::value<string>(&sInputFile)->default_value("data.in"), "data input file")
                ("help,h", "opcje programu")
                ("fsm,f" ,"test fsm" )
                ("datafile,d", "data file tests")
                ("server,s","server test")
                ("client,c","client test")
                ("rlutil,l","rlutil test")
                ("classtest,z","class test")
                ("tree,t","server tree test")
                ("ctree,e","client tree test")
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
        else if ( vm.count("fsm"))
            return fsmTest();
        else if ( vm.count("datafile"))
            return fileTest();
        else if ( vm.count("server"))
            return server();
        else if ( vm.count("client"))
            return client( argc , & argv[0] );
        else if ( vm.count("rlutil"))
            return main_rlutil();
        else if ( vm.count("classtest"))
            return main_class();
        else if ( vm.count("tree"))
            return main_bpt_server();
        else if ( vm.count("ctree"))
            return main_bpt_client();
        else
            throw std::out_of_range("wybierz odpowiedni parametr/test");
    }
    catch(std::exception& e)
    {
        cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
