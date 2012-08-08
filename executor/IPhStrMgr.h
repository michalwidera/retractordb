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

$Rev:: 36            $:  Revision of last commit
$Author:: widera     $:  Author of last commit
$Date:: 2012-07-31 2#$:  Date of last commit
$Id:: IPhStrMgr.h 36#$:  Id of last commit
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

    int
    size();
    /**< zwraca ilosc przetwarzanych/zarejestrowanych w systemie strumieni danych */

    stream
    descriptor( int pos );
    /**< pos[0..getStreamCount()-1] - zwraca nazwe i szybkosc strumienia z tablicy jesli odwolamy sie do strumienia poza tablica zwraca ciag pusty */

    //
    // dostep do danych w strumieniach
    //

    int
    size( std::string streamName );
    /**< biezaca ilosc rekordow w strumieniu */

    std::list< column >
    schema( std::string streamName );
    /**< zwrot schematu */

    bool
    record( boost::shared_array< char > pRawData , std::string StreamName , int rPos );
    /**< pobranie rekordu ze strumienia, rPos = 0 <- biezacy */

};

#endif //IPHSTRMGR_H
