#ifndef NETSERVER_H
#define NETSERVER_H

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

class session
{
public:
  session(boost::asio::io_service& io_service) ;

  boost::asio::ip::tcp::socket& socket();
  void start();

private:
  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred);

  void handle_write(const boost::system::error_code& error);

  boost::asio::ip::tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class tcp_server {

public:
  tcp_server(boost::asio::io_service& io_service);

private:
  void start_accept();
  void handle_accept(session* new_session,
      const boost::system::error_code& error);

  boost::asio::io_service& io_service_;
  boost::asio::ip::tcp::acceptor acceptor_;
};

#endif // NETSERVER_H
