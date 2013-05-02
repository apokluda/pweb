/*
 * pollerconnector.hpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
 */

#ifndef POLLERCONNECTOR_HPP_
#define POLLERCONNECTOR_HPP_

#include "stdhdr.hpp"
#include "messages.hpp"

class pollerconnection;
typedef boost::shared_ptr< pollerconnection > pollerconnection_ptr;

class pollerconnection : private boost::noncopyable, public boost::enable_shared_from_this< pollerconnection >
{
    typedef std::pair< crawler_protocol::header, std::vector< boost::uint8_t > > bufitem_t;
    typedef boost::shared_ptr< bufitem_t > bufitem_ptr;

public:
    pollerconnection( boost::asio::io_service& io_service )
    : socket_( io_service )
    , strand_( io_service )
    , send_in_progress_( false )
    {
    }

    void start();
    void assign_home_agent(std::string const& hostname);

private:


    void read_header();
    void handle_header_read( boost::system::error_code const& ec, std::size_t const bytes_transferred );
    void handle_home_agent_discovered( boost::system::error_code const& ec, std::size_t const bytes_transferred );
    void send_bufitem(bufitem_ptr& bufitem);
    void handle_bufitem_sent( bufitem_ptr&, boost::system::error_code const& ec, std::size_t const bytes_transferred );
    void disconnect();

    friend class pollerconnector;
    friend std::ostream& operator<<(std::ostream&, pollerconnection const&);

    boost::asio::ip::tcp::socket const& socket() const;
    boost::asio::ip::tcp::socket& socket();

    boost::asio::ip::tcp::socket socket_;
    boost::asio::strand strand_;
    std::queue< bufitem_ptr > send_queue_;
    crawler_protocol::header recv_header_;
    boost::array< boost::uint8_t, 256 > recv_buf_;
    bool send_in_progress_;
};

inline std::ostream& operator<<(std::ostream& out, pollerconnection const& conn)
{
    boost::system::error_code ec;
    out << conn.socket().remote_endpoint(ec);
    return out;
}

class pollerconnector :
        private boost::noncopyable
{
public:
    pollerconnector(boost::asio::io_service& io_service, std::string const& interface, boost::uint16_t const port);

    void start();

private:
    void handle_accept(boost::system::error_code const& ec);

    typedef boost::shared_ptr< pollerconnection > recv_connection_ptr;

    recv_connection_ptr new_connection_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

#endif /* POLLERCONNECTOR_HPP_ */
