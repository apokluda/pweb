/*
 * main.cpp
 *
 *  Created on: Mar 23, 2014
 *      Author: alex
 */

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <log4cpp/Category.hh>
#include "asynchttprequester.hpp"

void done(CURLcode const code, std::string const& content)
{
    std::cout << content << std::endl;

    std::cout << "\nResult: CURLcode " << code << ": " << curl_easy_strerror(code) << std::endl;
}

int main(int argc, char const* argv[])
{
    if ( argc != 2 ) {
        std::cerr << "Usage: " << argv[0] << " url" << std::endl;
        return 1;
    }

    try
    {
        curl::init();

        boost::asio::io_service io_service;
        curl::Context context(io_service);

        curl::AsyncHTTPRequester requester(context, false);
        requester.fetch(argv[1], done);

        boost::chrono::steady_clock::time_point start = boost::chrono::steady_clock::now();

        io_service.run();

        boost::chrono::duration< double > elapsed = boost::chrono::steady_clock::now() - start;
        std::cerr << "Fetching took " << elapsed << std::endl;

        curl::cleanup();
    }
    catch ( std::exception const& e )
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

