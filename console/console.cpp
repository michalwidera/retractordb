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

using namespace std ;
using namespace boost ;
using namespace QS ;

#include <cassert>

#define FOR(i,a,b) for(int i=(a);i<(b);++i)
#define FORD(i,a,b) for(int i=(b)-1;i>=(a);--i)
#define REP(i,n) FOR(i,0,n)
#define REPD(i,n) FORD(i,0,n)

extern "C" {
    qTree coreInstance ;
    config configInstance ;
}

#define waitkey std::cout << "Press any key to continue..." << std::endl; rlutil::anykey()

int tc = rlutil::tcols() - 1 ;
int kchain = 0;

void showkey( int k )
{
    return ;

    if ( k != 0 )
    {
    rlutil::locate( 1 , 2  + kchain );
    printf("%d  ",k);
    kchain += 1 ;
    kchain %= 10 ;  //dziesiec ostatnich
    }
}

int terminal()
{
    rlutil::cls();
    //rlutil::locate( 1, 35 ); std::cout << "Data Stream Management System Console" << std::endl ;

    rlutil::CursorHider hideCursor ;

    int pos = 0 ;
    int k = -1 ;

    int curline(1); //aktualna lina w ktorej wprowadzamy tekst

    char line[80]; //linia edytowana
    REP(i,80) line[i] = 0 ;

    while(true)
    {
        if (kbhit())
        {
            k = getch();

            if ( k == 27 )
            {
                k = getch();

                if ( k == 91 )
                {
                    k = getch();

                    if ( k == 67 )
                    {
                        if ( pos <= 80 ) pos ++ ;
                        k = 0 ; // -- sprawdzic
                    }

                    if ( k == 68 )
                    {
                        if ( pos > 0 ) pos -- ;
                        k = 0 ; //  -- sprawdzic
                    }
                }

            }
            if ( k == 13 || k == 10 ) { k = -1 ; break ; }

            if ( k == 224 ) //control sequence
            {
                int k_ = k ;

                k = getch();

                if ( k == 75 ) //left arrow
                {
                    if ( pos > 0 ) pos -- ;
                }

                if ( k == 77 )  //right arrow
                {
                    if ( pos <= 80 ) pos ++ ;
                }

                k = k_ ;
            }

            if ( k == 8 || k == 127) //backspace
            {                
                if ( pos > 0 ) pos -- ;
                REP( i , 80 - pos ) line[pos+i] = line[pos+i+1];
            }
            else //ascii char
            {
                if ( k >= 32 && k <= 126 )
                {
                    REPD( i , 80-pos ) line[pos+i+1] = line[pos+i];
                    line[pos++] = k ;
                }
            };
        }

        if ( k != 0 )
        {
            rlutil::locate( 1 , curline );
            REP(i,80)
            {
                if ( i == pos )
                    printf("_");
                else
                    printf("%c" , line[i] );
            }
        }

        k = 0 ;

        if ( ! strcmp( line , "quit" ) )
        {
            k = -1 ;
            break ;
        }

        if ( ! strcmp( line , "dir" ) )
        {
            REP( n , 5 )
            {
                rlutil::locate( 1 , curline + n + 1 );
                printf("test dir");
            }
            curline += 5 + 1 ;
            REP(i,80) line[i] = 0 ;
            pos = 0 ;
            k = -1 ;
        }

    }

    return 0 ;
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
                ("infile,i", po::value<string>(&sInputFile)->default_value("data.in"), "nazwa pliku z danymi")
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
