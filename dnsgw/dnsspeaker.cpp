/*
 * dnsspeaker.cpp
 *
 *  Created on: 2013-02-20
 *      Author: apokluda
 */

#include "stdhdr.h"
#include "dnsspeaker.hpp"

using namespace boost::asio;
using std::string;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

log4cpp::Category& log4 = log4cpp::Category::getRoot();

udp_dnsspeaker::udp_dnsspeaker(io_service& io_service, string const& iface, uint16_t port)
: socket_( io_service )
{
    try
    {
        ip::udp::endpoint endpoint(ip::udp::v6(), port);
        if ( !iface.empty() )
        {
            ip::address bind_addr(ip::address::from_string(iface));
            endpoint = ip::udp::endpoint(bind_addr, port);
        }

        socket_.open(endpoint.protocol());
        socket_.bind(endpoint);

        log4.infoStream() << "Listening on " << (iface.empty() ? "all interfaces" : iface.c_str()) << " port " << port << " for UDP connections";
    }
    catch ( boost::system::system_error const& )
    {
        log4.fatalStream() << "Unable to bind to interface";
        if (port < 1024 )
            log4.fatalStream() << "Does the program have sufficient privileges to bind to port " << port << '?';
        throw;
    }
}

tcp_dnsspeaker::tcp_dnsspeaker(io_service& io_service, string const& iface, uint16_t port)
: socket_( io_service )
{
    try
    {
        ip::tcp::endpoint endpoint(ip::tcp::v6(), port);
        if ( !iface.empty() )
        {
            ip::address bind_addr(ip::address::from_string(iface));
            endpoint = ip::tcp::endpoint(bind_addr, port);
        }

        socket_.open(endpoint.protocol());
        socket_.bind(endpoint);

        log4.infoStream() << "Listening on " << (iface.empty() ? "all interfaces" : iface.c_str()) << " port " << port << " for TCP connections";
    }
    catch ( boost::system::system_error const& )
    {
        log4.fatalStream() << "Unable to bind to interface";
        if (port < 1024 )
            log4.fatalStream() << "Does the program have sufficient privileges to bind to port " << port << '?';
        throw;
    }
}

void udp_dnsspeaker::start()
{
    boost::asio::async_read_from( socket_, sender_endpoint_, header_.buffer(),
            boost::bind( &udp_dnsspeaker::handle_header_read, this, ph::error, ph::bytes_transferred ) );
}

void udp_dnsspeaker::handle_header_read( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        if ( bytes_transferred != header_.length() )
        {
            log4.noticeStream() << "Received UDP DNS query with malformed header (" << bytes_transferred << " bytes)";
            return start();
        }


    }
    else
    {
        log4.errorStream() << "An error occurred while reading UDP DNS query header: " << ec.message();
        start();
    }
}

void tcp_dnsspeaker::start()
{
    //boost::asio::async_read( udp_socket_, header_.buffer() );
}
