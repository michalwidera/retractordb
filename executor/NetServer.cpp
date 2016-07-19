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

#include "NetServer.h"

#include <string>
#include <set>
#include <exception>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <sstream>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using boost::asio::ip::tcp;
using namespace std ;

typedef boost::property_tree::ptree ptree ;

boost::function< ptree( ptree )> onCommandProcess ;

session::session(boost::asio::io_service& io_service)
    : socket_(io_service)
{
}

session::~session()
{
}

tcp::socket& session::socket()
{
    return socket_;
}

void session::start()
{
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
                            boost::bind(&session::handle_read, this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code& error,
                          size_t bytes_transferred)
{
    if (!error)
    {
        //
        // tutaj jest interpretowany protokol
        // czyli rozkladane sa zapytania/polecenia
        // w xml i wysylane sa odpowiedzi xml
        //

        // Create empty property tree object
        using boost::property_tree::ptree;
        ptree pt;

        {
            std::stringstream strstream;
            data_[ bytes_transferred ] = 0 ;
            strstream << data_ ;
            read_xml( strstream , pt );
        }

        //
        //onCommandProcess musi zawierac wskaznik do funkcji
        //tworzone jest to na konstruktrze
        //
        assert ( onCommandProcess != NULL );

        pt = onCommandProcess( pt );

        //      pt.put("db.stream.first", "dupa" );
        //      pt.put("db.stream.second", 60 );

        std::stringstream strstream;
        write_xml( strstream , pt );

        assert( strstream.str().size() > 0 );

        //
        // tutaj jest wysylana odpowiedz po interpretacji protokolu
        //

        boost::asio::async_write(socket_,
                                 boost::asio::buffer( strstream.str().c_str() , strstream.str().length() ),
                                 boost::bind(&session::handle_write, this,
                                             boost::asio::placeholders::error));
    }
    else
    {
        delete this;
    }
}

void session::handle_write(const boost::system::error_code& error)
{
    if (!error)
    {
        socket_.close();
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                boost::bind(&session::handle_read, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        delete this;
    }
}

//
// Server TCP - Klasa odpowiedzialna za komunikacje
// pytanie-odpowiedz
//

tcp_server::tcp_server(boost::asio::io_service& io_service,
                       boost::function< ptree( ptree )> onCommandProcessFn )
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), 1971))
{
    onCommandProcess = onCommandProcessFn ;
    start_accept();
}


void tcp_server::start_accept()
{
    session* new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(),
                           boost::bind(&tcp_server::handle_accept, this, new_session,
                                       boost::asio::placeholders::error));
}

void tcp_server::handle_accept(session* new_session,
                               const boost::system::error_code& error)
{
    if (!error)
    {        
        new_session->start();
    }
    else
    {
        delete new_session;
    }

    start_accept();
}


//
//  Server UDP - Czesc implementacyjna.
//  Klasa odpowiedzialna na transmisje strumieni danych
//

/*
const short multicast_port = 30001;
const int max_message_count = 100;

udp_server::udp_server(boost::asio::io_service& io_service,
                       const boost::asio::ip::address& multicast_address)
  : endpoint_(multicast_address, multicast_port),
    socket_(io_service, endpoint_.protocol()),
    timer_(io_service),
    message_count_(0)
{
//    *** ASYNC_SEND_TO ***
  socket_.async_send_to(
        boost::asio::buffer(handle_message()), endpoint_,
        boost::bind(&udp_server::handle_send_to,
                    this,
                    boost::asio::placeholders::error));
}

void  udp_server::handle_send_to(const boost::system::error_code& error)
{
  if (!error && message_count_ < max_message_count)
    {
      timer_.expires_from_now(boost::posix_time::milliseconds(500));
      timer_.async_wait(
            boost::bind(&udp_server::handle_timeout,
                        this,
                        boost::asio::placeholders::error));
    }
}

std::string udp_server::handle_message()
{
    using boost::property_tree::ptree;
    ptree pt;

    pt.put("stream", message_count_++ % 4 );
    pt.put("field_a", "A" );
    pt.put("field_b", "B" );

    std::stringstream strstream;
    write_xml( strstream , pt );

    return strstream.str();
}

void udp_server::handle_timeout(const boost::system::error_code& error)
{
  if (!error)
    { //  *** ASYNC_SEND_TO ***
      socket_.async_send_to(
            boost::asio::buffer(handle_message()), endpoint_,
            boost::bind(&udp_server::handle_send_to,
                        this,
                        boost::asio::placeholders::error));
    }
}

*/
