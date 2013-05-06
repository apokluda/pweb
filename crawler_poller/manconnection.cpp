/*
 * manconnection.cpp
 *
 *  Created on: 2013-05-02
 *      Author: apokluda
 */

#include "manconnection.hpp"
#include "messages.hpp"
#include "signals.hpp"
#include "bufreadwrite.hpp"
#include "protocol_helper.hpp"

using std::string;

namespace bs = boost::system;
namespace ph = boost::asio::placeholders;

using namespace boost::asio;
using namespace boost::asio::ip;

extern log4cpp::Category& log4;

manconnection::manconnection(io_service& io_service, string const& mhostname, string const& mport)
: resolver_( io_service )
, mhostname_( mhostname )
, mport_( mport )
, socket_( io_service )
, bufread_( &socket_ )
, bufwrite_( &socket_ )
, connected_( false )
{
    // Read callbacks are implicitly synchronized
    bufread_.add_handler( crawler_protocol::HOME_AGENT_ASSIGNMENT, boost::ref( signals::home_agent_assigned ) );
    bufread_.error_handler( boost::bind( &manconnection::disconnect, this ) );

    // Write callbacks are implicitly synchronized
    bufwrite_.success_handler( boost::bind( &manconnection::send_success, this, _1, _2 ) );
    bufwrite_.error_handler( boost::bind( &manconnection::send_failure, this, _1, _2 ) );

    connect();
}

void manconnection::connect( bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.infoStream() << "Connecting to Crawler Manager at '" << mhostname_ << "' port " << mport_;

        tcp::resolver::query q(mhostname_, mport_);
        resolver_.async_resolve(q,
                boost::bind( &manconnection::handle_resolve, this, ph::error, ph::iterator ) );
    }
    else
    {
        log4.errorStream() << "An error occurred while retry timer was running for connection to Crawler Manager";
        disconnect();
    }
}

void manconnection::handle_resolve(bs::error_code const& ec, tcp::resolver::iterator iter)
{
    if ( !ec )
    {
        async_connect(socket_, iter,
                boost::bind( &manconnection::handle_connect, this, ph::error ) );
    }
    else
    {
        log4.errorStream() << "Unable to resolve Crawler Manager hostname";
        disconnect();
    }
}

void manconnection::handle_connect(bs::error_code const& ec)
{
    if ( !ec )
    {
        log4.infoStream() << "Connected to Crawler Manager process";
        connected_.store(true, boost::memory_order_relaxed);
        signals::manager_connected( this );

        bufread_.start();
    }
    else
    {
        log4.errorStream() << "An error occurred while connecting to Crawler Manager: " << ec.message();
        disconnect();
    }
}

void manconnection::disconnect()
{
    // It is possible that this method could be called twice simultaneously if
    // a read and write operation fail at the exact same instant

    if ( connected_.exchange(false, boost::memory_order_acquire) )
    {
        signals::manager_disconnected( this );

        bs::error_code shutdown_ec;
        socket_.shutdown(ip::tcp::socket::shutdown_both, shutdown_ec);
        if ( !shutdown_ec ) log4.errorStream() << "An error occurred while shutting down connection to Crawler Manager: " << shutdown_ec.message();

        bs::error_code close_ec;
        socket_.close(close_ec);
        if ( !close_ec ) log4.errorStream() << "An error occurred while closing connection to Crawler Manager: " << close_ec.message();
    }

    // This will case the whole process to shutdown. The daemon utility
    // will take care of starting us again. (It event implements
    // a backoff algorithm).
    socket_.get_io_service().stop();
}

void manconnection::send_success( crawler_protocol::message_type const type, std::string const& str )
{
    log4.infoStream() << "Successfully sent a " << type << " message with body '" << str << '\'';
}

void manconnection::send_failure( crawler_protocol::message_type const type, std::string const& str )
{
    log4.errorStream() << "Failed to send a " << type << " message with body '" << str << '\'';
    disconnect();
}
