/*
 * manconnection.hpp
 *
 *  Created on: 2013-05-02
 *      Author: apokluda
 */

#ifndef MANCONNECTION_HPP_
#define MANCONNECTION_HPP_

#include "stdhdr.hpp"
#include "messages.hpp"
#include "bufreadwrite.hpp"

class manconnection;
typedef manconnection* manconnection_ptr;
typedef manconnection const* cmanconnection_ptr;

class manconnection : private boost::noncopyable
{
public:
    manconnection(boost::asio::io_service&, std::string const& mhostname, std::string const& mport);

    void home_agent_discovered( std::string const& hostname )
    {
        if ( connected_ ) bufwrite_.sendmsg( crawler_protocol::HOME_AGENT_DISCOVERED, hostname );
        else send_failure( crawler_protocol::HOME_AGENT_ASSIGNMENT, hostname );
    }

private:
    void connect( boost::system::error_code const& ec = boost::system::error_code() );

    void handle_resolve( boost::system::error_code const&, boost::asio::ip::tcp::resolver::iterator );
    void handle_connect( boost::system::error_code const& );

    void disconnect();
    void reconnect();

    void send_success( crawler_protocol::message_type const, std::string const& );
    void send_failure( crawler_protocol::message_type const, std::string const& );

    typedef boost::asio::ip::tcp::socket* socket_ptr;

    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::deadline_timer retrytimer_;
    std::string const mhostname_;
    std::string const mport_;
    boost::posix_time::time_duration errwait_;
    boost::asio::ip::tcp::socket socket_;
    bufread< socket_ptr > bufread_;
    bufwrite< socket_ptr > bufwrite_;
    bool connected_;
};

#endif /* MANCONNECTION_HPP_ */
