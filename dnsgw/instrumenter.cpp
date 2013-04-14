/*
 * instrumenter.cpp
 *
 *  Created on: 2013-04-08
 *      Author: apokluda
 */

#include "instrumenter.hpp"
#include "protocol_helper.hpp"

using namespace protocol_helper;
using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;
using std::string;
using std::vector;
using boost::shared_ptr;

extern log4cpp::Category& log4;

static const int NANOSECONDS_VERSION = 1;
BOOST_CLASS_VERSION(boost::chrono::nanoseconds, NANOSECONDS_VERSION)
BOOST_SERIALIZATION_SPLIT_FREE(boost::chrono::nanoseconds)

namespace boost {
namespace serialization {

template < class Archive >
void save(Archive& ar, boost::chrono::nanoseconds const& nanos, unsigned const version)
{
    boost::chrono::nanoseconds::rep count = nanos.count();
    ar << count;
}

template < class Archive >
void load(Archive& ar, boost::chrono::nanoseconds& nanos, unsigned const version)
{
    if ( version != NANOSECONDS_VERSION ) throw std::runtime_error("invalid version for boost::chrono::nanoseconds");
    boost::chrono::nanoseconds::rep count;
    ar >> count;
    nanos = boost::chrono::nanoseconds( count );
}

} // namespace serialization
} // namespace boost

template <class Archive>
void metric::serialize(Archive& ar, const unsigned int version)
{
    if ( version != VERSION ) throw std::runtime_error("metric: invalid class version");
    ar & device_name_;
    ar & start_time_;
    ar & duration_;
    ar & result_;
}

const boost::asio::deadline_timer::duration_type udp_instrumenter::NAGLE_PERIOD = boost::posix_time::milliseconds(500);

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

void udp_instrumenter::add_metric(metric const& metric)
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
        log4.infoStream() << "Successfully sent instrumentation datagram";
    }
    else
    {
        log4.infoStream() << "An error occurred while sending instrumentation datagram: " << ec.message();
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
