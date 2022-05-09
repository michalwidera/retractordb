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

namespace boost
{
    namespace serialization
    {

        template<class Archive, class T>
        inline void serialize(
            Archive &ar,
            boost::rational<T> &p,
            unsigned int /* file_version */
        )
        {
            T _num(p.numerator()) ;
            T _den(p.denominator());
            ar &_num ;
            ar &_den ;
            p.assign(_num, _den);
        }
    } // namespace serialization
} // namespace boost


boost::rational<int> Rationalize(double inValue, double DIFF = 1E-6,  int ttl = 11) ;

enum command_id {
#define DEF_CASE( _A_ ) _A_ ,
#include "tokendefset.h"
#undef DEF_CASE
} ;

class token
{

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, unsigned int version)
    {
        ar &command ;
        ar &crValue ;
        ar &rcValue ;
        ar &sValue_ ;
    }

    command_id command ;
    boost::rational<int> crValue ;
    boost::rational<int> rcValue ;
    std::string sValue_ ;

public:
    std::string getValue();
    boost::rational<int> getCRValue() ;

    token(command_id id = VOID_COMMAND, std::string sValue = "");
    token(command_id id, boost::rational<int> crValue, std::string sValue = "");
    token(command_id id, double dValue);

    std::string getStrTokenName() ;
    command_id getTokenCommand();
} ;

class field
{

private:

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, unsigned int version)
    {
        ar &setFieldName ;
        ar &dFieldType ;
        ar &lProgram ;
        ar &sFieldText ;
    }

    std::string sFieldText;

public:

    enum eType {BAD, BYTE, INTEGER, UNSIGNED, RATIONAL, FLOAT, STRING} ;

    std::set<std::string> setFieldName;
    eType dFieldType ;
    std::list<token> lProgram;

    field() ;
    field(std::string sFieldName, std::list<token> &lProgram, eType fieldType, std::string sFieldText);

    std::string getFirstFieldName();
    std::string getFieldNameSet();
    std::string getFieldText();
    token  getFirstFieldToken() ;

    friend std::ostream &operator<<(std::ostream &os, const field::eType s);
} ;

class query
{

private:

    friend class boost::serialization::access ;

    template<class Archive>
    void serialize(Archive &ar, unsigned int version)
    {
        ar &id ;
        ar &filename ;
        ar &rInterval ;
        ar &lSchema ;
        ar &lProgram ;
    }

public:

    query(boost::rational<int> rInterval, std::string id) ;
    query() ;

    std::list<std::string> getFieldNamesList();

    std::string id ;
    std::string filename;
    boost::rational<int> rInterval ;
    std::list<field> lSchema;
    std::list < token > lProgram ;

    bool isDeclaration();
    bool isReductionRequired();
    bool isGenerated();

    field &getField(std::string sField);

    std::list<std::string> getDepStreamNameList(int reqDep = 0);

    int getFieldIndex(field f);

    void reset();
} ;

bool operator< (const query &lhs, const query &rhs) ;

query &getQuery(std::string query_name) ;
int getSeqNr(std::string query_name) ;
bool isDeclared(std::string query_name) ;
bool isExist(std::string query_name);

class qTree :
    public std::vector < query >
{

private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, unsigned int version)
    {
        ar &boost::serialization::base_object< vector < query >> (*this) ;
    }

public:
    query &operator[](std::string query_name)
    {
        return getQuery(query_name) ;
    } ;

    void sort()
    {
        std::sort(begin(), end());
    };

    /** Topological sort*/
    void tsort() ;

    boost::rational<int> getDelta(std::string query_name);
};
