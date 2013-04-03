/*
 * file: asiohiper.cpp
 *
 * Based on example program to demonstrate the use of multi socket interface
 * with boost::asio written by Lijo Antony based on hiperfifo.c by Jeff Pohlmeyer
 * and available from http://curl.haxx.se/libcurl/c/asiohiper.html
 *
 */

#include "stdhdr.hpp"
#include "asiohelper.hpp"

namespace curl
{

std::map<curl_socket_t, boost::asio::ip::tcp::socket *> socket_map;

void timer_cb(const boost::system::error_code & error, Context *g);

/* Update the event timer after curl_multi library calls */
int multi_timer_cb(CURLM *multi, long timeout_ms, Context* c)
{
    /* cancel running timer */
    c->timer_.cancel();

    if ( timeout_ms > 0 )
    {
        /* update timer */
        c->timer_.expires_from_now(boost::posix_time::millisec(timeout_ms));
        c->timer_.async_wait(boost::bind(&timer_cb, _1, c));
    }
    else
    {
        /* call timeout function immediately */
        boost::system::error_code error; /*success*/
        timer_cb(error, c);
    }

    return 0;
}

/* Throw if we get a bad CURLMcode somewhere */
void mcode_or_throw(const char *where, CURLMcode code)
{
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
    char *eff_url;
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
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);

            conn->done(res);
        }
    }
}

/* Called by asio when there is an action on a socket */
void event_cb(Context* c, boost::asio::ip::tcp::socket * tcp_socket, int action)
{
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
    if ( !error)
    {
        CURLMcode rc = curl_multi_socket_action(c->multi_, CURL_SOCKET_TIMEOUT, 0, &c->still_running_);

        mcode_or_throw("timer_cb: curl_multi_socket_action", rc);
        check_multi_info(c);
    }
}

/* Clean up any data */
void remsock(int *f, Context* c)
{
    if ( f ) free( f );
}

void setsock(int *fdp, curl_socket_t s, CURL*e, int act, Context* c)
{
    std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(s);

    if ( it == socket_map.end() ) return;

    boost::asio::ip::tcp::socket* tcp_socket = it->second;

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
    int *fdp = (int *)calloc(sizeof(int), 1); /* fdp is used to store current action */

    setsock(fdp, s, easy, action, c);
    curl_multi_assign(c->multi_, s, fdp);
}

/* CURLMOPT_SOCKETFUNCTION */
int sock_cb(CURL *e, curl_socket_t s, int what, Context* c, int* actionp)
{
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
    size_t written = size * nmemb;
    std::size_t oldsize = r->buf_.tellp();

    if ( oldsize + written > AsyncHTTPRequester::MAX_BUF_SIZE )
        written = AsyncHTTPRequester::MAX_BUF_SIZE - oldsize;

    r->buf_.write(ptr, written);

    return written;
}

/* CURLOPT_OPENSOCKETFUNCTION */
curl_socket_t opensocket(AsyncHTTPRequester* r, curlsocktype purpose, struct curl_sockaddr *address)
{
    //fprintf(MSG_OUT, "\nopensocket :");

    curl_socket_t sockfd = CURL_SOCKET_BAD;

    /* restrict to ipv4 */
    if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET)
    {
        /* create a tcp socket object */
        boost::asio::ip::tcp::socket *tcp_socket = new boost::asio::ip::tcp::socket(r->c_.io_service_);

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
int closesocket(void *clientp, curl_socket_t item)
{
    std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(item);

    if ( it != socket_map.end() )
    {
        delete it->second;
        socket_map.erase(it);
    }

    return 0;
}

void AsyncHTTPRequester::fetch(std::string const& url, boost::function< void(CURLcode, std::string const&) > cb )
{
    ptr_to_this_ = shared_from_this();
    cb_ = cb;
    easy_ = curl_easy_init();

    curl_easy_setopt(easy_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy_, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(easy_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(easy_, CURLOPT_PRIVATE, this);
    curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_TIME, 3L);
    curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_LIMIT, 10L);

    /* call this function to get a socket */
    curl_easy_setopt(easy_, CURLOPT_OPENSOCKETFUNCTION, opensocket);
    curl_easy_setopt(easy_, CURLOPT_OPENSOCKETDATA, this);

    /* call this function to close a socket */
    curl_easy_setopt(easy_, CURLOPT_CLOSESOCKETFUNCTION, closesocket);
    curl_easy_setopt(easy_, CURLOPT_CLOSESOCKETDATA, this);

    CURLMcode rc = curl_multi_add_handle(c_.multi_, easy_);
    mcode_or_throw("new_conn: curl_multi_add_handle", rc);
}

AsyncHTTPRequester::~AsyncHTTPRequester()
{
    curl_multi_remove_handle(c_.multi_, easy_);
    curl_easy_cleanup(easy_);

    std::cerr << "DEBUG: AsyncHTTPRequester destroyed!" << std::endl;
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
