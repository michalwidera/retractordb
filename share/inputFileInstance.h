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

$Rev::$:  Revision of last commit
$Author::$:  Author of last commit
$Date::$:  Date of last commit
$Id::$:  Id of last commit
*/
#pragma once

#include <string>
#include <fstream>
#include <list>
#include <vector>
#include <QStruct.h>

#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>

/** Klasa przeznaczona jako implemtacja plikowego zrodla danych
    przyklad uzycia w pliku stage.cpp
    Mozliwie minimalny interfejs - docelowo zrobc pimpl
  */
class inputFileInstance
{
    /** Wskaznik na plik - sam sie posprzata i zadba o nie powielanie strumieni */
    boost::shared_ptr< std::ifstream > psFile ;

    /** dlugosc pliku w bajtach */
    int len ;

    /** aktualna pozycja w pliku */
    int curPos ;

    /** Funkcja przesuwajaca seek na poczatek pliku - wywolywana po przeczytaniu ostatniego elementu*/
    void goBegin();

public:

    /** Wymagany zestaw konstruktorow */
    inputFileInstance( std::string inputFileName ) ;
    inputFileInstance() ;

    /** Pobranie elementu z pliku. Patrze specjalizacje w cpp */
    template < typename T > T get();
};

class inputDF :
        public inputFileInstance
{
    /** Co processrow w tej zmiennej laduje
        zawartosc plikui jest nastepnie
        zwracana po kawalku przez getCR
    */
    std::vector < boost::any > lResult ;

    /** wypelniny w konstruktorze wykorzystywanay
      do sk³adania danych z danych binarynych.
      Nakladany schemat na strumien binarny
    */
    std::list < QS::field > lSchema ;

public:

    inputDF();

    /**
      * Parametrem konstruktora jest nazwa pliku na dysku
      * oraz schemat strumienia danych który bêdzie
      * opisywa³ jego zwartoœæ
      */
    inputDF( std::string inputFileName , std::list < QS::field > &lSchema ) ;

    /** Zadaniem tej funkcji jest pobranie wiersza
    wiersz z pliku tworzony jest w oparciu o schemat
    zapisany w zapytaniu. Utworzony schemat
    zapisywany jest w obiekcie i tam przechowywany*/
    void processRow() ;

    /** Funkcja zwracajaca pobrana wartosc z pliku skojarzonego ze strumieniem
    pobierana jest z bufora lResult, pobranego przez processRow
    zwracana jest taka wartosc na kolejnosc ktorej wskazuje field
    parametr nie idzie po referencji bo jest bajzel z const
    Tutaj nastepuje tez konwersja na typ CR
    */
    std::complex< R > getCR( QS::field f );
};
