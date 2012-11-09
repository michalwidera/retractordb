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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <set>
#include <exception>
#include <iostream>
#include <conio.h>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

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

int main_netthread( int *arg )
{

    // Create empty property tree object
    using boost::property_tree::ptree;
    ptree pt;

    // Put log filename in property tree
    pt.put("debug.filename", "id" );

    // Put debug level in property tree
    pt.put("debug.level", 10 );

    // Put debug level in property tree
    pt.put("debug.level", 'd' );

  try
  {
    boost::asio::io_service io_service;

    tcp::endpoint endpoint(tcp::v4(), 1971);
    tcp::acceptor acceptor(io_service, endpoint);

    for (;;)
    {
    std::cout << "s2" << std::endl ;
      tcp::iostream stream;
      boost::system::error_code ec;
//      stream.expires_from_now( boost::posix_time::seconds(2));
      acceptor.accept(*stream.rdbuf(), ec);
    std::cout << "s3" << std::endl ;
      if (!ec)
      {
          write_xml(std::cout, pt );
          write_xml(stream, pt);
          boost::this_thread::yield();
      }
      std::cout << "1" ;
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}

int main_bpt_client()
{
    try
    {
        tcp::iostream s( "localhost", "1971");
        if (!s)
        {
            std::cout << "Unable to connect: " << s.error().message() << std::endl;
            return 1;
        }
        s.expires_from_now(boost::posix_time::seconds(60));

        std::string line;
        std::getline(s, line);
        std::cout << line << std::endl;
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
        return 1 ;
    }
    return 0;
}

int main_bpt()
{
    using namespace boost ;
    try
    {
        debug_settings ds;
#ifndef WIN32
        ds.load("regtest/debug_settings.xml");
#endif
        ds.save("debug_settings_out.xml");
        std::cout << "\nSuccess?\n";

        int arg ;
        thread bt(bind(&main_netthread, &arg));  //puszczenie serwisu w watku

        std::cout << "Any key..." << std::endl ;
        _getch(); //no wait ... feed key from kbhit
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
    return 0;
}
