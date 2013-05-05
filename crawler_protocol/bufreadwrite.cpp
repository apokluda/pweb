#include "bufreadwrite.hpp"
#include "protocol_helper.hpp"

using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

extern log4cpp::Category& log4;

template < typename socket_ptr >
void bufwrite< socket_ptr >::sendmsg( crawler_protocol::message_type const type, std::string const& str )
{
    // Note: calls to this function are not serialized!
    bufitem_ptr bufitem( new bufitem_t );
    bufitem->first.type( type );
    std::size_t const length = str.length() + 1; // +1 for null character
    bufitem->first.length( length );
    bufitem->second = str;
    strand_.dispatch( boost::bind( &bufwrite::send_bufitem, this->get_this(), bufitem.release() ) );
}

template < typename socket_ptr >
void bufwrite< socket_ptr >::send_bufitem( bufitem_t* raw_bufitem )
{
    // Calls to this function are serialized using the strand
    bufitem_ptr bufitem( raw_bufitem );
    if ( !send_in_progress_ )
    {
        log4.debugStream() << "Sending bufitem to " << remote_endpoint();

        send_in_progress_ = true;
        boost::array< const_buffer, 2 > bufs;
        bufs[0] = buffer( bufitem->first.buffer() );
        bufs[1] = buffer( bufitem->second );
        async_write( *socket_, bufs,
                strand_.wrap( boost::bind( &bufwrite::handle_bufitem_sent, this->get_this(), ph::error, ph::bytes_transferred, bufitem.release() ) ) );
    }
    else
    {
        log4.debugStream() << "Queuing bufitem for " << remote_endpoint();

        send_queue_.push_back( bufitem ); // ownership transferred
        std::size_t const size = send_queue_.size();
        if ( size == 10 || size == 25 || size == 50 )
        {
            // Ok, technically, the queue size doesn't exceed 'size' when this message is printed,
            // but if another message is added to the queue after this message is printed, then it will!
            log4.warnStream() << "Bufwriter queue size exceeds " << size;
        }
    }
}

// Last parameter is need to ensure that the bufitem lives until this handler is called
template < typename socket_ptr >
void bufwrite< socket_ptr >::handle_bufitem_sent( bs::error_code const& ec, std::size_t const bytes_transferred, bufitem_t* raw_bufitem )
{
    bufitem_ptr bufitem( raw_bufitem );

    send_in_progress_ = false;
    if ( !ec )
    {
        log4.debugStream() << "Successfully sent bufitem to " << remote_endpoint();
        if ( succb_ ) succb_( bufitem->first.type(), bufitem->second );

        if ( !send_queue_.empty() )
        {
            // The documentation for the pointer containers is horrible.
            // I think this is okay, but I'm not really sure why queue_t::auto_type
            // is not compatible with std::auto_ptr??
            bufitem_t* nextbufitem = send_queue_.pop_front().release();
            send_bufitem( nextbufitem );
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while sending bufitem to " << remote_endpoint() << "; the message has been lost!!";
        if ( errcb_ )
        {
            errcb_( bufitem->first.type(), bufitem->second );
            for ( queue_t::iterator i = send_queue_.begin(); i != send_queue_.end(); ++i ) errcb_( i->first.type(), i->second );
        }
        send_queue_.clear();
    }

}

template < typename socket_ptr >
void bufread< socket_ptr >::read_header()
{
    async_read( *socket_, buffer( header_.buffer() ),
            boost::bind( &bufread::handle_header_read, this->get_this(), ph::error, ph::bytes_transferred ) );
}

template < typename socket_ptr >
void bufread< socket_ptr >::handle_header_read( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        if ( header_.length() > (sizeof buf_ - 1) ) // -1 because we insert a null character into the buffer in handle_msg_read
        {
            log4.errorStream() << "Message with size " << header_.length() << " is too big for buffer!";
            if ( errcb_ ) errcb_();
        }
        else if ( cbmap_.find( header_.type() ) != cbmap_.end() )
        {
            async_read( *socket_, buffer( buf_, header_.length() ),
                    boost::bind( &bufread::handle_body_read, this->get_this(), ph::error, ph::bytes_transferred ) );
        }
        else
        {
            log4.errorStream() << "Received unknown message type: " << header_.type();
            if ( errcb_ ) errcb_();
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while reading message header: " << ec.message();
        if ( errcb_ ) errcb_();
    }
}

template < typename socket_ptr >
void bufread< socket_ptr >::handle_body_read( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        std::string str;
        buf_[bytes_transferred] = '\0';
        protocol_helper::parse_string(str, bytes_transferred + 1, buf_.begin(), buf_.end()); // +1 for null insterted into buffer

        // This should work because we already checked that there is a cb present and the owner
        // is not supposed to mess with our callback map after we start!
        cbmap_t::iterator cb( cbmap_.find( header_.type() ) );
        if (cb != cbmap_.end() )
        {
            cb->second(str);
            read_header();
            return;
        }
    }

    log4.errorStream() << "An error occurred while reading message body: " << ec.message();
    if ( errcb_ ) errcb_();
}

typedef boost::asio::ip::tcp::socket socket_t;
template class bufread< socket_t* >;
template class bufread< boost::shared_ptr< socket_t > >;
template class bufwrite< socket_t* >;
template class bufwrite< boost::shared_ptr< socket_t > >;
