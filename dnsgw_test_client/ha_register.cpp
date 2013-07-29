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

#include "ha_register.hpp"
#include "token_counter.hpp"
#include "asynchttprequester.hpp"

extern log4cpp::Category& log4;

void ha_register::async_run()
{
    std::size_t const requestedtoks = std::min(maxconns_ - activeconns_, namenum_);
    std::size_t const receivedtoks  = tc_.request( requestedtoks );
    for (std::size_t i = 0; i < receivedtoks; ++i) register_name(boost::shared_ptr< curl::AsyncHTTPRequester >( new curl::AsyncHTTPRequester( c_ ) ));
    activeconns_ += receivedtoks;
    if ( receivedtoks < requestedtoks ) tc_.wait( boost::bind(&ha_register::async_run, shared_from_this()) );
}

void ha_register::register_name(boost::shared_ptr< curl::AsyncHTTPRequester > r)
{
    std::ostringstream name;
    name << 'd' << std::setw(3) << std::setfill('0') << (--namenum_) << '.' << owner_ << '.' << haconfig_.haname;

    std::ostringstream url;
    url << "http://" << haconfig_.url << ":20005/?method=register&name=" << name << "&port=12345";

    r->fetch(url.str(), boost::bind(&ha_register::handle_register_name, shared_from_this(), _1, _2, name.str(), r));
}

void ha_register::handle_register_name(CURLcode const code, std::string const& content, std::string const& name, boost::shared_ptr< curl::AsyncHTTPRequester > rptr)
{
    // check that code is CURLE_OK, log result, and start a new request
    if ( code == CURLE_OK )
    {
        log4.infoStream() << "Successfully registered '" << name << '\'';
        if ( namenum_ > 0 ) register_name( rptr );
        else tc_.release(1);
    }
    else
    {
        log4.errorStream() << "An error occurred while registering the name '" << name << "', CURLcode " << code;
        tc_.release(1);
    }
}
