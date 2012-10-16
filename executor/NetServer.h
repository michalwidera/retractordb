#ifndef NETSERVER_H
#define NETSERVER_H

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

#endif // NETSERVER_H
