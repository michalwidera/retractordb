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

/*
 * Za³o¿enia projektowe programu konsoli u¿ytkownika
 * Konsola:
 * 1. jest najprostszym interfejsem u¿ytkownika dla systemu zarz¹dzania danymi
 * 2. udostêpnima dane na bie¿¹co, zgodnie ze zmianami danych w bazie
 * 3. dane raz wyswietlone ulegaj¹ póŸniejszym zmianom (bie¿¹cej aktualizacji)
 * 4. komunikuje siê z baz¹ uzgodnionym protoko³em sieciowym
 * 5. jest przenoœna pomiêdzy systemami (Lin/Win)
 * 6. dzia³a w œrodowisku tekstowym
 * TODO: WeŸ siê w garœæ to posprz¹taj ten bajzel tutaj
 */

#include "stdafx.h"

#include <ctime>

//#include <QStruct.h>

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

#include <cassert>

#define FOR(i,a,b) for(int i=(a);i<(b);++i)
#define FORD(i,a,b) for(int i=(b)-1;i>=(a);--i)
#define REP(i,n) FOR(i,0,n)
#define REPD(i,n) FORD(i,0,n)

int tc = rlutil::tcols() - 1 ;
int kchain = 0;

//Domysla ilosc linii odpowiedzi na polecenie dir
const int defaultLinieCount = 5 ;

//Takie tam z Atari/C64
const char* welcomeString = "Ready." ;

struct record
{
    string commandTxt ;     //issued command
    string streamName ;  //nazwa strumienia z systemu
    int linebeg ;        //linia od ktorej sie zaczyna wyswietlanie
    int linecnt ;        //ilosc linii do wyswietlenia
    int timing ;         //tutaj potem rational
};

//
// Zestawienie polaczenia
//
namespace dbNet {
    string sHostName ;

    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::socket socket(io_service);
}


ptree netClient( string netCommand )
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
                    break ;
            }

            if (error != boost::asio::error::eof)
                throw boost::system::system_error(error);

            read_xml( response_stream , pt_response );
        }
        else
        {
            pt_response.put("error.response" , "invalid" );
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "exception catch:" << e.what() << std::endl;
        pt_response.put("error.response" , e.what() );
    }

    return pt_response ;
}

enum cmd_e { Quit , Dir , Cls , Connect , Help , NoCmd }  ;

class termClass {

    std::vector <record> queryVect ;

    boost::cmatch what;

    std::map < cmd_e, boost::regex > commands ;

    int pos ;
    int k ;
    char line[81]; //linia edytowana
    int curline; //aktualna lina w ktorej wprowadzamy tekst
    bool stopLoop;

    int terminal();

    void printLine( int curline , int pos , char * line );
    cmd_e processCommand( char * commandStr );
    bool processRequests(cmd_e command) ; //deklaracja funkcji

    void register_dir_cmd( int &curline , int pos , char * line , string tableName , int l );
    void clearRegisteredQueries();
    void processRegisteredQueries();


public:
    termClass( bool stopLoop = true )
        : stopLoop(stopLoop),
          pos(0) ,
          k(-1),
          curline(2)
    {
        // ---- inicjacja tablicy poleceñ
        commands[ Quit ] = boost::regex("quit.*") ;
        commands[ Dir ] = boost::regex("\\s*dir\\s*(\\w*)\\s*(\\d*).*") ;
        commands[ Cls ] = boost::regex("cls.*") ;
        commands[ Connect ] = boost::regex("connect.*") ;
        commands[ Help ] = boost::regex("help.*") ;
        // ----

        if ( !stopLoop ) postMortemRetCode = terminal();
        else postMortemRetCode = 0 ;
    }

    int postMortemRetCode ;

    ~termClass() {} ;

    bool issueCommand( char * commandStr )
    {
        return processRequests( processCommand ( commandStr ) );
    }
};

cmd_e termClass::processCommand( char * commandStr )
{
    assert( commands.size() != 0 );

    using namespace boost::adaptors;

    BOOST_FOREACH( cmd_e i, commands | map_keys )
       if ( regex_match(commandStr,what,commands[i] ) )
        {
            return i ;
        }

    return NoCmd ;
}

void termClass::printLine( int curline , int pos , char * line )
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

void termClass::register_dir_cmd( int &curline , int pos , char * line , string tableName , int l )
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

void termClass::clearRegisteredQueries()
{
    rlutil::cls();
    rlutil::locate( 1 , 1 );
    printf( "%s" , welcomeString );

    queryVect.clear();
}

void termClass::processRegisteredQueries()
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
int termClass::terminal()
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


bool termClass::processRequests( cmd_e command )
{
    using namespace dbNet ;

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

int main(int argc, char* argv[])
{
    //Intro
    cerr << "Console - konsola systemu.\n";
    cerr << "used compiler tag:" + string(COMPILER) +
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
                     char line[81]; //linia edytowana
                     strcpy( line , rline.c_str() );
                     cout << ":" << line ;
                     termClass termClassInstance ;
                     if ( termClassInstance.issueCommand(line) )
                     {
                         cout << ",OK" << endl ;
                     }
                     else
                     {
                         cout << ",?" << endl ;
                     }
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
        {
            termClass instance(false);
            return instance.postMortemRetCode ;
        }

    }    
    catch(std::exception& e)
    {
        cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
