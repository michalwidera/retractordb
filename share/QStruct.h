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

//Standard template library
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <stack>
#include <complex>
#include <sstream>

//Boost libraries
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/rational.hpp>

//Ten kod w przestrzeni boost::serialization umozliwia 
//serializacje rational< T > i complex< rational < T > > 
//Michal Widera/
namespace boost {
  namespace serialization {

    template<class Archive, class T>
    inline void serialize(
        Archive & ar,
        boost::rational<T> & p,
        unsigned int /* file_version */
        )
    {
      T _num( p.numerator() ) ;
      T _den( p.denominator() );
      ar & _num ;
      ar & _den ;
      p.assign( _num, _den );
    }

    template<class Archive, class T>
    inline void serialize(
        Archive & ar,
        std::complex< rational<T> >& p,
        unsigned int /* file_version */
        )
    {
      T _numr( p.real().numerator() ) ;
      T _denr( p.real().denominator() );
      T _numi( p.imag().numerator() ) ;
      T _deni( p.imag().denominator() );
      ar & _numr;
      ar & _denr;
      ar & _numi;
      ar & _deni;
      boost::rational<T> r , i ;
      r.assign( _numr, _denr );
      i.assign( _numi, _deni );
      p = std::complex< rational <T> > ( r , i ) ;
    }

  } // namespace serialization
} // namespace boost

namespace QS {

  using namespace std ;
  using namespace boost ;

  /** Funkcja ktora potrafi zrobic ze zmiennej typu rzeczywistego zmienna typu wymiernego */
  R Rationalize ( double inValue , double DIFF=1E-6 ,  int ttl=11 ) ;

  enum command_id
  {
#define DEF_CASE( _A_ ) _A_ ,
#include "tokendefset.h"
#undef DEF_CASE
  } ;

  /** Struktura konfiguracyjna zbioru strumieni */
  class config
  {

  private:

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version)
    {
      ar & transactionInterval ;
      ar & transHead ;
    }

  public:

    config() ;
    R transactionInterval ;     //Default - 1 transaction per sec set by constructor
    int transHead ;             //Default - 1 ilosc danych przetrzymywanych w oknie komunikacyjnym (wzgledem najdluzej trwajacego strumienia)

  };

  /** Podstawoa klasa przechowjaca tokeny-isntrukcje stosowane do budowy pol i strumieni */
  class token
  {

  private:

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version)
    {
      ar & command ;
      ar & crValue ;
      ar & sValue ;
    }

    command_id command ;                /**< Polecenie */
    complex< R > crValue ;              /**< Wartosc wymierna urojona */
    string sValue ;                     /**< Wartosc typu string */

  public:

    string getValue();
    complex< R > getCRValue() ;

    /** Construktor set */
    token( command_id id = VOID_COMMAND , string sValue = "" ) ;
    token( command_id id , complex< R > crValue , string sValue = "" ) ;
    token( command_id id , double dValue );

    string getStrTokenName();       /**< metoda dostepowa dekoduje w oparciu o tokendefset.h kod tokena na nazwe*/
    command_id getTokenCommand() ;  /**< Metoda dostepowa zwraca token id */
  } ;

  /** Klasa przechowujaca pojedyncze pole - wraz z insturkcja jego zbudowania*/
  class field
  {

  private:

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version)
    {
      ar & setFieldName ;
      ar & dFieldType ;
      ar & lProgram ;
    }

  public:

    enum eType { BAD , BYTE , INTEGER , RATIONAL , CRATIONAL } ;

    set< string > setFieldName ; /**< Zbior zawierajacy wsztkie mozliwe nazwy pola */
    eType dFieldType ;           /**< typ pola*/
    list < token > lProgram ;    /**< instrukcja budowy pola*/

    field () ;
    field ( string sFieldName, list < token > & lProgram, eType fieldType ) ;

    string getFirstFieldName();  /**< Funkcja zwraca pierwsza nazwe pola */
    string getFieldNameSet();    /**< Funkcja zwraca zagreowany w stringu zbior nazw pola (dla celow prezentacyjnych)*/
  } ;

  /** Klasa/Struktura przechowujaca zapytania - program relizacji i schemat */
  class query
  {

  private:

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version)
    {
      ar & id ;
      ar & filename ;
      ar & rInterval ;
      ar & lSchema ;
      ar & lProgram ;
      ar & inMulticast ;
    }

  public:

    query( R rInterval , string id ) ;
    query() ;

    list< string > getFieldNamesList() ;

    string id ;                         /**< nazwa strumienia/zapytania */
    string filename ;                   /**< nazwa pliku skojarzona ze strumieniem. dot. deklaracji*/
    R rInterval ;                       /**< interwal pomiedzy krotkami strumienia */
    list < field > lSchema ;            /**< Schemat strumienia zlozony z pol */
    list < token > lProgram ;           /**< Program budowy strumienia */
    bool inMulticast ;                  /**< Informacja o tym czy strumien wystepuje w strumieniu transmitowanym */

    bool isDeclaration();               /**< Funkcja zwrcajaca informacje czy strumien jest deklaracja*/
    bool isReductionRequired();         /**< Funkcja zwraca true jestli program budowy strumienia wymaga optymalizacji */
    bool isGenerated();                 /**< Funkcja zwraca true jest program budowy strumienia zostal wygenerowany w trakcie kompilacji */
    bool isTransmitedByUDP();           /**< Funkcja informuje czy strumien jest transmitowany */
    void setMulticastStream(bool);       /**< Przelacznik dla transmisji strumienia*/

    field & getField( string sField );

    /** Funkcja zwraca dostepna liste nazw schematow z poziomu zapytania
        funkcja jesli poda jej sie argument zero (domyslny)
        zwroci cala liste strumieni
        jesli jednak poda sie numer - moze zwrocic wskazany, kolejny strumien */
    list < string > getDepStreamNameList( int reqDep = 0 ) ;

  } ;

  /** potrzebne do sortowania (ustalenia relacji porzadku wzgledem interwalu czasowego) w qTree */
  bool operator<(const query &lhs,const query &rhs) ;

  query & getQuery( string query_name );              /**< Funkcja zwraca referencje do zapytania */
  int getSeqNr( string query_name ) ;                 /**< Funkcja zwraca numer kolejny w zbiorze qTree */

  /** Deklaracja typu qTree - uporzadkowanego wzgledem czasow wykoniania zbioru strumeni */
  class qTree :
      public vector < query >
  {

  private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, unsigned int version)
    {
      ar & boost::serialization::base_object< vector < query > >(*this);
    }

  public:

    query & operator[] ( string query_name ) { return getQuery( query_name ) ; } ;
    void sort() { std::sort( begin() , end() ) ; }; /**< Sortowanie wzgledem delty - patrz zdefiniowany operator */
    void tsort() ;                                  /**< Sortowanie topologiczne */
    R getDelta( string query_name ) ;               /**< Funkcja zwraca interwal czasu (rInterval) dla zadanego strumienia */
  };

}
