/*
 * ha_config.hpp
 *
 *  Created on: 2013-04-02
 *      Author: alex
 */

#pragma once
#include "stdhdr.hpp"

enum hastate_t
{
    GOOD,
    DNS_LOOKUP_FAILED,
    URL_IP_MISMATCH,
    UNABLE_TO_CONNECT,
    UNKNOWN
};

char const* hastate_to_string( hastate_t const state );

struct haconfig
{
    std::string url;
    std::string port;
    std::string haname;
    boost::asio::ip::address ip;
    hastate_t status;
};
typedef std::vector< haconfig > halist_t;

void load_halist(halist_t& halist, std::istream& is);

template < typename InputIterator >
class ha_checker : private boost::noncopyable
{
public:
    ha_checker( InputIterator begin, InputIterator const end );
    void sync_run( std::size_t maxconn );

private:
    void start_check();

    boost::asio::io_service io_service_;
    InputIterator iter_;
    InputIterator const end_;
};
