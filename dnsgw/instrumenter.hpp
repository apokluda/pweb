/*
 *  Copyright (C) 2013 Alexander Pokluda
 *
 *  This file is part of pWeb.
 *
 *  pWeb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pWeb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pWeb.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INSTRUMENTER_HPP_
#define INSTRUMENTER_HPP_

#include "stdhdr.hpp"

namespace instrumentation
{

class metric;

class instrumenter
{
public:
    void add_metric(metric const&);
    virtual ~instrumenter() {}

private:
    virtual void do_add_metric(metric const&) = 0;
};

class null_instrumenter : public instrumenter
{
private:
    virtual void do_add_metric(metric const&) {}
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

private:
    void do_add_metric(metric const& pmetric);
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

} // namespace instrumentation

#endif /* INSTRUMENTER_HPP_ */
