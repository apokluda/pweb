/*
 * instrumentation.hpp
 *
 *  Created on: Mar 10, 2014
 *      Author: alex
 */

#ifndef INSTRUMENTATION_HPP_
#define INSTRUMENTATION_HPP_

#include "stdhdr.hpp"

namespace instrumentation
{

// Current limitations: The instrumentation infrastructure is
// currently unaware of any errors that occur when fetching/parsing
// device updates and posting the results to Solr. It is only notified
// about errors that occurr when fetching device metadata using the
// "getall" homeagent REST API call.

enum query_result
{
    SUCCESS,
    NETWORK_ERROR,
    TIMEOUT,
    PARSE_ERROR,
    NA
};

const char* query_result_str( const query_result result );

namespace impl
{
    struct record
    {
        record()
        : discovered_ts( boost::date_time::second_clock< boost::posix_time::ptime >::local_time() )
        , lastpoll_result( NA )
        {
        }

        std::string lastfailure_msg;
        boost::posix_time::ptime const discovered_ts;
        boost::posix_time::ptime lastsuccess_ts;
        boost::posix_time::ptime lastfailure_ts;
        query_result lastpoll_result;
    };
}

typedef boost::shared_ptr< boost::asio::streambuf > streambuf_ptr;

class database
{
public:
    database( boost::asio::io_service& io_service )
    : strand_( io_service )
    {
    }

    boost::asio::io_service& get_io_service()
    {
        return strand_.get_io_service();
    }

    void insert_record( const std::string& hostname )
    {
        impl::record record;
        strand_.dispatch( boost::bind( &database::insert_record_, this, hostname, record ) );
    }

    void update_record( const std::string& hostname, const query_result result, const std::string& errmsg )
    {
        strand_.dispatch( boost::bind( &database::update_record_, this, hostname, result, errmsg ) );
    }

    void serialize_as_python( boost::function<void (streambuf_ptr)> callback)
    {
        // In the curret implementation, the Python and JSON representations are the same
        strand_.dispatch( boost::bind( &database::serialize_as_json_, this, callback ) );
    }

    void serialize_as_json( boost::function<void (streambuf_ptr)> callback) const
    {
        strand_.dispatch( boost::bind( &database::serialize_as_json_, this, callback ) );
    }

private:
    void insert_record_( const std::string& hostname, const impl::record& record )
    {
        typedef std::pair< const std::string, const impl::record > val_t;
        db_.insert( val_t(hostname, record) );
    }

    void update_record_( const std::string& hostname, const query_result result, const std::string& errmsg);

    void serialize_as_json_( boost::function<void (streambuf_ptr)> callback ) const;

    typedef std::map< const std::string, impl::record > map_t;
    map_t db_;
    mutable boost::asio::strand strand_;
};

class session;
typedef boost::shared_ptr< session > session_ptr;

class instrumenter
{
public:
    instrumenter( boost::asio::io_service& io_service, const boost::uint16_t instport );

    boost::asio::io_service& get_io_service()
    {
        return acceptor_.get_io_service();
    }

    void home_agent_discovered( const std::string& hostname )
    {
        database_.insert_record( hostname );
    }

    void query_result( const std::string& hostname, query_result result, const std::string& errmsg = std::string())
    {
        database_.update_record( hostname, result, errmsg );
    }

private:
    void start_accept();
    void handle_accept( session_ptr new_session, const boost::system::error_code& ec);

    database database_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

}

#endif /* INSTRUMENTATION_HPP_ */
