/*
 * signals.hpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
 */

#ifndef SIGNALS_HPP_
#define SIGNALS_HPP_

#include "stdhdr.hpp"
#include "pollerconnector.hpp"

namespace signals
{
    extern boost::signals2::signal<void (pollerconnection_ptr)> poller_connected;
    extern boost::signals2::signal<void (pollerconnection_ptr)> poller_disconnected;

    typedef boost::signals2::signal<void (std::string const&)> home_agent_discovered_sigt;
    extern boost::signals2::signal<void (std::string const&)> home_agent_discovered;
}

#endif /* SIGNALS_HPP_ */
