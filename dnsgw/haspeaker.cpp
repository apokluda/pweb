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
    const boost::uint16_t VERSION = 0x0001;
    const boost::uint16_t VERSION_NBO = 0x0100; // The version number in network byte order
}

boost::uint8_t* compose_ha_query(boost::uint16_t const id, string const& name, boost::uint8_t* buf, boost::uint8_t const* const end)
{
    using namespace protocol_helper;

    buf = write_short(HA_QUERY, buf, end);
    buf = write_short(id, buf, end);
    buf = write_string(name, buf, end);

    return buf;
}

std::string remove_suffix(std::string const& name, std::string const& suffix)
{
    std::string const name_suffix = name.substr(name.length() - suffix.length());
    if ( name_suffix != suffix)
    {
        sstream ss;
        ss << "Name '" << name  << "' contains an invalid suffix: " << name_suffix;
    }
    return name.substr(0, name.length() - suffix.length());
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
        boost::uint16_t const version_from_server = ntohs( *reinterpret_cast< boost::uint16_t* >( recv_buf_.data() ) );
        if ( version_from_server == ha_protocol::VERSION )
        {
            // Version OK, all systems go!
            connected_ = true;

            // Start an async read to read replies from home agent
            async_read(socket_, recv_header_.buffer(),
                    strand_.wrap( boost::bind( &haspeaker::handle_reply_header_read, shared_from_this(), ph::error, ph::bytes_transferred ) ));
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

void haspeaker::handle_reply_header_read(bs::error_code const& ec, std::size_t const bytes_transferred)
{
    if ( !ec )
    {
        if ( recv_header_.msgid() == HA_REPLY )
        {
            if ( recv_header_.status() == HA_SUCCESS )
            {
                // Must read IP version and IP address
                async_read(socket_, buffer(&recv_ip_ver_, 1),
                        strand_.wrap( boost::bind( &haspeaker::handle_ip_ver_read, shared_from_this(), ph::error, ph::bytes_transferred ) ) );
            }
            else
            {
                // Failed to look up name
                query_map_t::iterator queryi = queries_.find(recv_header_.serial());
                if ( queryi != queries_.end() )
                {
                    log4.noticeStream() << "Received failure response from home agent at address " << haaddress_<< " for query with serial " << queryi->first;

                    queryi->second->rcode(R_NAME_ERROR);
                    queryi->second->send_reply();
                    queries_.erase(queryi);
                }
                else
                {
                    log4.warnStream() << "Received a query reply from home agent at address " << haaddress_ << " for query with serial " << recv_header_.serial() << " but corresponding query does not exist in map";
                }

                // Start an async read to read replies from home agent
                async_read(socket_, recv_header_.buffer(),
                        strand_.wrap( boost::bind( &haspeaker::handle_reply_header_read, shared_from_this(), ph::error, ph::bytes_transferred ) ));
            }
            return;
        }
        else
        {
            log4.errorStream() << "Received a reply with invalid message ID from home agent at address " << haaddress_;
            // Fall through to reconnect
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while reading query reply header from home agent at address " << haaddress_ << ": " << ec.message();
        // Fall through to reconnect
    }
    reconnect();
}

void haspeaker::handle_ip_ver_read(bs::error_code const& ec, std::size_t const bytes_transferred)
{
    if ( !ec)
    {
        std::size_t addr_len;
        if ( recv_ip_ver_ == 4) addr_len = 4;
        else if ( recv_ip_ver_ == 6 ) addr_len = 16;
        else
        {
            log4.errorStream() << "Received an invalid IP address version in reply message from home agent at address " << haaddress_;
            goto reconnect;
        }
        async_read(socket_, buffer(recv_buf_, addr_len),
                strand_.wrap( boost::bind( &haspeaker::handle_ip_read, shared_from_this(), ph::error, ph::bytes_transferred ) ) );
        return;
    }
    else
    {
        log4.errorStream() << "An error occurred while reading IP version in reply message from home agent at addres " << haaddress_ << ": " << ec.message();
        // Fall through
    }
reconnect:
    reconnect();
}

void haspeaker::handle_ip_read(bs::error_code const& ec, std::size_t const bytes_transferred)
{
    if ( !ec )
    {
        query_map_t::iterator queryi = queries_.find(recv_header_.serial());
        if ( queryi != queries_.end() )
        {
            log4.noticeStream() << "Received IP address from home agent at address " << haaddress_<< " for query with serial " << queryi->first;

            dnsrr rr;
            rr.owner = ns_name_;
            rr.rtype = recv_ip_ver_ == 4 ? T_A : T_AAAA;
            rr.rclass = C_IN;
            rr.ttl = ttl_;
            rr.rdlength = recv_ip_ver_ == 4 ? 4 : 16;
            memcpy(rr.rdata.data(), recv_buf_.data(), rr.rdlength);

            queryi->second->add_answer(rr);
            queryi->second->send_reply();
            queries_.erase(queryi);
        }
        else
        {
            log4.warnStream() << "Received a query reply from home agent at address " << haaddress_ << " for query with serial " << recv_header_.serial() << " but corresponding query does not exist in map";
        }
        // Start an async read to read replies from home agent
        async_read(socket_, recv_header_.buffer(),
                strand_.wrap( boost::bind( &haspeaker::handle_reply_header_read, shared_from_this(), ph::error, ph::bytes_transferred ) ));
    }
    else
    {
        log4.errorStream() << "An error occurred while reading IP address in reply message from home agent at address " << haaddress_;
        reconnect();
    }
}

void haspeaker::disconnect()
{
    connected_ = false;
    send_in_progress_ = false;

    bs::error_code shutdown_ec;
    socket_.shutdown(ip::tcp::socket::shutdown_both, shutdown_ec);
    if (!shutdown_ec) log4.errorStream() << "An error occurred while shutting down connection to home agent at address " << haaddress_;

    bs::error_code close_ec;
    socket_.close(close_ec);
    if (!close_ec) log4.errorStream() << "An error occurred while closing connection to home agent at address " << haaddress_;

    // Cancel all outstanding queries
    while( !query_queue_.empty() )
    {
        query_queue_.front()->rcode(R_SERVER_FAILURE);
        query_queue_.front()->send_reply();
        query_queue_.pop_front();
    }
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
        if ( send_in_progress_ )
        {
            if ( query_queue_.full() )
            {
                log4.warnStream() << "Query queue for home agent at address " << haaddress_ << " full. Returning failure to " << query->remote_udp_endpoint();
                query->rcode(R_SERVER_FAILURE);
                query->send_reply();
                return;
            }
            query_queue_.push_back( query );
            return;
        }
        send_in_progress_ = true;

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

        string device_name;
        try
        {
            device_name = remove_suffix(question.name, suffix_);
        }
        catch ( std::runtime_error const& e )
        {
            log4.noticeStream() << "Received a DNS query with an invalid name: " << e.what();
            query->rcode(R_NAME_ERROR);
            query->send_reply();
            return;
        }

        std::pair< query_map_t::iterator, bool > res = queries_.insert( std::make_pair( query->id(), query ) );
        if ( !res.second )
        {
            query->rcode(R_REFUSED);
            query->send_reply();
            return;
        }

        boost::uint8_t* const end = compose_ha_query(query->id(), device_name, send_buf_.data(), send_buf_.data() + send_buf_.size());

        async_write(socket_, buffer(send_buf_, end - send_buf_.data()),
                strand_.wrap(boost::bind( &haspeaker::handle_query_sent, shared_from_this(), ph::error, ph::bytes_transferred )) );
    }
    else
    {
        log4.errorStream() << "Unable to process query from " << query->remote_address() << ": disconnected from home agent at address " << haaddress_;
        query->rcode(R_SERVER_FAILURE);
        query->send_reply();
    }
}

void haspeaker::handle_query_sent(bs::error_code const& ec, std::size_t bytes_transferred)
{
    if ( !ec )
    {
        log4.infoStream() << "Successfully sent query to home agent at address " << haaddress_;
        // Fall through
    }
    else
    {
        log4.errorStream() << "An error occurred while sending query to home agent at address " << haaddress_ << ": " << ec.message();
        reconnect();
        return;
    }

    send_in_progress_ = false;

    if ( !query_queue_.empty() )
    {
        query_ptr query = query_queue_.front();
        query_queue_.pop_front();
        process_query_( query );
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
