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
#pragma once

#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <string>
#include <set>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#ifndef ARRAY_POINTER
#define ARRAY_POINTER boost::shared_array<char>
#endif

enum BlockType 
{ 
    BF ,    /** Block in File */
    BM ,    /** Block in Memory */
    BCF ,   /** Block in Cyclic File */
    BCM     /** Block in Cyclic Memory */
} ;

template <class IDTYPE>
class CBufferImpl ;

template <class IDTYPE>
class CBuffer
{
	boost::shared_ptr< CBufferImpl< IDTYPE > > pImpl ;

public:

	CBuffer() ;

    void Load( const char * filename ) ;    /**< Odczytanie CBufferImpl z pliku */
    void Save( const char * filename ) ;    /**< Zapisanie CBufferImpl do pliku */
    void PutBlock( IDTYPE blockID, ARRAY_POINTER pBlockBuffer , long count = 1 ) ;  /**< Umieszczenie ramki w zadanym Bloku (przed t¹ operacj¹ pamiêtaæ ¿e raz trzeba DefBlock lub Load) */
    long GetBlock( IDTYPE blockID, long position, ARRAY_POINTER pBlockBuffer ) ; /**< Pobranie ramki  */
    void DefBlock( IDTYPE blockID, long frameSize , BlockType btType = BF , long aux1 = 0 , long aux2 = 0) ; /**< Zdefiniowanie bloku Jeœli BCF - Cykliczny binarny plik to aux1 - iloœc podplików, aux2 - iloœæ ramek w podpliku	Jeœli BCM - j.w. else aux1,aux2 musi byc 0 ! */
    void DelBlock( IDTYPE blockID )  ;      /**< Usuniêcie bloku */
    long GetLen( IDTYPE blockID )  ;        /**< Zwrócenie iloœci ramek */
    long GetSizeOfFrame( IDTYPE blockID ) ; /**< Zwrócenie d³ugoœci pojedyñczej ramki */	
    long GetSizeOfBlock( IDTYPE blockID , long firstFrm=0 , long lastFrm=-1 ) ; /**< Zwrócenie d³ugoœci w bajtach ca³ego bloku */
    void Clear() ;                          /**< Obciêcie - wyczyszczenie - Truncate */
    void DecycleBlockType( IDTYPE blockID , ARRAY_POINTER pBlockBuffer ) ; /**< Rozpiêcie cyklu - zmiana typu - z typu Cyklicznego na typ p³aski */
    void MemorizeBlockType( IDTYPE blockID , ARRAY_POINTER pBlockBuffer ) ; /**< Przpisanie bloku do pamiêci */
    long syncToDisk(IDTYPE blockID ) ;      /** Funkcja zatwierdza strumien na dysku @return zwraca dlugosc strumienia w ramkach */    
    void Dump( std::ostream &os , std::set < std::string > * pQset ) ;         /** Do celów testowych i raportowych */
};
#endif // DBSTREAM_H
