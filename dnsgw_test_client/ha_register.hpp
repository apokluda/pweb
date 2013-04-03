/*
 * ha_register.hpp
 *
 *  Created on: 2013-04-03
 *      Author: apokluda
 */

#ifndef HA_REGISTER_HPP_
#define HA_REGISTER_HPP_

#include "stdhdr.hpp"
#include "ha_config.hpp"

struct haconfig;
namespace curl
{
    class Context;
    class AsyncHTTPRequester;
}

class ha_register : public boost::enable_shared_from_this< ha_register >
{
public:
    ha_register(curl::Context& context, haconfig const& haconfig, int numnames)
    : namenum_(numnames)
    , c_(context)
    , haconfig_(haconfig)
    {
    }

    void async_run(std::size_t maxconn);

private:
    void register_name(boost::shared_ptr< curl::AsyncHTTPRequester > r);
    void handle_register_name(CURLcode const code, std::string const& content, std::string const& name, boost::shared_ptr< curl::AsyncHTTPRequester > rptr);

    int namenum_;
    curl::Context& c_;
    haconfig const& haconfig_;
};

template < typename InputIterator >
void register_names(InputIterator first, InputIterator const last, curl::Context& context, int numnames, std::size_t const maxconn)
{
    while ( first != last )
    {
        if ( first->status == GOOD )
        {
            boost::shared_ptr< ha_register > r( new ha_register(context, *first, numnames) );
            r->async_run( maxconn );
        }
        ++first;
    }
}

#endif /* HA_REGISTER_HPP_ */
