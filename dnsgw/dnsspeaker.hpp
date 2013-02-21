/*
 * dnsspeaker.hpp
 *
 *  Created on: 2013-02-20
 *      Author: apokluda
 */

#ifndef DNSSPEAKER_HPP_
#define DNSSPEAKER_HPP_

#include "stdhdr.h"

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
    enum opcode
    {
        O_QUERY,
        O_IQUERY,
        O_STATUS,
        O_RESERVED
    };

    enum rcode
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

    uint16_t id() const
    {
        return read_short(0);
    }

    void id( uint16_t const id)
    {
        write_short(0, id);
    }

    bool query() const
    {
        return get_bit(2, 0x80);
    }

    void query( bool const query )
    {
        set_bit(2, 0x80, query);
    }

    opcode opcode() const
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

    void opcode( opcode const code )
    {
        *(buf_.data() + 2 ) |= static_cast< uint8_t >( code << 3 );
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

    rcode rcode() const
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

    void rcode( rcode const code )
    {
        *(buf_.data() + 3 ) |= static_cast< uint8_t >( code );
    }

    uint16_t qdcount() const
    {
        return read_short(4);
    }

    void qdcount( uint16_t const val )
    {
        write_short(4, val);
    }

    uint16_t ancount() const
    {
        return read_short(6);
    }

    void ancount( uint16_t const val )
    {
        write_short(6, val);
    }

    uint16_t nscount() const
    {
        return read_short(8);
    }

    void nscount( uint16_t const val )
    {
        write_short(8, val);
    }

    uint16_t arcount() const
    {
        return read_short(10);
    }

    void arcount( uint16_t const val )
    {
        write_short(10, val);
    }

private:
    inline bool get_bit( int byte, uint8_t mask ) const
    {
        return *(buf_.data() + byte) & mask;
    }

    void set_bit( int byte, uint8_t mask, bool val )
    {
        if ( val )
            *(buf_.data() + byte) |= mask;
        else
            *(buf_.data() + byte) &= ~mask;
    }

    inline uint16_t read_short( int offset ) const
    {
        return ntohs( *reinterpret_cast< uint16_t const* >( buf_.data() + offset ) );
    }

    void write_short( int offset, uint16_t const val )
    {
        *reinterpret_cast< uint16_t* >( buf_.data() + offset ) = htons(val);
    }

    static size_t const HEADER_LENGTH = 12;
    boost::array< uint8_t, HEADER_LENGTH > buf_;
};

class dnsspeaker : private boost::noncopyable
{
protected:
    dnsspeaker() {}


};

class udp_dnsspeaker : public dnsspeaker
{
public:
    explicit udp_dnsspeaker(boost::asio::io_service& io_service, std::string const& iface, uint16_t port);

    void start();

private:
    boost::asio::ip::udp::endpoint sender_endpoint_;
    boost::asio::ip::udp::socket socket_;
    dns_query_header header_;
};

class tcp_dnsspeaker : public dnsspeaker
{
public:
    explicit tcp_dnsspeaker(boost::asio::io_service& io_service, std::string const& iface, uint16_t port);

    void start();

private:
    boost::asio::ip::tcp::socket socket_;
};

#endif /* DNSSPEAKER_HPP_ */
