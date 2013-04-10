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

instrumenter::instrumenter( io_service& io_service, string const& server, string const& port)
: socket_( io_service )
, buf_size_( 0 )
, buf_( new vector< shared_ptr< string > > )
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

void instrumenter::add_metric(boost::shared_ptr<metric> pmetric)
{
    std::ostringstream oss;
    // NOTE: The boost serialization library provides three archive formats:
    // binary, text, and XML. Naturally, the binary format is the most efficient
    // but it is non-portable!! If there is ever a requirement to run the application
    // and instrumentation service on heterogeneous hardware, the text archive
    // format should be used.
    boost::archive::text_oarchive oa(oss);
    oa << *pmetric;
    boost::shared_ptr< string > buf( new string( oss.str() ) );


}
