/*
 * pollerconnector.hpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
 */

#ifndef POLLERCONNECTOR_HPP_
#define POLLERCONNECTOR_HPP_

#include "stdhdr.hpp"

class pollerconnection;

class pollerconnector :
        private boost::noncopyable
{
public:
    pollerconnector(boost::asio::io_service& io_service, std::string const& interface, boost::uint16_t const port, boost::posix_time::time_duration interval);

    void start();

private:
    void handle_accept(boost::system::error_code const& ec);

    typedef boost::shared_ptr< pollerconnection > recv_connection_ptr;

    recv_connection_ptr new_connection_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

#endif /* POLLERCONNECTOR_HPP_ */
