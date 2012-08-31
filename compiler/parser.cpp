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

//Boost libraries
#include <boost/any.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/bind.hpp>

#include <boost/regex.hpp>

//bjam "-sTOOLS=vc-8_0"

using namespace std ;
using namespace boost ;
using namespace boost::spirit::classic;
using namespace boost::lambda;

using namespace QS ;

qTree coreInstance_parser ;
config configInstance_parser ;

stack < boost::shared_ptr<query> > stk ;

// ---------- SET OF TEMP VARIABLES

list < token > lProgram ;
list < field > lSchema ;
std::string sFieldName = "" ;
field::eType fieldType = field::BAD ;

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

        string vStr( string( str , end ) );

        if ( vStr == "BYTE" || vStr == "BTE" ) fieldType = field::BYTE ;
        if ( vStr == "INTEGER" || vStr == "INT" ) fieldType = field::INTEGER ;
        if ( vStr == "RATIONAL" || vStr == "RAT" ) fieldType = field::RATIONAL ;
        if ( vStr == "CRATIONAL" || vStr == "CRAT" ) fieldType = field::CRATIONAL ;

    }


    void do_stream_assign_id(char const* str, char const* end) {
        stk.top()->id = string( str,end );
        stk.top()->filename = "";
    }

    void do_stream_assign_file(char const* str, char const* end) {
        stk.top()->filename = string( str,end );
    }

    void pushVal(double val)
    {
        lProgram.push_back( token( PUSH_VAL , val ) ) ;
    }

    void pushValI(int val)
    {
        lProgram.push_back( token( PUSH_VAL , val ) ) ;
    }



    //int indexval ;
    //int indexval2 ;
    //string indexsch ;

    //void do_indexval( int val )
    //{
    //    indexval = val ;
    //}

    //void do_indexval2( int val )
    //{
    //    indexval2 = val ;
    //}

    //void do_indexsch( char const* str, char const* end )
    //{
    //    string s(str,end); indexsch = s ;
    //}

    /*
    void do_ID2(char const* str, char const* end)  //bylo: RECPSTRTK( PUSH_ID2)
    {
        lProgram.push_back( token( PUSH_ID2 , string( str,end ) ) ) ;
        return ;

        to przetwarzanie zostanie przesuniete do wyzszej warstwy (na regexpach)
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
            stringstream sTokenArgument;
            sTokenArgument << indexsch << "." << indexval ;
            lProgram.push_back( token( PUSH_ID2 , sTokenArgument.str() ) ) ;

            //lProgram.push_back( token( PUSH_ID2 , indexval ) ) ;
            //lProgram.push_back( token( PUSH_ID2 , indexsch ) ) ;
        }
        else
            throw std::logic_error("Sub-parsowanie do_ID2 'stream by index' - Bd");
    }

    void do_ID4(char const* str, char const* end)  //bylo: RECPSTRTK( PUSH_ID2)
    {
        lProgram.push_back( token( PUSH_ID4 , string( str,end ) ) ) ;
        return ;

        to przetwarzanie zostanie przesuniete do wyzszej warstwy (na regexpach)
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
                stringstream sTokenArgument;
                sTokenArgument << indexsch << "." << indexval ; ;
                lProgram.push_back( token( PUSH_ID2 , sTokenArgument.str() ) ) ;

                //lProgram.push_back( token( PUSH_ID2 , indexval ) ) ;
                //lProgram.push_back( token( PUSH_ID2 , indexsch ) ) ;
            }
            else
            {
                stringstream sTokenArgument;
                sTokenArgument << indexsch << "." << R ( indexval , indexval2 ) ;
                lProgram.push_back( token( PUSH_ID4 , sTokenArgument.str() ) ) ;

                //lProgram.push_back( token( PUSH_ID4 , R ( indexval , indexval2 )  ) );
                //lProgram.push_back( token( PUSH_ID4 , indexsch ) );
            }
        }
        else
            throw std::logic_error("Sub-parsowanie do_ID4 'stream by index,index' - Bd");
    }
    */

    void do_TScan(char const* str, char const* end)         RECPSTRTK( PUSH_TSCAN )
    void do_stime(int const val)                            RECPTOKEN( STREAM_TIMEMOVE )
    void do_ID3(char const* str, char const* end)           RECPSTRTK( PUSH_ID3 )
    void do_ID2(char const* str, char const* end)           RECPSTRTK( PUSH_ID2 )
    void do_ID4(char const* str, char const* end)           RECPSTRTK( PUSH_ID4 )
    void do_ID1(char const* str, char const* end)           RECPSTRTK( PUSH_ID1 )
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

    R r_val ;

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

    R rcx_real ;
    R rcx_imag ;
    void do_complex_real( char const* str, char const* end )
    {
        rcx_real = r_val ;
    }
    void do_complex_imag( char const* str, char const* end )
    {
        rcx_imag = r_val ;
    }
    void do_complex_real_i( int d_real  )
    {
        rcx_real = d_real ;
    }
    void do_complex_imag_i( int d_imag )
    {
        rcx_imag = d_imag ;
    }

    void do_complex(char const* str, char const* end )
    {
        complex< R > rcx_val( rcx_real , rcx_imag );
        lProgram.push_back( token( PUSH_VAL , rcx_val ) ) ;
    }

    void do_transaction( char const* str, char const* end )
    {
        configInstance_parser.transactionInterval = r_val ;
    }

    void do_trans_head( int r_val )
    {
        configInstance_parser.transHead = r_val ;
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
        lSchema.push_back( field( sFieldName , lProgram , fieldType ) );
        lProgram.clear() ;
        sFieldName = "" ;
        fieldType = field::BAD ;
    }

    void do_Inner_Stream_Begin  (char const*, char const*)
    {
        stk.push( boost::shared_ptr<query>( new query() ) );
    }

    void do_Inner_Stream_End    (char const*, char const*)
    {
        string streamNameLoc( stk.top()->id );
        stk.pop();
        lProgram.push_back( token ( PUSH_STREAM , streamNameLoc ) );
    };

    void do_print(char const*str, char const*end)
    {
        string s(str, end);
        clog << "compiler:" << "[[" << s << "]]" << endl ;
    }


    void do_reset()
    {
        lProgram.clear();
        lSchema.clear();
        sFieldName = "" ;        
        fieldType = field::BAD ;
    }

    void do_insert_into_schema  (char const* str, char const* end )
    {
        BOOST_FOREACH ( query & q , coreInstance_parser )
            if ( q.id == ( stk.top() )->id )
                throw std::invalid_argument(string( "Duplicate stream name:" ) + q.id );

        if ( ! stk.top()->id.empty() )
            coreInstance_parser.push_back( * ( stk.top() ) );

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
            Token_t DECLARE = as_lower_d["declare"] ;
            Token_t TRANSACTION = as_lower_d["transaction"] ;
            Token_t AS = as_lower_d["as"] ;
            Token_t STREAM = as_lower_d["stream"] ;
            Token_t FILE = as_lower_d["file"] ;
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
                | empty_line
                ;

            empty_line
                =
                (
                * ( ch_p(' ') | ch_p('\t') )
                )
                ;

            remarque_command
                =
                (
                ( ch_p('#') )
                >> * anychar_p
                )
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
                                    //>> !
                                    //( ch_p('(') >> uint_p[&do_flen] >> ch_p(')') ) //(dlugosc)
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
                    >>
                    ! (
                        FILE
                            >>
                            ( id >> * ( ch_p('.') >> id ) ) [&do_stream_assign_file]
                    )
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
                            table_scan                     [&do_alias]     //To jest po to zeby zainicjowac trescia nazwe pola '*'
                            |( expression                  [&do_alias]     //To jest po to zeby zainicjowac trescia nazwe pola
                                >> ! ( AS >> id[&do_alias] )
                              )
                            )                               [&do_arg_separator]
                            % ch_p(',')
                        ]
                    )                                       [&do_select_section]
                    >> ( STREAM >> id[&do_stream_assign_id])//[&do_as_stream_section]
                    >> ( FROM >> stream_expression )        [&do_from_section]
                    >> !(
                            ( FILTER_BY >> condition )          //[&do_filter_section]
                            >> ! ( ch_p(',')
                            >> ( rational  [&do_static_delta] )
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
                        >> ch_p('(')
                        >> (            uint_p              [&do_complex_real_i]
                              >> "," >> uint_p              [&do_complex_imag_i]
                           )                                [&do_complex]
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

            transaction_command
                =
                TRANSACTION
                >> rational [&do_transaction]
                >> ! ( ch_p(',') >> uint_p [&do_trans_head] )
                ;
        }

        symbols<> keywords;
        rule<ScannerT>
            expression, term, factor, complex, agregator,
            select_command,
            declare_command,
            transaction_command,
            remarque_command,
            empty_line,
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

string parser( string sPlikDanych, string sPlikWynikowy )
{
    //
    // Main parser body
    //

    ql_parser g;
    stk.push( boost::shared_ptr<query>( new query() ) );

    if ( sPlikDanych == "regtest" )
    {
        string str[] = { "transaction 0.5,1" ,
           "declare a INTEGER stream core0, 1" ,
           "select core0[1,1],core0 stream str1 from core0" } ;

        for ( int i = 0 ; i < 3 ; i++ )
        {
            do_reset();
            cout << "testing:" << str[i] << endl ;

            if ( !parse( str[i].c_str() , g , space_p ).full )
            {
                cerr << "compiler:" << str[i].c_str() << endl ;
                throw std::invalid_argument("Syntax Error.");
            }
        }
    }
    else
    {
        ifstream input( sPlikDanych.c_str() );

        string str;
        while ( getline( input, str ) )
        {
            if ( str == "stop" ) break ;
            do_reset();
            if ( ! parse( str.c_str() , g , space_p ).full )
            {
                cerr << "compiler:" << str.c_str() << endl ;
                throw std::invalid_argument("Syntax Error.");
            }
        }
    }

    assert( ! stk.empty() );
    stk.pop();

    //zapisanie skompilowanych zapytan na dysku
    {
        cout << "utworzenie obiektow serializacji" << endl ;

        const config configInstance2( configInstance_parser ) ;
        const qTree coreInstance2( coreInstance_parser ) ;

        std::ofstream ofs( sPlikWynikowy.c_str() );

        if ( ! ofs.good() )
        {
            cerr << "plik dla serializacji nie utworzony" << endl ;
            throw std::invalid_argument("Runtime Error.");
        }

        boost::archive::text_oarchive oa(ofs);

        if ( ! ofs.good() )
        {
            cerr << "podlaczenie serializacji do pliku zawiodlo" << endl ;
            throw std::invalid_argument("Runtime Error.");
        }

        cout << "zapis na dysku - serializacja:" << sPlikWynikowy << endl ;
        oa << configInstance2 ;
        oa << coreInstance2 ;
        cout << "zapis na dysku zakonczony" << endl ;
    }

    return string("OK");
}

