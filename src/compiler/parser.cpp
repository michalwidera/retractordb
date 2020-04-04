#include "QStruct.h"

//Boost libraries
#include <boost/any.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cctype>
#include <locale>
#include <memory>

//bjam "-sTOOLS=vc-8_0"

using namespace std ;
using namespace boost ;
using namespace boost::spirit::classic;
using namespace boost::lambda;

qTree coreInstance_parser ;

stack < std::shared_ptr<query>> stk ;

// ---------- SET OF TEMP VARIABLES

list < token > lProgram ;
list < field > lSchema ;
std::string sFieldName = "" ;
field::eType fieldType = field::BAD ;
int fieldCount(0);
int flen = 1 ;

bool invalidChar(char c) {
    return !(c >= 32 && c < 128);
}

void stripUnicode(std::string &str) {
    str.erase(remove_if(str.begin(), str.end(), invalidChar), str.end());
}

//
// Source for ltrim/rtrim:
// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
//

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

//
//  Semantic actions
//

namespace {

#define RECPTOKEN(x)  { lProgram.push_back( token( x ) ) ; }
#define RECPSTRTK(x)  { lProgram.push_back( token( x , string( str,end ) ) ) ; }

    void do_fcall(char const* str, char const* end) {
        string s(str, end);
        s.erase(s.find('('));
        lProgram.push_back(token(CALL, s)) ;
    }

    void do_alias(char const* str, char const* end) {
        sFieldName = string(str, end) ;
        flen = 1 ;
    }

    void do_alias_f(char const* str, char const* end) {
        sFieldName = "Field_" + boost::lexical_cast<std::string> (fieldCount ++);
    }

    void do_flen(int val) {
        flen = val ;
    }

    void do_ftype(char const* str, char const* end) {
        fieldType = field::BAD ;
        string vStr(string(str, end));

        if (vStr == "BYTE" || vStr == "BTE") {
            fieldType = field::BYTE ;
        }

        if (vStr == "INTEGER" || vStr == "INT") {
            fieldType = field::INTEGER ;
        }

        if (vStr == "RATIONAL" || vStr == "RAT") {
            fieldType = field::RATIONAL ;
        }
    }


    void do_stream_assign_id(char const* str, char const* end) {
        stk.top()->id = string(str, end);
        stk.top()->filename = "";
    }


    void do_stream_assign_file(char const* str, char const* end) {
        std::string filename = string(str, end);
        ltrim(filename);
        rtrim(filename);
        stk.top()->filename = filename;
    }


    void do_TScan(char const* str, char const* end)  RECPSTRTK(PUSH_TSCAN)
    void do_ID3(char const* str, char const* end)    RECPSTRTK(PUSH_ID3)
    void do_ID2(char const* str, char const* end)    RECPSTRTK(PUSH_ID2)
    void do_ID4(char const* str, char const* end)    RECPSTRTK(PUSH_ID4)
    void do_ID5(char const* str, char const* end)    RECPSTRTK(PUSH_ID5)
    void do_ID1(char const* str, char const* end)    RECPSTRTK(PUSH_ID1)
    void do_IDX(char const* str, char const* end)    RECPSTRTK(PUSH_IDX)
    void do_Stream(char const* str, char const* end) RECPSTRTK(PUSH_STREAM)
    void do_add(char const*, char const*)            RECPTOKEN(ADD)
    void do_subt(char const*, char const*)           RECPTOKEN(SUBTRACT)
    void do_mult(char const*, char const*)           RECPTOKEN(MULTIPLY)
    void do_div(char const*, char const*)            RECPTOKEN(DIVIDE)
    void do_neg(char const*, char const*)            RECPTOKEN(NEGATE)
    void do_band(char const*, char const*)           RECPTOKEN(AND)
    void do_bor(char const*, char const*)            RECPTOKEN(OR)
    void do_bneg(char const*, char const*)           RECPTOKEN(NOT)
    void do_shash(char const* a, char const* b)        RECPTOKEN(STREAM_HASH)
    void do_sdein(char const*, char const*)          RECPTOKEN(STREAM_DEHASH_DIV)
    void do_sdmin(char const*, char const*)          RECPTOKEN(STREAM_DEHASH_MOD)
    void do_sadd(char const*, char const*)           RECPTOKEN(STREAM_ADD)
    void do_sagse(char const*, char const*)          RECPTOKEN(STREAM_AGSE)

    void do_CMP_eq(char const*, char const*)         RECPTOKEN(CMP_EQUAL)
    void do_CMP_lt(char const*, char const*)         RECPTOKEN(CMP_LT)
    void do_CMP_gt(char const*, char const*)         RECPTOKEN(CMP_GT)
    void do_CMP_le(char const*, char const*)         RECPTOKEN(CMP_LE)
    void do_CMP_ge(char const*, char const*)         RECPTOKEN(CMP_GE)
    void do_CMP_neq(char const*, char const*)        RECPTOKEN(CMP_NOT_EQUAL)
    void do_avg(char const*, char const*)            RECPTOKEN(STREAM_AVG)
    void do_min(char const*, char const*)            RECPTOKEN(STREAM_MIN)
    void do_max(char const*, char const*)            RECPTOKEN(STREAM_MAX)
    void do_sum(char const*, char const*)            RECPTOKEN(STREAM_SUM)

    boost::rational<int> r_val ;

    void pushVal(double val) {
        lProgram.push_back(token(PUSH_VAL, val)) ;
    }

    void pushValI(int val) {
        lProgram.push_back(token(PUSH_VAL, val)) ;
    }

    void pushValR(char const* str, char const* end) {
        lProgram.push_back(token(PUSH_VAL, r_val)) ;
    }

    void do_static_delta(char const*, char const*) {
        stk.top()->rInterval = r_val ;
    }

    void rational_nominator(int rat_nom_param) {
        r_val = rat_nom_param ;
    }

    void rational_denominator(int rat_denom_param) {
        r_val = r_val / rat_denom_param ;
    }

    void rational_irrational(double rat_irrational_param) {
        r_val = Rationalize(rat_irrational_param);
    }

    void do_streamsubstract(char const* str, char const* end) {
        lProgram.push_back(token(STREAM_SUBSTRACT, r_val)) ;
    }

    void do_timemove(int const val) {
        lProgram.push_back(token(STREAM_TIMEMOVE, val)) ;
    }

    void do_select_section(char const*, char const*) {
        stk.top()->lSchema = lSchema ;
        lSchema.clear() ;
    }

    void do_from_section(char const* str, char const* end) {
        stk.top()->lProgram = lProgram ;
        lProgram.clear() ;
    }

    void do_arg_separator(char const* str, char const* end) {
        for (int i = 0 ; i < flen ; i ++) {
            string s(str, end);

            if (flen == 1) {
                lSchema.push_back(field(sFieldName, lProgram, fieldType, s));
            } else {
                string name = sFieldName + "_" + boost::lexical_cast<std::string> (i) ;
                lSchema.push_back(field(name, lProgram, fieldType, s));
            }

            lProgram.clear() ;
        }

        sFieldName = "" ;
        fieldType = field::BAD ;
        flen = 1 ;
    }

    void do_Inner_Stream_Begin(char const*, char const*) {
        stk.push(std::shared_ptr<query> (new query()));
    }

    void do_Inner_Stream_End(char const*, char const*) {
        string streamNameLoc(stk.top()->id);
        stk.pop();
        lProgram.push_back(token(PUSH_STREAM, streamNameLoc));
    };

    void do_print(char const* str, char const* end) {
        string s(str, end);
    }


    void do_reset() {
        lProgram.clear();
        lSchema.clear();
        sFieldName = "" ;
        fieldType = field::BAD ;
    }

    void do_insert_into_schema(char const* str, char const* end) {
        for (auto   &q : coreInstance_parser) {
            if (q.id == (stk.top())->id) {
                throw std::invalid_argument(string("Duplicate stream name:") + q.id);
            }
        }

        if (! stk.top()->id.empty()) {
            coreInstance_parser.push_back(* (stk.top()));
        }

        do_reset();
    }


    void do_aggregate(char const* str, char const* end) {
        string s(str, end);
        stripUnicode(s);
        parse(s.c_str(),
            // Begin grammar
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
            ),
            //  End grammar
            space_p).full ;
    }

} //End namespace

////////////////////////////////////////////////////////////////////////////
//
//  Our ql_parser grammar
//
////////////////////////////////////////////////////////////////////////////
struct ql_parser : public grammar<ql_parser> {
    template <typename ScannerT>
    struct definition {
        definition(ql_parser const &) {
            keywords =  "avg",
            "min",
            "max",
            "agse",
            "select",
            "from",
            "stream",
            "as"
            ;
            typedef inhibit_case<strlit<>> Token_t;
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
            Token_t COUNT = as_lower_d["count"] ;
            Token_t AGSE = as_lower_d["agse"] ;
            Token_t SELECT = as_lower_d["select"] ;
            Token_t DECLARE = as_lower_d["declare"] ;
            Token_t AS = as_lower_d["as"] ;
            Token_t STREAM = as_lower_d["stream"] ;
            Token_t FILE = as_lower_d["file"] ;
            Token_t FROM = as_lower_d["from"] ;
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
            Token_t FLOAT = as_lower_d["float"] ;
            Token_t INT = as_lower_d["int"] ;
            command
                =
                    remarque_command
                    | select_command
                    | declare_command
                    | empty_line
                    ;
            empty_line
                =
                    (
                        * (ch_p(' ') | ch_p('\t'))
                    )
                    ;
            remarque_command
                =
                    (
                        (ch_p('#'))
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
                                    id[&do_alias] >> !
                                    (
                                        (id [&do_ftype])
                                        >> !
                                        (ch_p('[') >> uint_p[&do_flen] >> ch_p(']'))
                                    )
                                )                               [&do_arg_separator]
                            ) % ch_p(',')
                        )                                       [&do_select_section]
                        >>
                        (
                            STREAM
                            >> id                               [&do_stream_assign_id]
                            >> ch_p(',') >> (rational          [&do_static_delta]
                            )
                        )
                        >>
                        !(
                            FILE
                            >>
                            (* anychar_p) [&do_stream_assign_file]
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
                            (
                                table_scan                     [&do_alias_f]
                                | (expression                  [&do_alias_f]
                                    >> !(AS >> id[&do_alias])
                                )
                            )                                  [&do_arg_separator]
                            % ch_p(',')
                        )                                      [&do_select_section]
                        >> (STREAM >> id[&do_stream_assign_id])
                        >> (FROM >> stream_expression)       [&do_from_section]
                    )
                    [&do_insert_into_schema]
                    ;
            rational
                =
                    (uint_p [&rational_nominator] >> ch_p('/') >> uint_p[&rational_denominator])
                    | ch_p('(') >> rational >> ch_p(')')
                    | real_p[&rational_irrational]
                    ;
            table_scan
                =
                    (!(stream_id >> ch_p('.')) >> ch_p('*'))       [&do_TScan]
                    ;
            condition
                =
                    !(NOT [ &do_bneg])
                    >> logical_term
                    >> * ((OR >> logical_term)                       [ &do_bor ]
                    )
                    ;
            logical_term
                =
                    logical_factor >>  * ((AND >> logical_factor)     [ &do_band ])
                    ;
            logical_factor
                =
                    (ch_p('(') >> condition >> ch_p(')'))
                    | expression >>
                    (
                        (EQUAL >> expression)         [&do_CMP_eq]
                        | (NOT_EQUAL >> expression)   [&do_CMP_neq]
                        | (GT >> expression)          [&do_CMP_gt]
                        | (LT >> expression)          [&do_CMP_lt]
                        | (GE >> expression)          [&do_CMP_ge]
                        | (LE >> expression)          [&do_CMP_le]
                    )
                    ;
            stream_id
                =
                    id
                    ;
            field_id
                =
                    (
                        (id >> ch_p('.') >> id)                                   [&do_ID1]      //a stream.field
                        | (id >> ch_p('[') >> uint_p >> ch_p(']') >> ch_p('[') >> uint_p >> ch_p(']')) [&do_ID5]               //b stream[idx][timemove]
                        | (id >> ch_p('[') >> uint_p >> ch_p(',') >> uint_p >> ch_p(']')) [&do_ID4]            //b stream[idx,timemove]
                        | (id >> ch_p('[') >> uint_p >> ch_p(']'))                   [&do_ID2]         //b stream[idx]
                        | (id >> ch_p('[') >> ch_p('_') >> ch_p(']'))                [&do_IDX]
                        | id                                                     [&do_ID3]  //a field
                    )
                    ;
            id
                =
                    as_lower_d
                    [
                        lexeme_d
                        [
                            (alpha_p >> * (alnum_p | '_'))
                            - (keywords >> anychar_p - (alnum_p | '_'))
                        ]
                    ]
                    ;
            stream_expression
                =
                    stream_term >>
                    * (
                        (ch_p('>') >> int_p[&do_timemove])
                        | (ch_p('+') >> stream_term)          [&do_sadd]
                        | (ch_p('-') >> rational)             [&do_streamsubstract]
                    )
                    ;
            stream_term
                =
                    stream_factor >>
                    * (
                        (ch_p('#') >> stream_factor)          [&do_shash]
                        | (ch_p('&') >> rational[&pushValR])  [&do_sdein]
                        | (ch_p('%') >> rational[&pushValR])  [&do_sdmin]
                        | (
                            ch_p('@')
                            >> ch_p('(') >> uint_p              [&pushValI]
                            >> ch_p(',') >> int_p               [&pushValI]
                            >> ch_p(')')
                        )                                       [&do_sagse]
                        | (ch_p('.') >> agregator)
                    )
                    ;
            agregator
                = (MIN           [&do_min]
                        | MAX       [&do_max]
                        | AVG       [&do_avg]
                        | SUM       [&do_sum]
                    )
                    ;
            stream_factor
                =
                    stream_id                                   [&do_Stream]
                    |   ch_p('(') >> stream_expression >> ch_p(')')
                    | (
                        IN_BEGIN                             [&do_Inner_Stream_Begin]
                        >> select_command
                        >> IN_END
                    )                                       [&do_Inner_Stream_End]
                    ;
            expression
                =
                    term
                    >> * (
                        ('+' >> term)               [&do_add]
                        | ('-' >> term)               [&do_subt]
                    )
                    ;
            term
                =   factor
                    >> * (
                        ('*' >> factor)             [&do_mult]
                        | ('/' >> factor)             [&do_div]
                    )
                    ;
            factor
                =   real_p                      [&pushVal]
                    | ('-' >> factor)             [&do_neg]
                    | ('+' >> factor)
                    |   funct
                    |   field_id
                    |   agregator                   [&do_aggregate]
                    |   '(' >> expression >> ')'
                    ;
            funct
                =
                    (
                        (SQRT
                            | CEIL
                            | ABS
                            | FLOOR
                            | SIGN
                            | CHR
                            | LENGTH
                            | TO_NUMBER
                            | TO_TIMESTAMP
                            | FLOAT
                            | INT
                            | COUNT
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
        }

        symbols<> keywords;
        rule<ScannerT>
        expression, term, factor, agregator,
                    select_command,
                    declare_command,
                    remarque_command,
                    empty_line,
                    id, stream_id, field_id,
                    stream_expression, stream_term, stream_factor, inner_stream,
                    natural, schema, type_id,
                    funct,
                    rational,
                    condition, logical_term, logical_factor,
                    table_scan,
                    command
                    ;

        rule<ScannerT> const &
        start() const {
            return command;
        }
    };
};

string storeParseResult(string sOutputFile) {
    // Store of compiled queries on disk
    const qTree coreInstance2(coreInstance_parser) ;
    std::ofstream ofs(sOutputFile.c_str());

    if (! ofs.good()) {
        cerr << "No serialization file:" << sOutputFile << endl ;
        throw std::invalid_argument("Runtime Error");
    }

    boost::archive::text_oarchive oa(ofs);

    if (! ofs.good()) {
        cerr << "File chain serialization fail" << endl ;
        throw std::invalid_argument("Runtime Error");
    }

    oa << coreInstance2 ;
    return string("OK");
}

string parser(vector<string> vsInputFile) {
    //
    // Main parser body
    //
    ql_parser g;
    stk.push(std::shared_ptr<query> (new query()));
    string str;

    for (string line : vsInputFile) {
        do_reset();
        stripUnicode(line);

        if (! parse(line.c_str(), g, space_p).full) {
            cerr << "error:\t" << line << endl ;
            throw std::invalid_argument("Syntax Error");
        }
    }

    assert(! stk.empty());
    stk.pop();
    return string("OK");
}
