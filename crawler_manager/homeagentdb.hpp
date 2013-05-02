/*
 * homeagentdb.hpp
 *
 *  Created on: 2013-05-02
 *      Author: apokluda
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
    typedef std::map< pollerconnection_ptr, std::list< std::string const* > > pollerdb_t;

    boost::asio::strand strand_;
    pollerq_t pollerq_;
    hadb_t hadb_;
    pollerdb_t pollerdb_;
};


#endif /* HOMEAGENTDB_HPP_ */
