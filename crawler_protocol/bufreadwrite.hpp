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

#ifndef BUFITEM_HPP_
#define BUFITEM_HPP_

#include "stdhdr.hpp"
#include "messages.hpp"

typedef boost::shared_ptr< boost::asio::ip::tcp::socket > socket_shared_ptr;
typedef boost::asio::ip::tcp::socket* socket_raw_ptr;

template< typename socket_ptr, typename T >
class bufreadwritebase
{
};

template< typename T >
class bufreadwritebase< socket_shared_ptr, T > : public boost::enable_shared_from_this< T >
{
protected:
    // If the socket is shared, so are we!
    boost::shared_ptr< T > get_this() { return this->shared_from_this(); }
};

template< typename T >
class bufreadwritebase< socket_raw_ptr, T >
{
protected:
    // If the socket is not shared, neither are we!
    T* get_this() { return static_cast< T* >( this ); }
};

template< typename socket_ptr >
class bufwrite : private boost::noncopyable, public bufreadwritebase< socket_ptr, bufwrite< socket_ptr > >
{
    typedef std::pair< crawler_protocol::header, std::string > bufitem_t;
    typedef std::auto_ptr< bufitem_t > bufitem_ptr;
    typedef boost::ptr_deque< bufitem_t > queue_t;

public:
    typedef boost::function< void (crawler_protocol::message_type const type, std::string const&) > cb_t;

    bufwrite( socket_ptr socket )
    : strand_( socket->get_io_service() )
    , socket_( socket )
    , send_in_progress_( false )
    {
    }

    void sendmsg( crawler_protocol::message_type const type, std::string const& str );

    void reset();

    void error_handler(cb_t cb) { errcb_ = cb; }
    void success_handler(cb_t cb) { succb_ = cb; }

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
    cb_t errcb_;
    cb_t succb_;
    socket_ptr socket_;
    bool send_in_progress_;
};

template< typename socket_ptr >
class bufread : private boost::noncopyable, public bufreadwritebase< socket_ptr, bufread< socket_ptr > >
{
public:
    typedef boost::function< void () > errcb_t;

    bufread( socket_ptr socket )
    : socket_( socket )
    {
    }

    // Note: All handler's should be added before calling start and should not be touched thereafter!
    // (This class lacks the syncronization/logic needed to handle changing callbacks).
    void add_handler( crawler_protocol::message_type const type, boost::function< void (std::string const& str) > cb ) { cbmap_[type] = cb; }
    void start() { read_header(); }

    void error_handler(errcb_t errcb) { errcb_ = errcb; }

private:
    void read_header();
    void handle_header_read( boost::system::error_code const& ec, std::size_t const bytes_transferred );
    void handle_body_read( boost::system::error_code const& ec, std::size_t const bytes_transferred );


    typedef std::map< crawler_protocol::message_type, boost::function< void (std::string const& str) > > cbmap_t;

    cbmap_t cbmap_;
    errcb_t errcb_;
    socket_ptr socket_;
    crawler_protocol::header header_;
    boost::array< boost::uint8_t, 256 > buf_;
};

#endif /* BUFITEM_HPP_ */
