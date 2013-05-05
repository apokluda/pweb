/*
 * protocol.hpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
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
