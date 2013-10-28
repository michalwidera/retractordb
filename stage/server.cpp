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

//
// netserver.h
//

#include <iostream>
#include <ctime>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>

using namespace boost ;
using namespace std;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;



class tcp_connection
  : public boost::enable_shared_from_this<tcp_connection>
{
public:
  typedef boost::shared_ptr<tcp_connection> pointer;
  static pointer create(boost::asio::io_service& io_service){
      return pointer(new tcp_connection(io_service));
  }

  tcp::socket& socket();
  void start();
private:
  tcp_connection(boost::asio::io_service& io_service) ;
  void handle_write() ;
  tcp::socket socket_;
  std::string message_;
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_service& io_service) ;
private:
  void start_accept();
  void handle_accept(tcp_connection::pointer new_connection,
      const boost::system::error_code& error);
  tcp::acceptor acceptor_;
};

class udp_server
{
public:
  udp_server(boost::asio::io_service& io_service);
private:
  void start_receive();
  void handle_receive(const boost::system::error_code& error);
  void handle_send(boost::shared_ptr<std::string> /*message*/) ;
  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  boost::array<char, 1> recv_buffer_;
};

class udp_soserver
{
public:
  udp_soserver(boost::asio::io_service& io_service);
private:
  void start_receive();
  void handle_receive(const boost::system::error_code& error);
  void handle_send(boost::shared_ptr<std::string> /*message*/) ;
  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  boost::array<char, 1> recv_buffer_;
};

//
// netserver.cpp
//

std::string make_daytime_string()
{
    using namespace std; // For time_t, time and ctime;
    time_t now = time(0);
    return ctime(&now);
}

//
// ------ tcp_connection
//

tcp::socket& tcp_connection::socket()
{
    return socket_;
}

void tcp_connection::start()
{
    message_ = make_daytime_string();

    boost::asio::async_write(socket_, boost::asio::buffer(message_),
                             boost::bind(&tcp_connection::handle_write, shared_from_this()));
}

tcp_connection::tcp_connection(boost::asio::io_service& io_service)
    : socket_(io_service)
{
}

void tcp_connection::handle_write()
{
}


//
//   --------- tcp_server
//

tcp_server::tcp_server(boost::asio::io_service& io_service)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), 1971 ))
{
    start_accept();
}


void tcp_server::start_accept()
{
    tcp_connection::pointer new_connection =
            tcp_connection::create(acceptor_.get_io_service());

    acceptor_.async_accept(new_connection->socket(),
                           boost::bind(&tcp_server::handle_accept, this, new_connection,
                                       boost::asio::placeholders::error));
}

void tcp_server::handle_accept(tcp_connection::pointer new_connection,
                               const boost::system::error_code& error)
{
    if (!error)
    {
        new_connection->start();
    }

    start_accept();
}


//
//    ------------- udp_server
//

udp_server::udp_server(boost::asio::io_service& io_service)
    : socket_(io_service, udp::endpoint(udp::v4(), 1971 ))
{
    start_receive();
}

void udp_server::start_receive()
{
    socket_.async_receive_from(
                boost::asio::buffer(recv_buffer_), remote_endpoint_,
                boost::bind(&udp_server::handle_receive, this,
                            boost::asio::placeholders::error));
}

void udp_server::handle_receive(const boost::system::error_code& error)
{
    if (!error || error == boost::asio::error::message_size)
    {
        boost::shared_ptr<std::string> message(
                    new std::string(make_daytime_string()));

        socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
                              boost::bind(&udp_server::handle_send, this, message));

        start_receive();
    }
}

void udp_server::handle_send(boost::shared_ptr<std::string> /*message*/)
{
}


//
//    ------------- udp_soserver
//

udp_soserver::udp_soserver(boost::asio::io_service& io_service)
    : socket_(io_service, udp::endpoint(udp::v4(), 1971 ))
{
    start_receive();
}

void udp_soserver::start_receive()
{
    socket_.async_receive_from(
                boost::asio::buffer(recv_buffer_), remote_endpoint_,
                boost::bind(&udp_soserver::handle_receive, this,
                            boost::asio::placeholders::error));
}

void udp_soserver::handle_receive(const boost::system::error_code& error)
{
    if (!error || error == boost::asio::error::message_size)
    {
        boost::shared_ptr<std::string> message(
                    new std::string(make_daytime_string()));

        socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
                              boost::bind(&udp_soserver::handle_send, this, message));

        start_receive();
    }
}

void udp_soserver::handle_send(boost::shared_ptr<std::string> /*message*/)
{
}

//
//
//
