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
    buf_arr_[0] = buffer(header_.buffer());
    buf_arr_[1] = buffer(buf_);
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
: io_service_(io_service)
, acceptor_( io_service )
{
    try
    {
        ip::tcp::endpoint endpoint(ip::tcp::v6(), port);
        if ( !iface.empty() )
        {
            ip::address bind_addr(ip::address::from_string(iface));
            endpoint = ip::tcp::endpoint(bind_addr, port);
        }

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option( ip::tcp::acceptor::reuse_address( true ) );
        acceptor_.bind(endpoint);
        acceptor_.listen();

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
    socket_.async_receive_from( buf_arr_, sender_endpoint_,
            boost::bind( &udp_dnsspeaker::handle_datagram_received, this, ph::error, ph::bytes_transferred ) );
}

void udp_dnsspeaker::handle_datagram_received( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        // Validate header
        if ( header_.qr() &&
                header_.opcode() == dns_query_header::O_QUERY &&
                header_.z() == 0 )
        {
            // Header format OK
            log4.infoStream() << "Received UDP DNS query from " << sender_endpoint_;

            // TODO: Parse body, create a "request object" and send the query to the home agent
        }
        else
        {
            // Malformed header
            log4.noticeStream() << "Received UDP DNS query with invalid header from " << sender_endpoint_;
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while reading UDP DNS query header: " << ec.message();
    }
    // Receive another datagram
    start();
}

void tcp_dnsspeaker::start()
{
    new_connection_.reset( new dns_connection(io_service_) );
    acceptor_.async_accept( new_connection_->socket(),
            boost::bind( &tcp_dnsspeaker::handle_accept, this, ph::error ) );
}

void tcp_dnsspeaker::handle_accept( bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.infoStream() << "Established a new TCP DNS connection with " << new_connection_->socket().remote_endpoint();
        new_connection_->start();
    }
    else
    {
        log4.errorStream() << "An error occurred while accepting a new TCP DNS connection: " << ec.message();
    }
    // Accept additional connections
    start();
}

void dns_connection::start()
{
    async_read( socket_, buffer(&msg_len_, 2),
            boost::bind( &dns_connection::handle_msg_len_read, shared_from_this(),
                    ph::error, ph::bytes_transferred));
}

void dns_connection::handle_msg_len_read( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        msg_len_ = ntohs(msg_len_);
        log4.infoStream() << "Received DNS message length of " << msg_len_ << " from " << socket_.remote_endpoint();

        if (msg_len_ > header_.length() + buf_.size())
        {
            log4.errorStream() << "TCP DNS message length " << msg_len_ << " from " << socket_.remote_endpoint() << " is too large! Closing connection.";
            return;
        }

        buf_arr_[1] = buffer(buf_, msg_len_ - header_.length());
        async_read( socket_, buf_arr_, boost::bind(
                &dns_connection::handle_query_read,
                shared_from_this(), ph::error, ph::bytes_transferred ) );
    }
    else
    {
        log4.errorStream() << "An error occurred while reading TCP DNS message length from " << socket_.remote_endpoint();
    }
}

void dns_connection::handle_query_read( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        // Validate header
        if ( header_.qr() &&
                header_.opcode() == dns_query_header::O_QUERY &&
                header_.z() == 0 )
        {
            // Header format OK
            log4.infoStream() << "Received TCP DNS query from " << socket_.remote_endpoint();

            // TODO: Parse body, create a "request object" and send the query to the home agent
        }
        else
        {
            // Malformed header
            log4.noticeStream() << "Received TCP DNS query with invalid header from " << socket_.remote_endpoint();
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while receiving TCP DNS query from " << socket_.remote_endpoint() << ": " << ec.message();
    }
}
