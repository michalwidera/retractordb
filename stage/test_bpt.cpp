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

#include <string>
#include <set>
#include <exception>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using boost::asio::ip::tcp;
using namespace std ;

struct debug_settings
{
    std::string m_file;               // log filename
    int m_level;                      // debug level
    std::set<std::string> m_modules;  // modules where logging is enabled
    void load(const std::string &filename);
    void save(const std::string &filename);
};

void debug_settings::load(const std::string &filename)
{

    // Create empty property tree object
    using boost::property_tree::ptree;
    ptree pt;

    // Load XML file and put its contents in property tree.
    // No namespace qualification is needed, because of Koenig
    // lookup on the second argument. If reading fails, exception
    // is thrown.
    read_xml(filename, pt);

    // Get filename and store it in m_file variable. Note that
    // we specify a path to the value using notation where keys
    // are separated with dots (different separator may be used
    // if keys themselves contain dots). If debug.filename key is
    // not found, exception is thrown.
    m_file = pt.get<std::string>("debug.filename");

    // Get debug level and store it in m_level variable. This is
    // another version of get method: if debug.level key is not
    // found, it will return default value (specified by second
    // parameter) instead of throwing. Type of the value extracted
    // is determined by type of second parameter, so we can simply
    // write get(...) instead of get<int>(...).
    m_level = pt.get("debug.level", 0);

    // Iterate over debug.modules section and store all found
    // modules in m_modules set. get_child() function returns a
    // reference to child at specified path; if there is no such
    // child, it throws. Property tree iterator can be used in
    // the same way as standard container iterator. Category
    // is bidirectional_iterator.
    BOOST_FOREACH(ptree::value_type &v, pt.get_child("debug.modules"))
        m_modules.insert(v.second.data());

}

void debug_settings::save(const std::string &filename)
{

    // Create empty property tree object
    using boost::property_tree::ptree;
    ptree pt;

    // Put log filename in property tree
    pt.put("debug.filename", m_file);

    // Put debug level in property tree
    pt.put("debug.level", m_level);

    // Iterate over modules in set and put them in property
    // tree. Note that the add function places new key at the
    // end of list of keys. This is fine in most of the
    // situations. If you want to place item at some other
    // place (i.e. at front or somewhere in the middle),
    // this can be achieved using a combination of the insert
    // and put_value functions
    BOOST_FOREACH(const std::string &name, m_modules)
        pt.add("debug.modules.module", name);

    // Write property tree to XML file
    write_xml(filename, pt);

    // Write property tree to cout file
    write_xml( std::cout , pt);

}

string sHostName = "localhost" ;

boost::asio::io_service io_serviceInst;
tcp::resolver resolverInst(io_serviceInst);
tcp::socket socketInst(io_serviceInst);

const int max_length = 1024;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

int main_bpt_client()
{
    using boost::property_tree::ptree;
    ptree pt_request;
    pt_request.put("debug.querytest", "id" );

    ptree pt_response ;

    try
    {
        boost::asio::io_service io_service;
        tcp::socket socket(io_service);
        boost::asio::connect(socket,
                             tcp::resolver(io_service).resolve(
                                 tcp::resolver::query(
                                     tcp::v4(), "localhost", "1971" )));

        std::cout << "Enter message: ";
        char request_tbl[max_length];
        std::cin.getline(request_tbl, max_length);

        //
        //request part
        //

        // Form the request. We specify the "Connection: close" header so that the
        // server will close the socket after transmitting the response. This will
        // allow us to treat all data up until the EOF as the content.
        boost::asio::streambuf request;
        std::ostream request_stream(&request);

        if ( !strcmp( request_tbl , "xml") )
            write_xml( request_stream , pt_request );
        else
            request_stream << request_tbl ;

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
            // Write whatever content we already have to output.
            if (response.size() > 0)
                std::cout << &response;

            // Read until EOF, writing data to output as we go.
            boost::system::error_code error;
            while (boost::asio::read(socket, response,
                             boost::asio::transfer_at_least(1), error))
            {
                if ( error == boost::asio::error::eof )
                    break ;
                std::cout << &response;
            }

            if (error != boost::asio::error::eof)
                throw boost::system::system_error(error);
        }
        else
        {
            std::cout << "Invalid response" << std::endl ;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "exception catch:" << e.what() << std::endl;
    }

    return 0;
}

class session
{
public:
  session(boost::asio::io_service& io_service)
    : socket_(io_service)
  {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

private:
  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
      if (!error)
      {
          if ( !strcmp(data_,"dupa") )
          {
              strcpy( data_ , "b1ada");
              bytes_transferred = strlen("b1ada") ;
          }

          boost::asio::async_write(socket_,
             boost::asio::buffer(data_, bytes_transferred),
             boost::bind(&session::handle_write, this,
                boost::asio::placeholders::error));
      }
      else
      {
          delete this;
      }
  }

  void handle_write(const boost::system::error_code& error)
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

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server {

public:
  server(boost::asio::io_service& io_service)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), 1971))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    session* new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(session* new_session,
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

  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
};

int server_thread()
{
    boost::asio::io_service io_service;

    using namespace std; // For atoi.
    server s(io_service);

    io_service.run();
}

int main_bpt_server()
{
    using namespace boost ;
    using namespace std;
    try
    {
        debug_settings ds;
#ifndef WIN32
        ds.load("regtest/debug_settings.xml");
#endif
        ds.save("debug_settings_out.xml");
        cout << "\nSuccess?\n";

        thread bt(bind(&server_thread));  //puszczenie serwisu w watku

        cout << "Press enter to terminate whenever you want!" << endl;
        string request;
        getline(cin, request);
    }
    catch (std::exception &e)
    {
        cout << "Error: " << e.what() << "\n";
    }
    return 0;
}
