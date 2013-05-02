/*
 * pollerconnector.cpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
 */

#ifndef POLLERCONNECTOR_CPP_
#define POLLERCONNECTOR_CPP_

#include "pollerconnector.hpp"
#include "protocol_helper.hpp"
#include "signals.hpp"

using std::string;

using namespace boost::asio;
namespace bs = boost::system;
namespace ph = boost::asio::placeholders;
namespace ptime = boost::posix_time;

extern log4cpp::Category& log4;

void pollerconnection::start()
{
    log4.noticeStream() << "Received a new poller connection from " << *this;

    signals::poller_connected( shared_from_this() );

    read_header();
}

void pollerconnection::assign_home_agent(std::string const& hostname)
{
    // Note: calls to this method are not serialized!
    bufitem_ptr bufitem( new bufitem_t );
    bufitem->first.type( crawler_protocol::HOME_AGENT_ASSIGNMENT );
    std::size_t const length = hostname.length() + 1; // +1 for null character
    bufitem->first.length( length );
    bufitem->second.resize( length );
    char const* str = hostname.c_str();
    std::copy(str, str + length, bufitem->second.begin());

    strand_.dispatch(boost::bind(&pollerconnection::send_bufitem, shared_from_this(), bufitem));
}


void pollerconnection::read_header()
{
    async_read( socket_, buffer( recv_header_.buffer() ),
            boost::bind( &pollerconnection::handle_header_read, shared_from_this(), ph::error, ph::bytes_transferred ) );
}

void pollerconnection::handle_header_read( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        if ( recv_header_.type() == crawler_protocol::HOME_AGENT_DISCOVERED )
        {
            async_read( socket_, buffer( recv_buf_, recv_header_.length() ),
                    boost::bind( &pollerconnection::handle_home_agent_discovered, shared_from_this(), ph::error, ph::bytes_transferred ) );
        }
        else
        {
            log4.errorStream() << "Poller sent unknown message type: " << recv_header_.type();
            disconnect();
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while reading poller message header: " << ec.message();
        disconnect();
    }
}

void pollerconnection::handle_home_agent_discovered( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        std::string hahostname;
        protocol_helper::parse_string(hahostname, recv_header_.length(), recv_buf_.begin(), recv_buf_.end());
        log4.infoStream() << "New Home Agent '" << hahostname << "' discovered by " << *this;

        signals::home_agent_discovered( hahostname );

        read_header();
    }
    else
    {
        log4.errorStream() << "An error occurred while reading message body from poller: " << ec.message();
        disconnect();
    }
}

void pollerconnection::send_bufitem(bufitem_ptr& bufitem)
{
    if ( !send_in_progress_ )
    {
        log4.debugStream() << "Sending bufitem to " << *this;

        send_in_progress_ = true;
        boost::array< const_buffer, 2 > bufs;
        bufs[0] = buffer( bufitem->first.buffer() );
        bufs[1] = buffer( bufitem->second );
        async_write( socket_, bufs,
                strand_.wrap( boost::bind( &pollerconnection::handle_bufitem_sent, shared_from_this(), bufitem, ph::error, ph::bytes_transferred ) ) );
    }
    else
    {
        log4.debugStream() << "Queuing bufitem for " << *this;

        send_queue_.push( bufitem );
        std::size_t const size = send_queue_.size();
        if ( size == 10 || size == 25 || size == 50 )
        {
            // Ok, technically, the queue size doesn't exceed 'size' when this message is printed,
            // but if another message is added to the queue after this message is printed, then it will!
            log4.warnStream() << "Poller connection send queue size exceeds " << size;
        }
    }
}

void pollerconnection::handle_bufitem_sent( bufitem_ptr&, bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        log4.debugStream() << "Successfully sent bufitem to " << *this;

        send_in_progress_ = false;
        if ( !send_queue_.empty() )
        {
            bufitem_ptr bufitem = send_queue_.front();
            send_queue_.pop();
            send_bufitem( bufitem );
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while sending bufitem to poller";
    }
}

void pollerconnection::disconnect()
{
    socket_.shutdown( ip::tcp::socket::shutdown_both );
    socket_.close();
    signals::poller_disconnected( shared_from_this() );
}

ip::tcp::socket const& pollerconnection::socket() const
{
    return socket_;
}

ip::tcp::socket& pollerconnection::socket()
{
    return socket_;
}

pollerconnector::pollerconnector(io_service& io_service, string const& interface, boost::uint16_t const port, ptime::time_duration interval)
: acceptor_( io_service )
{
    try
    {
        ip::tcp::endpoint endpoint(ip::tcp::v6(), port);
        if ( !interface.empty() )
        {
            ip::address bind_addr(ip::address::from_string(interface));
            endpoint = ip::tcp::endpoint(bind_addr, port);
        }

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option( ip::tcp::acceptor::reuse_address( true ) );
        acceptor_.bind(endpoint);
        acceptor_.listen();

        log4.infoStream() << "Listening on " << (interface.empty() ? "all interfaces" : interface.c_str()) << " port " << port << " for TCP connections";
    }
    catch ( boost::system::system_error const& )
    {
        log4.fatalStream() << "Unable to bind to interface " << interface << " on port " << port;
        throw;
    }
}

void pollerconnector::start()
{
    new_connection_.reset( new pollerconnection( acceptor_.get_io_service() ) );
    acceptor_.async_accept( new_connection_->socket(),
            boost::bind( &pollerconnector::handle_accept, this, ph::error ) );
}

void pollerconnector::handle_accept(bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.infoStream() << "Received a new poller connection from " << new_connection_->socket().remote_endpoint();
        new_connection_->start();
        // Fall through
    }
    else
    {
        log4.errorStream() << "An error occurred while accepting a new poller connection: " << ec.message();
        // Fall through
    }
    start();
}

#endif /* POLLERCONNECTOR_CPP_ */
