/*
 * messages.cpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
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


