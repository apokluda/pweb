/*
 * haproxy.cpp
 *
 *  Created on: 2013-03-04
 *      Author: apokluda
 */

#include "stdhdr.hpp"
#include "haproxy.hpp"

typedef std::stringstream sstream;
using std::vector;
using std::string;
using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

hasendproxy::hasendproxy(io_service& io_service,
        string const& hahostname, boost::uint16_t const haport,
        string const& nshostname, boost::uint16_t const nsport,
        boost::uint16_t const ttl, string const& suffix)
: io_service_(io_service)
, hahostname_(hahostname)
, nshostname_(nshostname)
, suffix_(suffix)
, haport_(haport)
, nsport_(nsport)
, ttl_(ttl)
{
}

void hasendproxy::process_query( query_ptr query )
{
    // TODO: create a connection and send the query (note: connections managed with smart pointers)
}

harecvproxy::harecvproxy(io_service& io_service, string const& nshostname, boost::uint16_t const port)
: io_service_(io_service)
{
    // TODO: stuff
}


void harecvproxy::start()
{
    // TODO: stuff
}
