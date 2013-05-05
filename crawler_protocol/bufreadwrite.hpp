/*
 * bufitem.hpp
 *
 *  Created on: 2013-05-04
 *      Author: alex
 */

#ifndef BUFITEM_HPP_
#define BUFITEM_HPP_

#include "stdhdr.hpp"
#include "messages.hpp"

class bufwrite : private boost::noncopyable, public boost::enable_shared_from_this< bufwrite >
{
    typedef std::pair< crawler_protocol::header, std::vector< boost::uint8_t > > bufitem_t;
    typedef std::auto_ptr< bufitem_t > bufitem_ptr;
    typedef boost::ptr_deque< bufitem_t > queue_t;

public:
    typedef boost::shared_ptr< boost::asio::ip::tcp::socket > socket_ptr;

    bufwrite( boost::asio::io_service& io_service,  socket_ptr& socket )
    : strand_( io_service )
    , socket_( socket )
    , send_in_progress_( false )
    {
    }

    void sendmsg( crawler_protocol::message_type const type, std::string const& str );

    void reset();

private:
    inline boost::asio::ip::tcp::endpoint remote_endpoint() const
    {
        boost::system::error_code ec;
        return socket_->remote_endpoint( ec );
    }

    void send_bufitem(bufitem_t* bufitem);
    void handle_bufitem_sent( boost::system::error_code const& ec, std::size_t const bytes_transferred, bufitem_t* );

    queue_t send_queue_;
    boost::asio::strand strand_;
    socket_ptr socket_;
    bool send_in_progress_;
};


#endif /* BUFITEM_HPP_ */
