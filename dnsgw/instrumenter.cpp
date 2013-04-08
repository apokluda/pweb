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

extern log4cpp::Category& log4;

instrumenter::instrumenter( io_service& io_service, string const& server, string const& port)
: socket_( io_service )
, buf_size_( 0 )
{
    ip::udp::resolver resolver( io_service_ );
    ip::udp::resolver::query const query( server, port );
    // NOTE: SYNCRONOUS OPERATION!
    // At this point, this is the only blocking network operation in the program.
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
    boost::archive::binary_archive oa(oss);
    oa << *pmetric;
    std::size_t const size = oss.tellp();
    boost::shared_ptr<boost::uint8_t[size]> buf( new boost::uint8_t[size] );
    memcpy(&(*buf), oss.str().data(), size);
}
