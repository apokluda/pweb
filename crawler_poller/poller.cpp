/*
 * scheduler.cpp
 *
 *  Created on: 2013-05-05
 *      Author: alex
 */

#include "poller.hpp"

using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

static boost::random::mt19937 gen;

extern log4cpp::Category& log4;

namespace poller
{

poller::poller( boost::asio::io_service& io_service, std::string const& hostname, boost::posix_time::time_duration const interval )
: hostname_( hostname )
, timer_( io_service)
{
    using namespace boost::posix_time;
    boost::random::uniform_int_distribution< long > dist(0, interval.total_milliseconds());
    time_duration const first_interval = milliseconds( dist( gen ) );

    timer_.expires_from_now( first_interval );
    timer_.async_wait( boost::bind( &poller::do_poll, this, ph::error ) );
}

void poller::do_poll( bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.infoStream() << "Polling Home Agent at '" << hostname_ << '\'';

        // TODO: DO POLL!!!! just like in test home agent client
    }
    else
    {
        // We will get operation_aborted on shutdown
        if ( ec != error::operation_aborted ) log4.errorStream() << "An error occurred while waiting for timer: " << ec.message();
    }
}

}
