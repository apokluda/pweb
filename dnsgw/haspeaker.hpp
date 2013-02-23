/*
 * haspeaker.hpp
 *
 *  Created on: 2013-02-23
 *      Author: alex
 */

#ifndef HASPEAKER_HPP_
#define HASPEAKER_HPP_

#include "stdhdr.hpp"

class dnsquery;
typedef boost::shared_ptr< dnsquery > query_ptr;

class haspeaker
{
public:
    haspeaker(boost::asio::io_service& io_service)
    : io_service_(io_service)
    {
    }

    void process_query( query_ptr query )
    {
        // TODO: Send query to home agent!
    }

private:
    boost::asio::io_service& io_service_;
};

// In future, the ha_load_balancer could be made into a
// class hierarchy with subclasses implementing different
// load balancing algorithms. The algorithm/subclass to
// instantiate could be selected from a config parameter.

// hacontainer = random access container
template < class hacontainer >
class ha_load_balancer
{
public:
    ha_load_balancer( hacontainer& haspeakers)
    : haspeakers_(haspeakers)
    {
    }

    void process_query( query_ptr query )
    {
        // TODO: select random haspeaker
        // haspeaker->process_query( query );
    }

private:
    hacontainer& haspeakers_;
};

#endif /* HASPEAKER_HPP_ */
