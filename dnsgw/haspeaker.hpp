/*
 * haspeaker.hpp
 *
 *  Created on: 2013-02-23
 *      Author: alex
 */

#ifndef HASPEAKER_HPP_
#define HASPEAKER_HPP_

#include "stdhdr.hpp"

extern log4cpp::Category& log4;

class dnsquery;
typedef boost::shared_ptr< dnsquery > query_ptr;

enum msgid_t
{
    HA_QUERY = 1,
    HA_REPLY = 2,
    HA_UNKNOWN
};

enum status_t
{
    HA_SUCCESS = 0,
    HA_GENERAL_FAILURE = 1
};

class ha_reply_header
{
    // Example reply message header:
    //
    // Msg ID  Ser #   Stts
    // +---+---+---+---+---+
    // |     1 | 24513 | 1 |
    // +---+---+---+---+---+
    //
    // See the protocol spec in the Google Doc for more info

public:
    static std::size_t length()
    {
        return HEADER_LENGTH;
    }

    boost::asio::mutable_buffers_1 buffer()
    {
        return boost::asio::buffer( buf_ );
    }

    msgid_t msgid() const
    {
        uint16_t msgid = read_short(0);
        if ( msgid == HA_QUERY ) return HA_QUERY;
        if ( msgid == HA_REPLY ) return HA_REPLY;
        return HA_UNKNOWN;
    }

    uint16_t serial() const
    {
        return read_short(2);
    }

    status_t status() const
    {
        if ( buf_[4] == HA_SUCCESS ) return HA_SUCCESS;
        return HA_GENERAL_FAILURE;
    }

private:
    inline boost::uint16_t read_short( int offset ) const
    {
        return ntohs( *reinterpret_cast< boost::uint16_t const* >( buf_.data() + offset ) );
    }


    static std::size_t const HEADER_LENGTH = 5;
    boost::array< boost::uint8_t, HEADER_LENGTH > buf_;
};

class haspeaker
: private boost::noncopyable
, public boost::enable_shared_from_this<haspeaker>
{
public:
    haspeaker(boost::asio::io_service& io_service,
            std::string const& haaddr,
            std::string const& ns_name,
            uint16_t const ttl,
            std::string const& suffix)
    : haaddress_(haaddr)
    , ns_name_(ns_name)
    , suffix_(suffix)
    , strand_(io_service)
    , resolver_(io_service)
    , retrytimer_(io_service)
    , socket_(io_service)
    , ttl_(ttl)
    , connected_(false)
    , send_in_progress_(false)
    {
    }

    bool connected() const
    {
        return connected_;
    }

    void process_query( query_ptr query )
    {
        strand_.dispatch(
                boost::bind( &haspeaker::process_query_, shared_from_this(), query ) );
    }

    void connect( boost::system::error_code const& ec = boost::system::error_code() );

private:
    void handle_resolve(boost::system::error_code const& ec, boost::asio::ip::tcp::resolver::iterator iter);
    void handle_connect(boost::system::error_code const& ec);
    void handle_version_sent(boost::system::error_code const& ec, std::size_t const bytes_transferred);
    void handle_version_received(boost::system::error_code const& ec, std::size_t const bytes_transferred);

    void handle_reply_header_read(boost::system::error_code const& ec, std::size_t const bytes_transferred);
    void handle_ip_ver_read(boost::system::error_code const& ec, std::size_t const bytes_transferred);
    void handle_ip_read(boost::system::error_code const& ec, std::size_t const bytes_transferred);

    void process_query_( query_ptr query );
    void handle_query_sent(boost::system::error_code const& ec, std::size_t const bytes_transferred);

    void disconnect();
    void reconnect();

    typedef std::map< boost::uint16_t, query_ptr > query_map_t;
    query_map_t queries_;
    boost::circular_buffer< query_ptr > query_queue_;
    boost::posix_time::time_duration errwait_;
    std::string haaddress_;
    std::string ns_name_;
    std::string suffix_;
    boost::asio::io_service::strand strand_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::deadline_timer retrytimer_;
    boost::asio::ip::tcp::socket socket_;
    uint16_t ttl_;
    boost::array<uint8_t, 2 + 2 + 2 + 255 > send_buf_; // Max message size in version 1 of protocol (max DNS name length = 255)
    ha_reply_header recv_header_;
    uint8_t recv_ip_ver_;
    boost::array<uint8_t, 16 > recv_buf_; // Max message body size is 16 bytes for IPv6 address
    bool connected_; // potentially accessed concurrently, but *should* be ok
    bool send_in_progress_;
};

// In future, the ha_load_balancer could be made into a
// class hierarchy with subclasses implementing different
// load balancing algorithms. The algorithm/subclass to
// instantiate could be selected from a config parameter.

// hacontainer_iter_t = random access iterator
// Note: right now, we're using plain but const STL containers.
// If we want to add/remove on the fly, then we will need to use
// concurrent containers, such as the ones provide by Intel TBB

// Perhaps the number of template parameters could be reduced from 2
// down to 1 with some sort of traits magic, but this is fine for now...
template < class hacontainer_iter_t, class hacontainer_diff_t >
class ha_load_balancer
{
public:
    ha_load_balancer( boost::asio::io_service& io_service, hacontainer_iter_t const haspeakers_begin, std::size_t const num_elements)
    : offset_(0)
    , begin_(haspeakers_begin)
    , size_(num_elements)
    {
    }

    void process_query( query_ptr query );

private:
    boost::atomic< hacontainer_diff_t > mutable offset_;
    hacontainer_iter_t const begin_;
    hacontainer_diff_t const size_;
};

#endif /* HASPEAKER_HPP_ */
