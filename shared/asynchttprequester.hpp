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
    // BIG FAT WARNING!!! BIG FAT WARNING!!! BIG FAT WARNING!!!
    //
    // The implementation of Context and AsyncHTTPRequester does not
    // have all the necessary synchronization necessary to support
    // multiple boost.asio application threads.
    //
    // All operations
    // involving the multi handle and each easy handle associated with it
    // should be synchronized. This means that the Context should be
    // responsible for this.
    //
    // BIG FAT WARNING!!! BIG FAT WARNING!!! BIG FAT WARNING!!!


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
        // This is only a start, we need a lot more synchronization!  The following methods are a start but by no
        // means sufficient.
        void add_handle_( CURL* easy );
        void perform_();


        boost::asio::deadline_timer timer_;
        boost::asio::strand strand_;
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
        AsyncHTTPRequester(Context& c, bool const selfmanage = true);

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
