/*
 * haspeaker.cpp
 *
 *  Created on: 2013-02-23
 *      Author: alex
 */

#include "stdhdr.hpp"
#include "haspeaker.hpp"
#include "dnsquery.hpp"
#include "protocol_helper.hpp"

extern log4cpp::Category& log4;

typedef std::stringstream sstream;
using std::vector;
using std::string;
using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

namespace ha_protocol
{
    const uint16_t VERSION = 1;
    const uint16_t VERSION_NBO = htons(VERSION); // The version number in network byte order
}

enum msgid_t
{
    HA_QUERY = 1,
    HA_REPLY = 2
};

enum status_t
{
    HA_SUCCESS = 0,
    HA_GENERAL_FAILURE = 1
};

uint8_t* compose_ha_query(dnsquery const& query, string const& suffix, uint8_t* buf, uint8_t const* const end)
{
    using namespace protocol_helper;

    buf = write_short(HA_QUERY, buf, end);


    return buf;
}

void haspeaker::connect(bs::error_code const& ec)
{
    if ( !ec )
    {
        vector<string> address;
        boost::split( address, haaddress_, boost::is_any_of(":") );
        if ( address.size() == 2 )
        {
            ip::tcp::resolver::query query(address[0], address[1]);
            resolver_.async_resolve(query,
                    boost::bind(&haspeaker::handle_resolve, shared_from_this(), ph::error, ph::iterator));
        }
        else
        {
            log4.errorStream() << "Invalid home agent address format: " << haaddress_;
            log4.noticeStream() << "Unable to connect to home agent at address " << haaddress_;
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while the connection retry timer was running for connection to home agent at address " << haaddress_;
        log4.noticeStream() << "No longer trying to connect to home agent at address " << haaddress_;
    }
}

void haspeaker::handle_resolve(bs::error_code const& ec, ip::tcp::resolver::iterator iter)
{
    if ( !ec )
    {
        async_connect(socket_, iter,
                boost::bind(&haspeaker::handle_connect, shared_from_this(), ph::error));
    }
    else
    {
        log4.errorStream() << "Unable to resolve home agent address " << haaddress_ << ": " << ec.message();
        log4.noticeStream() << "No longer trying to connect to home agent at address " << haaddress_;
    }
}

void haspeaker::handle_connect(bs::error_code const& ec)
{
    using boost::posix_time::seconds;

    if ( !ec )
    {
        // Connected to home agent
        log4.infoStream() << "Connected to home agent at address " << haaddress_;

        errwait_ = seconds(0); // reset connection error timer

        // Hand shake consists of sending an 2-octet version number
        // The server should echo the same version number back

        async_write(socket_, buffer(&ha_protocol::VERSION_NBO, sizeof(ha_protocol::VERSION_NBO)),
                boost::bind( &haspeaker::handle_version_sent, shared_from_this(), ph::error, ph::bytes_transferred ));
    }
    else
    {
        log4.errorStream() << "Unable to connect to home agent at address " << haaddress_ << ": " << ec.message();
        reconnect();
    }
}

void haspeaker::handle_version_sent(bs::error_code const& ec, std::size_t const bytes_transferred)
{
    if ( !ec )
    {
        // Read version sent by server
        async_read(socket_, buffer(recv_buf_, sizeof(ha_protocol::VERSION_NBO)),
                boost::bind( &haspeaker::handle_version_received, shared_from_this(), ph::error, ph::bytes_transferred ));
    }
    else
    {
        log4.errorStream() << "An error occurred while sending protocol version to home agent at address " << haaddress_ << ": " << ec.message();
        reconnect();
    }
}

void haspeaker::handle_version_received(bs::error_code const& ec, std::size_t const bytes_transferred)
{
    if ( !ec )
    {
        // Verify protocol version matches
        uint16_t const version_from_server = ntohs( *reinterpret_cast< uint16_t* >( recv_buf_.data() ) );
        if ( version_from_server == ha_protocol::VERSION )
        {
            // Version OK, all systems go!
            connected_ = true;
        }
        else
        {
            // Oh no! Not good.
            log4.errorStream() << "Received unsupported protocol version from home agent at address " << haaddress_ << ": " << version_from_server;
            disconnect();
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while receiving protocol version from home agent at address " << haaddress_ << ": " << ec.message();
        reconnect();
    }
}

void haspeaker::disconnect()
{
    bs::error_code shutdown_ec;
    socket_.shutdown(ip::tcp::socket::shutdown_both, shutdown_ec);
    if (!shutdown_ec) log4.errorStream() << "An error occurred while shutting down connection to home agent at address " << haaddress_;

    bs::error_code close_ec;
    socket_.close(close_ec);
    if (!close_ec) log4.errorStream() << "An error occurred while closing connection to home agent at address " << haaddress_;

    connected_ = false;
    send_in_progress_ = false;
}

void haspeaker::reconnect()
{
    disconnect();

    using boost::posix_time::seconds;

    if (errwait_ < seconds(10)) errwait_ += seconds(2);

    log4.noticeStream() << "Retrying connection to " << haaddress_ << " in " << errwait_;

    retrytimer_.expires_from_now(errwait_);
    retrytimer_.async_wait(boost::bind( &haspeaker::connect, shared_from_this(), ph::error ));
}

void haspeaker::process_query_( query_ptr query)
{
    if ( connected_ )
    {
        // NOTE: At the moment, we only support looking up the first name in a DNS query!
        // This shouldn't be a problem, because in our use case, DNS queries should
        // contain only one name to resolve.

        if (query->num_questions() == 0)
        {
            log4.noticeStream() << "Received a DNS query with no question from " << query->remote_address();
            // Just ignore it?
            return;
        }

        if (query->num_questions() > 1)
        {
            log4.noticeStream() << "Received a DNS query with " << query->num_questions() << " from " << query->remote_address() << " (will only respond to first question)";
        }

        dnsquestion const& question = *query->questions_begin();

        if (question.qtype != T_A || question.qclass != C_IN)
        {
            log4.noticeStream() << "Received a DNS query with unsupported qtype or qclass from " << query->remote_address();
            query->rcode(R_NOT_IMPLEMENTED);
            query->send_reply();
            return;
        }

        string device_name = remove_suffix(question.name, suffix_);

        // Add query to map with based on ID with 31 second timer [we are in a strand, use ordinary map?]
        // (fail if id already exists in map)

        // Compose message in buffer

        // Async send
    }
    else
    {
        log4.errorStream() << "Unable to process query from " << query->remote_address() << ": disconnected from home agent at address " << haaddress_;
        query->rcode(R_SERVER_FAILURE);
        query->send_reply();
    }
}

template < class hacontainer_iter_t, class hacontainer_diff_t >
void ha_load_balancer< hacontainer_iter_t, hacontainer_diff_t >::process_query( query_ptr query )
{
    for (hacontainer_diff_t cnt = 0; cnt < size_; ++cnt)
    {
        hacontainer_diff_t my_offset = offset_.fetch_add(1, boost::memory_order_relaxed);
        hacontainer_iter_t hs = begin_ + (my_offset % size_);
        if ( (*hs)->connected() )
        {
            (*hs)->process_query( query );
            return;
        }
    }
    log4.warnStream() << "Unable to process query from " << query->remote_address() << ": Not connected to any home agents";

    query->rcode(R_SERVER_FAILURE);
    query->send_reply();
}

typedef boost::shared_ptr< haspeaker > haspeaker_ptr;
typedef std::vector< haspeaker_ptr > haspeakers_t;
template class ha_load_balancer< haspeakers_t::iterator, haspeakers_t::difference_type >;
