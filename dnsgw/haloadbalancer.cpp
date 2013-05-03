/*
 * haloadbalancer.cpp
 *
 *  Created on: 2013-03-04
 *      Author: apokluda
 */

#include "stdhdr.hpp"
#include "haloadbalancer.hpp"
#include "dnsquery.hpp"

extern log4cpp::Category& log4;

using namespace instrumentation::result_types;

bool sanity_check( dnsquery& query )
{
    // Sanity check: We can only handle queries for A recods
        // This is done here because it currently seems like the most logical
        // place to do it. Before this point, queries received over TCP and UDP
        // are handled separately, and we don't want to duplicate the sanity checks.
        // We also don't want to reject querise in the protocol parsing code (this
        // type of application logic doesn't belong there.
        //
        // After this point is logic for dealing with an individual Home Agent. This
        // sort of universal check doesn't belong there either!!
        //
        // TODO: The checking of the domain suffix is done in the haproxy object.
        // Base on the above, it should be done here too!

        // There should be [exactly] one question for an A record
        if ( query.num_questions() > 0 )
        {
            dnsquestion const& question = *query.questions_begin();
            if ( question.qtype != T_A || question.qclass != C_IN )
            {
                complete_query(query, R_NOT_IMPLEMENTED, INVALID_REQUEST);
                return false;
            }
        }
        else
        {
            complete_query(query, R_SERVER_FAILURE, HA_CONNECTION_ERROR);
            return false;
        }
        return true;
}

template < class hacontainer_iter_t, class hacontainer_diff_t >
void ha_load_balancer< hacontainer_iter_t, hacontainer_diff_t >::process_query( query_ptr query )
{
    // Sanity check does not need to be templated
    if ( !sanity_check( *query ) ) return;

    hacontainer_diff_t my_offset = offset_.fetch_add(1, boost::memory_order_relaxed);
    hacontainer_iter_t hs = begin_ + (my_offset % size_);

    for (hacontainer_diff_t cnt = 0; cnt < size_; ++cnt)
    {
        log4.debugStream() << "Selected home agent " << (*hs)->hostname() << " (offset = " << my_offset << ", cnt =" << cnt << ")";
        if ( (*hs)->enabled() )
        {
            (*hs)->process_query( query );
            return;
        }
        if ( ++hs == (begin_ + size_) ) hs = begin_;
    }
    log4.warnStream() << "Unable to process query from " << query->remote_address() << ": Not connected to any home agents";

    complete_query(*query, R_SERVER_FAILURE, HA_CONNECTION_ERROR);
}

#include "haproxy.hpp"

typedef boost::shared_ptr< hasendproxy > haspeaker_ptr;
typedef std::vector< haspeaker_ptr > haspeakers_t;
template class ha_load_balancer< haspeakers_t::iterator, haspeakers_t::difference_type >;
