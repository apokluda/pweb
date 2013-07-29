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

#ifndef HALOADBALANCER_HPP_
#define HALOADBALANCER_HPP_

#include "stdhdr.hpp"

class dnsquery;
typedef boost::shared_ptr< dnsquery > query_ptr;

// In future, the ha_load_balancer could be made into a
// class hierarchy with subclasses implementing different
// load balancing algorithms. The algorithm/subclass to
// instantiate could be selected from a config parameter.

// hacontainer_iter_t = random access iterator
// Note: right now, we're using plain but const STL containers.
// If we want to add/remove on the fly, then we will need to use
// concurrent containers, such as the ones provide by Intel TBB

// Perhaps the number of template parameters could be reduced from 2
// down to 1 with some sort of traits magic, but this is fine for now...
template < class hacontainer_iter_t, class hacontainer_diff_t >
class ha_load_balancer
{
public:
    ha_load_balancer( boost::asio::io_service& io_service, hacontainer_iter_t const haspeakers_begin, std::size_t const num_elements)
    : offset_(0)
    , begin_(haspeakers_begin)
    , size_(num_elements)
    {
    }

    void process_query( query_ptr query );

private:
    boost::atomic< hacontainer_diff_t > mutable offset_;
    hacontainer_iter_t const begin_;
    hacontainer_diff_t const size_;
};

#endif /* HALOADBALANCER_HPP_ */
