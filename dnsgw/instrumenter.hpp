/*
 * instrumenter.hpp
 *
 *  Created on: 2013-04-08
 *      Author: apokluda
 */

#ifndef INSTRUMENTER_HPP_
#define INSTRUMENTER_HPP_

#include "stdhdr.hpp"

class metric;

class instrumenter
{
public:
    virtual ~instrumenter() {}
    virtual void add_metric(metric const&) = 0;
};

class null_instrumenter : public instrumenter
{
public:
    virtual void add_metric(metric const&) {}
};

// Instances of "metric" will be added to the instrumenter, which will
// package one or more serialzied metric instances into a UDP packet
// and send them to an instrumentation daemon (possibly using a Nagle timer)
// to reduce the number of datagrams/packets sent).
class udp_instrumenter : public instrumenter
{
    typedef boost::shared_ptr< std::vector< std::string > > buf_ptr_t;

public:
    udp_instrumenter( boost::asio::io_service& io_service, std::string const& server, std::string const& port);

    void add_metric(metric const& pmetric);

private:
    void add_metric_(std::string const& buf);

    void send_now();
    void handle_datagram_sent(buf_ptr_t, boost::system::error_code const&, std::size_t const);
    void start_timer();
    void handle_nagle_timeout(boost::system::error_code const&);

    static const boost::asio::deadline_timer::duration_type NAGLE_PERIOD;
    static const std::size_t MAX_BUF_SIZE = 65507; // practical limit for UDP datagram over IPv4

    boost::asio::ip::udp::socket socket_;
    boost::asio::strand strand_;
    boost::asio::deadline_timer timer_;
    std::size_t buf_size_; // the number of bytes currently in the buffer
    buf_ptr_t buf_;
};

#endif /* INSTRUMENTER_HPP_ */
