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

#ifndef PROTOCOL_HPP_
#define PROTOCOL_HPP_

#include "stdhdr.hpp"

namespace crawler_protocol
{
    namespace message_types
    {
        enum message_type
        {
            NOT_A_MESSAGE,
            HOME_AGENT_DISCOVERED,
            HOME_AGENT_ASSIGNMENT
        };
    }
    using namespace message_types;

    std::ostream& operator<<(std::ostream& out, message_type const type);

    class header
    {
    public:
        boost::asio::mutable_buffers_1 buffer()
        {
            return boost::asio::buffer( buf_ );
        }

        void type(message_type const type)
        {
            buf_[0] = static_cast< boost::uint8_t >( type );
        }

        message_type type() const
        {
            switch ( buf_[0] )
            {
            case HOME_AGENT_DISCOVERED:
                return HOME_AGENT_DISCOVERED;
            case HOME_AGENT_ASSIGNMENT:
                return HOME_AGENT_ASSIGNMENT;
            default:
                return NOT_A_MESSAGE;
            }
        }

        void length(boost::uint16_t length)
        {
            length = htons(length);
            memcpy(&buf_[0], &length, sizeof(boost::uint16_t) );
        }

        boost::uint16_t length() const
        {
            boost::uint16_t length;
            memcpy(&length, &buf_[1], sizeof(boost::uint16_t) );
            return ntohs(length);
        }

    private:
        boost::array< boost::uint8_t, 3 > buf_;
    };
}

#endif /* PROTOCOL_HPP_ */
