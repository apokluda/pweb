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
, mhostname_( mhostname )
, mport_( mport )
, errwait_( boost::posix_time::seconds( 0 ) )
, socket_( io_service )
, bufread_( &socket_ )
, bufwrite_( &socket_ )
, connected_( false )
{
    bufread_.add_handler( crawler_protocol::HOME_AGENT_ASSIGNMENT, boost::ref( signals::home_agent_assigned ) );
    bufread_.error_handler( boost::bind( &manconnection::disconnect, this ) );

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

        // We don't know why the timer was interrupted, but simply restarting it is probably
        // not a good idea--we may get into an infinite loop? Let's shut down the process
        // and the daemon utility will start us up again if necessary.
        socket_.get_io_service().stop();
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
        reconnect();
    }
}

void manconnection::handle_connect(bs::error_code const& ec)
{
    if ( !ec )
    {
        log4.infoStream() << "Connected to Crawler Manager process";
        errwait_ = boost::posix_time::seconds( 0 );
        connected_ = true;
        signals::manager_connected( this );

        bufread_.start();
    }
    else
    {
        log4.errorStream() << "An error occurred while connecting to Crawler Manager: " << ec.message();
        reconnect();
    }
}

void manconnection::disconnect()
{
    if ( connected_ )
    {
         connected_ = false;
         signals::manager_disconnected( this );

         bs::error_code shutdown_ec;
         socket_.shutdown(ip::tcp::socket::shutdown_both, shutdown_ec);
         if ( !shutdown_ec ) log4.errorStream() << "An error occurred while shutting down connection to Crawler Manager: " << shutdown_ec.message();

         bs::error_code close_ec;
         socket_.close(close_ec);
         if ( !close_ec ) log4.errorStream() << "An error occurred while closing connection to Crawler Manager: " << close_ec.message();
    }
}

void manconnection::reconnect()
{
    if ( connected_ )
    {
        disconnect();

        using boost::posix_time::seconds;

        if (errwait_ < seconds(10)) errwait_ += seconds(2);

        log4.noticeStream() << "Retrying connection to Crawler Manager in " << errwait_;

        retrytimer_.expires_from_now( errwait_ );
        retrytimer_.async_wait(boost::bind( &manconnection::connect, this, ph::error ));
    }
}

void manconnection::send_success( crawler_protocol::message_type const type, std::string const& str )
{
    log4.infoStream() << "Successfully sent a " << type << " message with body '" << str << '\'';
}

void manconnection::send_failure( crawler_protocol::message_type const type, std::string const& str )
{
    log4.errorStream() << "Failed to send a " << type << " message with body '" << str << '\'';
    reconnect();
}
