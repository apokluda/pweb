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
        Context(std::string const& solrurl, boost::posix_time::time_duration const interval)
        : solrurl( solrurl )
        , interval( interval )
        {}

        std::string const solrurl;
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
        void handle_post( CURLcode const code, std::string const& content );

        static parser::getall_parser< std::string::const_iterator > const g_;
        curl::AsyncHTTPRequester requester_;
        std::string const hostname_;
        boost::asio::deadline_timer timer_;
        time_t timestamp_;
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

        void create_poller( std::string const& hostname )
        {
            strand_.dispatch( boost::bind( &pollercreator::create_poller_, this, hostname ) );
        }

    private:
        void create_poller_( std::string const& hostname )
        {
            // The pointer must be stored in the container immediately for exception safety
            pollers_.push_back( new poller( pollerctx_, curlctx_, hostname ) );
        }

        boost::ptr_vector< poller > pollers_;
        boost::asio::strand strand_;
        Context const& pollerctx_;
        curl::Context& curlctx_;
    };
}

#endif /* SCHEDULER_HPP_ */
