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

#ifndef CONTINIOUSTERMINAL_H
#define CONTINIOUSTERMINAL_H

#include <vector>
#include <map>
#include <boost/regex.hpp>
#include <string>

enum cmd_e { Quit , Dir , Cls , Connect , Help , NoCmd }  ;

using namespace std;

class ContiniousTerminal {

    struct record
    {
        string commandTxt ;     //issued command
        string streamName ;  //nazwa strumienia z systemu
        int linebeg ;        //linia od ktorej sie zaczyna wyswietlanie
        int linecnt ;        //ilosc linii do wyswietlenia
        int timing ;         //tutaj potem rational
    };

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
    bool processRequests(cmd_e command) ;
    void register_dir_cmd( int &curline , int pos , char * line , string tableName , int l );
    void clearRegisteredQueries();
    void processRegisteredQueries();

public:
    ContiniousTerminal( bool stopLoop = true )
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

    ~ContiniousTerminal() {} ;

    bool issueCommand( char * commandStr )
    {
        return processRequests( processCommand ( commandStr ) );
    }
};

#endif // CONTINIOUSTERMINAL_H
