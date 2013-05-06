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
    typedef boost::shared_ptr< bufwrite< socket_ptr > > bufwrite_ptr;
    typedef boost::shared_ptr< bufread< socket_ptr > > bufread_ptr;

public:
    pollerconnection( boost::asio::io_service& io_service );

    void start();

    void assign_home_agent(std::string const& hostname)
    {
        // If we're not connected, this object will eventually die and disappear.
        // (A disconnected signal will be emitted and the pollerconncetion will be removed from
        // the homeagentdb). Otherwise, we could get into an infinite loop.

        if ( connected_.load(boost::memory_order_acquire) ) bufwrite_->sendmsg( crawler_protocol::HOME_AGENT_ASSIGNMENT, hostname );
        else send_failure( crawler_protocol::HOME_AGENT_ASSIGNMENT, hostname );
    }

private:
    void disconnect();

    void send_success( crawler_protocol::message_type const, std::string const& );
    void send_failure( crawler_protocol::message_type const, std::string const& );

    friend class pollerconnector;
    friend std::ostream& operator<<(std::ostream&, pollerconnection const&);

    boost::asio::ip::tcp::socket const& socket() const;
    boost::asio::ip::tcp::socket& socket();

    socket_ptr socket_; // order of initialization matters, socket must come first!!
    bufread_ptr bufread_;
    bufwrite_ptr bufwrite_;
    boost::atomic< bool > connected_;
};

inline std::ostream& operator<<(std::ostream& out, pollerconnection const& conn)
{
    boost::system::error_code ec;
    out << conn.socket().remote_endpoint(ec);
    return out;
}

class pollerconnector: private boost::noncopyable
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
