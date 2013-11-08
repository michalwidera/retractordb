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

//Standard template library
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <stack>

//Boost libraries
#include <boost/rational.hpp>
#include <boost/any.hpp>
#include <boost/spirit/core.hpp>
#include <boost/spirit/symbols/symbols.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/bind.hpp>

#include <boost/program_options.hpp>

#include <boost/regex.hpp>

//bjam "-sTOOLS=vc-8_0"

//Default c++ libraries
#include <conio.h>
#include <fstream>
#include <complex>

using namespace std ;
using namespace boost ;
using namespace boost::spirit;
using namespace boost::lambda;

using namespace QS ;

qTree coreInstance ;
config configInstance ;

stack < shared_ptr<query> > stk ;

// ---------- SET OF TEMP VARIABLES

list < token > lProgram ;
list < field > lSchema ;
std::string sFieldName = "" ;
field::eType fieldType = field::BAD ;
int fieldLen = 0 ;

////////////////////////////////////////////////////////////////////////////
//
//  Semantic actions
//
////////////////////////////////////////////////////////////////////////////

namespace {

#define RECPTOKEN(x)  { lProgram.push_back( token( x ) ) ; }
#define RECPSTRTK(x)  { lProgram.push_back( token( x , string( str,end ) ) ) ; }

    void do_fcall(char const* str, char const* end) 
    {
        string s(str, end);
        s.erase(s.find('('));
        lProgram.push_back( token ( CALL , s ) ) ;
    }

    void do_alias(char const* str, char const* end) 
    {
        sFieldName = string( str,end ) ;
    }

    void do_ftype(char const* str, char const* end) 
    {
        fieldType = field::BAD ;
        if ( string( str , end ) == "INTEGER" ) fieldType = field::INTEGER ;
        if ( string( str , end ) == "CHAR" ) fieldType = field::CHAR ;
    }

    void do_flen( int val ) 
    {
        fieldLen = val ;
    } 

    void do_stream_assign_id(char const* str, char const* end) {
        stk.top()->id = string( str,end );
    } 

    void pushVal(double val) 
    {
        lProgram.push_back( token( PUSH_VAL , val ) ) ;
    } 

    void pushValI(int val) 
    {
        lProgram.push_back( token( PUSH_VAL , val ) ) ;
    } 

    void do_TScan(char const* str, char const* end)       RECPSTRTK( PUSH_TSCAN )

    void do_ID1(char const* str, char const* end) 
    {
            lProgram.push_back( token( PUSH_ID1 , string( str,end ) ) ) ;
    } 

    int indexval ;
    int indexval2 ;
    string indexsch ;

    void do_indexval( int val ) 
    { 
        indexval = val ; 
    } 

    void do_indexval2( int val ) 
    { 
        indexval2 = val ; 
    } 

    void do_indexsch( char const* str, char const* end ) 
    { 
        string s(str,end); indexsch = s ; 
    } 

    void do_ID2(char const* str, char const* end)  //bylo: RECPSTRTK( PUSH_ID2)
    {
        string s(str, end);
        if
            (
            parse(s.c_str(),

            //  Begin grammar - schemat[index]
            (
                as_lower_d [ lexeme_d [ alpha_p >> *(alnum_p | '_') ] ] [&do_indexsch]
                >> ch_p('[')
                >> uint_p[&do_indexval]
                >> ch_p(']')
            )
            ,
            //  End grammar

            space_p).full
            )
        {
            lProgram.push_back( token( PUSH_ID2 , indexval ) ) ;
            lProgram.push_back( token( PUSH_ID2 , indexsch ) ) ;
        }
        else
            throw std::logic_error("Sub-parsowanie do_ID2 'stream by index' - Bd");
    }

    void do_ID4(char const* str, char const* end)  //bylo: RECPSTRTK( PUSH_ID2)
    {
        string s(str, end);

        if
            (
            parse(s.c_str(),

            //  Begin grammar  - schemat[index,index]
            (
            as_lower_d [ lexeme_d [ alpha_p >> *(alnum_p | '_') ] ] [&do_indexsch]
                >> ch_p('[')
                >> uint_p[&do_indexval]
                >> ch_p(',')
                >> uint_p[&do_indexval2]
                >> ch_p(']')
            )
            ,
            //  End grammar

            space_p).full
            )
        {
            if ( indexval2 == 0 )
            {
                lProgram.push_back( token( PUSH_ID2 , indexval ) ) ;
                lProgram.push_back( token( PUSH_ID2 , indexsch ) ) ;
            }
            else
            {
                lProgram.push_back( token( PUSH_ID4 , rational<int> ( indexval , indexval2 )  ) );
                lProgram.push_back( token( PUSH_ID4 , indexsch ) );
            }
        }
        else
            throw std::logic_error("Sub-parsowanie do_ID4 'stream by index,index' - Bd");
    }

    void do_stime(int const val)                            RECPTOKEN( STREAM_TIMEMOVE );  
    void do_ID3(char const* str, char const* end)           RECPSTRTK( PUSH_ID3 )
    void do_Stream(char const* str, char const* end)        RECPSTRTK( PUSH_STREAM )
    void pushType(char const* str, char const* end)         RECPSTRTK( TYPE )
    void do_add(char const*, char const*)                   RECPTOKEN( ADD )
    void do_subt(char const*, char const*)                  RECPTOKEN( SUBTRACT )
    void do_mult(char const*, char const*)                  RECPTOKEN( MULTIPLY )
    void do_div(char const*, char const*)                   RECPTOKEN( DIVIDE )
    void do_neg(char const*, char const*)                   RECPTOKEN( NEGATE )
    void do_band(char const*, char const*)                  RECPTOKEN( AND )
    void do_bor(char const*, char const*)                   RECPTOKEN( OR )
    void do_bneg(char const*, char const*)                  RECPTOKEN( NOT )
    void do_shash(char const*a, char const*b)               RECPTOKEN( STREAM_HASH )
    void do_sdein(char const*, char const*)                 RECPTOKEN( STREAM_DEHASH_DIV )
    void do_sdmin(char const*, char const*)                 RECPTOKEN( STREAM_DEHASH_MOD )
    void do_sadd(char const*, char const*)                  RECPTOKEN( STREAM_ADD )
    void do_ssubt(char const*, char const*)                 RECPTOKEN( STREAM_SUBSTRACT )
    void do_sagse(char const*, char const*)                 RECPTOKEN( STREAM_AGSE )

    void do_CMP_eq(char const*, char const*)                RECPTOKEN( CMP_EQUAL )
    void do_CMP_lt(char const*, char const*)                RECPTOKEN( CMP_LT )
    void do_CMP_gt(char const*, char const*)                RECPTOKEN( CMP_GT )
    void do_CMP_le(char const*, char const*)                RECPTOKEN( CMP_LE )
    void do_CMP_ge(char const*, char const*)                RECPTOKEN( CMP_GE )
    void do_CMP_neq(char const*, char const*)               RECPTOKEN( CMP_NOT_EQUAL )
    void do_agse(char const*, char const*)                  RECPTOKEN( AGSE )
    void do_avg(char const*, char const*)                   RECPTOKEN( AVG )
    void do_min(char const*, char const*)                   RECPTOKEN( MIN )
    void do_max(char const*, char const*)                   RECPTOKEN( MAX )
    void do_sum(char const*, char const*)                   RECPTOKEN( SUM )

    rational<int> r_val ;

    void do_static_delta(char const*, char const*) {
        stk.top()->rInterval = r_val ;
    }

    void rational_nominator( int rat_nom_param ){
        r_val = rat_nom_param ;
    }

    void rational_denominator( int rat_denom_param ){
        r_val = r_val / rat_denom_param ;
    }

    void rational_irrational( double rat_irrational_param ){
        r_val = QS::Rationalize( rat_irrational_param );
    }

    rational<int> rcx_real ;
    rational<int> rcx_imag ;
    void do_complex_real( char const* str, char const* end ) 
    {
        rcx_real = r_val ;
    }
    void do_complex_imag( char const* str, char const* end ) 
    {
        rcx_imag = r_val ;
    }

    void do_complex(char const* str, char const* end )
    {
        complex< rational<int> > rcx_val( rcx_real , rcx_imag );
        lProgram.push_back( token( PUSH_VAL , rcx_val ) ) ;
    }

    void do_transaction( char const* str, char const* end ) 
    {
        configInstance.transactionInterval = r_val ;
    }

    void do_select_section(char const*, char const*) 
    {
        stk.top()->lSchema = lSchema ;
        lSchema.clear() ;
    }

    void do_from_section(char const*str, char const*end) 
    {
        stk.top()->lProgram = lProgram ;
        lProgram.clear() ;
    }

    void do_arg_separator(char const*str, char const*end) 
    {
        lSchema.push_back( field( sFieldName , lProgram , fieldType , fieldLen ) );
        lProgram.clear() ;
        sFieldName = "" ;
        fieldType = field::BAD ;
        fieldLen = 0 ;
    }

    void do_Inner_Stream_Begin  (char const*, char const*)
    {
        stk.push( shared_ptr<query>( new query() ) );
    }

    void do_Inner_Stream_End    (char const*, char const*) 
    {
        string streamNameLoc( stk.top()->id );
        stk.pop();
        lProgram.push_back( token ( PUSH_STREAM , streamNameLoc ) );
    };

    void do_print(char const*str, char const*end) 
    {
        string s(str, end); cerr << "[[" << s << "]]" << endl ;
    }


    void do_reset()
    {
        lProgram.clear();
        lSchema.clear();
        sFieldName = "" ;
        fieldLen = 0 ;
        fieldType = field::BAD ;
    }

    void do_insert_into_schema  (char const* str, char const* end )
    {
        if ( ! stk.top()->id.empty() )
            coreInstance.insert( * ( stk.top() ) );

        do_reset();
    }


    void do_aggregate(char const* str, char const* end)
    {
        string s(str, end);
        parse(s.c_str(),

            //  Begin grammar - MIN ( strumien )
            (
                    (
                      as_lower_d["min"][&do_min]
                    | as_lower_d["max"][&do_max]
                    | as_lower_d["avg"][&do_avg]
                    | as_lower_d["sum"][&do_sum]
                    )
                >> ch_p('(')
                >> alnum_p
                >> ch_p(')')
            ) ,
            //  End grammar
            space_p).full ;
    }


} //Koniec namespace

////////////////////////////////////////////////////////////////////////////
//
//  Our ql_parser grammar
//
////////////////////////////////////////////////////////////////////////////
struct ql_parser : public grammar<ql_parser>
{
    template <typename ScannerT>
    struct definition
    {
        definition(ql_parser const& )
        {
            keywords =  "avg" ,
                "min" ,
                "max" ,
                "agse" ,
                "select" ,
                "from" ,
                "stream" ,
                "as" ,
                "transaction" ,
                "filter by" ;

            typedef inhibit_case<strlit<> > Token_t;

            //Operators
            chlit<>     EQUAL('=');
            strlit<>    NOT_EQUAL("<>");
            chlit<>     LT('<');
            strlit<>    LE("<=");
            strlit<>    GE(">=");
            chlit<>     GT('>');

            Token_t     IN_BEGIN = as_lower_d["{"];
            Token_t     IN_END = as_lower_d["}"];

            Token_t NUMBER = as_lower_d["number"] ;
            Token_t CHAR = as_lower_d["char"] ;
            Token_t AVG = as_lower_d["avg"] ;
            Token_t MAX = as_lower_d["max"] ;
            Token_t MIN = as_lower_d["min"] ;
            Token_t SUM = as_lower_d["sum"] ;
            Token_t AGSE = as_lower_d["agse"] ;
            Token_t SELECT = as_lower_d["select"] ;
            Token_t REM = as_lower_d["rem"] ;
            Token_t DECLARE = as_lower_d["declare"] ;
            Token_t TRANSACTION = as_lower_d["transaction"] ;
            Token_t AS = as_lower_d["as"] ;
            Token_t STREAM = as_lower_d["stream"] ;
            Token_t FROM = as_lower_d["from"] ;
            Token_t FILTER_BY = as_lower_d["filter by"];
            Token_t NOT = as_lower_d["not"];
            Token_t AND = as_lower_d["and"];
            Token_t OR = as_lower_d["or"];
            Token_t SQRT = as_lower_d["sqrt"] ;
            Token_t CEIL = as_lower_d["ceil"] ;
            Token_t ABS = as_lower_d["abs"] ;
            Token_t FLOOR = as_lower_d["floor"] ;
            Token_t SIGN = as_lower_d["sign"] ;
            Token_t CHR = as_lower_d["chr"] ;
            Token_t LENGTH = as_lower_d["length"] ;
            Token_t TO_CHAR = as_lower_d["to_char"] ;
            Token_t TO_NUMBER = as_lower_d["to_number"] ;
            Token_t TO_TIMESTAMP = as_lower_d["to_timestamp"] ;

            command
                =
                  remarque_command
                | select_command
                | declare_command
                | transaction_command
                ;

            remarque_command
                =
                (
                ( REM | ch_p('#') )
                >> * anychar_p
                )
                ;

            transaction_command
                =
                ( TRANSACTION >> rational )                 //wynik jest w r_val                
                [&do_transaction]
                ;


            declare_command
                =
                (
                    DECLARE
                    >>
                    (
                        (
                            ( 
                                id[&do_alias]
                                >> !
                                (
                                    ( id [&do_ftype] )          //typ - INTEGER/CHAR/...
                                    >> ! 
                                    ( ch_p('(') >> uint_p[&do_flen] >> ch_p(')') ) //(dlugosc)
                                )
                            )                               [&do_arg_separator]
                        ) % ch_p(',')
                    )                                       [&do_select_section]
                    >>
                    (
                        STREAM
                        >> id                               [&do_stream_assign_id]
                        >> ch_p(',') >> ( rational          [&do_static_delta]
                                        )
                    )                                   //[&do_as_stream_section]
                )
                [&do_insert_into_schema]
                ;

            select_command
                =
                (
                    SELECT
                    >>
                    (
                        longest_d
                        [
                            agse_expr |
                            (
                            table_scan |
                            ( expression                    [&do_alias]     //To jest po to zeby zainicjowac trescia nazwe pola
                            >> ! ( AS >> id[&do_alias] ) )
                            )                               [&do_arg_separator]
                            % ch_p(',')
                        ]
                    )                                       [&do_select_section]
                    >> ( STREAM >> id[&do_stream_assign_id])//[&do_as_stream_section]
                    >> ( FROM >> stream_expression )        [&do_from_section]
                    >> !(
                            ( FILTER_BY >> condition )          //[&do_filter_section]
                            >> ! ( ch_p(',') >> ( rational  [&do_static_delta]
                                                )
                        )
                    )
                )
                [&do_insert_into_schema]
                ;

            rational
                =
                ( uint_p [&rational_nominator] >> ch_p('/') >> uint_p[&rational_denominator] )
                |
                real_p[&rational_irrational]
                ;

            table_scan
                =
                ( ! ( stream_id >> ch_p('.') ) >> ch_p('*') )       [&do_TScan]
                ;

            condition
                =
                   ! ( NOT [ &do_bneg] )
                >> logical_term
                >> * ( ( OR >> logical_term )                       [ &do_bor ]
                )
                ;

            logical_term
                =
                logical_factor >>  *( ( AND >> logical_factor )     [ &do_band ] )
                ;

            logical_factor
                =
                ( ch_p('(') >> condition >> ch_p(')') )
                | expression >>
                (
                    ( EQUAL >> expression )         [&do_CMP_eq]
                    | ( NOT_EQUAL >> expression )   [&do_CMP_neq]
                    | ( GT >> expression )          [&do_CMP_gt]
                    | ( LT >> expression )          [&do_CMP_lt]
                    | ( GE >> expression )          [&do_CMP_ge]
                    | ( GT >> expression )          [&do_CMP_gt]
                )
                ;

            stream_id
                =
                id
                ;

            field_id
                =
                (
                ( id>>ch_p('.')>>id )                                   [&do_ID1]   //a stream.field
                |( id>>ch_p('[')>>uint_p>>ch_p(']') )                   [&do_ID2]   //b stream[idx]
                |( id>>ch_p('[')>>uint_p>>ch_p(',')>>uint_p>>ch_p(']')) [&do_ID4]   //b stream[idx,timemove]
                |id                                                     [&do_ID3]   //a field
                )
                ;

            id
                =
                as_lower_d
                [
                    lexeme_d
                    [
                        (alpha_p >> *(alnum_p | '_'))
                        - ( keywords >> anychar_p - (alnum_p | '_'))
                    ]
                ]
            ;

            stream_expression
                =
                stream_term >>
                *(
                    ( ch_p('+') >> stream_term )            [&do_sadd]
                    | 
                    ( 
                           ch_p('-')
                        >> ch_p('(') >> uint_p              [&pushValI]
                        >> ch_p(',') >> uint_p              [&pushValI]
                        >> ch_p(')')
                    )                                       [&do_ssubt]
                    | ( int_p[&pushValI] )                  [&do_stime]
                )
                ;

            stream_term
                =
                stream_factor >>
                *(
                      ( ch_p('#') >> stream_factor )        [&do_shash]
                    | ( ch_p('&') >> real_p[&pushVal] )     [&do_sdein]
                    | ( ch_p('%') >> real_p[&pushVal] )     [&do_sdmin]
                    | ( 
                        ch_p('@')
                        >> ch_p('(') >> uint_p              [&pushValI]
                        >> ch_p(',') >> uint_p              [&pushValI]
                        >> ch_p(')')
                       )                                    [&do_sagse]
                )
                ;

            stream_factor
                =
                stream_id                                   [&do_Stream]
                |   ch_p('(') >> stream_expression >> ch_p(')')
                |   (
                       IN_BEGIN                             [&do_Inner_Stream_Begin]
                    >> select_command                       //[&do_print]
                    >> IN_END
                    )                                       [&do_Inner_Stream_End]
                ;

            agregator
                =  (  MIN
                    | MAX
                    | AVG
                    | SUM
                    )   >> ch_p('(')
                        >> stream_id            [&do_ID1]
                        >> ch_p(')')  ;

            expression
                =
                term
                >> *(
                    ('+' >> term)               [&do_add]
                |   ('-' >> term)               [&do_subt]
                )
                ;

            term
                =   factor
                >> *(
                    ('*' >> factor)             [&do_mult]
                |   ('/' >> factor)             [&do_div]
                )
                ;

            factor
                =   real_p                      [&pushVal]
                |   ('-' >> factor)             [&do_neg]
                |   ('+' >> factor)
                |   funct
                |   field_id
                |   agregator                   [&do_aggregate]
                |   '(' >> expression >> ')'
                |   ( '{' >> complex  >> '}' )  [&do_complex]
                ;

            complex
                =
                rational                        [&do_complex_real]
                >> ","
                >> rational                     [&do_complex_imag]
                ;

            funct
                =
                (
                    ( SQRT
                    | CEIL
                    | ABS
                    | FLOOR
                    | SIGN
                    | CHR
                    | LENGTH
                    | TO_NUMBER
                    | TO_TIMESTAMP
                    )
                >> ch_p('(') >> expression % ch_p(',') >> ch_p(')')
                )
                [&do_fcall]
                ;

            schema
                =
                   ch_p('(')
                >> field_id % ch_p(',')
                >> ch_p(')')
                ;

            agse
                =
                (
                AGSE 
                >> ch_p('(')
                >> schema
                >> ch_p(',')
                >> type_id                          [&pushType]
                >> ch_p('<') >> uint_p              [&pushValI]
                >> ch_p('>') >> ch_p(',') >> uint_p [&pushValI]
                >> ch_p(')')
                )
                [&do_agse]
                ;

            agse_expr
                =
                (     
                  AVG>> ( ch_p('(') >> agse >> ch_p(')') )  [&do_avg]
                | MAX>> ( ch_p('(') >> agse >> ch_p(')') )  [&do_max]
                | MIN>> ( ch_p('(') >> agse >> ch_p(')') )  [&do_min]
                )
                | agse
                ;

            type_id
                =
                NUMBER | CHAR
                ;

        }

        symbols<> keywords;
        rule<ScannerT>
            expression, term, factor, complex, agregator,
            select_command,
            declare_command,
            transaction_command,
            remarque_command,
            id,stream_id,field_id,
            stream_expression, stream_term,stream_factor,inner_stream,
            agse,agse_expr,natural,schema,type_id,
            funct,
            rational,
            condition , logical_term , logical_factor,
            table_scan  ,
            command
            ;

        rule<ScannerT> const&
            start() const { return command; }
    };
};

#ifdef _MSC_VER
#define COMPILER "MS"
#else
#ifdef __GNUC__
#define COMPILER "GNU"
#else
#define COMPILER "Unknown"
#endif
#endif

int main(int argc, char * argv[])
{
    //Intro
    cerr << "Parser - parser of query language.\n";
    string revision(  "$Rev: 2 $" );  //This string is svn keyword
    replace( revision.begin() , revision.end() , '$' , ' ' );
    revision.erase( std::remove( revision.begin() , revision.end() , ' ' ) , revision.end()) ; //erase-remove idiom
    cerr <<
        "used compiler tag:" + string(COMPILER) +
        "," + revision <<
        ", CTS:" << string( __TIMESTAMP__) <<   //http://msdn2.microsoft.com/en-us/library/b0084kay(VS.80).aspx
        endl ;

    try {
        namespace po = boost::program_options;

        string sOutputFile ;
        string sInputFile ;

        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "program options")
            ("infile,i", po::value<string>(&sInputFile)->default_value("query.txt"), "data input file")
            ("outfile,o", po::value<string>(&sOutputFile)->default_value("out_p.qry"), "output data file")
            ;

        po::positional_options_description p;       //Assume that infile is the first option
        p.add("infile", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            cout << desc << endl ;
            return 1;
        }

        string sPlikDanych   = vm["infile"].as< string >() ;
        string sPlikWynikowy = vm["outfile"].as< string >() ;       //default ompiler_output.xml

        cerr << "Input :" << sPlikDanych << endl ;

        //
        // Main body
        //

        ql_parser g;

        string str;

        stk.push( shared_ptr<query>( new query() ) );
        ifstream input( sPlikDanych.c_str() );

        while ( getline( input, str ) )
        {
            do_reset();
            if ( ! parse( str.c_str() , g , space_p ).full )
            {
                cerr << str.c_str() << endl ;
                throw std::invalid_argument("Syntax Error.");
            }
        }

        assert( ! stk.empty() );
        stk.pop();

        cerr << "Output:" << sPlikWynikowy ;
        cerr << "\tCount:" << (int)coreInstance.size() << endl ;
        {
            const config configInstance2( configInstance ) ;
            const qTree coreInstance2( coreInstance ) ;

            std::ofstream ofs( sPlikWynikowy.c_str() );
            boost::archive::text_oarchive oa(ofs);

            oa << configInstance2 ;
            oa << coreInstance2 ;

        }
    }
    catch(exception& e)
    {
        cerr << e.what() << endl ;
        return 1;
    }
    return 0;
}
