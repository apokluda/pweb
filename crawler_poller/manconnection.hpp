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

#ifndef MANCONNECTION_HPP_
#define MANCONNECTION_HPP_

#include "stdhdr.hpp"
#include "messages.hpp"
#include "bufreadwrite.hpp"

class manconnection;
typedef manconnection* manconnection_ptr;
typedef manconnection const* cmanconnection_ptr;

class manconnection : private boost::noncopyable
{
public:
    manconnection(boost::asio::io_service&, std::string const& mhostname, std::string const& mport);

    void home_agent_discovered( std::string const& hostname )
    {
        //if ( connected_.load(boost::memory_order_acquire) ) bufwrite_.sendmsg( crawler_protocol::HOME_AGENT_DISCOVERED, hostname );
        //else send_failure( crawler_protocol::HOME_AGENT_ASSIGNMENT, hostname );
    	bufwrite_.sendmsg( crawler_protocol::HOME_AGENT_DISCOVERED, hostname );
    }

private:
    void connect( boost::system::error_code const& ec = boost::system::error_code() );

    void handle_resolve( boost::system::error_code const&, boost::asio::ip::tcp::resolver::iterator );
    void handle_connect( boost::system::error_code const& );

    void disconnect();

    void send_success( crawler_protocol::message_type const, std::string const& );
    void send_failure( crawler_protocol::message_type const, std::string const& );

    typedef boost::asio::ip::tcp::socket* socket_ptr;

    boost::asio::ip::tcp::resolver resolver_;
    std::string const mhostname_;
    std::string const mport_;
    boost::asio::ip::tcp::socket socket_;
    bufread< socket_ptr > bufread_;
    bufwrite< socket_ptr > bufwrite_;
    boost::atomic< bool > connected_;
};

#endif /* MANCONNECTION_HPP_ */
