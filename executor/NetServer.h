#ifndef NETSERVER_H
#define NETSERVER_H

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/property_tree/ptree.hpp>

/**
 * Na podstawie:
 * http://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/example/echo/
 */

/**
  * Klasa odpowiedzialna za obsluge sesji - tworzona przez tcp_server
  * nie uzywana na zewnatrz - trzeba by chyba ja schowac
  */
class session
{

    enum { max_length = 1024 };     //
    char data_[max_length];         //To mi sie nie podoba - trzeba sprawdzic czy przypadkiem nie ma jakis auto-buforow w asio

    boost::asio::ip::tcp::socket socket_;

    void handle_read(const boost::system::error_code& error,
                     size_t bytes_transferred);

    void handle_write(const boost::system::error_code& error);

public:

    session(boost::asio::io_service& io_service) ;
    boost::asio::ip::tcp::socket& socket();
    void start();
};

/**
  * Klasa odpowiedzialna obsluge polaczenia
  * Server TCP
  */
class tcp_server {

    boost::asio::io_service& io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;

    void start_accept();
    void handle_accept(session* new_session,
                       const boost::system::error_code& error);

public:
    typedef boost::property_tree::ptree ptree ;

    /** Konstruktor
      @param io_service obiekt nadrzedny
      @param wskaznik na funkcje interpretujaca komendy sieciowe
      */
    tcp_server(boost::asio::io_service& io_service ,
               boost::function< ptree( ptree )> onCommandProcessFn );
};

/**
 *  Klasa odpowiedzialna za transmisje strumienia danych
 *  Server UDP
 */
class udp_server {

    boost::asio::io_service& io_service_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint sender_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];

public:

    udp_server(boost::asio::io_service& io_service);

    void handle_receive_from(const boost::system::error_code& error,
          size_t bytes_recvd);

    void handle_send_to(const boost::system::error_code& /*error*/,
        size_t /*bytes_sent*/);
};

#endif // NETSERVER_H
