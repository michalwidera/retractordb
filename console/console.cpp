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

/*
 * Zalozenia projektowe programu konsoli uzytkownika
 * Konsola:
 * 1. jest najprostszym interfejsem uzytkownika dla systemu zarzadzania danymi
 * 2. udostepnima dane na biezaco, zgodnie ze zmianami danych w bazie
 * 3. dane raz wyswietlone ulegaja pozniejszym zmianom (biezacej aktualizacji)
 * 4. komunikuje sie z baza uzgodnionym protokolem sieciowym
 * 5. jest przenosna pomiedzy systemami (Lin/Win)
 * 6. dziala w srodowisku tekstowym
 * TODO: Wez sie w garsc to posprzataj ten bajzel tutaj
 */

#include "stdafx.h"

#include <ctime>

#include <cstdio>

#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/thread.hpp>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <istream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

#include "continiousterminal.h"
#include "mxterm.h"

using namespace std ;
using namespace boost ;

using boost::asio::ip::tcp ;
using boost::property_tree::ptree;

//
// Zestawienie polaczenia
//
namespace dbNet {
  string sHostName ;

  boost::asio::io_service io_service;
  tcp::resolver resolver(io_service);
  tcp::socket socket(io_service);
}


ptree netClient( string netCommand , string netArgument )
{
  const int max_length = 1024;

  ptree pt_response ; //Zwracana wartosc/odpowiedz przez funkcje
  ptree pt_request;   //Tutaj konstruowane jest pytanie

  pt_request.put("db.query", "hello" ); //potem to wywalic

  try
  {
    boost::asio::io_service io_service;
    tcp::socket socket(io_service);
    boost::asio::connect(socket,
                         tcp::resolver(io_service).resolve(
                           tcp::resolver::query(
                             tcp::v4(), "localhost", "1971" )));

    pt_request.put("db.message", netCommand );

    if ( netArgument != "" )
      pt_request.put("db.argument", netArgument );

    //
    //request part
    //

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);

    write_xml( request_stream , pt_request );

    // Send the request.
    boost::asio::write(socket, request);

    //
    //response part
    //

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;

    // Check that response is OK.
    std::istream response_stream(&response);

    if (response_stream)
      {
        // Read until EOF, writing data to output as we go.
        boost::system::error_code error;
        while (boost::asio::read(socket, response,
                                 boost::asio::transfer_at_least(1), error))
          {
            if ( error == boost::asio::error::eof )
              {
                break ;
              }
          }

        if (error != boost::asio::error::eof)
          {
            throw boost::system::system_error(error);
          }

        read_xml( response_stream , pt_response );
      }
    else
      {
        pt_response.put("error.response" , "invalid" );
      }
  }
  catch (std::exception& e)
  {
    std::cerr << "exception catch:" << e.what() << std::endl ;
    pt_response.put("error.response" , e.what() );
  }

  return pt_response ;
}

int main(int argc, char* argv[])
{
  //Intro
  cerr << "Console - konsola systemu.\n";
  cerr << "Tag kompilatora:" + string(COMPILER) +
          ", CTS:" << string( __TIMESTAMP__) <<	//http://msdn2.microsoft.com/en-us/library/b0084kay(VS.80).aspx
          std::endl ;
  cerr << "Boost wersja:" << BOOST_LIB_VERSION << std::endl ;

  try {
    namespace po = boost::program_options;

    string sInputFile ;

    po::options_description desc("Dozwolone opcja");
    desc.add_options()
        ("infile,i", po::value<string>(&sInputFile) , "z poleceniami")
        ("old,d", "old terminal stage")
        ("server,s", po::value<string>(&dbNet::sHostName)->default_value("localhost") , "nazwa serwera")
        ("help,h", "program options")
        ;

    po::positional_options_description p;       //Assume that infile is the first option
    p.add("infile", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
        cerr << desc << endl ;
        return 1;
      }

    // To dziala tylko z Pliku

    if (vm.count("infile"))
      {
        cout << "Load file:" << sInputFile << endl ;
        std::ifstream s( sInputFile.c_str() );
        if(s)
          {
            boost::iostreams::stream<boost::iostreams::file_source>
                file( sInputFile.c_str() );
            std::string rline;
            while (std::getline(file, rline))
              {
                char line[81]; //linia edytowana
                strcpy( line , rline.c_str() );
                cout << ":" << line ;
                ContiniousTerminal termClassInstance ;
                if ( termClassInstance.issueCommand(line) )
                  {
                    cout << ",OK" << endl ;
                  }
                else
                  {
                    cout << ",?" << endl ;
                  }
              }
          }
        else
          {
            cerr << "no input file." << endl ;
            return 1 ;
          }
        return 0;
      }

    //Tutaj jest wlasciwe wywolanie terminala w trybie tekstowym

    if (vm.count("old"))
      {
        ContiniousTerminal instance(false);
        return instance.postMortemRetCode ;
      }
    else
      {
        MxTerm instance(false);
        return instance.postMortemRetCode ;
      }

  }
  catch(std::exception& e)
  {
    cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
