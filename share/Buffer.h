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

typedef boost::shared_array<char> arrayPointer_t ;

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
    void PutBlock( IDTYPE blockID, arrayPointer_t pBlockBuffer , long count = 1 ) ;  /**< Umieszczenie ramki w zadanym Bloku (przed ta operacja pamietac ze raz trzeba DefBlock lub Load) */
    long GetBlock( IDTYPE blockID, long position, arrayPointer_t pBlockBuffer ) ; /**< Pobranie ramki  */
    void DefBlock( IDTYPE blockID, long frameSize , BlockType btType = BF , long aux1 = 0 , long aux2 = 0) ; /**< Zdefiniowanie bloku Jesli BCF - Cykliczny binarny plik to aux1 - ilosc podplikow, aux2 - ilosc ramek w podpliku	Jesli BCM - j.w. else aux1,aux2 musi byc 0 ! */
    void DelBlock( IDTYPE blockID )  ;      /**< Usuniecie bloku */
    long GetLen( IDTYPE blockID )  ;        /**< Zwrocenie ilosci ramek */
    long GetSizeOfFrame( IDTYPE blockID ) ; /**< Zwrocenie dlugosci pojedynczej ramki */	
    long GetSizeOfBlock( IDTYPE blockID , long firstFrm=0 , long lastFrm=-1 ) ; /**< Zwrocenie dlugosci w bajtach calego bloku */
    void Clear() ;                          /**< Obciecie - wyczyszczenie - Truncate */
    void DecycleBlockType( IDTYPE blockID , arrayPointer_t pBlockBuffer ) ; /**< Rozpiecie cyklu - zmiana typu - z typu Cyklicznego na typ plaski */
    void MemorizeBlockType( IDTYPE blockID , arrayPointer_t pBlockBuffer ) ; /**< Przpisanie bloku do pamieci */
    long syncToDisk(IDTYPE blockID ) ;      /** Funkcja zatwierdza strumien na dysku @return zwraca dlugosc strumienia w ramkach */    
    void Dump( std::ostream &os , std::set < std::string > * pQset ) ;         /** Do celow testowych i raportowych */
};
#endif // DBSTREAM_H
