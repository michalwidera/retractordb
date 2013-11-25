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
#include "stdafx.h"

#include "QStruct.h"

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/function.hpp>

using namespace boost;
using namespace boost::lambda;

namespace QS {

#define UNDEFINIED_TOKEN "UNDEFINIED TOKEN"

int _floor( R & value )
{
    return static_cast<int> (
                floor ( rational_cast<double>( value ) )
                ) ;
}

bool operator<(const query &lhs,const query &rhs) { return lhs.rInterval < rhs.rInterval; }

	//{
	//	//
	//	//        |  b[n-floor(n*z)] dla floor(n*z)=floor((n+1)z)
	//	// c[n] = <
	//	//        |  a[floor(n*z)] dla floor(n*z)<>floor((n+1)z)
	//	//
	//	// z = delta_b / ( delta_a + delta_b )
	//	//
	//	// delta_c = ( delta_a * delta_b ) / ( delta_a + delta_b )
	//	//

	//	// Test required
	//	// result.delta = delta ;

	//	R zet = inB.getDelta() / ( inA.getDelta() + inB.getDelta() ) ;

	//	int n = result.position ;

	//	if ( _floor( n * zet ) == _floor( ( n + 1 ) * zet ) )
	//	{
	//		result.position = n - _floor( n * zet ) ;
	//		result.tupleRef = inB.tupleRef ;
	//	}
	//	else
	//	{
	//		result.position = _floor( n * zet );
	//		result.tupleRef = inA.tupleRef ;
	//	}
	//}

    command_id token::getTokenCommand()
    {
    return command ;
    }

	string token::getStrTokenName()
	{
	switch ( getTokenCommand() )
	{
#define DEF_CASE( _A_ ) case _A_ : return #_A_ ;
#include "tokendefset.h"
#undef DEF_CASE
	}
	return UNDEFINIED_TOKEN ;
	}

    R Rationalize ( double inValue , double DIFF/*=1E-6*/ ,  int ttl/*=11*/ )
    {
        stack<int> st ;
        double startx( inValue ),diff,err1,err2;
        unsigned int val ;

        for( ;; )
        {
            val = static_cast<unsigned int>( startx ) ;
            st.push ( static_cast<int>( val ) ) ;
            if ( ( ttl -- ) == 0 ) break ;
            diff = startx - val  ;
            if ( diff < DIFF ) break ;
            else startx = 1 / diff ;
            if ( startx > ( 1 / DIFF ) ) break ;
        }

        if ( st.empty() ) return R(0,1) ;

        R result1(0,1),result2(0,1);
        while ( ! st.empty() ) {
            if ( result1.numerator() !=0 )
                result2 = st.top() + ( 1 / result1 ) ;
            else
                result2 = st.top() ;
            st.pop() ;
            result1 = result2 ;
        }
        err1 = abs( rational_cast<double>(result1) - inValue );
        err2 = abs( rational_cast<double>(result2) - inValue );
        return err1 > err2 ? result2 : result1 ;
    }

    field & query::getField( string sField )
    {
        BOOST_FOREACH( field & f , lSchema )
                if ( f.setFieldName.find( sField ) != f.setFieldName.end() )
                return f ;

        throw std::logic_error("Brak poszukiwanego pola w schemacie");
        return * new field();
    }

	extern "C" qTree coreInstance ;

    bool isThere( vector < query > v , string query_name )
    {
        BOOST_FOREACH( query & q , v )
                if ( q.id == query_name )
                return true ;

        return false ;
    }



    //http://pl.wikipedia.org/wiki/Sortowanie_topologiczne
    void qTree::tsort()
    {
        vector < query > v = *this ;
        vector < query > des ;

        //najpierw na poczatek przenosze deklaracje
        //deklaracje z definicji nie maja zadnych potomokow
        /*
        for ( vector < query >::iterator it = v.begin() ;
              it != v.end() ;
              it ++ )
        {
            if ( (*it).isDeclaration() )
            {
                des.push_back( *it );
                v.erase( it );
                it = v.begin();
            }
        }
        */
        //
        while ( ! v.empty() )
            for ( vector < query >::iterator it = v.begin() ;
                  it != v.end() ;
                  it ++ )
            {
                if ( v.empty() ) break ;

                list < string > ls = (*it).getDepStreamNameList() ;

                bool fullDependent(true);
                BOOST_FOREACH( string s , ls )
                        if ( ! isThere( des , s ) ) fullDependent = false ;

                if ( fullDependent )
                {
                    des.push_back( *it );
                    v.erase( it );
                    it = v.begin();
                }
            }

        erase( begin() , end() );
        BOOST_FOREACH( query & q , des )
                push_back( q );
    }

    R qTree::getDelta( string query_name )
    {
        return getQuery( query_name ).rInterval ;
    }

	query & getQuery( string query_name )
	{
        assert ( query_name != "" );

		BOOST_FOREACH ( query & q , coreInstance )
		{
			if ( q.id == query_name )
				return q ;
		}
        cerr << "Missing:" << query_name << endl ;
        throw std::logic_error("Brak poszukiwanego strumienia w zbiorze - getQuery");

		static query void_query ;
		return ( void_query ) ; //proforma
	}

	int getSeqNr( string query_name )
	{
		int cnt(0);
		BOOST_FOREACH ( query & q , coreInstance )
				if ( query_name == q.id )
					return cnt ;
		else
			cnt ++ ;

        throw std::logic_error("Brak poszukiwanego strumienia w zbiorze - getSeqNr");

		return -1 ; //INVALID QUERY_NR
	}

	//query & qTree::getQuery( string query_name )
	//{
	//    for ( iterator it = begin() ; it != end() ; it ++ )
	//       if ( (*it).id == query_name )
	//            return (*it) ;

	//    cerr << "? " << query_name << endl ;
	//    throw std::logic_error("Brak poszukiwanego strumienia w zbiorze");
	//    return ( * end() ) ;
	//}

    complex< R > token::getCRValue()
    {
        return crValue ;
    }

    string token::getValue()
    {
        if ( sValue.empty() )
        {
            stringstream ss ;
            ss << crValue.real().numerator() ;
            if ( crValue.real().denominator() != 1 )
            {
                ss << "_" ;
                ss << crValue.real().denominator() ;
            }

            if ( crValue.imag().numerator() != 0 )
            {
                ss << "_i_" ;
                ss << crValue.imag().numerator() ;
                if ( crValue.imag().denominator() != 1 )
                {
                    ss << "_" ;
                    ss << crValue.imag().denominator() ;
                }
            }
            return string( ss.str() ) ;
        }
        else
            return sValue ;
    }

	bool query::isDeclaration()
	{
	return lProgram.empty() ;
	}

    bool query::isGenerated()
    {
        return ! id.compare( 0 , 7 , "STREAM_");
    }

	/** Construktor set */

	field::field ()
	{}

	field::field (
			string sFieldName  ,
			list < token > & lProgram ,
			eType dFieldType ) :
			lProgram ( lProgram ) ,
			dFieldType( dFieldType )
    {
        setFieldName.insert( sFieldName ) ;
    }

    string field::getFirstFieldName()
    {
        return * setFieldName.begin() ;
    }

    string field::getFieldNameSet()
    {
        stringstream retVal;
        BOOST_FOREACH ( string s , setFieldName )
                retVal << s << " " ;
        return retVal.str() ;
    }

	/** Construktor set */

	token::token( command_id id , string sValue ) :
			command( id ), sValue( sValue )
	{}

	token::token( command_id id , complex< R > crValue , string sValue ) :
			command( id ), crValue( crValue ), sValue( sValue )
	{}

	token::token( command_id id, double dValue ):
			command( id ), crValue( Rationalize( dValue )  , 0 )
	{}

	/** Construktor set */

	config::config() :
            transactionInterval(1) ,
            transHead(1)
	{}         //Default - 1 transaction per sec

	/** Construktor set */


	query::query( R rInterval , string id ) :
			rInterval( rInterval ),  id( id )
	{}

	query::query()
	{}

    list< string > query::getFieldNamesList()
    {
        list < string > schema ;

        BOOST_FOREACH( field  & f , lSchema )
        {
                schema.push_back( f.getFirstFieldName() );
        }

        return schema ;
    }

    void query::setMulticastStream( bool isInMulticast )
    {
        inMulticast = isInMulticast ;
    }

    bool query::isReductionRequired()
    {
        return lProgram.size() > 3 ;
    }

    list < string > query::getDepStreamNameList( int reqDep )
    {
        int iDep(0);
        list < string > lRetVal ;
        BOOST_FOREACH ( token t , lProgram )
        {
            if ( reqDep == 0 )
            {   //domyslne zachowanie
                if ( t.getTokenCommand() == PUSH_STREAM )
                  {
                    lRetVal.push_back( t.getValue() )  ;
                  }
            }
            else
            {   //wyciaganie wskazanego argumentu
                if ( reqDep == iDep )
                {
                    if ( t.getTokenCommand() == PUSH_STREAM )
                      {
                        lRetVal.push_back( t.getValue() )  ;
                      }
                }
                reqDep ++ ;
            }
        }
        return lRetVal ;
    }

} //End namespace QS
