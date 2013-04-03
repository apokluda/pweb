/*
 * ha_register.cpp
 *
 *  Created on: 2013-04-03
 *      Author: apokluda
 */

#include "ha_register.hpp"
#include "asiohelper.hpp"

extern log4cpp::Category& log4;

void ha_register::async_run(std::size_t maxconn)
{
    for (int i = 0; i < maxconn && namenum_ > 0; ++i) register_name(boost::shared_ptr< curl::AsyncHTTPRequester >( new curl::AsyncHTTPRequester( c_ ) ));
}

void ha_register::register_name(boost::shared_ptr< curl::AsyncHTTPRequester > r)
{
    std::ostringstream name;
    name << 'd' << std::setw(3) << std::setfill('0') << (--namenum_) << ".alex." << haconfig_.haname;

    std::string url("http://pwebproject.net");

    r->fetch(url, boost::bind(&ha_register::handle_register_name, shared_from_this(), _1, _2, name.str(), r));
}

void ha_register::handle_register_name(CURLcode const code, std::string const& content, std::string const& name, boost::shared_ptr< curl::AsyncHTTPRequester > rptr)
{
    // check that code is CURLE_OK, log result, and start a new request
    if ( code == CURLE_OK )
    {
        log4.infoStream() << "Successfully registered '" << name << '\'';
        if ( namenum_ > 0 ) register_name( rptr );
    }
    else
    {
        log4.errorStream() << "An error occurred while registering the name '" << name << "', CURLcode " << code;
    }
}



