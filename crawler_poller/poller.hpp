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
#include "instrumentation.hpp"

namespace poller
{
    struct Context
    {
        Context(instrumentation::instrumenter& instrumenter, std::string const& hadevurl, std::string const& haconurl, std::string const& deviceurl, std::string const& contenturl, boost::posix_time::time_duration const interval)
        : hadevurlfmt( hadevurl )
        , haconurlfmt( haconurl )
        , solr_deviceurl( deviceurl )
        , solr_contenturl( contenturl )
        , interval( interval )
        , io_service( instrumenter.get_io_service() )
        , instrumenter( instrumenter )
        {
            // The format object is shared between all poller instances.
            // It is const to ensure that the pollers must make a copy of
            // it before using it. (Making a copy is a lot cheaper than
            // creating one from scratch because it avoids the template
            // processing overhead). However, we want to change the exception
            // flags so that exceptions are not thrown if the user does not
            // use all placeholders in the passed-in string. We need a
            // const cast to do this on a const object.
            const_cast< boost::format* >( &hadevurlfmt )->exceptions(
                    boost::io::all_error_bits ^ boost::io::too_many_args_bit );
            const_cast< boost::format* >( &haconurlfmt )->exceptions(
                    boost::io::all_error_bits ^ boost::io::too_many_args_bit );
        }

        boost::format const hadevurlfmt;
        boost::format const haconurlfmt;
        std::string const solr_deviceurl;
        std::string const solr_contenturl;
        boost::posix_time::time_duration const interval;
        boost::asio::io_service& io_service;
        instrumentation::instrumenter& instrumenter;
    };

    class poller: private boost::noncopyable
    {
    public:
        poller(Context const& pollerctx, std::string const& hostname );

    private:
        void start();
        void do_poll( boost::system::error_code const& );
        void handle_poll( CURLcode const code, std::string const& content );
        void handle_post( CURLcode const code, std::string const& content, time_t const newtimestamp );

        // Originally there was one curlctx for the whole program, but
        // all curl library operations are synchronized by a strand in
        // the curlctx. This program does almost nothing but HTTP requests,
        // thus, this could be a real limit to scalability! Thus, there
        // is now one curlctx for every poller.

        static parser::getall_parser< std::string::const_iterator > const g_;
        static boost::atomic< std::size_t > num_pollers_;
        static boost::atomic< std::size_t > num_falling_behind_;
        // curlctx_ must precede requester_
        curl::Context curlctx_;
        curl::AsyncHTTPRequester requester_;
        std::string const hostname_;
        boost::asio::deadline_timer timer_;
        time_t timestamp_;
        Context const& pollerctx_;
        bool falling_behind_;
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
