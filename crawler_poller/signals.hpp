/*
 * signals.hpp
 *
 *  Created on: 2013-05-04
 *      Author: alex
 */

#ifndef SIGNALS_HPP_
#define SIGNALS_HPP_

#include "stdhdr.hpp"
#include "manconnection.hpp"

namespace signals
{
    template < typename T, typename F = boost::function< void(T) > >
    class duplicate_filter: private boost::noncopyable
    {
    public:
        void operator()( T t )
        {
            // This is one of the very very rare occasions where I'll
            // use a mutex in an ASIO program. The mutex is held for
            // only a single operation. What we really need here is
            // a concurrent set, but sadly Boost doesn't provide one.
            // (Boost does have lockfree queues however). We could
            // use a strand here, but that is almost overkill. Also,
            // we don't need to synchronize calls to f, we let other
            // parts of the program take care of that.

            bool newitem = false;
            {
                boost::lock_guard< boost::mutex > _( mutex_ );
                newitem = set_.insert( t ).second;
            }
            if ( newitem && f_ ) f_( t );
        }

        // Provide an interface similar to signals2. (Actually, it would
        // probably be better to use an actuals signanls2 signal in order
        // to provide the same interface, but I know this is sufficient
        // for this program for now.
        void connect(F slot)
        {
            f_ = slot;
        }

    private:
        std::set< T > set_;
        boost::mutex mutex_;
        F f_;
    };

    extern boost::signals2::signal<void (manconnection_ptr)> manager_connected;
    extern boost::signals2::signal<void (manconnection_ptr)> manager_disconnected;

    typedef boost::signals2::signal<void (std::string const&)> home_agent_discovered_sigt;
    extern boost::signals2::signal<void (std::string const&)> home_agent_discovered;
    extern boost::signals2::signal<void (std::string const&)> home_agent_assigned;
}

#endif /* SIGNALS_HPP_ */
