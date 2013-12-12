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

#ifndef DBSTREAM_H
#define DBSTREAM_H

#include <complex>
#include <list>
#include <map>
#include <boost/shared_array.hpp>
#include <boost/variant.hpp>

namespace UR {

  using namespace std ;
  using namespace boost ;

  /** Tego typu sa dane przechowywane w strumieniu */
  typedef boost::variant< complex< R > , R , int >  varfield ;

  /** To jest typ rekordu w strumieniu */
  typedef map < string , varfield  >  record ;

  /**
    Klasa udostepniajaca metody dostepu do danych w strumieniu
    Przykrywa caly CBuffer wlasna abstrakcja
    dostep odbywa sie poprzez obiekt_strumien[1] ,lub obiekt_strumien["nazwa_pola"]
    dostep do danych archiwalnych obiekt_strumien.get(1)[1] lub obiekt_strumien.get(1)["nazwa_pola"]
    po wykoaniu store()
    dane nie sa juz dostepne przez obiekt_strumien[1] ,lub obiekt_strumien["nazwa_pola"]
    a przygotowane jest miejsce na kolejna krotke
    */
  class dbStream : private boost::noncopyable
  {
    /** Rozmiar ramki w bajtach */
    int frameSize ;

    /** Wskaznik na binarne dane uzywany przez cbuffer */
    shared_array< char >  pRawData ;

    /** Nazwa strumienia */
    string streamName ;

    /** Schemat - inaczej - Lista nazw pol zawartych w strumieniu (kazde pole typu varfield) */
    list < string > schema ;

    /** zmienna opisujace dane przechowywane w strumieniu */
    record mpShadow ;

    /** Ofset - przesuniecie wzgledem poczatku strumienia - jaki obsluguje w danej chiwli klasa metodami []*/
    int m_offset ;

  public:

    /** Informacja czy zworcone dane maja sens*/
    bool resultValid ;

    /** Konstruktor klasy */
    dbStream( string streamName , list < string > schema ) ;

    /** Operatory dostepu - operator po pozycji  */
    varfield& operator[](const int& _Keyval);

    /** Operatory dostepu - operator po nazwie w schemacie (wola operator po pozycji)*/
    varfield& operator[](const string& _Keyval);

    /** Metoda zapisujaca dane do strumienia - znikaja tez wartosci z mpStrVal, pozostaja w mpShadow */
    void store();

    /** Pobranie danych ze strumienia z okreslonej pozycji -
        use case: str1.get(1)["pole1"] */
    dbStream & get( int offset = 0 );

    /** Metoda diagnostyczna - docelowo do schowania/usuniecia. Pobiera
        dane inna droga, wolniej ale bez calego mpShadow i mpStrVal
        */
    varfield get( string fieldName , int offset = 0 );
  };

  void saveStreamsToFile( const string filename ) ;
  varfield getFromRawData( shared_array< char > pRawData , int position );

  /** Raportowanie tekstowe stanu cbuffera */
  void Dump( std::ostream &os );
}

#endif // DBSTREAM_H
