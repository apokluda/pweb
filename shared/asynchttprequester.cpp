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

#define LOGSTREAM if ( debug ) log4.debugStream()

#ifndef NDEBUG
#define TRACE(func) LOGSTREAM << "Entering: " func << std::endl
#define TRACE_MSG(msg) LOGSTREAM << "Trace: " msg << std::endl
#else
#define TRACE(func)
#define TRACE_MSG(msg)
#endif

extern log4cpp::Category& log4;

namespace curl
{
bool debug = true;

std::map<curl_socket_t, boost::asio::ip::tcp::socket *> socket_map;

void timer_cb(const boost::system::error_code & error, Context *g);

/* Update the event timer after curl_multi library calls */
int multi_timer_cb(CURLM *multi, long timeout_ms, Context* c)
{
	TRACE("multi_timer_cb");

	//if ( c->mutex_.try_lock() ) c->mutex_.unlock();
	//else std::cerr << "multi_timer_cb - MUTEX IS ALREADY LOCKED!!" << std::endl;
	//Context::guard_t l( c->mutex_ );


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
        c->timer_.async_wait(boost::bind(&timer_cb, _1, c));
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

	//if ( c->mutex_.try_lock() ) c->mutex_.unlock();
	//else std::cerr << "check_multi_info - MUTEX IS ALREADY LOCKED!!" << std::endl;
	//Context::lock_t l( c->mutex_ );

	//char *eff_url;
    CURLMsg *msg;
    int msgs_left;
    AsyncHTTPRequester* conn;

    while ((msg = curl_multi_info_read(c->multi_, &msgs_left)))
    {
        if (msg->msg == CURLMSG_DONE)
        {
            CURL* easy = msg->easy_handle;
            CURLcode res = msg->data.result;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            //curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);

            // Note: We unlock the mutex before calling conn->done() because
            // done() will execute the callback to the user code that may start
            // another AsyncHTTRequest operation. This would cause a deadlock
            // if we were still holding the lock. (In reality, done() is going
            // will reacquire the lock for a brief period. Releasing and
            // reacquiring the lock is the cleanest solution without an elegant
            // way to transfer the ownership of the lock from one function
            // to another)
            //l.unlock();
            conn->done(res);
        }
    }
}

/* Called by asio when there is an action on a socket */
void event_cb(Context* c, boost::asio::ip::tcp::socket * tcp_socket, int action)
{
    TRACE("event_cb");

	//if ( c->mutex_.try_lock() ) c->mutex_.unlock();
	//else std::cerr << "event_cb - MUTEX IS ALREADY LOCKED!!" << std::endl;
	//Context::guard_t l( c->mutex_ );

    CURLMcode rc = curl_multi_socket_action(c->multi_, tcp_socket->native_handle(), action, &c->still_running_);

    mcode_or_throw("event_cb: curl_multi_socket_action", rc);
    check_multi_info(c);

    if ( c->still_running_ <= 0 )
    {
        c->timer_.cancel();
    }
}

/* Called by asio when our timeout expires */
void timer_cb(const boost::system::error_code & error, Context* c)
{
	TRACE("timer_cb");

	//if ( c->mutex_.try_lock() ) c->mutex_.unlock();
	//else std::cerr << "timer_cb - MUTEX IS ALREADY LOCKED!!" << std::endl;
	//Context::guard_t l( c->mutex_ );

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

/* Clean up any data */
void remsock(int *f, Context* c)
{
    TRACE("remsock");

    if ( f ) free( f );
}

void setsock(int *fdp, curl_socket_t s, CURL*e, int act, Context* c)
{
    TRACE("setsock");

    boost::asio::ip::tcp::socket* tcp_socket;
    {
    	//if ( c->mutex_.try_lock() ) c->mutex_.unlock();
    	//else std::cerr << "setsock - MUTEX IS ALREADY LOCKED!!" << std::endl;
    	//Context::guard_t l( c->mutex_ );

        std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(s);
        if ( it == socket_map.end() ) return;
        tcp_socket = it->second;
    }

    // Note: The following is not protected by the context mutex!

    *fdp = act;
    if ( act == CURL_POLL_IN )
    {
        tcp_socket->async_read_some(boost::asio::null_buffers(),
                boost::bind(&event_cb, c,
                        tcp_socket,
                        act));
    }
    else if ( act == CURL_POLL_OUT )
    {
        tcp_socket->async_write_some(boost::asio::null_buffers(),
                boost::bind(&event_cb, c,
                        tcp_socket,
                        act));
    }
    else if ( act == CURL_POLL_INOUT )
    {
        tcp_socket->async_read_some(boost::asio::null_buffers(),
                boost::bind(&event_cb, c,
                        tcp_socket,
                        act));

        tcp_socket->async_write_some(boost::asio::null_buffers(),
                boost::bind(&event_cb, c,
                        tcp_socket,
                        act));
    }
}


void addsock(curl_socket_t s, CURL *easy, int action, Context *c)
{
	TRACE("addsock");

	//if ( c->mutex_.try_lock() ) c->mutex_.unlock();
	//else std::cerr << "addsock - MUTEX IS ALREADY LOCKED!!" << std::endl;
	//Context::guard_t l( c->mutex_ );

	int *fdp = (int *)calloc(sizeof(int), 1); /* fdp is used to store current action */

    setsock(fdp, s, easy, action, c);
    curl_multi_assign(c->multi_, s, fdp);
}

/* CURLMOPT_SOCKETFUNCTION */
int sock_cb(CURL *e, curl_socket_t s, int what, Context* c, int* actionp)
{
	// Note: We don't synchronize access to the context here!
	// Synchronization is implemented in the remsock, addsock and setsock
	// functions.

	TRACE("sock_cb");

    if ( what == CURL_POLL_REMOVE )
    {
        remsock(actionp, c);
    }
    else
    {
        if ( !actionp )
        {
            addsock(s, e, what, c);
        }
        else
        {
            setsock(actionp, s, e, what, c);
        }
    }
    return 0;
}

/* CURLOPT_WRITEFUNCTION */
size_t write_cb(char* ptr, size_t size, size_t nmemb, AsyncHTTPRequester* r)
{
	// I think that we can trust curl not to call our write_cb concurrently...

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

	//if ( c->mutex_.try_lock() ) c->mutex_.unlock();
	//else std::cerr << "opensocket - MUTEX IS ALREADY LOCKED!!" << std::endl;
	//Context::guard_t l( c->mutex_ ); // protects socket map

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
            socket_map.insert(std::pair<curl_socket_t, boost::asio::ip::tcp::socket *>(sockfd, tcp_socket));
        }
    }

    return sockfd;
}

/* CURLOPT_CLOSESOCKETFUNCTION */
int closesocket(Context* c, curl_socket_t item)
{
	TRACE("closesocket");

	//if ( c->mutex_.try_lock() ) c->mutex_.unlock();
	//else std::cerr << "closesocket - MUTEX IS ALREADY LOCKED!!" << std::endl;
	//Context::guard_t l( c->mutex_ ); // protects map

    std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(item);

    if ( it != socket_map.end() )
    {
        delete it->second;
        socket_map.erase(it);
    }

    return 0;
}

AsyncHTTPRequester::AsyncHTTPRequester(Context& c, bool const selfmanage)
: rc_( CURLM_OK )
, easy_( curl_easy_init() )
, c_( c )
, headers_( curl_slist_append(0, "Content-type: text/xml") )
, selfmanage_( selfmanage )
{
    if ( !easy_ || !headers_ ) throw std::runtime_error("liburl initialization error");
}

void context_multi_add_handle(Context& c, CURL* easy)
{
	CURLMcode rc;
	{
		//if ( c_.mutex_.try_lock() ) c_.mutex_.unlock();
		//else std::cerr << "fetch - MUTEX IS ALREADY LOCKED!!" << std::endl;
		//Context::guard_t l( c_.mutex_ );

		rc = curl_multi_add_handle(c_.multi_, easy_);
	}
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

    c_.strand_.dispatch(boost::bind(context_multi_add_handle, c_, easy_));
}

void AsyncHTTPRequester::done(CURLcode const rc)
{
    TRACE("AsyncHTTPRequester::done");

    CURLMcode mc;
    {
    	//if ( c_.mutex_.try_lock() ) c_.mutex_.unlock();
    	//else std::cerr << "done - MUTEX IS ALREADY LOCKED!!" << std::endl;
    	//Context::guard_t l( c_.mutex_ );

    	mc = curl_multi_remove_handle(c_.multi_, easy_);
    }
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

Context::Context( boost::asio::io_service& io_service )
: timer_( io_service )
, io_service_( io_service )
, still_running_( 0 )
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
