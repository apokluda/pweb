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
