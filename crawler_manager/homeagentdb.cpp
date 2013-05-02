/*
 * homeagentdb.cpp
 *
 *  Created on: 2013-05-02
 *      Author: apokluda
 */

#include "homeagentdb.hpp"

extern log4cpp::Category& log4;

homeagentdb::homeagentdb( boost::asio::io_service& io_service )
: strand_( io_service )
{
}

void homeagentdb::poller_connected_( pollerconnection_ptr )
{
    throw std::runtime_error("Not implemented yet.");
}

void homeagentdb::poller_disconnected_( pollerconnection_ptr )
{
    throw std::runtime_error("Not implemented yet.");
}

void homeagentdb::add_home_agent_( std::string const& hahostname )
{
    if ( !pollerdb_.empty() )
    {
        assign_home_agent( hahostname );
    }
    else
    {
        log4.debugStream() << "Adding '" << hahostname << "' to poller queue";
        pollerq_.push( hahostname );
    }
}

void homeagentdb::assign_home_agent( std::string const& hahostname )
{
    // Note: The caller must ensure that there is at least one connected poller!

    std::pair< hadb_t::iterator, bool> res = hadb_.insert( hahostname );
    if ( res.second )
    {
        // hahostname has not yet been assigned to a poller

        pollerdb_t::iterator min = pollerdb_.begin();
        for ( pollerdb_t::iterator i = min; i != pollerdb_.end(); ++i )
            if ( i->second.size() < min->second.size() ) min = i;

        log4.infoStream() << "Assigning Home Agent '" << hahostname << "' to " << *min->first;
        min->second.push_back( &(*res.first) );
        min->first->assign_home_agent( hahostname );
    }
    else
    {
        // hahostname has already been assigned to a poller
        log4.warnStream() << "Home Agent '" << hahostname << "' has already been assigned to a poller";
    }
}

void homeagentdb::process_queue()
{
    while ( !pollerq_.empty() && !pollerdb_.empty() )
    {
        assign_home_agent( pollerq_.front() );
        pollerq_.pop();
    }
}
