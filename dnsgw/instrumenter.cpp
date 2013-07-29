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

#include "stdhdr.hpp"
#include "instrumenter.hpp"
#include "metric.hpp"
#include "protocol_helper.hpp"

using namespace protocol_helper;
using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;
using std::string;
using std::vector;
using boost::shared_ptr;

using namespace instrumentation;

extern log4cpp::Category& log4;

const boost::asio::deadline_timer::duration_type udp_instrumenter::NAGLE_PERIOD = boost::posix_time::milliseconds(500);

using namespace instrumentation;

void instrumenter::add_metric(metric const& metric)
{
    log4.infoStream() << "Statistics: " << metric;
    do_add_metric(metric);
}

udp_instrumenter::udp_instrumenter( io_service& io_service, string const& server, string const& port)
: socket_( io_service )
, strand_( io_service )
, timer_( io_service )
, buf_size_( 0 )
, buf_( new vector< string > )
{
    ip::udp::resolver resolver( io_service );
    ip::udp::resolver::query const query( server, port );
    // NOTE: SYNCRONOUS OPERATION!
    // At this point, this is the only blocking network operation in the program.
    // (Ok, now 1 of 2 since I added the connect() below).
    // This happens early at program startup, so blocking here shouldn't be a problem.
    ip::udp::endpoint const endpoint = *resolver.resolve( query ); // Safe to dereference because resolve throws on error
    // NOTE: "SYNCRONOUS" OPERATION!
    // This shouldn't really block for any amount of time because UDP sockets are
    // "connectionless." It should just associate this socket with the remote address
    // (a local operation).
    socket_.connect(endpoint);
}

void udp_instrumenter::do_add_metric(metric const& metric)
{
    std::ostringstream oss;
    // NOTE: The boost serialization library provides three archive formats:
    // binary, text, and XML. Naturally, the binary format is the most efficient
    // but it is non-portable!! If there is ever a requirement to run the application
    // and instrumentation service on heterogeneous hardware, the text archive
    // format should be used.
    boost::archive::text_oarchive oa(oss);
    oa << metric;
    strand_.dispatch( boost::bind(&udp_instrumenter::add_metric_, this, oss.str()) );
}

void udp_instrumenter::add_metric_(string const& buf)
{
    // Note: calls to this method must be serialized

    std::size_t const my_buf_size = buf.size();
    if ( buf_size_ + my_buf_size > MAX_BUF_SIZE ) send_now();
    buf_size_ += my_buf_size;
    buf_->push_back(buf);
    if ( buf_size_ == MAX_BUF_SIZE ) send_now();
    if ( buf_size_ == my_buf_size ) start_timer();
}

void udp_instrumenter::send_now()
{
    // Note: calls to this method must be serialized

    timer_.cancel();

    vector< const_buffer > bufseq;
    bufseq.reserve(buf_->size());
    for ( vector<string>::iterator begin = buf_->begin(); begin != buf_->end(); ++begin)
        bufseq.push_back(buffer(*begin));
    socket_.async_send(bufseq, boost::bind(&udp_instrumenter::handle_datagram_sent, this, buf_, ph::error, ph::bytes_transferred));

    buf_ptr_t new_buf( new vector< string > );
    buf_.swap( new_buf );
    buf_size_ = 0;
}

void udp_instrumenter::handle_datagram_sent(buf_ptr_t buf, bs::error_code const& ec, std::size_t const bytes_transferred)
{
    // Note: calls to this method are not serialized

    if ( !ec )
    {
        log4.debugStream() << "Successfully sent instrumentation datagram";
    }
    else
    {
        log4.errorStream() << "An error occurred while sending instrumentation datagram: " << ec.message();
    }
}

void udp_instrumenter::start_timer()
{
    // Note: calls to this method must be serialized

    timer_.expires_from_now(NAGLE_PERIOD);
    timer_.async_wait(strand_.wrap(boost::bind(&udp_instrumenter::handle_nagle_timeout, this, ph::error)));
}

void udp_instrumenter::handle_nagle_timeout(bs::error_code const& ec)
{
    // Note: calls to this method must be serialized

    if ( !ec )
    {
        send_now();
    }
    else if ( ec != boost::asio::error::operation_aborted )
    {
        log4.errorStream() << "An error occurred while waiting for the instrumentation nagle timer: " << ec;
    }
}
