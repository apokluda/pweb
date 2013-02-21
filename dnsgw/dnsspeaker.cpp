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

log4cpp::Category& log4 = log4cpp::Category::getRoot();

dnsspeaker::dnsspeaker(io_service& io_service, string const& iface, uint16_t port, size_t num_threads)
: udp_socket_( io_service )
, tcp_socket_( io_service )
{
    try
    {
        ip::udp::endpoint udp_endpoint(ip::udp::v6(), port);
        ip::tcp::endpoint tcp_endpoint(ip::tcp::v6(), port);
        if ( !iface.empty() )
        {
            ip::address bind_addr(ip::address::from_string(iface));
            udp_endpoint = ip::udp::endpoint(bind_addr, port);
            tcp_endpoint = ip::tcp::endpoint(bind_addr, port);
        }

            udp_socket_.open(udp_endpoint.protocol());
            tcp_socket_.open(tcp_endpoint.protocol());

            udp_socket_.bind(udp_endpoint);
            tcp_socket_.bind(tcp_endpoint);

            log4.infoStream() << "Listening on " << (iface.empty() ? "all interfaces" : iface.c_str()) << " port " << port << " for UDP and TCP connections";
    }
    catch ( std::exception const& )
    {
        log4.fatalStream() << "Unable to bind to interface";
        if (port < 1024 )
            log4.fatalStream() << "Does the program have sufficient privileges to bind to port " << port << '?';
        throw;
    }
}
