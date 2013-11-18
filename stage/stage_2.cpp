#include <boost/chrono.hpp>
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <ctime>
#include <boost/bind.hpp>

#include "stdafx.h"
//http://stackoverflow.com/questions/15993180/boost-c-udp-example-can-not-transmit-over-internet
//
//
//
//

using boost::asio::ip::udp;

enum { max_length = 1024 };

int main_u1()
{
char request[max_length];
try
{

    boost::asio::io_service io_service;

    udp::socket s(io_service, udp::endpoint(udp::v4(), 0));

    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), "MyPublicIp", "3002");
    udp::resolver::iterator iterator = resolver.resolve(query);

    using namespace std; // For strlen.
    std::cout << "Write to me: ";
    while(1)
    {
        std::cin.getline(request, max_length);
        size_t request_length = strlen(request);
        s.send_to(boost::asio::buffer(request, request_length), *iterator);

        char reply[max_length];
        udp::endpoint sender_endpoint;
        size_t reply_length = s.receive_from(boost::asio::buffer(reply, max_length), sender_endpoint);
        std::cout << "Say:";
        std::cout.write(reply, reply_length);
        std::cout << "\n";
        std::cout << "Say What?" ;
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  std::cin.getline(request, max_length);
  return 0;
}

class server
{
public:
  server(boost::asio::io_service& io_service, short port)
    : io_service_(io_service),
      socket_(io_service, udp::endpoint( boost::asio::ip::address_v4::any(), port))
  {
      std::fill(data_, data_ + max_length, 0);
    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&server::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void handle_receive_from(const boost::system::error_code& error,
      size_t bytes_recvd)
  {
    if (!error && bytes_recvd > 0)
    {
      std::cout  << "Connection from: " << sender_endpoint_.address().to_string() << " " << sender_endpoint_.port() << std::endl;

      socket_.async_send_to(
          boost::asio::buffer(data_, strlen(data_)), sender_endpoint_,
          boost::bind(&server::handle_send_to, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      socket_.async_receive_from(
          boost::asio::buffer(data_, max_length), sender_endpoint_,
          boost::bind(&server::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
  }

  void handle_send_to(const boost::system::error_code& /*error*/,
      size_t /*bytes_sent*/)
  {

      std::fill(data_, data_ + max_length, 0);
    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&server::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));

  }

private:
  boost::asio::io_service& io_service_;
  udp::socket socket_;
  udp::endpoint sender_endpoint_;
  enum { max_length = 300 };
  char data_[max_length];
};

int main_s1()
{
  try
  {
    std::cout << "Starting server" << std::endl;

    boost::asio::io_service io_service;
    short port = 3002;
    using namespace std;
    server s(io_service, port);
    std::cout << "Server started. Listening on port:" <<port << std::endl;
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}

int main(int argc, char* argv[])
{
    //Intro
    std::cerr << "Stage - staging area of database algorithms.\n";
    std::cerr << "Boost wersja:" << BOOST_LIB_VERSION << std::endl ;

    try {
        namespace po = boost::program_options;

        std::string sInputFile ;

        po::options_description desc("Obslugiwane opcje programu");
        desc.add_options()
                ("u1,u" ,"udp client 1 test")
                ("s1,v" ,"udp server 1 test")
                ;

        po::positional_options_description p;       //Assume that infile is the first option
        p.add("infile", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cerr << desc << std::endl ;
            return 1;
        }
        else if ( vm.count("u1") )
            return main_u1();
        else if ( vm.count("s1") )
            return main_s1();
        else
            throw std::out_of_range("wybierz odpowiedni parametr/test");
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
