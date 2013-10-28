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

#include <iostream>
#include <ctime>
#include <string>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic.hpp>
#include <queue>

using namespace boost ;
using namespace std;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

std::queue<int> qq;
boost::lockfree::queue<int> qqw(128);

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class tcp_connection
  : public boost::enable_shared_from_this<tcp_connection>
{
public:
  typedef boost::shared_ptr<tcp_connection> pointer;

  static pointer create(boost::asio::io_service& io_service)
  {
    return pointer(new tcp_connection(io_service));
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    message_ = make_daytime_string();

    boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&tcp_connection::handle_write, shared_from_this()));
  }

private:
  tcp_connection(boost::asio::io_service& io_service)
    : socket_(io_service)
  {
  }

  void handle_write()
  {
  }

  tcp::socket socket_;
  std::string message_;
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_service& io_service)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), 1974 ))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    tcp_connection::pointer new_connection =
      tcp_connection::create(acceptor_.get_io_service());

    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&tcp_server::handle_accept, this, new_connection,
          boost::asio::placeholders::error));
  }

  void handle_accept(tcp_connection::pointer new_connection,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_connection->start();
    }

    start_accept();
  }

  tcp::acceptor acceptor_;
};

class udp_server
{
public:
  udp_server(boost::asio::io_service& io_service)
    : socket_(io_service, udp::endpoint(udp::v4(), 1973 ))
  {
    start_receive();
  }

private:
  void start_receive()
  {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        boost::bind(&udp_server::handle_receive, this,
          boost::asio::placeholders::error));
  }

  void handle_receive(const boost::system::error_code& error)
  {
    if (!error || error == boost::asio::error::message_size)
    {
      std::string data(recv_buffer_.begin(), recv_buffer_.end());

      cout << data << endl ;

      boost::shared_ptr<std::string> message(
          new std::string(make_daytime_string()));

      socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
          boost::bind(&udp_server::handle_send, this, message));

      start_receive();
    }
  }

  void handle_send(boost::shared_ptr<std::string> message)
  {
     //cout << * message.get() << endl ;
  }

  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  boost::array<char, 1> recv_buffer_;
};


class udp_soserver
{
public:
  udp_soserver(boost::asio::io_service& io_service)
    : socket_(io_service, udp::endpoint(udp::v4(), 1975 ))
  {
    start_send();
  }

private:
  void start_send()
  {
      cout << "start_send" << endl ;

      int value ;

      cout << "ret " << qq.front() << endl ;
      qq.pop();
      cout << "ret " << qq.front() << endl ;
      qq.pop();

      while (qqw.pop(value));
      cout << "rez " << value << endl ;

      //boost::this_thread::sleep(boost::posix_time::milliseconds( 1 ));
      int vaule ;
      socket_.async_send_to(
          boost::asio::buffer( &value , sizeof(int) ) , remote_endpoint_,
          boost::bind(
                      &udp_soserver::handle_send, this,
                      boost::asio::placeholders::error , boost::asio::placeholders::bytes_transferred ));
  }

  void handle_send(
          const boost::system::error_code& error,
          std::size_t bytes_transferred )
  {
      if (!error || error == boost::asio::error::message_size)
      {
          cout << "Send OK" << endl ;
          start_send();
      }
      else
      {
 //         cout << "Send FAIL" << endl ;
      }

  }

  udp::socket socket_;
  udp::endpoint remote_endpoint_;
};


//http://thisthread.blogspot.com/2011/06/running-ioservice-in-another-thread.html
int main(int argc, char* argv[])
{

    asio::io_service ios;

    for ( int i = 0 ; i < 100 ; i ++)
        qq.push( i + 1 );
    qq.push( 3 );

    cout << "boost::lockfree::queue is ";
    if (!qqw.is_lock_free())
        cout << "not ";
    cout << "lockfree" << endl;


    for (int i = 0; i != 100; ++i) {
        int value = i ;
        while (!qqw.push(value))
            ;
    }

    tcp_server server1(ios);
    udp_server server2(ios);
    udp_soserver server3(ios);


    thread bt(boost::bind(&asio::io_service::run, &ios));  //puszczenie serwisu w watku

    cout << "Press enter to send sth." << endl;
    string request;
    getline(cin, request);


    cout << "Press enter to terminate whenever you want!!" << endl;
    getline(cin, request);

    ios.stop();

    cout << "main: done" << endl;

    return 0;
}
