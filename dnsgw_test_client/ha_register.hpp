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

#ifndef HA_REGISTER_HPP_
#define HA_REGISTER_HPP_

#include "stdhdr.hpp"
#include "ha_config.hpp"

struct haconfig;
class token_counter;
namespace curl
{
    class Context;
    class AsyncHTTPRequester;
}

class ha_register : public boost::enable_shared_from_this< ha_register >
{
public:
    ha_register(curl::Context& context, haconfig const& haconfig, std::string const& owner, token_counter& tc, std::size_t numnames, std::size_t const maxconns)
    : owner_( owner )
    , c_( context )
    , haconfig_( haconfig )
    , tc_( tc )
    , maxconns_( maxconns )
    , activeconns_( 0 )
    , namenum_( numnames )
    {
    }

    void async_run();

private:
    void register_name(boost::shared_ptr< curl::AsyncHTTPRequester > r);
    void handle_register_name(CURLcode const code, std::string const& content, std::string const& name, boost::shared_ptr< curl::AsyncHTTPRequester > rptr);

    std::string const owner_;
    curl::Context& c_;
    haconfig const& haconfig_;
    token_counter& tc_;
    std::size_t const maxconns_;
    std::size_t activeconns_;
    std::size_t namenum_;
};

template < typename InputIterator >
void register_names(InputIterator first, InputIterator const last, curl::Context& context, token_counter& tc, std::string const& owner, std::size_t numnames, std::size_t const maxconn)
{
    while ( first != last )
    {
        if ( first->status == GOOD )
        {
            boost::shared_ptr< ha_register > r( new ha_register(context, *first, owner, tc, numnames, maxconn) );
            r->async_run();
        }
        ++first;
    }
}

#endif /* HA_REGISTER_HPP_ */
