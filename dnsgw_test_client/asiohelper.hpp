/*
 * asiohelper.hpp
 *
 *  Created on: 2013-04-02
 *      Author: alex
 */

#ifndef ASIOHELPER_HPP_
#define ASIOHELPER_HPP_

namespace curl
{
    class Context : boost::noncopyable
    {
        friend class AsyncHTTPRequester;
        friend curl_socket_t curl::opensocket(void*, curlsocktype, struct curl_sockaddr*);
        friend void addsock(curl_socket_t, CURL*, int, Context*);
        friend void timer_cb(const boost::system::error_code&, Context*);
        friend void event_cb(Context*, boost::asio::ip::tcp::socket*, int);
        friend void check_multi_info(Context*);
        friend int multi_timer_cb(CURLM*, long, Context*);

    public:
        Context(boost::asio::io_service& io_service);
        ~Context();

    private:
        boost::asio::deadline_timer timer_;
        boost::asio::io_service& io_service_;
        CURLM* multi_;
        int still_running_;
    };

    class AsyncHTTPRequester
    {
    public:
        AsyncHTTPRequester(Context& c)
        : rc_( CURLM_OK )
        , easy_( 0 )
        , c_( c )
        {
        }

        void fetch(std::string const& url, boost::function< void(CURLMcode, std::string const&) > cb );

    private:
        boost::function< void(CURLMcode, std::string const&) > cb_;
        CURLMcode rc_;
        CURL* easy_;
        Context& c_;
    };
}

#endif /* ASIOHELPER_HPP_ */
