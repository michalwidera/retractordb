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

//Experimental area

#include "QStruct.h"

//#include <boost/spirit.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <stack>

using namespace std ;
using namespace boost ;

using namespace QS ;

#define LOG(value) { std::cerr << "LOG:" << value << std::endl ; }

qTree coreInstance_parser_v2 ;
config configInstance_parser_v2 ;

stack < boost::shared_ptr<query> > stk_v2 ;

//http://www.boost.org/doc/libs/1_47_0/libs/spirit/example/qi/
//http://www.oreillynet.com/network/2003/05/06/examples/calculatorexample.html


namespace client
{
namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;
namespace ascii = boost::spirit::ascii;


struct do_help {
  template<typename Iter>
  void operator()(Iter beg, Iter end) const
  {
    std::cout << string( beg,end ) << std::endl;
    std::cout << "help - to be implemented\n";
  }
};


template <typename Iterator>
struct statemenet : qi::grammar<Iterator, ascii::space_type>
{
    statemenet() : statemenet::base_type(command)
    {
        using namespace qi::labels;
        using qi::on_error;
        using qi::fail;
        using qi::char_;
        using qi::uint_;
        using qi::double_;
        using qi::string;
        using qi::lexeme;
        using qi::raw;

        using ascii::alnum;
        using ascii::alpha;

        using phoenix::ref;
        using phoenix::val;
        using phoenix::construct;
        using phoenix::push_back;
        using phoenix::begin;
        using phoenix::end;

        command
                =
                remarque_command
                //| select_command
                | declare_command
                //| transaction_command
                ;

        remarque_command
                =
                (
                    string("#")
                    >> * char_
                )//[do_help()]
                ;

        rational
                =
                ( uint_ >> char_('/') >> uint_ )
                |
                double_
                ;

        identifier
                =
                raw[lexeme[alpha >> *(alnum | '_')]]
                ;

        declare_command
                =
                (
                    string("DECLARE")
                    >>
                    (
                        (
                            (
                                identifier
                                >> !
                                (
                                    identifier           //typ - INTEGER/CHAR/...
                                    //>> !
                                    //( ch_p('(') >> uint_p[&do_flen] >> ch_p(')') ) //(dlugosc)
                                )
                            )
                         ) % char_(',')
                    )
                    >>
                    (
                        string("STREAM")
                        >> identifier
                        >> char_(',')
                        >> rational
                    )
                )//[do_help()]
                ;

        on_error<fail>
                (
                    command
                    , std::cout
                    << val("Error! Expecting ")
                    << _4                               // what failed?
                    << val(" here: \"")
                    << construct<std::string>(_3, _2)   // iterators to error-pos, end
                    << val("\"")
                    << std::endl
                    );
    }

    qi::rule<Iterator, ascii::space_type>
    command,
    remarque_command,
    rational,
    declare_command,
    identifier;
};

}

void removeWhite (std::string &str)
{
    std::string temp;
    for (unsigned int i = 0; i < str.length(); i++)
        if ( str[i] == '\t' ) str[i] = ' ';
    for (unsigned int i = 0; i < str.length(); i++)
        if ( str[i] == ' ' && str[i-1] == ' ' ) {}
        else temp += str[i];
    str = temp;
}

void do_reset()
{
    //lProgram.clear();
    //lSchema.clear();
    //sFieldName = "" ;
    //fieldType = field::BAD ;
}


string parser_v2( string sPlikDanych, string sPlikWynikowy )
{
    LOG( "parser_v2" );

    std::vector<int> code;              //  Our VM code

    client::statemenet<std::string::const_iterator> calc;            // Our grammar

    stk_v2.push( boost::shared_ptr<query>( new query() ) );

    if ( sPlikDanych == "regtest" )
    {
        string str[] = { "# transaction 0.5" ,
           "DECLARE a integer STREAM core0 , 1" ,
           "# select core0[1,1],core0 stream str1 from core0" } ;

        for ( int i = 0 ; i < 3 ; i++ )
        {
            do_reset();
            std::string::const_iterator iter = str[i].begin();
            std::string::const_iterator end = str[i].end();

            LOG( "phrase_parse" );

            if ( !phrase_parse(iter, end, calc, boost::spirit::ascii::space ) )
            {
                LOG( str[i].c_str() );
                LOG( "error" );
                throw std::invalid_argument("Syntax Error.");
            }
            else
            {
                LOG( str[i] );
                LOG( "sucess" );
            }
        }
    }
    else
    {
        ifstream input( sPlikDanych.c_str() );

        string str;
        while ( getline( input, str ) )
        {
            do_reset();
            /*
            if ( ! parse( str.c_str() , g , space_p ).full )
            {
                cerr << str.c_str() << endl ;
                throw std::invalid_argument("Syntax Error.");
            }
            */
        }
    }

    assert( ! stk_v2.empty() );
    stk_v2.pop();


    //zapisanie skompilowanych zapytan na dysku
    {
        const config configInstance2( configInstance_parser_v2 ) ;
        const qTree coreInstance2( coreInstance_parser_v2 ) ;

        std::ofstream ofs( sPlikWynikowy.c_str() );
        boost::archive::text_oarchive oa(ofs);

        oa << configInstance2 ;
        oa << coreInstance2 ;

    }
    return string("OK");
}
