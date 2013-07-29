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

#ifndef PROTOCOL_HELPER_HPP_
#define PROTOCOL_HELPER_HPP_

#include "stdhdr.hpp"

namespace protocol_helper
{
    using std::string;

    class parse_error : public std::runtime_error
    {
    public:
       inline parse_error(string const& msg)
       : runtime_error(msg)
       {
       }
    };

    inline void unexpected_end_of_message()
    {
       throw parse_error("Unexpected end of message/buffer");
    }

    inline void invalid_string_format()
    {
        throw parse_error("Invalid string format");
    }

    inline void check_end(boost::uint8_t const* const buf, boost::uint8_t const* const end)
    {
       if (buf == end) unexpected_end_of_message();
    }

    inline void check_end(std::ptrdiff_t const len, boost::uint8_t const* const buf, boost::uint8_t const* const end)
    {
       if ( (end - buf) < len ) unexpected_end_of_message();
    }

    inline boost::uint8_t const* parse_short(boost::uint16_t& val, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
       check_end(2, buf, end);
       memcpy(&val, buf, sizeof( boost::uint16_t ));
       val = ntohs( val );
       return buf + 2;
    }

    inline boost::uint8_t* write_short(boost::uint16_t val, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        check_end(2, buf, end);
        val = htons( val );
        memcpy(buf, &val, sizeof( boost::uint16_t ));
        return buf + 2;
    }

    inline boost::uint8_t const* parse_ulong(boost::uint32_t& val, boost::uint8_t const* buf, boost::uint8_t const* const end)
    { // ulong = unsigned long (ie. uint 32)
        check_end(4, buf, end);
        memcpy(&val, buf, sizeof( boost::uint32_t ));
        val = ntohl( val );
        return buf + 4;
    }

    inline boost::uint8_t* write_ulong(boost::uint32_t val, boost::uint8_t* buf, boost::uint8_t const* const end)
    { // ulong = unsigned long (ie. uint 32)
        check_end(4, buf, end);
        val = htonl( val );
        memcpy(buf, &val, sizeof( boost::uint32_t ));
        return buf + 4;
    }

    inline boost::uint8_t const* parse_string(std::string& str, std::string::size_type const len, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        check_end(len, buf, end);
        if ( buf[len - 1] != '\0' ) invalid_string_format();
        str = reinterpret_cast< char const* >( buf );
        return buf + len;
    }

    inline boost::uint8_t* write_string(std::string const& str, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        std::size_t const len = str.length() + 1; // length including null
        check_end(len, buf, end);
        memcpy(buf, str.c_str(), len);
        return buf + len;
    }
}


#endif /* PROTOCOL_HELPER_HPP_ */
