/*
 * protocol_helper.hpp
 *
 *  Created on: 2013-02-28
 *      Author: apokluda
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

    inline void check_end(boost::uint8_t const* const buf, boost::uint8_t const* const end)
    {
       if (buf == end) unexpected_end_of_message();
    }

    inline void check_end(std::ptrdiff_t const len, boost::uint8_t const* const buf, boost::uint8_t const* const end)
    {
       if ( (end - buf) < len ) unexpected_end_of_message();
    }

    inline boost::uint8_t* parse_short(boost::uint16_t& val, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
       check_end(2, buf, end);
       val = ntohs( *reinterpret_cast< boost::uint16_t const* >( buf ) );
       return buf + 2;
    }

    inline boost::uint8_t* write_short(boost::uint16_t const val, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        check_end(2, buf, end);
        *reinterpret_cast< boost::uint16_t* >( buf ) = htons(val);
        return buf + 2;
    }
}


#endif /* PROTOCOL_HELPER_HPP_ */
