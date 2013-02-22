/*
 * dnsquery.hpp
 *
 *  Created on: 2013-02-21
 *      Author: alex
 */

#ifndef DNSQUERY_HPP_
#define DNSQUERY_HPP_

#include "stdhdr.hpp"

class udp_dnsspeaker;

class dns_connection;
typedef boost::shared_ptr< dns_connection > dns_connection_ptr;

typedef std::pair< udp_dnsspeaker*, boost::asio::ip::udp::endpoint > udp_connection_t;

class dnsquery;
typedef boost::shared_ptr< dnsquery > query_ptr;

class dnsquery
: public boost::enable_shared_from_this< dnsquery >,
  private boost::noncopyable
{
public:
    dnsquery();

    uint16_t id() const
    {
        return id_;
    }

    void id( uint16_t const id )
    {
        id_ = id;
    }

    void sender( udp_dnsspeaker* const speaker,  boost::asio::ip::udp::endpoint const& endpoint )
    {
        sender_ = std::make_pair(speaker, endpoint);
    }

    void sender( dns_connection_ptr conn )
    {
        sender_ = conn;
    }

    boost::asio::ip::udp::endpoint remote_udp_endpoint() const;

    void send_reply();

private:
    boost::variant< udp_connection_t, dns_connection_ptr > sender_;

    uint16_t id_;
};


#endif /* DNSQUERY_HPP_ */
