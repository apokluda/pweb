/*
 * haloadbalancer.cpp
 *
 *  Created on: 2013-03-04
 *      Author: apokluda
 */

#include "stdhdr.hpp"
#include "haloadbalancer.hpp"
#include "dnsquery.hpp"

//extern log4cpp::Category& log4;

template < class hacontainer_iter_t, class hacontainer_diff_t >
void ha_load_balancer< hacontainer_iter_t, hacontainer_diff_t >::process_query( query_ptr query )
{
    hacontainer_diff_t my_offset = offset_.fetch_add(1, boost::memory_order_relaxed);
    hacontainer_iter_t hs = begin_ + (my_offset % size_);
    (*hs)->process_query( query );
}

#include "haproxy.hpp"

typedef boost::shared_ptr< hasendproxy > haspeaker_ptr;
typedef std::vector< haspeaker_ptr > haspeakers_t;
template class ha_load_balancer< haspeakers_t::iterator, haspeakers_t::difference_type >;
