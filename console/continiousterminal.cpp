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

#include "continiousterminal.h"

#include "stdafx.h"

#include <ctime>

#include "rlutil.h"
#include <cstdio>
#include <vector>

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

using namespace std ;
using namespace boost ;

using boost::asio::ip::tcp ;
using boost::property_tree::ptree;


//Domysla ilosc linii odpowiedzi na polecenie dir
const int defaultLinieCount = 5 ;

//Takie tam z Atari/C64
const char* welcomeString = "Ready." ;

int tc = rlutil::tcols() - 1 ;
int kchain = 0;



//deklaracja
ptree netClient( string netCommand );

cmd_e ContiniousTerminal::processCommand( char * commandStr )
{
    assert( commands.size() != 0 );

    using namespace boost::adaptors;

    BOOST_FOREACH( cmd_e i, commands | map_keys )
       if ( regex_match(commandStr,what,commands[i] ) )
          return i ;

    return NoCmd ;
}

void ContiniousTerminal::printLine( int curline , int pos , char * line )
{
    rlutil::locate( 1 , curline );
    REP(i,80)
    {
        if ( i == pos )
            printf("_");
        else
        {
            if ( line[i] >= 32 && line[i] <= 126 )
                printf("%c" , line[i] );
            else
                printf(" ");
        }
    }
}

void ContiniousTerminal::register_dir_cmd( int &curline , int pos , char * line , string tableName , int l )
{
    record recordInstance ;
    recordInstance.commandTxt = line ;
    recordInstance.linebeg = curline ;

    recordInstance.linecnt = l ;
    curline += l ;

    recordInstance.streamName = tableName ;
    queryVect.push_back( recordInstance );

    REP(i,80) line[i] = ' ' ;        //Przywrocenie wartosci domyslnych
}

void ContiniousTerminal::clearRegisteredQueries()
{
    rlutil::cls();
    rlutil::locate( 1 , 1 );
    printf( "%s" , welcomeString );

    queryVect.clear();
}

void ContiniousTerminal::processRegisteredQueries()
{
    BOOST_FOREACH( record r , queryVect )
    {
        REP( n , r.linecnt )
        {
            rlutil::locate( 1 , r.linebeg + n );
            printf("test dir %d str:%s rnd:%d",n,r.streamName.c_str(),rand());
        }
    }
}

//
// Glowna procedura terminala z petla zdarzen
//
int ContiniousTerminal::terminal()
{
    setbuf(stdout, NULL);   //niebuforowany printf

    rlutil::CursorHider hideCursor ;
    rlutil::ttyRawer ttyRaw ;

    rlutil::cls();
    rlutil::locate( 1 , 2 );

    printf( "%s" , "  **** STREAMS LAB DATABASE CONSOLE V1 ****" );

    rlutil::locate( 1 , 4 );
    printf( "%s" , welcomeString );

    curline = 5 ;
    REP(i,80) line[i] = ' ' ;    //wypelniam spacjami
    printLine( curline , 0 , line );

    while(true)
    {
        k = rlutil::kb_getc() ;

        if ( k != 0 )
        {
            if ( k == 27 ) // linux part
            {
                k = getch();

                if ( k == 91 )
                {
                    k = getch();

                    if ( k == 67 ) //strzalka w prawo
                    {
                        pos ++ ;
                        k = -1 ;
                    }

                    if ( k == 68 ) //strzalka w lewo
                    {
                        pos -- ;
                        k = -1 ;
                    }
                }

            }
            else if ( k == 13 || k == 10 )   //hit enter i przetwarzenie komendy
            {
                k = -1 ;
                pos = 0;
                printLine( curline , -1 , line );
                curline += 1 ;
                processRequests( processCommand( line ) ) ;
                REP(i,80) line[i] = ' ' ;    //wypelniam spacjami
                printf("\a");   //beep
            }
            else if ( k == 224 ) //control sequence - windows
            {
                int k_ = k ;

                k = getch();

                if ( k == 75 ) //left arrow
                {
                    assert( pos >= 0 );

                    if ( pos > 0 ) pos -- ;
                }

                if ( k == 77 )  //right arrow
                {
                    assert( pos <= 81 );

                    if ( pos <= 80 ) pos ++ ;
                }

                k = k_ ;
            }
            else if ( k == 8 || k == 127) //backspace
            {
                if ( pos > 0 ) pos -- ;
                REP( i , 80 - pos )
                {
                    assert( pos +i +1 >= 0 && pos +i +1 <= 80 );
                    assert( pos +i >= 0    && pos +i    <= 80 );

                    line[pos+i] = line[pos+i+1];
                }
            }
            else if ( k >= 32 && k <= 126 ) //ascii char
            {
                REPD( i , 80-pos )
                {
                    assert( pos +i +1 >= 0 && pos +i +1 <= 80 );
                    assert( pos +i >= 0    && pos +i    <= 80 );

                    line[pos+i+1] = line[pos+i];
                }

                assert( pos + 1 <= 80 );
                line[pos++] = k ;
            }

            printLine( curline , pos , line );

            if ( stopLoop ) break ;
        }

        processRegisteredQueries();
    }

    return 0 ;
}

bool ContiniousTerminal::processRequests( cmd_e command )
{

    switch ( command )
    {
    case Quit:
        {
            stopLoop = true ;
            return true ;
        }
    case Dir:
        {
            int l ;
            try
            {
                l = lexical_cast<int>( what[2].str() );
            }
            catch(const bad_lexical_cast &e)
            {   //ten catch jest oczekiwany - nie zawsze zostanie podany ten arg.
                l = defaultLinieCount ;
            }

            register_dir_cmd( curline , pos + 2 , line , what[1].str() , l );
            return true ;
        }
    case Cls:
        {
            clearRegisteredQueries();
            REP(i,80) line[i] = ' ' ;
            pos = 0 ;
            curline = 2 ;
            printLine( curline , pos , line );
            return true ;
        }
    case Connect:
        {
            ptree pt = netClient( "get" ) ;

            BOOST_FOREACH(ptree::value_type &v,
                          pt.get_child("db.stream"))
            {
                REP(i,80) line[i] = ' ' ;
                strcpy( line , v.second.data().c_str() );
                printLine( curline , -1 , line );
                curline += 1 ;
            }

            REP(i,80) line[i] = ' ' ;
            printLine( curline , pos , line );
            return true ;
        }
    case Help:
        {
            strcpy( line , "cls, quit, connect, dir [co][ile] (all case sensitive)" );
            printLine( curline , -1 , line );
            curline += 1 ;
            REP(i,80) line[i] = ' ' ;
            return true ;
        }
    default :
        return false ;
    }
}
