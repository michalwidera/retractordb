#pragma once

//Standard template library
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <stack>
#include <sstream>

//Boost libraries
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/rational.hpp>

namespace boost {
    namespace serialization {

        template<class Archive, class T>
        inline void serialize(
            Archive & ar,
            boost::rational<T> & p,
            unsigned int /* file_version */
        ) {
            T _num( p.numerator() ) ;
            T _den( p.denominator() );
            ar & _num ;
            ar & _den ;
            p.assign( _num, _den );
        }
    } // namespace serialization
} // namespace boost



using namespace std ;
using namespace boost ;

boost::rational<int> Rationalize ( double inValue, double DIFF=1E-6,  int ttl=11 ) ;

enum command_id {
#define DEF_CASE( _A_ ) _A_ ,
#include "tokendefset.h"
#undef DEF_CASE
} ;

class token {

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version) {
        ar & command ;
        ar & crValue ;
        ar & rcValue ;
        ar & sValue_ ;
    }

    command_id command ;
    boost::rational<int> crValue ;
    boost::rational<int> rcValue ;
    string sValue_ ;

  public:
    string getValue();
    boost::rational<int> getCRValue() ;

    token( command_id id = VOID_COMMAND, string sValue = "" ) ;
    token( command_id id, boost::rational<int> crValue, string sValue = "" ) ;
    token( command_id id, double dValue );

    string getStrTokenName() ;
    command_id getTokenCommand();
} ;

class field {

  private:

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version) {
        ar & setFieldName ;
        ar & dFieldType ;
        ar & lProgram ;
        ar & sFieldText ;
    }

    string sFieldText ;

  public:

    enum eType { BAD, BYTE, INTEGER, RATIONAL } ;

    set< string > setFieldName ;
    eType dFieldType ;
    list < token > lProgram ;

    field () ;
    field ( string sFieldName, list < token > & lProgram, eType fieldType, string sFieldText ) ;

    string getFirstFieldName() ;
    string getFieldNameSet() ;
    string getFieldText() ;
    token  getFirstFieldToken() ;
} ;

class query {

  private:

    friend class boost::serialization::access ;

    template<class Archive>
    void serialize(Archive & ar, unsigned int version) {
        ar & id ;
        ar & filename ;
        ar & rInterval ;
        ar & lSchema ;
        ar & lProgram ;
    }

  public:

    query( boost::rational<int> rInterval, string id ) ;
    query() ;

    list< string > getFieldNamesList() ;

    string id ;
    string filename ;
    boost::rational<int> rInterval ;
    list < field > lSchema ;
    list < token > lProgram ;

    bool isDeclaration();
    bool isReductionRequired();
    bool isGenerated();

    field & getField( string sField );

    list < string > getDepStreamNameList( int reqDep = 0 ) ;

    int getFieldIndex( field f );
} ;

bool operator<(const query &lhs,const query &rhs) ;

query & getQuery( string query_name ) ;
int getSeqNr( string query_name ) ;
bool isDeclared( string query_name ) ;
bool isExist(string query_name);

class qTree :
    public vector < query > {

  private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, unsigned int version) {
        ar & boost::serialization::base_object< vector < query > >(*this) ;
    }

  public:

    query & operator[] ( string query_name ) {
        return getQuery( query_name ) ;
    } ;

    void sort() {
        std::sort( begin(), end() ) ;
    };

    /** Topological sort*/
    void tsort() ;

    boost::rational<int> getDelta( string query_name ) ;
};
