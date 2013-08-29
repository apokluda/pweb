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

#ifndef SCHEDULER_HPP_
#define SCHEDULER_HPP_

#include "stdhdr.hpp"
#include "asynchttprequester.hpp"
#include "parser.hpp"

namespace poller
{
    struct Context
    {
        Context(boost::asio::io_service& io_service, std::string const& deviceurl, std::string const& contenturl, boost::posix_time::time_duration const interval)
        : io_service( io_service )
        , solr_deviceurl( deviceurl )
        , solr_contenturl( contenturl )
        , interval( interval )
        {}

        boost::asio::io_service& io_service;
        std::string const solr_deviceurl;
        std::string const solr_contenturl;
        boost::posix_time::time_duration const interval;
    };

    class poller: private boost::noncopyable
    {
    public:
        poller(Context const& pollerctx, std::string const& hostname );

    private:
        void start()
        {
            timer_.async_wait( boost::bind( &poller::do_poll, this, boost::asio::placeholders::error ) );
        }

        void do_poll( boost::system::error_code const& );
        void handle_poll( CURLcode const code, std::string const& content );
        void handle_post( CURLcode const code, std::string const& content, time_t const newtimestamp );

        // Originally there was one curlctx for the whole program, but
        // all curl library operations are synchronized by a strand in
        // the curlctx. This program does almost nothing but HTTP requests,
        // thus, this could be a real limit to scalability! Thus, there
        // is now one curlctx for every poller.

        static parser::getall_parser< std::string::const_iterator > const g_;
        // curlctx_ must precede requester_
        curl::Context curlctx_;
        curl::AsyncHTTPRequester requester_;
        std::string const hostname_;
        boost::asio::deadline_timer timer_;
        time_t timestamp_;
        Context const& pollerctx_;
    };

    class pollercreator : private boost::noncopyable
    {
    public:
        pollercreator( Context const& pollerctx )
        : strand_( pollerctx.io_service )
        , pollerctx_( pollerctx )
        {
        }

        void create_poller( std::string const& hostname );

    private:
        void create_poller_( std::string const& hostname );

        boost::ptr_vector< poller > pollers_;
        boost::asio::strand strand_;
        Context const& pollerctx_;
    };
}

#endif /* SCHEDULER_HPP_ */
