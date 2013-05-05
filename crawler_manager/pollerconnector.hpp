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
#include "bufreadwrite.hpp"

class pollerconnection;
typedef boost::shared_ptr< pollerconnection > pollerconnection_ptr;

class pollerconnection : private boost::noncopyable, public boost::enable_shared_from_this< pollerconnection >
{
    typedef boost::shared_ptr< boost::asio::ip::tcp::socket > socket_ptr;
    typedef boost::shared_ptr< bufwrite > bufwrite_ptr;
    typedef std::pair< crawler_protocol::header, std::vector< boost::uint8_t > > bufitem_t;
    typedef boost::shared_ptr< bufitem_t > bufitem_ptr;

public:
    pollerconnection( boost::asio::io_service& io_service )
    : socket_( new boost::asio::ip::tcp::socket( io_service ) )
    , bufwrite_( new bufwrite( io_service, socket_ ) )
    {
    }

    void start();
    void assign_home_agent(std::string const& hostname)
    {
        bufwrite_->sendmsg( crawler_protocol::HOME_AGENT_ASSIGNMENT, hostname );
    }

private:

    void read_header();
    void handle_header_read( boost::system::error_code const& ec, std::size_t const bytes_transferred );
    void handle_home_agent_discovered( boost::system::error_code const& ec, std::size_t const bytes_transferred );
    void disconnect();

    friend class pollerconnector;
    friend std::ostream& operator<<(std::ostream&, pollerconnection const&);

    boost::asio::ip::tcp::socket const& socket() const;
    boost::asio::ip::tcp::socket& socket();

    socket_ptr socket_;
    bufwrite_ptr bufwrite_;
    crawler_protocol::header recv_header_;
    boost::array< boost::uint8_t, 256 > recv_buf_;
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
