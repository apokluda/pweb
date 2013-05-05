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
    extern boost::signals2::signal<void (manconnection_ptr)> manager_connected;
    extern boost::signals2::signal<void (manconnection_ptr)> manager_disconnected;

    extern boost::signals2::signal<void (std::string const&)> home_agent_discovered;
    extern boost::signals2::signal<void (std::string const&)> home_agent_assigned;
}

#endif /* SIGNALS_HPP_ */
