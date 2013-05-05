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

class bufwrite;

class manconnection : private boost::noncopyable
{
public:
    manconnection(boost::asio::io_service&, std::string const& mhostname, std::string const& mport);

    void home_agent_discovered( std::string const& hostname )
    {
        bufwrite_->sendmsg( crawler_protocol::HOME_AGENT_DISCOVERED, hostname );
    }

private:
    void connect( boost::system::error_code const& ec = boost::system::error_code() );

    void handle_resolve( boost::system::error_code const&, boost::asio::ip::tcp::resolver::iterator );
    void handle_connect( boost::system::error_code const& );
    void handle_header_read( boost::system::error_code const&, std::size_t const bytes_transferred );
    void handle_home_agent_assignment( boost::system::error_code const&, std::size_t const bytes_transferred );

    void disconnect();
    void reconnect();

    // You could argue that this is pointless, but it hides the fact
    // that we're /not/ using smart pointers for this class. An
    // even better solution would be to create a base class like
    // boost::enable_shared_from_this that privides these method.
    manconnection_ptr get_this() { return this; }
    cmanconnection_ptr get_this() const { return this; }

    typedef boost::shared_ptr< boost::asio::ip::tcp::socket > socket_ptr;
    typedef boost::shared_ptr< bufwrite > bufwrite_ptr;

    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::deadline_timer retrytimer_;
    boost::asio::strand strand_;
    std::string const mhostname_;
    std::string const mport_;
    boost::posix_time::time_duration errwait_;
    socket_ptr socket_;
    bufwrite_ptr bufwrite_;
    crawler_protocol::header recv_header_;
    boost::array< boost::uint8_t, 256 > recv_buf_;
    bool connected_;
};

#endif /* MANCONNECTION_HPP_ */
