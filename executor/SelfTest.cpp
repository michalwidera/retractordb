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

#include "QStruct.h"
#include "dbstream.h"

template < class T >
void testMatch( T & fn1 , T & fn2 , UR::dbStream & str )
{
    T cp1( fn1 ) , cp2 ( fn2 );

    bool bOk( cp1 == cp2 ) ;

    if ( ! str.resultValid )
    {
        std::cerr << "NA   " ;
        str.resultValid = true ;
    }
    else
    {
        if ( bOk ) std::cout << "ok   " ;
        else       std::cerr << "FAIL " ;
    }

    std::cout
        << cp1 
        << " " 
        << cp2 
        << " " ;    

    std::cout << std::endl ;
}

template < class T >
void testMatch( T fn1 , T fn2 )
{
    bool bOk( fn1 == fn2 ) ;

    {
        if ( bOk ) std::cout << "ok   " ;
        else       std::cerr << "FAIL " ;
    }

    std::cout
        << fn1
        << " "
        << fn2
        << " " ;

    std::cout << std::endl ;
}

//Testing area of executor - to call c:\>executor -t
void iTest()
{
    using namespace UR ;
    using namespace std ;

    list < string > schema ;
    schema.push_back( "pole1" );
    schema.push_back( "pole2" );
    schema.push_back( "pole3" );

    dbStream str1( "testowy1" , schema );

    str1[ "pole1" ] = complex< R >( R(3,4) , R(1,2) ) ;
    str1[ "pole2" ] = R(3,4) ;
    str1[ "pole3" ] = 2 ;

    cout << "---------------------------------\t record(0)" << endl ;
    cout << "str1[ \"pole1\" ] =" << str1[ "pole1" ] << endl ;
    cout << "str1[ \"pole2\" ] =" << str1[ "pole2" ] << endl ;
    cout << "str1[ \"pole3\" ] =" << str1[ "pole3" ] << endl ;

    cout << "STORE" << endl ;

    str1.store();

    str1[ "pole1" ] = complex< R >( R(99,1) , R(1,1) ) ;
    str1[ "pole2" ] = R(98,1) ;
    str1[ "pole3" ] = 97 ;

    cout << "---------------------------------\t record(1)" << endl ;
    cout << "str1[ \"pole1\" ] =" << str1[ "pole1" ] << endl ;
    cout << "str1[ \"pole2\" ] =" << str1[ "pole2" ] << endl ;
    cout << "str1[ \"pole3\" ] =" << str1[ "pole3" ] << endl ;

    complex< R > test1 = boost::get< complex< R > > ( str1[ "pole1" ] );
    R test2            = boost::get< R >            ( str1[ "pole2" ] );
    int test3          = boost::get< int >          ( str1[ "pole3" ] );

    cout << "---------------------------------\t index/name" << endl ;

    testMatch( str1[ "pole1" ] ,  str1[ 0 ] , str1 );
    testMatch( str1[ "pole2" ] ,  str1[ 1 ] , str1 );
    testMatch( str1[ "pole3" ] ,  str1[ 2 ] , str1 );

    cout << "---------------------------------\t store issue" << endl ;

    cout << "before store:" << str1[ 0 ] << "," << str1[ 1 ] << "," << str1[ 2 ] << endl ;
    cout << "before store:" << str1.get(0)[ 0 ] << "," << str1.get(0)[ 1 ] << "," << str1.get(0)[ 2 ] << endl ;

    cout << "STORE" << endl ;

    str1.store();        //potem juz nie ma str1["pole1"] - dostep tylko przez get

    cout << "after store:" << str1[ 0 ] << "," << str1[ 1 ] << "," << str1[ 2 ] << endl ;
    cout << "after store:" << str1.get(0)[ 0 ] << "," << str1.get(0)[ 1 ] << "," << str1.get(0)[ 2 ] << endl ;

    cout << "---------------------------------\t position" << endl ;
    cout << "str1.get( \"pole1\" , 1 ) = " << str1.get( "pole1" , 1 ) << endl ;
    cout << "str1.get( \"pole1\" , 0 ) = " << str1.get( "pole1" , 0 ) << endl ;


    cout << "---------------------------------\t offset = 0" << endl ;

    testMatch( str1.get(0)[ 0 ] , str1.get( "pole1", 0 ) );
    testMatch( str1.get(0)[ 1 ] , str1.get( "pole2", 0 ) );
    testMatch( str1.get(0)[ 2 ] , str1.get( "pole3", 0 ) );

    cout << "---------------------------------\t offset = 1" << endl ;

    testMatch( str1.get(1)[ 0 ] , str1.get( "pole1", 1 ) );
    testMatch( str1.get(1)[ 1 ] , str1.get( "pole2", 1 ) );
    testMatch( str1.get(1)[ 2 ] , str1.get( "pole3", 1 ) );

    cout << "---------------------------------\t offset = 0" << endl ;

    testMatch( str1.get(0)[ 0 ] , str1.get( "pole1", 0 ) );
    testMatch( str1.get(0)[ 1 ] , str1.get( "pole2", 0 ) );
    testMatch( str1.get(0)[ 2 ] , str1.get( "pole3", 0 ) );

    cout << "---------------------------------\t offset = 1" << endl ;

    testMatch( str1.get(1)[ 0 ] , str1.get( "pole1", 1 ) );
    testMatch( str1.get(1)[ 1 ] , str1.get( "pole2", 1 ) );
    testMatch( str1.get(1)[ 2 ] , str1.get( "pole3", 1 ) );

    cout << "---------------------------------\t local test" << endl ;

    cout << "by one" << endl ;
    cout << "str1.get(1)[ 2 ] = " << str1.get(1)[ 2 ] << endl ;
    cout << "str1[ 2 ]        = " << str1[ 2 ] << endl ;
    cout << "str1.get(0)[ 2 ] = " << str1.get(0)[ 2 ] << endl ;

    cout << "---------------------------------\t" << endl ;
}
