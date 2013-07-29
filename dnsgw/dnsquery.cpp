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

#include "stdhdr.hpp"
#include "dnsquery.hpp"
#include "dnsspeaker.hpp"

boost::asio::ip::udp::endpoint dnsquery::remote_udp_endpoint() const
{
    typedef boost::asio::ip::udp::endpoint endpoint_t;

    endpoint_t endpoint;

    if ( udp_connection_t const* conn = boost::get< udp_connection_t const >( &sender_ ) )
    {
        endpoint = conn->second;
    }

    return endpoint;
}

class address_visitor
: public boost::static_visitor< boost::asio::ip::address >
{
public:
    boost::asio::ip::address operator()( udp_connection_t const& conn ) const
    {
        return conn.second.address();
    }

    boost::asio::ip::address operator()( dns_connection_ptr const conn ) const
    {
        boost::system::error_code ec;
        return conn->remote_endpoint(ec).address();
    }
};

class reply_visitor
    : public boost::static_visitor<>
{
public:
    reply_visitor( query_ptr query )
    : query_(query)
    {
    }

    void operator()( udp_connection_t const& conn ) const
    {
        conn.first->send_reply( query_ );
    }

    void operator()( dns_connection_ptr conn ) const
    {
        conn->send_reply( query_ );
    }

private:
    query_ptr query_;
};

boost::asio::ip::address dnsquery::remote_address() const
{
    return boost::apply_visitor( address_visitor(), sender_ );
}

void dnsquery::send_reply()
{
    boost::apply_visitor( reply_visitor( shared_from_this() ), sender_ );
}
