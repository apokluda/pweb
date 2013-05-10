/*
 * scheduler.cpp
 *
 *  Created on: 2013-05-05
 *      Author: alex
 */

#include "poller.hpp"
#include "asynchttprequester.hpp"
#include "signals.hpp"

using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

static boost::random::mt19937 gen;

extern log4cpp::Category& log4;

namespace poller
{

parser::getall_parser< std::string::const_iterator > const poller::g_;

poller::poller(Context const& pollerctx, curl::Context& curlctx, std::string const& hostname )
: requester_( curlctx )
, hostname_( hostname )
, timer_( curlctx.get_io_service() )
, timestamp_( 1 ) // For some bizarre reason, the Home Agents can't handle a request with timestamp 0
, pollerctx_( pollerctx )
{
    using namespace boost::posix_time;
    boost::random::uniform_int_distribution< long > dist(0, pollerctx_.interval.total_milliseconds());
    time_duration const first_interval = milliseconds( dist( gen ) );

    timer_.expires_from_now( first_interval );
    start();
}

void poller::do_poll( bs::error_code const& ec )
{
    // Do this now so that we actually poll the Home Agent once ever 'interval' seconds
    // (or as close as possible) rather than once ever 'interval + polling + parsing' seconds.
    timer_.expires_from_now( pollerctx_.interval );

    if ( !ec )
    {
        log4.infoStream() << "Polling Home Agent at '" << hostname_ << '\'';

        std::ostringstream url;
        url << "http://" << hostname_ << ":20005/?method=getall&timestamp=" << timestamp_;

        requester_.fetch(url.str(), boost::bind(&poller::handle_poll, this, _1, _2));
    }
    else
    {
        // We will get operation_aborted on shutdown
        if ( ec != error::operation_aborted ) log4.errorStream() << "An error occurred while waiting for timer: " << ec.message();
    }
}

void poller::handle_poll( CURLcode const code, std::string const& content )
{
    // check that code is CURLE_OK, log result, and start a new request
    if ( code == CURLE_OK )
    {
        log4.infoStream() << "Successfully polled '" << hostname_ << '\'';

        parser::getall gall;
        std::string::const_iterator iter = content.begin();
        std::string::const_iterator end = content.end();
        bool const r = boost::spirit::qi::parse(iter, end, g_, gall);

        if ( !r && iter != end)
        {
            log4.errorStream() << "Failed to parse content returned from '" << hostname_ << '\'';
            start(); return;
        }

        parser::getall::halist_t& homeagents = gall.homeagents;
        parser::getall::devinfolist_t& deviceinfos = gall.deviceinfos;

        log4.infoStream() << "Home Agent '" << hostname_ << "' returned list of " << homeagents.size() << " neighbours and " << deviceinfos.size() << " updated devices";

        typedef parser::getall::halist_t::const_iterator hiter_t;
        for ( hiter_t i = homeagents.begin(); i != homeagents.end(); ++i ) signals::home_agent_discovered( *i );

        if ( deviceinfos.empty() ) { start(); return; }

        // We could use the Spirit Karma library here, but this is sufficient for now
        std::ostringstream out;
        out << "<add overwrite=\"true\">";
        typedef parser::getall::devinfolist_t::const_iterator diter_t;
        for ( diter_t i = deviceinfos.begin(); i != deviceinfos.end(); ++i )
        {
            out << "<doc><field name=\"name\">" << i->name << "</field>";
            out << "<field name=\"timestamp\">" << i->timestamp << "</field></doc>";
            if (i->timestamp > timestamp_) timestamp_ = i->timestamp;
        }
        out << "</add>";

        log4.debugStream() << "Sending update to Solr";
        requester_.fetch(pollerctx_.solrurl + "?commit=true", boost::bind(&poller::handle_post, this, _1, _2), out.str());
    }
    else
    {
        log4.errorStream() << "An error occurred while polling '" << hostname_ << "', CURLcode " << code;
        start();
    }
}

void poller::handle_post( CURLcode const code, std::string const& content )
{
    if ( code == CURLE_OK )
    {
        if ( content.find("<int name=\"status\">0</int>") != std::string::npos )
        {
            log4.infoStream() << "Successfully sent update to Solr";
        }
        else
        {
            log4.errorStream() << "Unable to find success status in Solr output:\n" << content;
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while POST'ing data to Solr, CURLcode " << code;
    }
    start();
}

}
