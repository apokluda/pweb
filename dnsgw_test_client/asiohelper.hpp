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
    namespace internal
    {
        void init(GlobalInfo*);
        void cleanup(GlobalInfo*);
    }

    class Context : boost::noncopyable
    {
        friend class AsyncHTTPRequest;
        friend curl_socket_t curl::opensocket(void*, curlsocktype, struct curl_sockaddr*);
        friend void addsock(curl_socket_t, CURL*, int, Context*);

    public:
        Context(boost::asio::io_service& io_service);

        ~Context()
        {
            internal::cleanup(&g_);
        }

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
