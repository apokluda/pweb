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

    inline boost::uint8_t const* parse_short(boost::uint16_t& val, boost::uint8_t const* buf, boost::uint8_t const* const end)
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

    inline boost::uint8_t* write_slong(boost::int32_t const val, boost::uint8_t* buf, boost::uint8_t const* const end)
    { // slong = signed long (ie. int 32)
        check_end(4, buf, end);
        *reinterpret_cast< boost::int32_t* >( buf ) = htonl(val);
        return buf + 4;
    }

// UNUSED
//    inline boost::uint8_t* write_lpstring(std::string const& str, boost::uint8_t* buf, boost::uint8_t const* const end)
//    { // lpstring = length-prefixed string
//        std::size_t len = str.length();
//        buf = write_short(len, buf, end);
//        check_end(len, buf, end);
//        memcpy(buf, str.c_str(), len);
//        return buf + len;
//    }
}


#endif /* PROTOCOL_HELPER_HPP_ */
