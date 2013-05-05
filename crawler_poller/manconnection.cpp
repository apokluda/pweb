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
, retrytimer_( io_service )
, strand_( io_service )
, mhostname_( mhostname )
, mport_( mport )
, socket_( new tcp::socket( io_service ) )
, bufwrite_( new bufwrite( io_service, socket_ ) )
, connected_( false )
{
    connect();
}

void manconnection::connect( bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.infoStream() << "Connecting to Crawler Manager at '" << mhostname_ << "' port " << mport_;

        tcp::resolver::query q(mhostname_, mport_);
        resolver_.async_resolve(q,
                boost::bind( &manconnection::handle_resolve, get_this(), ph::error, ph::iterator ) );
    }
    else
    {
        log4.errorStream() << "An error occurred while retry timer was running for connection to Crawler Manager";

        // We don't know why the timer was interrupted, but simply restarting it is probably
        // not a good idea--we may get into an infinite loop? Let's shut down the process
        // and the daemon utility will start us up again if necessary.
        socket_->get_io_service().stop();
    }
}

void manconnection::handle_resolve(bs::error_code const& ec, tcp::resolver::iterator iter)
{
    if ( !ec )
    {
        async_connect(*socket_, iter,
                boost::bind( &manconnection::handle_connect, get_this(), ph::error ) );
    }
    else
    {
        log4.errorStream() << "Unable to resolve Crawler Manager hostname";
        reconnect();
    }
}

void manconnection::handle_connect(bs::error_code const& ec)
{
    if ( !ec )
    {
        log4.infoStream() << "Connected to Crawler Manager process";

        errwait_ = boost::posix_time::seconds(0); // reset connection error timer
        connected_ = true;
        signals::manager_connected( get_this() );

        async_read(*socket_, recv_header_.buffer(),
                boost::bind( &manconnection::handle_header_read, get_this(), ph::error, ph::bytes_transferred ) );
    }
    else
    {
        log4.errorStream() << "An error occurred while connecting to Crawler Manager: " << ec.message();
        reconnect();
    }
}

void manconnection::handle_header_read( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        if ( recv_header_.length() > sizeof recv_buf_ )
        {
            log4.errorStream() << "Message from Crawler Manager is too big for buffer!";
            reconnect();
        }
        else if ( recv_header_.type() == crawler_protocol::HOME_AGENT_ASSIGNMENT )
        {
            async_read( *socket_, buffer( recv_buf_, recv_header_.length() ),
                    boost::bind( &manconnection::handle_home_agent_assignment, get_this(), ph::error, ph::bytes_transferred ) );
        }
        else
        {
            log4.errorStream() << "Crawler Manager sent unrecognized message type: " << recv_header_.type();
            reconnect();
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while reading message header: " << ec.message();
        reconnect();
    }
}

void manconnection::handle_home_agent_assignment( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        log4.debugStream() << "Received a HOME_AGENT_ASSIGNMENT message from Crawler Manager";

        std::string hahostname;
        protocol_helper::parse_string( hahostname, bytes_transferred, recv_buf_.begin(), recv_buf_.end() );
        signals::home_agent_assigned( hahostname );
    }
    else
    {
        log4.errorStream() << "An error occurred while reading a HOME_AGENT_ASSIGNMENT message from the Crawler Manager: " << ec.message();
        reconnect();
    }
}

void manconnection::disconnect()
{
     connected_ = false;

     bs::error_code shutdown_ec;
     socket_->shutdown(ip::tcp::socket::shutdown_both, shutdown_ec);
     if ( !shutdown_ec ) log4.errorStream() << "An error occurred while shutting down connection to Crawler Manager: " << shutdown_ec.message();

     bs::error_code close_ec;
     socket_->close(close_ec);
     if ( !close_ec ) log4.errorStream() << "An error occurred while closing connection to Crawler Manager: " << close_ec.message();

     signals::manager_disconnected( get_this() );
}

void manconnection::reconnect()
{
    disconnect();

    using boost::posix_time::seconds;

    if (errwait_ < seconds(10)) errwait_ += seconds(2);

    log4.noticeStream() << "Retrying connection to Crawler Manager in " << errwait_;

    retrytimer_.expires_from_now( errwait_ );
    retrytimer_.async_wait(boost::bind( &manconnection::connect, get_this(), ph::error ));
}
