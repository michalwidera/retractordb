#include "QStruct.h"

#include <assert.h>                  // for assert
#include <ext/alloc_traits.h>        // for __alloc_traits<>::value_type
#include <boost/core/enable_if.hpp>  // for enable_if_c<>::type
#include <iostream>                  // for cerr
#include <sstream>                   // for operator<<, basic_ostream, endl
#include <stack>                     // for stack
#include <stdexcept>                 // for logic_error

#include <boost/lambda/lambda.hpp>   // IWYU pragma: keep
#include <boost/lambda/bind.hpp>     // IWYU pragma: keep
#include <boost/function.hpp>        // IWYU pragma: keep

using namespace boost;
using namespace boost::lambda;

#define UNDEFINIED_TOKEN "UNDEFINIED TOKEN"

//int _floor( boost::rational<int> & value ) {
//    return static_cast<int> (
//            floor ( rational_cast<double>( value ) )
//        ) ;
//}

bool operator< (const query &lhs, const query &rhs) {
    return lhs.rInterval < rhs.rInterval;
}

//{
//  //
//  //        |  b[n-floor(n*z)] dla floor(n*z)=floor((n+1)z)
//  // c[n] = <
//  //        |  a[floor(n*z)] dla floor(n*z)<>floor((n+1)z)
//  //
//  // z = delta_b / ( delta_a + delta_b )
//  //
//  // delta_c = ( delta_a * delta_b ) / ( delta_a + delta_b )
//  //

//  // Test required
//  // result.delta = delta ;

//  R zet = inB.getDelta() / ( inA.getDelta() + inB.getDelta() ) ;

//  int n = result.position ;

//  if ( _floor( n * zet ) == _floor( ( n + 1 ) * zet ) )
//  {
//      result.position = n - _floor( n * zet ) ;
//      result.tupleRef = inB.tupleRef ;
//  }
//  else
//  {
//      result.position = _floor( n * zet );
//      result.tupleRef = inA.tupleRef ;
//  }
//}

command_id token::getTokenCommand() {
    return command ;
}

string token::getStrTokenName() {
    switch (getTokenCommand()) {
#define DEF_CASE( _A_ ) case _A_ : return #_A_ ;
#include "tokendefset.h"
#undef DEF_CASE
    }

    return UNDEFINIED_TOKEN ;
}

boost::rational<int> Rationalize(double inValue, double DIFF/*=1E-6*/,  int ttl/*=11*/) {
    stack<int> st ;
    double startx(inValue), diff, err1, err2;
    unsigned int val ;

    for (;;) {
        val = static_cast<unsigned int>(startx) ;
        st.push(static_cast<int>(val)) ;

        if ((ttl --) == 0) {
            break ;
        }

        diff = startx - val  ;

        if (diff < DIFF) {
            break ;
        } else {
            startx = 1 / diff ;
        }

        if (startx > (1 / DIFF)) {
            break ;
        }
    }

    if (st.empty()) {
        return boost::rational<int> (0, 1) ;
    }

    boost::rational<int> result1(0, 1), result2(0, 1);

    while (! st.empty()) {
        if (result1.numerator() != 0) {
            result2 = st.top() + (1 / result1) ;
        } else {
            result2 = st.top() ;
        }

        st.pop() ;
        result1 = result2 ;
    }

    err1 = abs(rational_cast<double> (result1) - inValue);
    err2 = abs(rational_cast<double> (result2) - inValue);
    return err1 > err2 ? result2 : result1 ;
}

field &query::getField(string sField) {
    for (auto &f : lSchema) {
        if (f.setFieldName.find(sField) != f.setFieldName.end()) {
            return f ;
        }
    }

    throw std::logic_error("No such field in schema");
    return * new field();
}

extern "C" qTree coreInstance ;

bool isThere(vector < query > v, string query_name) {
    for (auto &q : v) {
        if (q.id == query_name) {
            return true ;
        }
    }

    return false ;
}


//https://en.wikipedia.org/wiki/Topological_sorting
//
//NOTE: Technical debt!! - if in compiled query set
//there is missing declaration of one stream this tsort
//function will go into infinite loop
//This is bug that need to be resolved
void qTree::tsort() {
    vector < query > v = *this ;
    vector < query > des ;

    while (! v.empty())
        for (vector < query >::iterator it = v.begin() ;
            it != v.end() ;
            it ++) {
            if (v.empty()) {
                break ;
            }

            list < string > ls = (*it).getDepStreamNameList() ;
            bool fullDependent(true);

            for (auto s : ls) {
                if (! isThere(des, s)) {
                    fullDependent = false ;
                }
            }

            if (fullDependent) {
                des.push_back(*it);
                v.erase(it);
                it = v.begin();
            }
        }

    erase(begin(), end());

    for (auto &q : des) {
        push_back(q);
    }
}

boost::rational<int> qTree::getDelta(string query_name) {
    return getQuery(query_name).rInterval ;
}

query &getQuery(string query_name) {
    assert(query_name != "");

    for (auto &q : coreInstance) {
        if (q.id == query_name) {
            return q ;
        }
    }

    cerr << "Missing:" << endl ;
    cerr << " " << query_name << endl ;
    cerr << "Avaiable:" << endl ;

    for (auto &q : coreInstance) {
        cerr << " " << q.id << endl ;
    }

    throw std::logic_error("No such stream in set - getQuery");
    static query void_query ;
    return (void_query) ;   //proforma
}

int getSeqNr(string query_name) {
    int cnt(0);

    for (auto &q : coreInstance) {
        if (query_name == q.id) {
            return cnt ;
        } else {
            cnt ++ ;
        }
    }

    throw std::logic_error("No such stream in set - getSeqNr");
    return -1 ; //INVALID QUERY_NR
}

bool isDeclared(string query_name) {
    for (auto &q : coreInstance) {
        if (query_name == q.id) {
            return q.isDeclaration() ;
        }
    }

    return false ;
}

bool isExist(string query_name) {
    for (auto &q : coreInstance) {
        if (query_name == q.id) {
            return true ;
        }
    }

    return false ;
}

boost::rational<int> token::getCRValue() {
    return crValue ;
}

string token::getValue() {
    if (sValue_.empty()) {
        stringstream ss ;
        ss << crValue.numerator() ;

        if (crValue.denominator() != 1) {
            ss << "_" ;
            ss << crValue.denominator() ;
        }

        return string(ss.str()) ;
    } else {
        return sValue_ ;
    }
}

bool query::isDeclaration() {
    return lProgram.empty() ;
}

bool query::isGenerated() {
    return ! id.compare(0, 7, "STREAM_");
}

/** Construktor set */

field::field()
{}

field::field(
    string sFieldName,
    list < token > &lProgram,
    eType dFieldType,
    string sFieldText) :
    lProgram(lProgram),
    dFieldType(dFieldType),
    sFieldText(sFieldText) {
    setFieldName.insert(sFieldName) ;
}
string field::getFieldText() {
    return sFieldText ;
}

string field::getFirstFieldName() {
    return * setFieldName.begin() ;
}

token field::getFirstFieldToken() {
    assert(lProgram.size() > 0);    //If this fails that means in field no program (decl!)
    return * lProgram.begin() ;
}

string field::getFieldNameSet() {
    stringstream retVal;

    for (const auto &s : setFieldName) {
        retVal << s << " " ;
    }

    return retVal.str() ;
}

/** Construktor set */

token::token(command_id id, string sValue) :
    command(id), crValue(0), sValue_(sValue)
{}

token::token(command_id id, boost::rational<int> crValue, string sValue) :
    command(id), crValue(crValue), sValue_(sValue)
{}

token::token(command_id id, double dValue) :
    command(id), crValue(Rationalize(dValue)), sValue_("")
{}

/** Construktor set */


query::query(boost::rational<int> rInterval, string id) :
    rInterval(rInterval),  id(id)
{}

query::query()
{}

list< string > query::getFieldNamesList() {
    list < string > schema ;

    for (auto &f : lSchema) {
        schema.push_back(f.getFirstFieldName());
    }

    return schema ;
}

int query::getFieldIndex(field f_arg) {
    int idx(0);

    for (auto f : lSchema) {
        if (f.getFirstFieldName() == f_arg.getFirstFieldName()) {   //Todo
            return idx ;
        }

        idx++ ;
    }

    return -1 ; //not found
}

bool query::isReductionRequired() {
    int streamOperatorCount(0) ;

    for (auto &t : lProgram) {
        if (t.getTokenCommand() == STREAM_HASH) {
            streamOperatorCount ++ ;
        }

        if (t.getTokenCommand() == STREAM_DEHASH_DIV) {
            streamOperatorCount ++ ;
        }

        if (t.getTokenCommand() == STREAM_DEHASH_MOD) {
            streamOperatorCount ++ ;
        }

        if (t.getTokenCommand() == STREAM_ADD) {
            streamOperatorCount ++ ;
        }

        if (t.getTokenCommand() == STREAM_SUBSTRACT) {
            streamOperatorCount ++ ;
        }

        if (t.getTokenCommand() == STREAM_TIMEMOVE) {
            streamOperatorCount ++ ;
        }

        if (t.getTokenCommand() == STREAM_AGSE) {
            streamOperatorCount ++ ;
        }
    }

    return streamOperatorCount > 1 ;
}

list < string > query::getDepStreamNameList(int reqDep) {
    int iDep(0);
    list < string > lRetVal ;

    for (auto &t : lProgram) {
        if (reqDep == 0) {
            //defult behaviour
            if (t.getTokenCommand() == PUSH_STREAM) {
                lRetVal.push_back(t.getValue())  ;
            }
        } else {
            if (reqDep == iDep) {
                if (t.getTokenCommand() == PUSH_STREAM) {
                    lRetVal.push_back(t.getValue())  ;
                }
            }

            reqDep ++ ;
        }
    }

    return lRetVal ;
}
