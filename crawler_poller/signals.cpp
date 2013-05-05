/*
 * signals.cpp
 *
 *  Created on: 2013-05-04
 *      Author: alex
 */

#include "stdhdr.hpp"
#include "manconnection.hpp"

namespace signals
{
    boost::signals2::signal<void (manconnection_ptr)> manager_connected;
    boost::signals2::signal<void (manconnection_ptr)> manager_disconnected;

    boost::signals2::signal<void (std::string const&)> home_agent_discovered;
    boost::signals2::signal<void (std::string const&)> home_agent_assigned;
}


