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
#include "stdafx.h"

#include "dbstream.h"
#include "Buffer.h"

CBuffer<std::string> cbuf ;

namespace UR {

using namespace std ;
using namespace boost ;

//po wykonaniu saveStreamsToFile
//- dane ze strumieni dostepne sa na dysku w postaci pliku
void saveStreamsToFile( const string filename )
{
    cbuf.Save( filename.c_str() );
}

dbStream::dbStream( string streamName , list < string > schema ) :
    streamName( streamName ) ,
    schema( schema ) ,
    m_offset(-1) ,
    bWasFeedByGet( false ),
    frameSize( schema.size() * sizeof( varfield ) ),
    pRawData( new char[ frameSize ] ),
    resultValid( true )
{
    cerr << ":" << streamName << endl ;
    cbuf.DefBlock( streamName , frameSize , BF );   //Deklaracja bloku w systemie strumieniowym
};

//Mapping po int (po pozycji)
varfield&  dbStream::operator[](const int& _Keyval)
{
    int pos(0) ;
    BOOST_FOREACH( string & str , schema )
    {
        if ( pos == _Keyval )
        {
            return (*this)[ str ] ;
        }
        pos ++ ;
    }

    assert( false );                      //Pro Forma - never called
    return mpStrVar[ * schema.begin() ] ; //Pro Forma - never called
}

//Mapping po string
varfield& dbStream::operator[](const string& _Keyval)
{
    if ( bWasFeedByGet == true )
    {
        bWasFeedByGet = false ;
        return mpShadow[ _Keyval ] ;
    }

    bWasFeedByGet = false ;
    return mpStrVar[ _Keyval ] ;
}

//po wykonaniu store - dostep do danych z indeksem 0
//w strumieniu tylko przez shadow
void dbStream::store()
{
    //tutaj musi byc przygotowanie zawartosci pRawData w oparciu o danych schemat
    char * p ( pRawData.get() );

    int cnt(0);
    BOOST_FOREACH( string & str , schema )
    {
        varfield fieldData ( (* this) [ cnt ] ) ;
        memcpy( p , & fieldData , sizeof( varfield ) );
        p += sizeof( varfield ) ;
        cnt ++;
    }

    mpStrVar.clear() ;                      //Po store - czytamy z shadow
    cbuf.PutBlock( streamName , pRawData ); //a tutaj jest wyslanie tej zawartosci
};

dbStream & dbStream::get( int offset )
{
    if ( offset >= cbuf.GetLen( streamName ) && offset != 0 )
    {
        resultValid = false ;
        return *this ;
    }

    bWasFeedByGet = true ;

    if ( offset != m_offset )
    {
        assert ( cbuf.GetLen( streamName ) != 0 );
        m_offset = offset ;
        long ret = cbuf.GetBlock( streamName , offset , pRawData );
        assert( ret != 0 );  //Znaczy cos sie odczytalo (jak padnie to nie)
        mpShadow.clear() ;   //usuniecie elementow z mapy
        char * p ( pRawData.get() );
        BOOST_FOREACH( string & str , schema )
        {
            varfield fieldData ;
            memcpy( & fieldData , p , sizeof( varfield ) );
            mpShadow.insert(mpShadow.lower_bound(str),record::value_type(str, fieldData));
            p += sizeof( varfield ) ;
        }
    }
    return *this ;
}

varfield dbStream::get( string fieldName , int offset )
{
    if ( offset >= cbuf.GetLen( streamName ) && offset != 0 )
    {
        resultValid = false ;
        varfield inValidData ;
        return inValidData;
    }

    shared_array< char > pRawData ;
    pRawData.reset( new char[ frameSize ] );
    long ret = cbuf.GetBlock( streamName , offset , pRawData );
    char * p ( pRawData.get() );

    if ( ret != 0 ) //Cos zostalo odczytane
    {
        BOOST_FOREACH( string & str , schema )
        {
            if ( str == fieldName )
            {
                varfield fieldData ;
                memcpy( & fieldData , p , sizeof( varfield ) );
                return fieldData ;
            }
            p += sizeof( varfield ) ;
        };
    }

    assert( false );     //Pro Forma
    varfield fieldData ; //Pro Forma
    return fieldData ;   //Pro Forma
}

//Funkcja zaklada ze rekord zawiera TYLKO! pola typu varfield
varfield getFromRawData( shared_array< char > pRawData , int position )
{
    char * p ( pRawData.get() );
    varfield retVal ;
    memcpy( & retVal , p + ( position * sizeof( varfield ) ), sizeof( varfield ) );
    return retVal ;
}
};
