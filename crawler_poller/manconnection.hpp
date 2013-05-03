/*
 * manconnection.hpp
 *
 *  Created on: 2013-05-02
 *      Author: apokluda
 */

#ifndef MANCONNECTION_HPP_
#define MANCONNECTION_HPP_

#include "stdhdr.hpp"

class manconnection : private boost::noncopyable
{
public:
    manconnection(boost::asio::io_service&, std::string const& mhostname, std::string const& mport);

private:
    void start();

    void handle_resolve( boost::system::error_code const&, boost::asio::ip::tcp::resolver::iterator );
    void handle_connect( boost::system::error_code const& );

    void reconnect();

    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket socket_;
    std::string const mhostname_;
    std::string const mport_;
};


#endif /* MANCONNECTION_HPP_ */
