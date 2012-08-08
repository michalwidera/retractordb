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
#include "stdafx.h"

#include <inputFileInstance.h>
#include <boost/rational.hpp>
#include <complex>

using namespace std ;
using namespace boost ;
using namespace QS ;

void inputFileInstance::goBegin()
{
    psFile->seekg (0, ios::beg);
    curPos = 0 ;
}

inputFileInstance::inputFileInstance( std::string inputFileName )
    : len(0) , curPos(0)
{
    shared_ptr< ifstream > ps( new ifstream( inputFileName.c_str() , ios::binary ) ) ;
    psFile = ps ;

    if ( ! psFile.get() )
    {
        len = -1 ;
    }
    else
    {
        psFile->seekg (0, ios::end);
        len = psFile->tellg();
        goBegin();
    }

    //nie rzucamy wyjatkow z konstruktora - zasada
}

inputFileInstance::inputFileInstance() : len(0) , curPos(0)
{
}

template < typename T >
T inputFileInstance::get()
{
    T retVal ;

    if ( len == -1 )
        throw std::out_of_range("no file connected to object");

    if ( curPos > ( len - sizeof ( T ) ) ) goBegin();
    psFile->read(  reinterpret_cast<char*>(&retVal), sizeof retVal ) ;
    curPos += sizeof ( T ) ;
    return retVal ;
}

//Specjalizacja char, int, rational, complex rational
template char inputFileInstance::get<char>() ;
template int inputFileInstance::get<int>() ;
template R inputFileInstance::get<R>() ;
template complex<R> inputFileInstance::get<complex<R> >() ;

//-------------------------------------------------------------
//input DISK FILE - part
//-------------------------------------------------------------

inputDF::inputDF(): inputFileInstance()
{
}

inputDF::inputDF( std::string inputFileName , std::list < QS::field > &lSchema ):
    inputFileInstance( inputFileName ) ,
    lSchema( lSchema )
{
}

void inputDF::processRow()
{
    lResult.clear();

    BOOST_FOREACH( field &f , lSchema )
    {
        switch( f.dFieldType )
        {
            case field::BYTE:
                lResult.push_back( get<char>() );
                break ;
            case field::INTEGER:
                lResult.push_back( get<int>() ) ;
                break ;
            case field::RATIONAL:
                lResult.push_back( get<R>() );
                break ;
            case field::CRATIONAL:
                lResult.push_back( get<complex<R> >() ) ;
                break ;
            case field::BAD:
            default:
                throw std::out_of_range( "processRow/niezdefiniowany typ" );
                break ; //proforma
        }
    }
}

complex< R > inputDF::getCR( QS::field f )
{
    complex< R > retValue(0) ;

    int cnt(0);

    BOOST_FOREACH( field & v , lSchema )
    {
        if (  v.getFirstFieldName() == f.getFirstFieldName() ) break ;
        cnt++ ;
    }

    if ( lResult.size() == 0 )
        processRow(); //taki patch - bo inaczej wylot

    assert( lResult.size() != 0 );

    if ( lResult[ cnt ].type() == typeid(char) )
        retValue  = any_cast<char>( lResult[ cnt ] );
    else if ( lResult[ cnt ].type() == typeid(int)  )
        retValue = any_cast<int>(  lResult[ cnt ] );
    else if ( lResult[ cnt ].type() == typeid(R)  )
        retValue = any_cast<R>(  lResult[ cnt ] );
    else if ( lResult[ cnt ].type() == typeid(complex<R>)  )
        retValue = any_cast<complex<R> >(  lResult[ cnt ] );
    else
        throw std::out_of_range( "getCR/niezdefiniowany typ" );
    return retValue ;
}



