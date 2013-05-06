/*
 * scheduler.cpp
 *
 *  Created on: 2013-05-05
 *      Author: alex
 */

#include "poller.hpp"
#include "asynchttprequester.hpp"

using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

static boost::random::mt19937 gen;

extern log4cpp::Category& log4;

namespace poller
{

poller::poller( curl::Context& c, std::string const& hostname, boost::posix_time::time_duration const interval )
: requester_( c )
, hostname_( hostname )
, timer_( c.get_io_service() )
, interval_( interval )
, timestamp_( 1 ) // For some bizarre reason, the Home Agents can't handle a request with timestamp 0
, poll_in_progress_( false )
{
    using namespace boost::posix_time;
    boost::random::uniform_int_distribution< long > dist(0, interval.total_milliseconds());
    time_duration const first_interval = milliseconds( dist( gen ) );

    timer_.expires_from_now( first_interval );
    timer_.async_wait( boost::bind( &poller::do_poll, this, ph::error ) );
}

void poller::do_poll( bs::error_code const& ec )
{
    if ( poll_in_progress_.exchange(true, boost::memory_order_acquire) )
    {
        // Do this now so that we actually poll the Home Agent once ever 'interval' seconds
        // (or as close as possible) rather than once ever 'interval + polling + parsing' seconds.
        timer_.expires_from_now( interval_ );

        if ( !ec )
        {
            log4.infoStream() << "Polling Home Agent at '" << hostname_ << '\'';

            std::ostringstream url;
            url << "http://" << hostname_ << ":20005/?method=getall&timestamp=" << timestamp_;

            requester_.fetch(url.str(), boost::bind(&ha_register::handle_register_name, this, _1, _2));
        }
        else
        {
            // We will get operation_aborted on shutdown
            if ( ec != error::operation_aborted ) log4.errorStream() << "An error occurred while waiting for timer: " << ec.message();
        }
    }
    else
    {
        // Should never really happen... the curl operations should time out if a Home Agent
        // or the Solr server is not responding properly/quickly, but we do this check
        // anyway.
        log4.warnStream() << "Overlapping polls detected!";
    }
}

void poller::handle_poll( CURLcode const code, std::string const& content )
{
    // check that code is CURLE_OK, log result, and start a new request
    if ( code == CURLE_OK )
    {
        log4.infoStream() << "Successfully polled '" << hostname_ << '\'';

        // TODO: Parse response
    }
    else
    {
        log4.errorStream() << "An error occurred while polling '" << hostname_ << "', CURLcode " << code;
    }

    // do_poll is guaranteed not to be executed from within this method
    timer_.async_wait( boost::bind( &poller::do_poll, this, ph::error ) );
    poll_in_progress_.exchange(false, boost::memory_order_release);
}

}
