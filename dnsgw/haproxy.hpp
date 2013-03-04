/*
 * haproxy.hpp
 *
 *  Created on: 2013-03-04
 *      Author: apokluda
 */

#ifndef HAPROXY_HPP_
#define HAPROXY_HPP_

#include "stdhdr.hpp"

class dnsquery;
typedef boost::shared_ptr< dnsquery > query_ptr;

class hasendproxy
{
public:
    hasendproxy(boost::asio::io_service& io_service,
            std::string const& hahostname, boost::uint16_t const haport,
            std::string const& nshostname, boost::uint16_t const nsport,
            boost::uint16_t const ttl, std::string const& suffix);

    void process_query( query_ptr query );

private:
    boost::asio::io_service& io_service_;
    std::string const hahostname_;
    std::string const nshostname_;
    std::string const suffix_;
    boost::uint16_t const haport_;
    boost::uint16_t const nsport_;
    boost::uint16_t const ttl_;
};

class harecvproxy
{
public:
    harecvproxy(boost::asio::io_service& io_service,
           std::string const& nshostname, boost::uint16_t const nsport);

    void start();

private:
    boost::asio::io_service& io_service_;
};

#endif /* HAPROXY_HPP_ */
