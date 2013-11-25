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
#include "stdafx.h"

#include "QStruct.h"
#include "CRSMath.h"
#include "dbstream.h"
#include "Processor.h"
#include "IPhHead.h"
#include "NetServer.h"

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

//dodatkowy naglowek trzeba wczytac jak sie kompiluje pod mingw
//#ifndef  _CONSOLE
//#include "windows.h"
//#endif

using namespace CRationalStreamMath ;
using namespace QS ;

extern "C" qTree coreInstance ;
extern "C" config configInstance ;

enum limits {
  limitToDeclarations,
  limitToQueries ,
  noLimits
};

typedef boost::property_tree::ptree ptree ;

void iTest();

bool bBreakOnLimits( int dProgSize , limits eLimit )
{
  const bool bStreamIsDecl( dProgSize == 0 );

  if ( eLimit == limitToDeclarations )
    if ( ! bStreamIsDecl )
      return true ;

  if ( eLimit == limitToQueries )
    if ( bStreamIsDecl )
      return true ;

  return false ;
}

set< R > getListFromCore( limits eLimit = noLimits )
{
  set< R > lstTimeIntervals ;

  BOOST_FOREACH( query it , coreInstance )
  {
    if ( bBreakOnLimits( it.lProgram.size() , eLimit ) )
      continue ;

    lstTimeIntervals.insert( it.rInterval );
  }

  return lstTimeIntervals ;
}

void dumpCore( std::ostream & xout , limits eLimit = noLimits )
{
  xout << "In core:" << endl ;

  xout << "Seqence\tInterval\tQuery id" << endl ;

  BOOST_FOREACH( const query & it , coreInstance )
  {
    if ( bBreakOnLimits( it.lProgram.size() , eLimit ) )
      continue ;

    xout << getSeqNr( it.id ) << "\t";
    xout << it.rInterval << "\t";
    xout << it.id  ;
    xout << endl ;
  }
}

set < string > getAwaitedStreamsSet(  TimeLine & tl , limits eLimit = noLimits )
{
  set < string > retVal ;

  BOOST_FOREACH( const query & it , coreInstance )
  {
    if ( bBreakOnLimits( it.lProgram.size() , eLimit ) )
      continue ;

    if ( ! tl.isThisDeltaAwaitCurrentTimeSlot( it.rInterval ) )
      continue ;

    retVal.insert( it.id );
  }

  return retVal ;
}

void showAwaitedStreams( TimeLine & tl ,
                         limits eLimit = noLimits ,
                         string streamName = "" )
{
  set< string > strSet = getAwaitedStreamsSet( tl , eLimit );
  BOOST_FOREACH( const string & str , strSet )
      if ( streamName == "" )
      cout << "-" << getSeqNr(str) << "-" ;
  else if ( streamName == str )
    cout << "-" << getSeqNr(str) << "-" ;
}

ptree commandProcessor( ptree ptInval )
{
  ptree ptRetval ;

  string command = ptInval.get("db.message" , "" ) ;

  //
  //to polecenie zwraca identyfikatory strumieni
  //
  if ( command == "get" )
    {
      BOOST_FOREACH( const query & it , coreInstance )
      {
        ptRetval.put( string("db.stream.")+it.id , it.id );
      }
    }

  //
  //to polecenie zwraca z czego sklada sie strumien
  //
  if ( command == "detail" )
    {
      string streamName = ptInval.get("db.argument" , "" ) ;

      assert( streamName != "" );

      BOOST_FOREACH( string s , coreInstance[streamName].getFieldNamesList() )
      {
        ptRetval.put( string("db.field.")+s , s );
      }
    }

  //
  //to polecenie spowoduje dolaczenie strumienia do listy przesylanych strumieni
  //
  if ( command == "show" )
    {
      string streamName = ptInval.get("db.argument" , "" ) ;

      assert( streamName != "" );

      BOOST_FOREACH( const query & it , coreInstance )
      {
        if ( it.id == streamName )
          {
            ptRetval.put( string("db.show") , streamName );
            coreInstance[streamName].setMulticastStream( true );
          }
      }
    }

  return ptRetval ; //sub for a while
}

#ifndef WIN32
//to sie kompiluje pod Linux

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int _kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
    {
      ungetc(ch, stdin);
      return 1;
    }

  return 0;
}

int _getch()
{
  return getchar();
}

#endif

#ifdef _CONSOLE
int main(int argc, char* argv[])
#else

int
usleep (unsigned int usecs)
{
  unsigned int msecs = usecs / 1000;
  if (msecs < 1)
    Sleep (1);
  else
    Sleep (msecs);
}

int mainFromWin32(int argc, char* argv[])
#endif
{
  cerr << "Executor - egzekutor planow zapytan tworzonych przez kompilator.\n";
  cerr << "Tag kompilatora:" + string(COMPILER) +
        #ifdef _CONSOLE
          ", CTS:" << string( __TIMESTAMP__) <<	//http://msdn2.microsoft.com/en-us/library/b0084kay(VS.80).aspx
        #endif
          std::endl ;
  cerr << "Boost wersja:" << BOOST_LIB_VERSION << std::endl ;

  try {
    namespace po = boost::program_options;

    string sInputFile ;
    string sLimit ;
    string sQuery ;
    string sDumpFile ;
    int iTimeLimitCnt ;

    po::options_description desc("Obslugiwane opcje programu");
    desc.add_options()
        ("help,h", "opcje programu")
        ("limit,l", po::value<string>(&sLimit)->default_value( "none" ), "limit (query,decl,none)")
        ("infile,i", po::value<string>(&sInputFile)->default_value("query.qry"), "nazwa pliku z danymi")
        ("query,q" , po::value<string>(&sQuery) , "wskazuje zapytanie do testu" )
        ("display,s" , po::value<string>(&sQuery) , "wyswietla pojedyncze zapytanie" )
        ("dump,d", po::value<string>(&sDumpFile)->default_value("query.dmp"), "plik z zrzutem strumienia")
        ("tlimitqry,m" , po::value<int>(&iTimeLimitCnt)->default_value(0) , "limit zapytan, 0 - no limit" )
        ("waterfall,f" , "wyswietlanie danych")
        ("test,t" , "unit testy" )
        ;

    po::positional_options_description p;       //Assume that infile is the first option
    p.add("infile", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
        cout << desc << "\n";
        return 0;
      }

    // <-- Do usuniecia wraz z SelfTest.cpp begin()
    if (vm.count("test"))
      {
        iTest() ;
        return 0;
      }
    // <-- Do usuniecia end()

    cout << "Input :" << sInputFile << endl ;

    std::ifstream ifs( sInputFile.c_str() , std::ios::binary);

    if ( ! ifs )
      {
        cerr << sInputFile << " - brak pliku wejsciowego\n";
        return 1;
      }

    boost::archive::text_iarchive ia(ifs);
    ia >> configInstance ;
    ia >> coreInstance ;

    Processor proc;
    //dbStreamHead head ;

    //opcja query powstala do testowania pojedynczych zapytan
    if (vm.count("query"))
      {
        cout << "Query :" << sQuery << "\n";

        if ( iTimeLimitCnt == 0 )
          std::cout << "Nacisnij dowolny klawisz aby przerwac." << std::endl ;

        R prev_interval(0);
        R timeSlot( coreInstance.getDelta( sQuery ) );
        R interval( timeSlot * 1000  ); // miliseconds

        while( ! _kbhit() )
          {
            std::cout << timeSlot << "\t" ;
            std::cout << proc.getRowValue( sQuery );

            BOOST_FOREACH ( string s , getQuery( sQuery ).getDepStreamNameList() )
                std::cout << "," << s ;

            std::cout << std::endl ;

            int ms ( rational_cast<int>( interval ) ) ; // miliseconds
            usleep( ms ) ;
          }

        _getch(); //no wait ... feed key from kbhit

        return 0;
      }

    limits lLimitVal ;

    std::cout << "Limit :" ;
    std::cout << sLimit << std::endl ;

    if      ( sLimit == "query") lLimitVal = limitToQueries ;
    else if ( sLimit == "decl" ) lLimitVal = limitToDeclarations ;
    else if ( sLimit == "none" ) lLimitVal = noLimits ;
    else throw std::out_of_range("Ustawiono niezdefiniowany limit: query,decl,none (dopuszczalne)");

    cout << "Objects:" ;
    dumpCore( cout , lLimitVal );

    TimeLine tl( getListFromCore( lLimitVal ) );



    //
    // Uruchomienie watku odpowiedzialnego za transmisje sieciowa
    //


    asio::io_service ios;

    tcp_server server1(ios,&commandProcessor);
    udp_server server2(ios,boost::asio::ip::address::from_string("239.255.0.1"));

    thread bt(boost::bind(&asio::io_service::run, &ios));  //puszczenie serwisu w watku

    std::cout << "Watek serwera sieciowego wystartowal" << std::endl ;

    //
    // Glowna petla przetwarzania danych
    //

    //
    // jesli ta wartosc jest 0 - to znaczy ze czekamy na klawisz, inaczej watchdog
    //
    if ( iTimeLimitCnt == 0 )
      std::cout << "Nacisnij dowolny klawisz aby przerwac." << std::endl ;
    else
      std::cout << "Po wyczerpaniu limitu zapytan (-m) oczekiwany jest koniec petli." << std::endl ;

    //head.prepareSharedArea();
    R prev_interval(0);
    while( ! _kbhit() )
      {
        if ( iTimeLimitCnt != 0 )
          {
            if ( iTimeLimitCnt != 1 )
              iTimeLimitCnt -- ;
            else
              break ;
          }

        R nextTimeSlot( tl.getNextTimeSlot() );
        R interval( nextTimeSlot * 1000 );
        int ms ( rational_cast<int> ( interval - prev_interval ) ) ; // miliseconds
        prev_interval = interval ;

        if ( vm.count("waterfall") )
          {
            std::cout << ms << "\t" ;
            showAwaitedStreams( tl ,
                                lLimitVal ,
                                vm.count("display") ? sQuery : "" );

            std::cout << std::endl ;
          }

        proc.processRows( getAwaitedStreamsSet( tl , lLimitVal ) );
        //head.storeInCommonArea( getAwaitedStreamsSet( tl , lLimitVal ) );


        usleep( ms ) ;
      }

    //
    // Zatrzymanie watku przesylania danych
    //

    ios.stop();

    //
    // Koniec glownej petli przetwarzania
    //

    if ( iTimeLimitCnt != 1 )
      _getch(); //no wait ... feed key from kbhit
    else
      cout << "Wyczerpany limit zapytan (-m) oczekiwanie zakonczyl martwa petle." << endl ;

    proc.saveStreamsToFile( sDumpFile );
    cout << "Dump  :" << sDumpFile << endl ;
    UR::Dump( std::cout );

  }
  catch(std::exception& e)
  {
    cerr << e.what() << endl ;
    return 1;
  }

  //end
  return 0;
}
