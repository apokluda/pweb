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
