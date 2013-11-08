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

#ifndef IPHSTRMGR_H
#define IPHSTRMGR_H

#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/rational.hpp>
#include <string>
#include <list>

class CPhStrMgr ;

struct column {
    std::string name ;              /**< nazwa kolumny   */
    int type ;                      /**< typ kolumny     */
};

struct stream {
    std::string name ;              /**< nazwa strumienia */
    boost::rational<int> speed ;    /**< szybkosc naplywu */
}
;

class IPhStrMgr {
    boost::shared_ptr< CPhStrMgr > pImpl ;
public:
    IPhStrMgr();

    //
    // dostep do zbioru nazw strumieni danych
    //

    /** zwraca ilosc przetwarzanych/zarejestrowanych w systemie strumieni danych */
    int size();

    /** pos[0..getStreamCount()-1] - zwraca nazwe i szybkosc strumienia z tablicy jesli odwolamy sie do strumienia poza tablica zwraca ciag pusty */
    stream descriptor( int pos );

    //
    // dostep do danych w strumieniach
    //

    /** biezaca ilosc rekordow w strumieniu */
    int size( std::string streamName );

    /** zwrot schematu */
    std::list< column > schema( std::string streamName );

    /** pobranie rekordu ze strumienia, rPos = 0 <- biezacy */
    bool record( boost::shared_array< char > pRawData , std::string StreamName , int rPos );

};

#endif //IPHSTRMGR_H
