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

#include "QStruct.h"
#include <boost/interprocess/offset_ptr.hpp>

namespace QS {

    using namespace std ;
    using namespace boost ;
    using namespace boost::interprocess ;

    class dbStreamHead : private boost::noncopyable
    {
        R minInterval;
        R maxInterval;

        /** Mapa z dlugosciami probek przechowanymi w head */
        struct head {
            int size ;      /** Wymiar w probkach */
            int current ;   /** Aktualna pozycja - current % size */
            int bufSize ;   /** Wymiar ramki , bufSize * size - wymiar bufora */
            int offset;  /** Adres bufora na dane*/
            head ( int size , int current , int bufSize , int offset ) : size(size) , current(current), bufSize(bufSize),offset(offset) {}
            head () : size(0),current(0),bufSize(0),offset(0) {}
        };

        typedef std::map<string, head> headMap_t;
        headMap_t headMap ;

    public:
        dbStreamHead();
        ~dbStreamHead();

        /** Zrzut danych do obszaru wymiany miedzyprocesowej */
        void storeInCommonArea( set < string > inSet );
        void prepareSharedArea();
    };

}
