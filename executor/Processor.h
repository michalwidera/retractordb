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
#pragma once

#include "QStruct.h"
#include "dbstream.h"
#include "inputFileInstance.h"

namespace QS {

  /** Klasa procesora zapytan */
  class Processor : private boost::noncopyable
  {
    /** Archiwum Strumienia danych - initStorage */
    std::map< std::string , boost::shared_ptr< UR::dbStream > > storage ;

    /** Funkcja zapewniajaca dostep do danych */
    complex< R > getCRValue(
        const field & f ,
        const query & q ,
        bool & resultValid /* return information */
        );

    /** Wskaznik na instancje obiektu core podawanego w konstruktorze */
    qTree * pCoreInstance ;

    /** zmienna przechowujaca zrodla danych */
    map < string , inputDF > gFileMap ;

  public:

    /** Klasa processor zapytan chodzi na core */
    Processor();

    /** Zwraca wartosc ze strumienia - TODO - zmienne typy */
    complex< R > getValueByOffset(
        const string query_name ,   /* in */
        int schemaOffset ,          /* in */
        int TimeOffset ,            /* in */
        bool & resultValid          /* out */
        );

    /** Prezentacja tekstowa zawartosci strumienia */
    string getRowValue( const string query_name );

    /** Ta funkcja pobiera w odpowiednim momencie dane do przetwarzania */
    void processRows( set < string > inSet );

    /** Zrzut zawartosci strumieni do pliku - default query.dmp */
    void saveStreamsToFile( const string filename );

  };
}
