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
    class AsyncHTTPRequester;

    class Context : boost::noncopyable
    {
        friend class AsyncHTTPRequester;
        friend curl_socket_t opensocket(AsyncHTTPRequester*, curlsocktype, struct curl_sockaddr*);
        friend void addsock(curl_socket_t, CURL*, int, Context*);
        friend void timer_cb(const boost::system::error_code&, Context*);
        friend void event_cb(Context*, boost::asio::ip::tcp::socket*, int);
        friend void check_multi_info(Context*);
        friend int multi_timer_cb(CURLM*, long, Context*);

    public:
        Context(boost::asio::io_service& io_service);
        ~Context();

        inline boost::asio::io_service& get_io_service()
        {
            return io_service_;
        }

        inline boost::asio::io_service const& get_io_service() const
        {
            return io_service_;
        }

    private:
        boost::asio::deadline_timer timer_;
        boost::asio::io_service& io_service_;
        CURLM* multi_;
        int still_running_;
    };

    class AsyncHTTPRequester : public boost::enable_shared_from_this< AsyncHTTPRequester >
    {
        friend curl_socket_t opensocket(AsyncHTTPRequester*, curlsocktype, struct curl_sockaddr*);
        friend size_t write_cb(char*, size_t, size_t, AsyncHTTPRequester*);
        friend void check_multi_info(Context*);

    public:
        AsyncHTTPRequester(Context& c, bool const selfmanage = false)
        : rc_( CURLM_OK )
        , easy_( 0 )
        , c_( c )
        , headers_( 0 )
        , selfmanage_( selfmanage )
        {
            headers_ = curl_slist_append(headers_, "Content-type: text/xml");
            if ( !headers_ ) throw std::runtime_error("liburl error: Unable to set Content-type header");
        }

        Context& get_context()
        {
            return c_;
        }

        Context const& get_context() const
        {
            return c_;
        }

        ~AsyncHTTPRequester();

        void fetch(std::string const& url, boost::function< void(CURLcode, std::string const&) > cb, std::string const& = std::string() );

    private:
        void done(CURLcode const rc);

        static const std::size_t MAX_BUF_SIZE = 1024*1024; // Max amount of data we are willing to receive/buffer (1 MiB)

        std::ostringstream buf_;
        boost::function< void(CURLcode, std::string const&) > cb_;
        boost::shared_ptr< AsyncHTTPRequester > ptr_to_this_;
        CURLMcode rc_;
        CURL* easy_;
        curl_slist* headers_;
        Context& c_;
        bool const selfmanage_;
    };
}

#endif /* ASIOHELPER_HPP_ */
