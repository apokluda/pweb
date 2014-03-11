/*
 * instrumentation.cpp
 *
 *  Created on: Mar 11, 2014
 *      Author: alex
 */

#include "instrumentation.hpp"

extern log4cpp::Category& log4;

using namespace boost::asio;
using namespace boost::asio::ip;
namespace ph = boost::asio::placeholders;

namespace instrumentation
{

const char* query_result_str( const query_result result )
{
    switch ( result )
    {
    case SUCCESS:
        return "Success";
    case NETWORK_ERROR:
        return "Network Error";
    case TIMEOUT:
        return "Timeout";
    case PARSE_ERROR:
        return "Parse Error";
    case NA:
        return "Not Applicable";
    default:
        return "Unknown";
    }
}

void database::update_record_( const std::string& hostname, const query_result result, const std::string& errmsg )
{
    try
    {
        impl::record& record( db_.at(hostname) );
        if ( result == SUCCESS )
        {
            record.lastsuccess_ts = boost::date_time::second_clock< boost::posix_time::ptime >::local_time();
        }
        else
        {
            record.lastfailure_ts = boost::date_time::second_clock< boost::posix_time::ptime >::local_time();
            record.lastfailure_msg = errmsg;
        }
        record.lastpoll_result = result;
    }
    catch ( const std::out_of_range& )
    {
        log4.errorStream() << "Instrumentation error: received update for non-existing record";
    }
}

void do_serialize_callback( boost::function<void (streambuf_ptr)> callback, streambuf_ptr sbuf_ptr )
{
    // This function ensures that the callback is not called from within the strand
    callback(sbuf_ptr);
}

void database::serialize_as_json_( boost::function<void (streambuf_ptr)> callback ) const
{
    streambuf_ptr sbuf_ptr( new streambuf );
    std::ostream os( sbuf_ptr.get() );
    os << "[\n";
    for (map_t::const_iterator it = db_.begin(); it != db_.end(); ++it)
    {
        os << "{\n";
        os << "\t'hostname':'" << it->first << "',\n";
        os << "\t'lastpoll_result':'" << query_result_str(it->second.lastpoll_result) << "',\n";
        os << "\t'discovered_ts':'" << it->second.discovered_ts << "',\n";
        os << "\t'lastsuccess_ts':'" << it->second.lastsuccess_ts << "',\n";
        os << "\t'lastfailure_ts':'" << it->second.lastfailure_ts << "',\n";
        os << "\t'lastfailure_msg':'" << it->second.lastfailure_msg << "'\n";
        os << "},\n";
    }
    os << "]\n";
    // call stand_.get_io_service() rather than get_io_service() because strand is mutable
    strand_.get_io_service().post( boost::bind( &do_serialize_callback, callback, sbuf_ptr ) );
}

class session
: public boost::enable_shared_from_this< session >
, private boost::noncopyable
{
public:
    session(boost::asio::io_service& io_service, const database& database)
    : cmdbuf_(1024) // limit command length to 1024 bytes
    , socket_(io_service)
    , database_( database )
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        async_read_until(socket_, cmdbuf_, "\n", boost::bind( &session::handle_command_read, shared_from_this(), ph::error ) );
    }

private:
    void handle_command_read(const boost::system::error_code& ec)
    {
        if (!ec)
        {
            // Parse command
            std::istream in(&cmdbuf_);
            std::string cmd;
            std::getline(in, cmd);
            log4.infoStream() << "Instrumentation service: received command \"" << cmd << "\" from " << socket_.remote_endpoint();

            // My intention is to receive commands in the form "command?param1=val1&param2=val2"
            // parse them, and send back the response. However, right now getting the Home Agents
            // status in JSON or Python markup (currently they are equivalent), we just send
            // back this result for any command string

            database_.serialize_as_json( boost::bind( &session::send_result, shared_from_this(), _1 ) );
        }
        else
        {
            log4.errorStream() << "Instrumentation service: an error occurred while reading command from " << socket_.remote_endpoint();
        }
    }

    void send_result( streambuf_ptr sbuf_ptr )
    {
        async_write(socket_, *sbuf_ptr, boost::bind( &session::handle_result_sent, shared_from_this(), ph::error, ph::bytes_transferred) );
    }

    void handle_result_sent( const boost::system::error_code& error, const std::size_t bytes_transferred )
    {
        if ( !error )
        {
            log4.infoStream() << "Instrumentation service: successfully sent response to " << socket_.remote_endpoint();
        }
        else
        {
            log4.errorStream() << "Instrumentation service: an error occurred while sending response to " << socket_.remote_endpoint() << ": " << error.message();
        }
        // Session will be deleted and socket closed automatically
    }

    streambuf cmdbuf_;
    tcp::socket socket_;
    const database& database_;
};

typedef boost::shared_ptr< session > session_ptr;

instrumenter::instrumenter( boost::asio::io_service& io_service, const boost::uint16_t instport )
: database_( io_service )
, acceptor_( io_service )
{
    if ( instport != 0 )
    {
        ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), instport);
        acceptor_.open( endpoint.protocol() );
        acceptor_.set_option( ip::tcp::acceptor::reuse_address( true ) );
        acceptor_.bind( endpoint );
        acceptor_.listen();
        start_accept();
    }
}

void instrumenter::start_accept()
{
    session_ptr new_session(new session( acceptor_.get_io_service(), database_ ) );
    acceptor_.async_accept( new_session->socket(), boost::bind( &instrumenter::handle_accept, this, new_session, ph::error) );
}

void instrumenter::handle_accept( session_ptr new_session, const boost::system::error_code& error )
{
    if ( !error )
    {
        new_session->start();
    }
    else
    {
        log4.errorStream() << "Instrumentation service: an error occured while starting a new session: " << error.message();
    }

    start_accept();
}

}

