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

#include <ctime>

#include <QStruct.h>

#include "rlutil.h"
#include <cstdio>
#include <vector>

#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <istream>

using namespace std ;
using namespace boost ;
using namespace QS ;

using boost::asio::ip::tcp ;

#include <cassert>

#define FOR(i,a,b) for(int i=(a);i<(b);++i)
#define FORD(i,a,b) for(int i=(b)-1;i>=(a);--i)
#define REP(i,n) FOR(i,0,n)
#define REPD(i,n) FORD(i,0,n)

extern "C" {
    qTree coreInstance ;
    config configInstance ;
}

int tc = rlutil::tcols() - 1 ;
int kchain = 0;

const int defaultLinieCount = 5 ;

const char* welcomeString = "Ready." ;

//identyfikatory wewnetrzne polecen dostepnych z konsolis
enum { NO_CMD , QUIT_CMD , DIR_CMD , CLS_CMD , CONNECT_CMD } ;

struct record
{
    string command ;     //issued command
    string streamName ;  //nazwa strumienia z systemu
    int linebeg ;        //linia od ktorej sie zaczyna wyswietlanie
    int linecnt ;        //ilosc linii do wyswietlenia
    int timing ;         //tutaj potem rational
};

std::vector <record> queryVect ;

std::string tableName ;
int linecnt = defaultLinieCount ;

int processCommand( char * commandStr )
{
    boost::regex matchDirCmd("\\s*dir\\s*(\\w*)\\s*(\\d*).*");
    boost::regex matchQuitCmd("quit.*");
    boost::regex matchClsCmd("cls.*");
    boost::regex matchConnectCmd("connect.*");

    boost::cmatch what;

    if ( regex_match(commandStr,what,matchQuitCmd)  )
        return QUIT_CMD ;
    else
    if ( regex_match(commandStr,what,matchDirCmd) )
    {
        tableName = what[1].str() ;
        try {
            linecnt = lexical_cast<int>( what[2].str() );
        }
        catch(const bad_lexical_cast &e)
        {   //ten catch jest oczekiwany - nie zawsze zostanie podany ten arg.
            linecnt = defaultLinieCount ;
        }

        return DIR_CMD ;
    }
    else
    if ( regex_match(commandStr,what,matchClsCmd) )
    {
        return CLS_CMD ;
    }
    else
    if ( regex_match(commandStr,what,matchConnectCmd) )
    {
        return CONNECT_CMD ;
    }
        return NO_CMD ;
}

void printLine( int curline , int pos , char * line )
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

void register_dir_cmd( int &curline , int pos , char * line )
{
    record recordInstance ;
    recordInstance.command = line ;
    recordInstance.linebeg = curline ;

    recordInstance.linecnt = linecnt ;
    curline += linecnt ;

    recordInstance.streamName = tableName ;
    queryVect.push_back( recordInstance );

    REP(i,80) line[i] = ' ' ;        //Przywrocenie wartosci domyslnych
    tableName = "" ;
    linecnt = defaultLinieCount ;
}

void clearRegisteredQueries()
{
    rlutil::cls();
    rlutil::locate( 1 , 1 );
    printf( "%s" , welcomeString );

    queryVect.clear();
}

void processRegisteredQueries()
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

int command = NO_CMD ;
int pos = 0 ;
int k = -1 ;
char line[81]; //linia edytowana
int curline(2); //aktualna lina w ktorej wprowadzamy tekst
bool stopLoop(false);

bool processRequests() ; //deklaracja funkcji

//
// Zestawienie polaczenia
//
namespace dbNet {
string sHostName ;

boost::asio::io_service io_service;
tcp::resolver resolver(io_service);
tcp::socket socket(io_service);
}

//
// Glowna procedura terminala z petla zdarzen
//
int terminal()
{
    setbuf(stdout, NULL);   //niebuforowany printf

    rlutil::CursorHider hideCursor ;
    rlutil::ttyRawer ttyRaw ;

    rlutil::cls();
    rlutil::locate( 1 , 1 );
    printf( "%s" , welcomeString );

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
                command = processCommand( line );  //rozpoznanie komendy
                if ( command != NO_CMD )
                {
                    printLine( curline , -1 , line );
                    REP(i,80) line[i] = ' ' ;    //wypelniam spacjami
                    curline += 1 ;
                }
                processRequests() ;
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

bool processRequests()
{
    using namespace dbNet ;

    if ( command == QUIT_CMD )
    {
        stopLoop = true ;
        return true ;
    }
    else if ( command == DIR_CMD )
    {
        register_dir_cmd( curline , pos , line );
        printLine( curline , pos , line );
        command = NO_CMD ;
        return true ;
    }
    else if ( command == CLS_CMD )  //wyczyszczenie ekranu
    {
        clearRegisteredQueries();
        REP(i,80) line[i] = ' ' ;
        pos = 0 ;
        curline = 2 ;
        printLine( curline , pos , line );
        command = NO_CMD ;
        return true ;
    }
    else if ( command == CONNECT_CMD )
    {
        boost::asio::connect
        (
            dbNet::socket ,
            resolver.resolve
            (
                tcp::resolver::query
                (
                    sHostName ,
                    boost::lexical_cast< std::string >(1971)
                )
            )
        );
        return true ;
    }

    return false ;
}

int main(int argc, char* argv[])
{
    //Intro
    cerr << "Console - konsola systemu.\n";
    string revision(  "$Rev: 68 $" );  //This string is svn keyword
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

        po::options_description desc("Dozwolone opcja");
        desc.add_options()
                ("infile,i", po::value<string>(&sInputFile) , "z poleceniami")
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
        if (vm.count("infile"))
        {
            cout << "Load file:" << sInputFile << endl ;
            std::ifstream s( sInputFile.c_str() );
            if(s)
            {
                 boost::iostreams::stream<boost::iostreams::file_source>
                    file( sInputFile.c_str() );
                 std::string rline;
                 while (std::getline(file, rline))
                 {
                     strcpy( line , rline.c_str() );
                     cout << ":" << line ;
                     command = processCommand( line );
                     if ( processRequests() )
                         cout << ",OK" << endl ;
                     else
                         cout << ",?" << endl ;
                 }
            }
            else
            {
                cerr << "no input file." << endl ;
                return 1 ;
            }
            return 0;
        }
        else
            return terminal();

    }    
    catch(std::exception& e)
    {
        cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
