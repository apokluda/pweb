/*
 * pollerconnector.cpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
 */

#ifndef POLLERCONNECTOR_CPP_
#define POLLERCONNECTOR_CPP_

#include "pollerconnector.hpp"
#include "protocol_helper.hpp"
#include "signals.hpp"

using std::string;

using namespace boost::asio;
namespace bs = boost::system;
namespace ph = boost::asio::placeholders;
namespace ptime = boost::posix_time;

extern log4cpp::Category& log4;

pollerconnection::pollerconnection( boost::asio::io_service& io_service )
: socket_( new boost::asio::ip::tcp::socket( io_service ) )
, bufread_( new bufread( socket_ ) )
, bufwrite_( new bufwrite( socket_ ) )
, connected_( true )
{
    bufread_->add_handler( crawler_protocol::HOME_AGENT_DISCOVERED, boost::ref( signals::home_agent_discovered ) );
    bufread_->error_handler( boost::bind( &pollerconnection::disconnect, shared_from_this() ) );

    bufwrite_->success_handler( boost::bind( &pollerconnection::send_success, shared_from_this(), _1, _2 ) );
    bufwrite_->error_handler( boost::bind( &pollerconnection::send_failure, shared_from_this(), _1, _2 ) );
}

void pollerconnection::start()
{
    log4.noticeStream() << "Received a new poller connection from " << *this;
    signals::poller_connected( shared_from_this() );
    bufread_->start();
}

void pollerconnection::disconnect()
{
    connected_ = false;
    socket_->shutdown( ip::tcp::socket::shutdown_both );
    socket_->close();
    signals::poller_disconnected( shared_from_this() );
}

void pollerconnection::send_success( crawler_protocol::message_type const type, std::string const& str )
{
    log4.debugStream() << "Successfully sent a " << type << " message to " << *this << " with body '" << str << '\'';
}

void pollerconnection::send_failure( crawler_protocol::message_type const type, std::string const& str )
{
    log4.errorStream() << "Error sending a " << type << " message to " << *this << " with body '" << str << '\'';
    disconnect();
    if ( type == crawler_protocol::HOME_AGENT_ASSIGNMENT ) signals::home_agent_discovered( str );
}

ip::tcp::socket const& pollerconnection::socket() const
{
    return *socket_;
}

ip::tcp::socket& pollerconnection::socket()
{
    return *socket_;
}

pollerconnector::pollerconnector(io_service& io_service, string const& interface, boost::uint16_t const port)
: acceptor_( io_service )
{
    try
    {
        ip::tcp::endpoint endpoint(ip::tcp::v6(), port);
        if ( !interface.empty() )
        {
            ip::address bind_addr(ip::address::from_string(interface));
            endpoint = ip::tcp::endpoint(bind_addr, port);
        }

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option( ip::tcp::acceptor::reuse_address( true ) );
        acceptor_.bind(endpoint);
        acceptor_.listen();

        log4.infoStream() << "Listening on " << (interface.empty() ? "all interfaces" : interface.c_str()) << " port " << port << " for TCP connections";
    }
    catch ( boost::system::system_error const& )
    {
        log4.fatalStream() << "Unable to bind to interface " << interface << " on port " << port;
        throw;
    }
}

void pollerconnector::start()
{
    new_connection_.reset( new pollerconnection( acceptor_.get_io_service() ) );
    acceptor_.async_accept( new_connection_->socket(),
            boost::bind( &pollerconnector::handle_accept, this, ph::error ) );
}

void pollerconnector::handle_accept(bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.infoStream() << "Received a new poller connection from " << new_connection_->socket().remote_endpoint();
        new_connection_->start();
        // Fall through
    }
    else
    {
        log4.errorStream() << "An error occurred while accepting a new poller connection: " << ec.message();
        // Fall through
    }
    start();
}

#endif /* POLLERCONNECTOR_CPP_ */
