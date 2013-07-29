/*
 *  Copyright (C) 2013 Alexander Pokluda
 *
 *  This file is part of pWeb.
 *
 *  pWeb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pWeb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pWeb.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HOMEAGENTDB_HPP_
#define HOMEAGENTDB_HPP_

#include "stdhdr.hpp"
#include "pollerconnector.hpp"

class homeagentdb : private boost::noncopyable
{
public:
    homeagentdb( boost::asio::io_service& );

    void poller_connected( pollerconnection_ptr ptr )
    {
        strand_.dispatch( boost::bind( &homeagentdb::poller_connected_, this, ptr ) );
    }

    void poller_disconnected( pollerconnection_ptr ptr )
    {
        strand_.dispatch( boost::bind( &homeagentdb::poller_disconnected_, this, ptr ) );
    }

    void add_home_agent( std::string const& hahostname )
    {
        strand_.dispatch( boost::bind( &homeagentdb::add_home_agent_, this, hahostname ) );
    }

private:
    void poller_connected_( pollerconnection_ptr );
    void poller_disconnected_( pollerconnection_ptr );

    void add_home_agent_( std::string const& hahostname );

    void assign_home_agent( std::string const& hahostname );
    void process_queue();

    typedef std::queue< std::string > pollerq_t;
    typedef std::set< std::string > hadb_t;
    typedef std::list< std::string const* > halist_t;
    typedef std::map< pollerconnection_ptr, halist_t > pollerdb_t;

    boost::asio::strand strand_;
    pollerq_t pollerq_;
    hadb_t hadb_;
    pollerdb_t pollerdb_;
};


#endif /* HOMEAGENTDB_HPP_ */
