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

#include "stdhdr.hpp"
#include "asynchttprequester.hpp"

//#define LOGSTREAM if ( debug ) log4.debugStream()
#define LOGSTREAM if ( debug ) std::cerr

#ifndef NDEBUG
#define TRACE(func) LOGSTREAM << "Entering: " func
#define TRACE_MSG(msg) LOGSTREAM << "Trace: " msg
#else
#define TRACE(func)
#define TRACE_MSG(msg)
#endif

//extern log4cpp::Category& log4;

namespace curl
{
bool debug = false;

void init() {
    curl_global_init( CURL_GLOBAL_DEFAULT );
}

void cleanup() {
    curl_global_cleanup();
}

void timer_cb(const boost::system::error_code & error, Context *g);

/* Update the event timer after curl_multi library calls */
int multi_timer_cb(CURLM *multi, long timeout_ms, Context* c)
{
	TRACE("multi_timer_cb");

    /* cancel running timer */
    c->timer_.cancel();

#ifndef NDEBUG
    // For some reason, libcurl often calls this function with a timeout of 1 millsecond.
    // When this happens, we set the timer and wait like normal, but every call to the timer
    // callback gets boost::asio::error::operation_aborted. Shouldn't at least one call
    // be a success?! Is this a bug in asio?
    if (timeout_ms == 1) TRACE_MSG("*****timount_ms == 1*****");
#endif

    if ( timeout_ms > 1 )
    {
        TRACE_MSG("updating timer to expire in " << timeout_ms << " milliseconds");
        /* update timer */
        c->timer_.expires_from_now(boost::posix_time::millisec(timeout_ms));
        c->timer_.async_wait(c->strand_.wrap(boost::bind(&timer_cb, _1, c)));
    }
    else
    {
        TRACE_MSG("call timeout");
        /* call timeout function immediately */
        boost::system::error_code error; /*success*/
        timer_cb(error, c);
    }

    return 0;
}

/* Throw if we get a bad CURLMcode somewhere */
void mcode_or_throw(const char *where, CURLMcode code)
{
    TRACE("mcode_or_throw");

    if ( CURLM_OK != code )
    {
        const char *s;
        switch ( code )
        {
            case CURLM_CALL_MULTI_PERFORM: s="CURLM_CALL_MULTI_PERFORM"; break;
            case CURLM_BAD_HANDLE:         s="CURLM_BAD_HANDLE";         break;
            case CURLM_BAD_EASY_HANDLE:    s="CURLM_BAD_EASY_HANDLE";    break;
            case CURLM_OUT_OF_MEMORY:      s="CURLM_OUT_OF_MEMORY";      break;
            case CURLM_INTERNAL_ERROR:     s="CURLM_INTERNAL_ERROR";     break;
            case CURLM_UNKNOWN_OPTION:     s="CURLM_UNKNOWN_OPTION";     break;
            case CURLM_LAST:               s="CURLM_LAST";               break;
            default:                       s="CURLM_unknown";            break;
            case CURLM_BAD_SOCKET:         s="CURLM_BAD_SOCKET";         return;
        }
        std::ostringstream ss;
        ss << "libucurl error: " << where << " returned " << s;
        throw std::runtime_error(ss.str());
    }
}

/* Check for completed transfers, and remove their easy handles */
void check_multi_info(Context* c)
{
    TRACE("check_multi_info");

    CURLMsg *msg;
    int msgs_left;
    AsyncHTTPRequester* r;

    while ((msg = curl_multi_info_read(c->multi_, &msgs_left)))
    {
        if (msg->msg == CURLMSG_DONE)
        {
            CURL* easy = msg->easy_handle;
            CURLcode res = msg->data.result;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &r);
            r->done(res);
        }
    }
}

/* Called by asio when there is an action on a socket */
void event_cb(Context* c, CURL* e, boost::asio::ip::tcp::socket * tcp_socket, int action, boost::system::error_code const& ec)
{
    TRACE("event_cb");

    AsyncHTTPRequester* r;
    curl_easy_getinfo(e, CURLINFO_PRIVATE, &r);
    r->reschedule_ = true;
    CURLMcode rc = curl_multi_socket_action(c->multi_, tcp_socket->native_handle(), action, &c->still_running_);

    mcode_or_throw("event_cb: curl_multi_socket_action", rc);
    check_multi_info(c);

    if ( c->still_running_ <= 0 )
    {
        c->timer_.cancel();
    }
    //else if ( !ec && r->reschedule_ )
    //{
    //    setsock(tcp_socket, e, action, c);
    //}
}

/* Called by asio when our timeout expires */
void timer_cb(const boost::system::error_code & error, Context* c)
{
	TRACE("timer_cb");

    if ( error != boost::asio::error::operation_aborted )
    {
        TRACE_MSG("no error");

        CURLMcode rc = curl_multi_socket_action(c->multi_, CURL_SOCKET_TIMEOUT, 0, &c->still_running_);

        mcode_or_throw("timer_cb: curl_multi_socket_action", rc);
        check_multi_info(c);
    }
#ifndef NDEBUG
    else
    {
        TRACE_MSG("Error: " << error.message());
    }
#endif
}

void setsock(curl_socket_t s, CURL*e, int act, Context* c)
{
    boost::asio::ip::tcp::socket* tcp_socket;
    tcp_socket = c->socket_map_.find(s);
    if (!tcp_socket) return;

    setsock(tcp_socket, e, act, c);
}

void setsock(boost::asio::ip::tcp::socket* tcp_socket, CURL*e, int act, Context* c)
{
    TRACE("setsock");

    AsyncHTTPRequester* r;
    curl_easy_getinfo(e, CURLINFO_PRIVATE, &r);
    r->reschedule_ = false;

    if ( act == CURL_POLL_IN )
    {
        tcp_socket->async_read_some(boost::asio::null_buffers(),
                c->strand_.wrap(boost::bind(
                        &event_cb, c, e,
                        tcp_socket,
                        act,
                        boost::asio::placeholders::error)));
    }
    else if ( act == CURL_POLL_OUT )
    {
        tcp_socket->async_write_some(boost::asio::null_buffers(),
                c->strand_.wrap(boost::bind(
                        &event_cb, c, e,
                        tcp_socket,
                        act,
                        boost::asio::placeholders::error)));
    }
    else if ( act == CURL_POLL_INOUT )
    {
        tcp_socket->async_read_some(boost::asio::null_buffers(),
                c->strand_.wrap(boost::bind(
                        &event_cb, c, e,
                        tcp_socket,
                        act,
                        boost::asio::placeholders::error)));

        tcp_socket->async_write_some(boost::asio::null_buffers(),
                c->strand_.wrap(boost::bind(
                        &event_cb, c, e,
                        tcp_socket,
                        act,
                        boost::asio::placeholders::error)));
    }
    else if ( act == CURL_POLL_REMOVE )
    {
        tcp_socket->cancel();
    }
}

/* CURLMOPT_SOCKETFUNCTION */
int sock_cb(CURL *e, curl_socket_t s, int what, Context* c, int* actionp)
{
    TRACE("sock_cb");

    setsock(s, e, what, c);
    return 0;
}

/* CURLOPT_WRITEFUNCTION */
size_t write_cb(char* ptr, size_t size, size_t nmemb, AsyncHTTPRequester* r)
{
    TRACE("write_cb");

    size_t written = size * nmemb;
    std::size_t oldsize = r->buf_.tellp();

    if ( oldsize + written > AsyncHTTPRequester::MAX_BUF_SIZE )
        written = AsyncHTTPRequester::MAX_BUF_SIZE - oldsize;

    r->buf_.write(ptr, written);

    return written;
}

/* CURLOPT_OPENSOCKETFUNCTION */
curl_socket_t opensocket(Context* c, curlsocktype purpose, struct curl_sockaddr *address)
{
	TRACE("opensocket");

    curl_socket_t sockfd = CURL_SOCKET_BAD;

    /* restrict to ipv4 */
    if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET)
    {
        /* create a tcp socket object */
        boost::asio::ip::tcp::socket *tcp_socket = new boost::asio::ip::tcp::socket( c->io_service_ );

        /* open it and get the native handle*/
        boost::system::error_code ec;
        tcp_socket->open(boost::asio::ip::tcp::v4(), ec);

        if (ec)
        {
            //An error occurred
            std::cerr << "Couldn't open socket [" << ec << "][" << ec.message() << "]" << std::endl;
        }
        else
        {
            sockfd = tcp_socket->native_handle();

            /* save it for monitoring */
            c->socket_map_.insert(sockfd, tcp_socket);
        }
    }

    return sockfd;
}

/* CURLOPT_CLOSESOCKETFUNCTION */
int closesocket(Context* c, curl_socket_t item)
{
	TRACE("closesocket");
	c->socket_map_.erase_and_delete(item);
    return 0;
}

AsyncHTTPRequester::AsyncHTTPRequester(Context& c, bool const selfmanage)
: rc_( CURLM_OK )
, easy_( curl_easy_init() )
, c_( c )
, headers_( curl_slist_append(0, "Content-type: text/xml") )
, reschedule_( false )
, selfmanage_( selfmanage )
{
    if ( !easy_ || !headers_ ) throw std::runtime_error("liburl initialization error");
}

void context_multi_add_handle(Context& c, CURL* easy)
{
	CURLMcode rc = curl_multi_add_handle(c.multi_, easy);
	// Throwing an exception here will cause the program to terminate, but
	// there is no easy way to recover from an error here
	mcode_or_throw("new_conn: curl_multi_add_handle", rc);
}

void AsyncHTTPRequester::fetch(std::string const& url, boost::function< void(CURLcode, std::string const&) > cb, std::string const& postdata)
{
    TRACE("AsyncHTTPRequester::fetch");

    if ( selfmanage_ ) ptr_to_this_ = shared_from_this();
    cb_ = cb;
    // Clear the buffer of any data from a previous request
    buf_.str( std::string() );

    curl_easy_setopt(easy_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy_, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(easy_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(easy_, CURLOPT_PRIVATE, this);
    curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_TIME, 3L);
    curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_LIMIT, 10L);
    if ( !c_.verify_ssl_peer_ )
    {
        curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    if ( !postdata.empty() )
    {
        curl_easy_setopt(easy_, CURLOPT_POST, 1);
        curl_easy_setopt(easy_, CURLOPT_HTTPHEADER, headers_);
        curl_easy_setopt(easy_, CURLOPT_POSTFIELDSIZE, postdata.length());
        curl_easy_setopt(easy_, CURLOPT_COPYPOSTFIELDS, postdata.c_str());
    }
    else
    {
        curl_easy_setopt(easy_, CURLOPT_HTTPGET, 1);
        curl_easy_setopt(easy_, CURLOPT_HTTPHEADER, 0);
    }

    /* call this function to get a socket */
    curl_easy_setopt(easy_, CURLOPT_OPENSOCKETFUNCTION, opensocket);
    curl_easy_setopt(easy_, CURLOPT_OPENSOCKETDATA, &c_);

    /* call this function to close a socket */
    curl_easy_setopt(easy_, CURLOPT_CLOSESOCKETFUNCTION, closesocket);
    curl_easy_setopt(easy_, CURLOPT_CLOSESOCKETDATA, &c_);

    c_.strand_.dispatch(boost::bind(context_multi_add_handle, boost::ref(c_), easy_));
}

void AsyncHTTPRequester::done(CURLcode const rc)
{
    TRACE("AsyncHTTPRequester::done");

    CURLMcode mc = curl_multi_remove_handle(c_.multi_, easy_);
    mcode_or_throw("new_conn: curl_multi_add_handle", mc);

    cb_(rc, buf_.str());
    ptr_to_this_.reset();

    // Note: We keep the contents of the buffer until the next fetch call
    // or this object is destroyed.
}

AsyncHTTPRequester::~AsyncHTTPRequester()
{
    TRACE("AsyncHTTPRequester::~AsyncHTTPRequester");

    curl_slist_free_all(headers_);
    curl_easy_cleanup(easy_);
}

Context::Context( boost::asio::io_service& io_service, bool verify_ssl_peer )
: timer_( io_service )
, strand_( io_service )
, io_service_( io_service )
, still_running_( 0 )
, verify_ssl_peer_( verify_ssl_peer )
{
    multi_ = curl_multi_init();

    curl_multi_setopt(multi_, CURLMOPT_SOCKETFUNCTION, sock_cb);
    curl_multi_setopt(multi_, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(multi_, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    curl_multi_setopt(multi_, CURLMOPT_TIMERDATA, this);
}

Context::~Context()
{
    curl_multi_cleanup(multi_);
}

} // namespace curl
