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
        //

        // Create empty property tree object
        using boost::property_tree::ptree;
        ptree pt;

        {
            std::stringstream strstream;
            strstream << data_ ;
            read_xml( strstream , pt );
        }

        if ( !onCommandProcess )  //safe
            throw std::out_of_range("cos nie tak - wolanie pustego wskaznika");

        pt = onCommandProcess( pt );

        //      pt.put("db.stream.first", "dupa" );
        //      pt.put("db.stream.second", 60 );

        std::stringstream strstream;
        write_xml( strstream , pt );

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


udp_server::udp_server(boost::asio::io_service& io_service)
    : io_service_(io_service),
      socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 1972))
  {
    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&udp_server::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

void udp_server::handle_receive_from(const boost::system::error_code& error,
      size_t bytes_recvd)
  {
    if (!error && bytes_recvd > 0)
    {
      socket_.async_send_to(
          boost::asio::buffer(data_, bytes_recvd), sender_endpoint_,
          boost::bind(&udp_server::handle_send_to, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      socket_.async_receive_from(
          boost::asio::buffer(data_, max_length), sender_endpoint_,
          boost::bind(&udp_server::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
  }

void udp_server::handle_send_to(const boost::system::error_code& /*error*/,
    size_t /*bytes_sent*/)
{
  socket_.async_receive_from(
      boost::asio::buffer(data_, max_length), sender_endpoint_,
      boost::bind(&udp_server::handle_receive_from, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}
