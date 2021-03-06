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

#include "stdhdr.hpp"

extern log4cpp::Category& log4;

void checked_io_service_run(boost::asio::io_service& io_service)
{
    try
    {
        io_service.run();
    }
    catch ( std::exception const& e )
    {
        log4.fatalStream() << "UNCAUGHT EXCEPTION: " << e.what();
        log4.alertStream() << "System will now shutdown";
        io_service.stop();
    }
}

void run( boost::asio::io_service& io_service, std::size_t num_threads )
{
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through asio.
    boost::asio::signal_set sig_set( io_service, SIGINT, SIGTERM );
    sig_set.async_wait( boost::bind( &boost::asio::io_service::stop, &io_service ) );

    if ( num_threads == 1 )
    {
        // Run one io_service in current thread
        log4.debug("Starting io_service event loop");
        checked_io_service_run( io_service );
    }
    else
    {
        if ( num_threads == 0 ) num_threads = boost::thread::hardware_concurrency();

        // Create a pool of threads to run all of the io_services.
        std::vector<boost::shared_ptr<boost::thread> > threads;
        threads.reserve( num_threads );
        for (std::size_t i = 0; i < num_threads; ++i)
        {
            boost::shared_ptr<boost::thread> thread( new boost::thread(
                    boost::bind( checked_io_service_run, boost::ref( io_service ) ) ) );
            threads.push_back( thread );

            log4.debugStream() << "Started io_service thread " << i << " with id " << thread->get_id();
        }

        // Wait for all threads in the pool to exit.
        for (std::size_t i = 0; i < threads.size(); ++i)
            threads[i]->join();
    }
}


