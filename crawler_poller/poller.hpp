/*
 * scheduler.hpp
 *
 *  Created on: 2013-05-05
 *      Author: alex
 */

#ifndef SCHEDULER_HPP_
#define SCHEDULER_HPP_

#include "stdhdr.hpp"

namespace poller
{
    class poller: private boost::noncopyable
    {
    public:
        poller( boost::asio::io_service& io_service, std::string const& hostname, boost::posix_time::time_duration const interval );

    private:
        void do_poll( boost::system::error_code const& );

        std::string const hostname_;
        boost::asio::deadline_timer timer_;
    };

    class pollercreator : private boost::noncopyable
    {
    public:
        pollercreator( boost::asio::io_service& io_service, boost::posix_time::time_duration const interval )
        : strand_( io_service )
        , interval_( interval )
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
            pollers_.push_back( new poller( strand_.get_io_service(), hostname, interval_ ) );
        }

        boost::ptr_vector< poller > pollers_;
        boost::asio::strand strand_;
        boost::posix_time::time_duration interval_;
    };
}

#endif /* SCHEDULER_HPP_ */
