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
    enum opcode_t
    {
        O_QUERY,
        O_IQUERY,
        O_STATUS,
        O_RESERVED
    };

    enum rcode_t
    {
        R_SUCCESS,
        R_FORMAT_ERROR,
        R_SERVER_FAILURE,
        R_NAME_ERROR,
        R_NOT_IMPLEMENTED,
        R_REFUSED,
        R_RESERVED
    };

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
        return *(buf_.data() + byte) & mask;
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

class dns_connection
    : public boost::enable_shared_from_this< dns_connection >,
      private boost::noncopyable
{
public:
    dns_connection(boost::asio::io_service& io_service)
    : socket_(io_service)
    {
        buf_arr_[0] = header_.buffer();
    }

    void start();

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void send_reply( query_ptr query );

private:
    void handle_msg_len_read(boost::system::error_code const&, std::size_t const);
    void handle_query_read(boost::system::error_code const&, std::size_t const);

    boost::asio::ip::tcp::socket socket_;
    boost::array< boost::uint8_t, 1024> buf_;
    boost::array< boost::asio::mutable_buffer, 2 > buf_arr_;
    dns_query_header header_;
    boost::uint16_t msg_len_;
};

typedef boost::shared_ptr< dns_connection > dns_connection_ptr;

class dnsspeaker : private boost::noncopyable
{
protected:
    dnsspeaker() {}
};

class udp_dnsspeaker : public dnsspeaker
{
public:
    udp_dnsspeaker(boost::asio::io_service& io_service, std::string const& iface, boost::uint16_t port);

    void start();

    void send_reply( query_ptr query );

private:
    void handle_datagram_received( boost::system::error_code const& ec, std::size_t const bytes_transferred );

    boost::asio::ip::udp::endpoint sender_endpoint_;
    boost::asio::ip::udp::socket socket_;
    dns_query_header header_;
    boost::array< boost::uint8_t, 500 > buf_; // max size defined in DNS proto spec
    boost::array< boost::asio::mutable_buffer, 2 > buf_arr_;
};

class tcp_dnsspeaker : public dnsspeaker
{
public:
    tcp_dnsspeaker(boost::asio::io_service& io_service, std::string const& iface, boost::uint16_t port);

    void start();

private:
    void handle_accept(boost::system::error_code const& ec);

    boost::asio::io_service& io_service_;
    dns_connection_ptr new_connection_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

#endif /* DNSSPEAKER_HPP_ */
