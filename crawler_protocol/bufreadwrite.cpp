#include "bufreadwrite.hpp"

using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

extern log4cpp::Category& log4;

void bufwrite::sendmsg( crawler_protocol::message_type const type, std::string const& str )
{
    // Note: calls to this function are not serialized!
    bufitem_ptr bufitem( new bufitem_t );
    bufitem->first.type( type );
    std::size_t const length = str.length() + 1; // +1 for null character
    bufitem->first.length( length );
    bufitem->second.resize( length );
    char const* cstr = str.c_str();
    std::copy(cstr, cstr + length, bufitem->second.begin());
    strand_.dispatch( boost::bind( &bufwrite::send_bufitem, shared_from_this(), bufitem.release() ) );
}

void bufwrite::send_bufitem( bufitem_t* raw_bufitem )
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
                strand_.wrap( boost::bind( &bufwrite::handle_bufitem_sent, shared_from_this(), ph::error, ph::bytes_transferred, bufitem.release() ) ) );
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
void bufwrite::handle_bufitem_sent( bs::error_code const& ec, std::size_t const bytes_transferred, bufitem_t* bufitem )
{
    delete bufitem;

    if ( !ec ) log4.debugStream() << "Successfully sent bufitem to " << remote_endpoint();
    else log4.errorStream() << "An error occurred while sending bufitem to " << remote_endpoint() << "; the message has been lost!!";
    // NOTE: We could do something with the butitem that is the first parameter to this method
    // to ensure that the message does /not/ get lost, but that's too complicated right now.
    // For example, in the crawler manager, we could ensure that the Home Agent is assigned
    // to another crawler process.

    send_in_progress_ = false;
    if ( !send_queue_.empty() )
    {
        // The documentation for the pointer containers is horrible.
        // I think this is okay, but I'm not really sure why queue_t::auto_type
        // is not compatible with std::auto_ptr??
        bufitem_t* bufitem = send_queue_.pop_front().release();
        send_bufitem( bufitem );
    }
}
