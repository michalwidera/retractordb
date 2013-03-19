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

class ContiniousTerminal {

    struct record
    {
        std::string commandTxt ;     //issued command
        std::string streamName ;  //nazwa strumienia z systemu
        int linebeg ;        //linia od ktorej sie zaczyna wyswietlanie
        int linecnt ;        //ilosc linii do wyswietlenia
        int timing ;         //tutaj potem rational
    };

    std::vector <record> queryVect ;

    boost::cmatch what;

    std::map < cmd_e, boost::regex > commands ;

    int pos ;       //<pozycja kursora 0..80
    char line[81];  //< edytowana linia
    int curline;    //< aktualna lina w ktorej wprowadzamy tekst
    bool stopLoop;  //< true - i wychodzimy z petli terminal

    int terminal();
    void printLine( int curline , int pos , char * line );
    cmd_e processCommand( char * commandStr );
    bool processRequests(cmd_e command) ;
    void registerDirCmd( int &curline , int pos , char * line , std::string tableName , int l );
    void clearRegisteredQueries();
    void processRegisteredQueries();

public:

    ContiniousTerminal( bool stopLoop = true );

    int postMortemRetCode ;

    ~ContiniousTerminal() {} ;

    bool issueCommand( char * commandStr )
    {
        return processRequests( processCommand ( commandStr ) );
    }
};

#endif // CONTINIOUSTERMINAL_H
