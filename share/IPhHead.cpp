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

$Rev:: 55            $:  Revision of last commit
$Author:: widera     $:  Author of last commit
$Date:: 2012-08-03 0#$:  Date of last commit
$Id:: IPhHead.cpp 55#$:  Id of last commit
*/
#include "stdafx.h"

#include "IPhHead.h"
#include "Buffer.h"

using namespace QS ;
using namespace boost ;

extern "C" {
    qTree coreInstance ;
    config configInstance ;
}

extern CBuffer<std::string> cbuf ;

/*
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>





using namespace boost::interprocess;

managed_shared_memory segment(open_or_create, "cbuf_db_shmem", 65536);

extern "C" {
    qTree coreInstance ;
    config configInstance ;
}

extern CBuffer<std::string> cbuf ;


//struct shm_remove
//{
//   shm_remove() {  shared_memory_object::remove("cbuf_db_shmem"); }
//   ~shm_remove(){  shared_memory_object::remove("cbuf_db_shmem"); }
//} remover;


dbStreamHead::dbStreamHead() :
        minInterval( std::numeric_limits<int>::max() )  ,
        maxInterval( std::numeric_limits<int>::min() )
{
    BOOST_FOREACH ( query q , coreInstance )
    {
        if ( minInterval > q.rInterval ) minInterval = q.rInterval ;
        if ( maxInterval < q.rInterval ) maxInterval = q.rInterval ;
    }



    //w zmiennej sizeOfHeadMap znajduja sie wielkosci/ilosci
    //probek jakie beda trzymane w Head
    //przyklad dla wartosci transHead = 1
    //najmniej - jedna odpowiadajaca strumieniowi n
    //potem tylko wiecej, im strumien szybciej nadchodzi
    //tym wiecej probek w headmap   



    int offset(0);
    BOOST_FOREACH ( query q , coreInstance )
    {
//        managed_shared_memory::size_type free_memory = segment.get_free_memory();

        int probeCount = rational_cast<int>( ( configInstance.transHead * q.rInterval ) / minInterval );

        headMap[ q.id ] =
            head (
                probeCount  ,                           // const - To sie nie powinno zmieniac
                0 ,                                     // (++) Brak aktualnych probek w head
                cbuf.GetSizeOfFrame( q.id ) ,           // const - Taki jest rozmiar ramki
                offset += ( probeCount * cbuf.GetSizeOfFrame( q.id ) )
                ) ;        
//        assert( free_memory <= segment.get_free_memory() ); // Wywalic kod jak braknie pamieci
    }

    cerr << "-------------" << endl ;
    cerr << "trans head\t" << configInstance.transHead << endl ;
    cerr << "size\tcurr\tbufsize\toffset\tname\n" ;
    BOOST_FOREACH (  headMap_t::value_type &hm , headMap )
            cerr
                 << hm.second.size << "\t"
                 << hm.second.current << "\t"
                 << hm.second.bufSize << "\t"
                 << hm.second.offset << "\t"
                 << hm.first << "\t"
                 << endl ;
    cerr << "-------------" << endl ;
    system("pause");
    ;
}

dbStreamHead::~dbStreamHead()
{
}

//ten kawalek kodu docelowo powinno sie wylaczyc
//do osobnego obiektu odpowiedzialnego za komunikacje
//miedyprocesowa
//--> extern CBuffer<std::string> cbuf ;
void dbStreamHead::storeInCommonArea( set < string > inSet )
{
    shared_array< char > pRawData ;
    BOOST_FOREACH ( query q , coreInstance )
    {
        if ( inSet.find( q.id ) == inSet.end() )
            continue ;

        pRawData.reset( new char[ cbuf.GetSizeOfFrame( q.id ) ] );
        cbuf.GetBlock( q.id , -1 , pRawData  );
    }
}

void dbStreamHead::prepareSharedArea()
{

}
*/
