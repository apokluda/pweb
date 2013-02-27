/*
 * dnsspeaker.hpp
 *
 *  Created on: 2013-02-20
 *      Author: apokluda
 */

#ifndef DNSSPEAKER_HPP_
#define DNSSPEAKER_HPP_

#include "stdhdr.hpp"
#include "dnsquery.hpp"

class dns_query_header
{
    // 4.1.1. Header section format
    //
    //  The header contains the following fields:
    //
    // 1  1  1  1  1  1
    // 0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                      ID                       |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                    QDCOUNT                    |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                    ANCOUNT                    |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                    NSCOUNT                    |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                    ARCOUNT                    |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //
    // http://tools.ietf.org/html/rfc1035

public:

    dns_query_header()
    {
        // Just to be safe... although this is really redundant
        // since the code that default constructs the header object
        // immediately overwrites its data with that read from the network
        clear();
    }

    void clear()
    {
        memset( &buf_, 0, sizeof( buf_ ) );
    }

    static std::size_t length()
    {
        return HEADER_LENGTH;
    }

    boost::asio::mutable_buffers_1 buffer()
    {
        return boost::asio::buffer( buf_ );
    }

    boost::uint16_t id() const
    {
        return read_short(0);
    }

    void id( boost::uint16_t const id)
    {
        write_short(0, id);
    }

    bool qr() const
    {
        return get_bit(2, 0x80);
    }

    void qr( bool const qr )
    {
        set_bit(2, 0x80, qr);
    }

    opcode_t opcode() const
    {
        unsigned val = (*(buf_.data() + 2) & 0x78) >> 3;
        switch ( val )
        {
        case 0:
            return O_QUERY;
        case 1:
            return O_IQUERY;
        case 2:
            return O_STATUS;
        default:
            return O_RESERVED;
        }
    }

    void opcode( opcode_t const code )
    {
        *(buf_.data() + 2 ) |= static_cast< boost::uint8_t >( code << 3 );
    }

    bool aa() const
    {
        return get_bit(2, 0x04);
    }

    void aa( bool const aa )
    {
        set_bit(2, 0x04, aa);
    }

    bool tc() const
    {
        return get_bit(2, 0x02);
    }

    void tc( bool const tc )
    {
        set_bit(2, 0x02, tc);
    }

    bool rd() const
    {
        return get_bit(2, 0x01);
    }

    void rd( bool const rd )
    {
        set_bit(2, 0x01, rd);
    }

    bool ra() const
    {
        return get_bit(3, 0x80);
    }

    void ra( bool const ra )
    {
        set_bit(3, 0x80, ra);
    }

    int z() const
    {
        return *(buf_.data() + 3) & 0x70;
    }

    rcode_t rcode() const
    {
        unsigned val = *(buf_.data() + 3) & 0x0F;
        switch ( val )
        {
        case 0:
            return R_SUCCESS;
        case 1:
            return R_FORMAT_ERROR;
        case 2:
            return R_SERVER_FAILURE;
        case 3:
            return R_NAME_ERROR;
        case 4:
            return R_NOT_IMPLEMENTED;
        case 5:
            return R_REFUSED;
        default:
            return R_RESERVED;
        }
    }

    void rcode( rcode_t const code )
    {
        *(buf_.data() + 3 ) |= static_cast< boost::uint8_t >( code );
    }

    boost::uint16_t qdcount() const
    {
        return read_short(4);
    }

    void qdcount( boost::uint16_t const val )
    {
        write_short(4, val);
    }

    boost::uint16_t ancount() const
    {
        return read_short(6);
    }

    void ancount( boost::uint16_t const val )
    {
        write_short(6, val);
    }

    boost::uint16_t nscount() const
    {
        return read_short(8);
    }

    void nscount( boost::uint16_t const val )
    {
        write_short(8, val);
    }

    boost::uint16_t arcount() const
    {
        return read_short(10);
    }

    void arcount( boost::uint16_t const val )
    {
        write_short(10, val);
    }

private:
    inline bool get_bit( int byte, boost::uint8_t mask ) const
    {
        return (*(buf_.data() + byte) & mask) != 0;
    }

    void set_bit( int byte, boost::uint8_t mask, bool val )
    {
        if ( val )
            *(buf_.data() + byte) |= mask;
        else
            *(buf_.data() + byte) &= ~mask;
    }

    inline boost::uint16_t read_short( int offset ) const
    {
        return ntohs( *reinterpret_cast< boost::uint16_t const* >( buf_.data() + offset ) );
    }

    void write_short( int offset, boost::uint16_t const val )
    {
        *reinterpret_cast< boost::uint16_t* >( buf_.data() + offset ) = htons(val);
    }

    static size_t const HEADER_LENGTH = 12;
    boost::array< boost::uint8_t, HEADER_LENGTH > buf_;
};

class udp_dnsspeaker
{
public:
    udp_dnsspeaker(boost::asio::io_service& io_service, boost::function<void (query_ptr)> processor, std::string const& iface, boost::uint16_t port);

    void start();

    void send_reply( query_ptr query );

private:
    void handle_datagram_received( boost::system::error_code const& ec, std::size_t const bytes_transferred );

    void send_reply_( query_ptr query );

    void handle_send_reply( boost::system::error_code const& ec );

    boost::function<void (query_ptr)> processor_;
    boost::asio::ip::udp::endpoint sender_endpoint_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::io_service::strand strand_;
    boost::circular_buffer< query_ptr > reply_buf_;
    dns_query_header recv_header_;
    dns_query_header send_header_;
    boost::array< boost::uint8_t, 500 > recv_buf_; // max size defined in DNS proto spec
    boost::array< boost::uint8_t, 500 > send_buf_; // max size defined in DNS proto spec
    boost::array< boost::asio::mutable_buffer, 2 > recv_buf_arr_;
    boost::array< boost::asio::mutable_buffer, 2 > send_buf_arr_;
    bool send_in_progress_;
};

class dns_connection
    : public boost::enable_shared_from_this< dns_connection >,
      private boost::noncopyable
{
    friend class tcp_dnsspeaker;
public:
    dns_connection(boost::asio::io_service& io_service, boost::function<void (query_ptr)>& processor)
    : processor_( processor )
    , socket_( io_service )
    , strand_( io_service )
    , reply_buf_( 5 )
    , recv_msg_len_( 0 )
    , send_msg_len_( 0 )
    , send_in_progress_( false )
    {
        recv_buf_arr_[0] = recv_header_.buffer();

        send_buf_arr_[0] = boost::asio::buffer( &send_msg_len_, 2 );
        send_buf_arr_[1] = send_header_.buffer();
    }

    void start();

    boost::asio::ip::tcp::endpoint remote_endpoint() const
    {
        // Assume that socket::remote_endpoint() is thread-safe
        return socket_.remote_endpoint();
    }

    void send_reply( query_ptr query );

private:
    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void handle_msg_len_read(boost::system::error_code const&, std::size_t const);
    void handle_query_read(boost::system::error_code const&, std::size_t const);

    void send_reply_( query_ptr query );

    void handle_send_reply(boost::system::error_code const&);

    boost::function<void (query_ptr)> processor_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand strand_;
    boost::circular_buffer< query_ptr > reply_buf_;
    dns_query_header recv_header_;
    dns_query_header send_header_;
    boost::array< boost::uint8_t, 1024> recv_buf_;
    boost::array< boost::uint8_t, 1024> send_buf_;
    boost::array< boost::asio::mutable_buffer, 2 > recv_buf_arr_;
    boost::array< boost::asio::mutable_buffer, 3 > send_buf_arr_;

    boost::uint16_t recv_msg_len_;
    boost::uint16_t send_msg_len_;
    bool send_in_progress_;
};

typedef boost::shared_ptr< dns_connection > dns_connection_ptr;

class tcp_dnsspeaker
{
public:
    tcp_dnsspeaker(boost::asio::io_service& io_service, boost::function<void (query_ptr)> processor, std::string const& iface, boost::uint16_t port);

    void start();

private:
    void handle_accept(boost::system::error_code const& ec);

    boost::function<void (query_ptr)> processor_;
    dns_connection_ptr new_connection_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

#endif /* DNSSPEAKER_HPP_ */
