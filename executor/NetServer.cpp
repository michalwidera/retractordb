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

std::string make_daytime_string()
{
    using namespace std; // For time_t, time and ctime;
    time_t now = time(0);
    return ctime(&now);
}


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



tcp_server::tcp_server(boost::asio::io_service& io_service)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), 13))
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

udp_server::udp_server(boost::asio::io_service& io_service)
    : socket_(io_service, udp::endpoint(udp::v4(), 13))
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


