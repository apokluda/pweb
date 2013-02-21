/*
 * dnsspeaker.hpp
 *
 *  Created on: 2013-02-20
 *      Author: apokluda
 */

#ifndef DNSSPEAKER_HPP_
#define DNSSPEAKER_HPP_

#include "stdhdr.h"

class dnsspeaker : private boost::noncopyable
{
public:
    explicit dnsspeaker(boost::asio::io_service& io_service, std::string const& iface, uint16_t port, size_t num_threads);

private:
    boost::asio::ip::udp::socket udp_socket_;
    boost::asio::ip::tcp::socket tcp_socket_;
};

#endif /* DNSSPEAKER_HPP_ */
