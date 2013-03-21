/*
 * dnsquery.cpp
 *
 *  Created on: 2013-02-21
 *      Author: alex
 */

#include "stdhdr.hpp"
#include "dnsquery.hpp"
#include "dnsspeaker.hpp"

qtype_t to_qtype( unsigned val  )
{
    if ( ( val > T_MIN && val < T_MAX ) || ( val > QT_MIN && val < QT_MAX ) )
        return static_cast< qtype_t >( val );

    throw std::range_error("Invalid qtype value");
}

qclass_t to_qclass( unsigned val  )
{
    if ( ( val > C_MIN && val < C_MAX ) || ( val > QC_MIN && val < QC_MAX ) )
        return static_cast< qclass_t >( val );

    throw std::range_error("Invalid qclass value");
}

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
