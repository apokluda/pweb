/*
 * signals.cpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
 */

#include "signals.hpp"

#include "pollerconnector.hpp"

namespace signals
{
    boost::signals2::signal<void (pollerconnection_ptr)> poller_connected;
    boost::signals2::signal<void (pollerconnection_ptr)> poller_disconnected;

    boost::signals2::signal<void (std::string const&)> home_agent_discovered;
}

