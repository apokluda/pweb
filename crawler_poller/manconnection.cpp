/*
 * manconnection.cpp
 *
 *  Created on: 2013-05-02
 *      Author: apokluda
 */

#include "manconnection.hpp"

using std::string;

namespace bs = boost::system;
namespace ph = boost::asio::placeholders;

using namespace boost::asio;
using namespace boost::asio::ip;

extern log4cpp::Category& log4;

manconnection::manconnection(io_service& io_service, string const& mhostname, string const& mport)
: resolver_( io_service )
, socket_( io_service )
, mhostname_( mhostname )
, mport_( mport )
{
    start();
}

void manconnection::start()
{
    log4.infoStream() << "Connecting to Crawler Manager at '" << mhostname_ << "' port " << mport_;

    tcp::resolver::query q(mhostname_, mport_);
    resolver_.async_resolve(q,
            boost::bind( &manconnection::handle_resolve, this, ph::error, ph::iterator ) );
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

        // START READING! so that we can receive a Home Agent assignment
    }
    else
    {
        log4.errorStream() << "An error occurred while connecting to Crawler Manager: " << ec.message();
        reconnect();
    }
}

void manconnection::reconnect()
{
    // Reconnect using a backoff timer!
    // Look at old implementantion in DNS Gateway?
}

