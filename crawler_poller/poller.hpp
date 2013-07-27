/*
 * scheduler.hpp
 *
 *  Created on: 2013-05-05
 *      Author: alex
 */

#ifndef SCHEDULER_HPP_
#define SCHEDULER_HPP_

#include "stdhdr.hpp"
#include "asynchttprequester.hpp"
#include "parser.hpp"

namespace poller
{
    struct Context
    {
        Context(std::string const& deviceurl, std::string const& contenturl, boost::posix_time::time_duration const interval)
        : solr_deviceurl( deviceurl )
        , solr_contenturl( contenturl )
        , interval( interval )
        {}

        std::string const solr_deviceurl;
        std::string const solr_contenturl;
        boost::posix_time::time_duration const interval;
    };

    class poller: private boost::noncopyable
    {
    public:
        poller(Context const& pollerctx, curl::Context& curlctx, std::string const& hostname );

    private:
        void start()
        {
            timer_.async_wait( boost::bind( &poller::do_poll, this, boost::asio::placeholders::error ) );
        }

        void do_poll( boost::system::error_code const& );
        void handle_poll( CURLcode const code, std::string const& content );
        void handle_post( CURLcode const code, std::string const& content, time_t const newtimestamp );

        static parser::getall_parser< std::string::const_iterator > const g_;
        std::string const hostname_;
        boost::asio::deadline_timer timer_;
        // NOTE: requester_ was changed from an object to an auto_ptr so that we can destroy and
        // recreate it for each request!! Without doing this, curl will reuse connections for
        // efficiency, but the Home Agents/mongoose can't handle it.
        std::auto_ptr< curl::AsyncHTTPRequester > requester_;
        time_t timestamp_;
        curl::Context& curlctx_;
        // Needed now that we recreate the AsyncHTTPRequester object
        Context const& pollerctx_;
    };

    class pollercreator : private boost::noncopyable
    {
    public:
        pollercreator( Context const& pollerctx, curl::Context& curlctx )
        : strand_( curlctx.get_io_service() )
        , pollerctx_( pollerctx )
        , curlctx_( curlctx )
        {
        }

        void create_poller( std::string const& hostname );

    private:
        void create_poller_( std::string const& hostname );

        boost::ptr_vector< poller > pollers_;
        boost::asio::strand strand_;
        Context const& pollerctx_;
        curl::Context& curlctx_;
    };
}

#endif /* SCHEDULER_HPP_ */
