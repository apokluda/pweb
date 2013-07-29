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

#include "messages.hpp"

using namespace crawler_protocol;

std::ostream& operator<<(std::ostream& out, message_type const type)
{
    switch (type)
    {
        case HOME_AGENT_DISCOVERED: return out << "HOME_AGENT_DISCOVERED";
        case HOME_AGENT_ASSIGNMENT: return out << "HOME_AGENT_ASSIGNMENT";
        default: return out << "NOT_A_MESSAGE";
    }
}


