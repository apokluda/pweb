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

void homeagentdb::poller_connected_( pollerconnection_ptr ptr )
{
    pollerdb_.insert( std::make_pair( ptr, pollerdb_t::mapped_type() ) );
    process_queue();
}

void homeagentdb::poller_disconnected_( pollerconnection_ptr ptr )
{
    pollerdb_t::iterator polleriter = pollerdb_.find( ptr );
    assert( polleriter != pollerdb_.end() );
    halist_t& halist = polleriter->second;

    // It would be nice to use std::for_each here, but we need to double dereference i which complicates things
    for ( halist_t::iterator i = halist.begin(); i != halist.end(); ++i )
    {
        // Note: We must push the string to the queue first, before removing the string
        // from the hadb because this will effectively delete the string pointed to by
        // the pointer in the interator!
        pollerq_.push( **i );
        hadb_.erase( **i );
    }

    pollerdb_.erase( polleriter );
    process_queue();
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

        log4.debugStream() << "Assigning Home Agent '" << hahostname << "' to " << *min->first;
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
