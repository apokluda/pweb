/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2012, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/*
 * file: asiohiper.cpp
 * Example program to demonstrate the use of multi socket interface
 * with boost::asio
 *
 * This program is in c++ and uses boost::asio instead of libevent/libev.
 * Requires boost::asio, boost::bind and boost::system
 *
 * This is an adaptation of libcurl's "hiperfifo.c" and "evhiperfifo.c"
 * sample programs. This example implements a subset of the functionality from
 * hiperfifo.c, for full functionality refer hiperfifo.c or evhiperfifo.c
 *
 * Written by Lijo Antony based on hiperfifo.c by Jeff Pohlmeyer
 *
 * When running, the program creates an easy handle for a URL and
 * uses the curl_multi API to fetch it.
 *
 * Note:
 *  For the sake of simplicity, URL is hard coded to "www.google.com"
 *
 * This is purely a demo app, all retrieved data is simply discarded by the write
 * callback.
 */

#include "stdhdr.hpp"
#include "asiohelper.hpp"
//#include <curl/curl.h>
//#include <boost/asio.hpp>
//#include <boost/bind.hpp>

namespace curl
{

//#define MSG_OUT stdout /* Send info to stdout, change to stderr if you want */

/* boost::asio related objects
 * using global variables for simplicity
 */
//boost::asio::io_service io_service_;
Context* context = 0;

//boost::asio::deadline_timer timer(io_service);

std::map<curl_socket_t, boost::asio::ip::tcp::socket *> socket_map;

/* Information associated with a specific easy handle */
//typedef struct _ConnInfo
//{
//    CURL *easy;
//    char *url;
//    GlobalInfo *global;
//    char error[CURL_ERROR_SIZE];
//} ConnInfo;

void timer_cb(const boost::system::error_code & error, Context *g);

/* Update the event timer after curl_multi library calls */
int multi_timer_cb(CURLM *multi, long timeout_ms, Context* c)
{
    //fprintf(MSG_OUT, "\nmulti_timer_cb: timeout_ms %ld", timeout_ms);

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
        default: s="CURLM_unknown";
        break;
        case     CURLM_BAD_SOCKET:         s="CURLM_BAD_SOCKET";
        //fprintf(MSG_OUT, "\nERROR: %s returns %s", where, s);
        /* ignore this error */
        return;
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
    CURL *easy;
    CURLcode res;

    //fprintf(MSG_OUT, "\nREMAINING: %d", g->still_running);

    while ((msg = curl_multi_info_read(c->multi_, &msgs_left)))
    {
        if (msg->msg == CURLMSG_DONE)
        {
            easy = msg->easy_handle;
            res = msg->data.result;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
            //fprintf(MSG_OUT, "\nDONE: %s => (%d) %s", eff_url, res, conn->error);

// TODO: call the AsyncHTTPRequester callback method here!!

            curl_multi_remove_handle(c->multi_, easy);
            //free(conn->url);
            curl_easy_cleanup(easy);
            //free(conn);
        }
    }
}

/* Called by asio when there is an action on a socket */
void event_cb(Context* c, boost::asio::ip::tcp::socket * tcp_socket, int action)
{
    //fprintf(MSG_OUT, "\nevent_cb: action=%d", action);

    CURLMcode rc;
    rc = curl_multi_socket_action(c->multi_, tcp_socket->native_handle(), action, &c->still_running_);

    mcode_or_throw("event_cb: curl_multi_socket_action", rc);
    check_multi_info(c);

    if ( c->still_running_ <= 0 )
    {
        //fprintf(MSG_OUT, "\nlast transfer done, kill timeout");
        c->timer_.cancel();
    }
}

/* Called by asio when our timeout expires */
void timer_cb(const boost::system::error_code & error, Context* c)
{
    if ( !error)
    {
        //fprintf(MSG_OUT, "\ntimer_cb: ");

        CURLMcode rc;
        rc = curl_multi_socket_action(c->multi_, CURL_SOCKET_TIMEOUT, 0, &c->still_running_);

        mcode_or_throw("timer_cb: curl_multi_socket_action", rc);
        check_multi_info(c);
    }
}

/* Clean up any data */
void remsock(int *f, Context* c)
{
    //fprintf(MSG_OUT, "\nremsock: ");

    if ( f )
    {
        free(f);
    }
}

void setsock(int *fdp, curl_socket_t s, CURL*e, int act, Context* c)
{
    //fprintf(MSG_OUT, "\nsetsock: socket=%d, act=%d, fdp=%p", s, act, fdp);

    std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(s);

    if ( it == socket_map.end() ) return;
    //{
        //fprintf(MSG_OUT, "\nsocket %d is a c-ares socket, ignoring", s);
    //    return;
    //}

    boost::asio::ip::tcp::socket * tcp_socket = it->second;

    *fdp = act;

    if ( act == CURL_POLL_IN )
    {
        //fprintf(MSG_OUT, "\nwatching for socket to become readable");

        tcp_socket->async_read_some(boost::asio::null_buffers(),
                boost::bind(&event_cb, c,
                        tcp_socket,
                        act));
    }
    else if ( act == CURL_POLL_OUT )
    {
        //fprintf(MSG_OUT, "\nwatching for socket to become writable");

        tcp_socket->async_write_some(boost::asio::null_buffers(),
                boost::bind(&event_cb, c,
                        tcp_socket,
                        act));
    }
    else if ( act == CURL_POLL_INOUT )
    {
        //fprintf(MSG_OUT, "\nwatching for socket to become readable & writable");

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
int sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp)
{
    //fprintf(MSG_OUT, "\nsock_cb: socket=%d, what=%d, sockp=%p", s, what, sockp);

    Context *c = (Context*) cbp;
    int *actionp = (int*) sockp;
    //const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE"};

    //fprintf(MSG_OUT,
    //        "\nsocket callback: s=%d e=%p what=%s ", s, e, whatstr[what]);

    if ( what == CURL_POLL_REMOVE )
    {
        //fprintf(MSG_OUT, "\n");
        remsock(actionp, c);
    }
    else
    {
        if ( !actionp )
        {
            //fprintf(MSG_OUT, "\nAdding data: %s", whatstr[what]);
            addsock(s, e, what, c);
        }
        else
        {
            //fprintf(MSG_OUT,
            //        "\nChanging action from %s to %s",
            //        whatstr[*actionp], whatstr[what]);
            setsock(actionp, s, e, what, c);
        }
    }
    return 0;
}


/* CURLOPT_WRITEFUNCTION */
size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
// NEEDS UPDATING!! data IS A POINTER TO AsyncHTTPRequester. Store data in buffer there.
    size_t written = size * nmemb;
    char* pBuffer = (char*)malloc(written + 1);

    strncpy(pBuffer, (const char *)ptr, written);
    pBuffer [written] = '\0';

    //fprintf(MSG_OUT, "%s", pBuffer);

    free(pBuffer);

    return written;
}


/* CURLOPT_PROGRESSFUNCTION */
//static int prog_cb (void *p, double dltotal, double dlnow, double ult,
//        double uln)
//{
//    ConnInfo *conn = (ConnInfo *)p;
//    (void)ult;
//    (void)uln;
//
//    fprintf(MSG_OUT, "\nProgress: %s (%g/%g)", conn->url, dlnow, dltotal);
//    fprintf(MSG_OUT, "\nProgress: %s (%g)", conn->url, ult);
//
//    return 0;
//}

/* CURLOPT_OPENSOCKETFUNCTION */
curl_socket_t opensocket(void *clientp,
        curlsocktype purpose,
        struct curl_sockaddr *address)
{
    //fprintf(MSG_OUT, "\nopensocket :");

    curl_socket_t sockfd = CURL_SOCKET_BAD;

    /* restrict to ipv4 */
    if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET)
    {
        /* create a tcp socket object */
        boost::asio::ip::tcp::socket *tcp_socket = new boost::asio::ip::tcp::socket(context->io_service_);

        /* open it and get the native handle*/
        boost::system::error_code ec;
        tcp_socket->open(boost::asio::ip::tcp::v4(), ec);

        if (ec)
        {
            //An error occurred
            std::cout << std::endl << "Couldn't open socket [" << ec << "][" << ec.message() << "]";
            //fprintf(MSG_OUT, "\nERROR: Returning CURL_SOCKET_BAD to signal error");
        }
        else
        {
            sockfd = tcp_socket->native_handle();
            //fprintf(MSG_OUT, "\nOpened socket %d", sockfd);

            /* save it for monitoring */
            socket_map.insert(std::pair<curl_socket_t, boost::asio::ip::tcp::socket *>(sockfd, tcp_socket));
        }
    }

    return sockfd;
}

/* CURLOPT_CLOSESOCKETFUNCTION */
int closesocket(void *clientp, curl_socket_t item)
{
    //fprintf(MSG_OUT, "\nclosesocket : %d", item);

    std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(item);

    if ( it != socket_map.end() )
    {
        delete it->second;
        socket_map.erase(it);
    }

    return 0;
}

/* Create a new easy handle, and add it to the global curl_multi */
//CURLMcode new_conn(char *url, CURL* easy, GlobalInfo *g )
//{
//    //ConnInfo *conn;
//    //CURLMcode rc;
//
//    //conn = (ConnInfo *)calloc(1, sizeof(ConnInfo));
//    //memset(conn, 0, sizeof(ConnInfo));
//    //conn->error[0]='\0';
//
//    //conn->easy = curl_easy_init();
//
//    //if ( !conn->easy )
//    //{
//    //    fprintf(MSG_OUT, "\ncurl_easy_init() failed, exiting!");
//    //    exit(2);
//    //}
//    //conn->global = g;
//    //conn->url = strdup(url);
//    curl_easy_setopt(easy, CURLOPT_URL, url);
//    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_cb);
//    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &conn);
//    //curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
//    //curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, conn->error);
//    curl_easy_setopt(easy, CURLOPT_PRIVATE, conn);
//    //curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 1L);
//    //curl_easy_setopt(easy, CURLOPT_PROGRESSFUNCTION, prog_cb);
//    //curl_easy_setopt(easy, CURLOPT_PROGRESSDATA, conn);
//    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, 3L);
//    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, 10L);
//
//    /* call this function to get a socket */
//    curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, opensocket);
//
//    /* call this function to close a socket */
//    curl_easy_setopt(easy, CURLOPT_CLOSESOCKETFUNCTION, closesocket);
//
//    //fprintf(MSG_OUT,
//    //        "\nAdding easy %p to multi %p (%s)", conn->easy, g->multi, url);
//    return curl_multi_add_handle(g->multi, conn->easy);
//    //mcode_or_die("new_conn: curl_multi_add_handle", rc);
//
//    /* note that the add_handle() will set a time-out to trigger very soon so
//     that the necessary socket_action() call will be called by this app */
//}

void AsyncHTTPRequester::fetch(std::string const& url, boost::function< void(CURLMcode, std::string const&) > cb )
{
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

    /* call this function to close a socket */
    curl_easy_setopt(easy_, CURLOPT_CLOSESOCKETFUNCTION, closesocket);

    CURLMcode rc = curl_multi_add_handle(c_.multi_, easy_);
    mcode_or_throw("new_conn: curl_multi_add_handle", rc);
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

    context = this;
}

Context::~Context()
{
    context = 0;

    curl_multi_cleanup(multi_);
}

//int run_curl_example(int argc, char **argv)
//{
//    GlobalInfo g;
//    CURLMcode rc;
//    (void)argc;
//    (void)argv;
//
//    memset(&g, 0, sizeof(GlobalInfo));
//    g.multi = curl_multi_init();
//
//    curl_multi_setopt(g.multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
//    curl_multi_setopt(g.multi, CURLMOPT_SOCKETDATA, &g);
//    curl_multi_setopt(g.multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
//    curl_multi_setopt(g.multi, CURLMOPT_TIMERDATA, &g);
//
//    new_conn((char *)"www.google.com", &g);  // add a URL
//
//    // enter io_service run loop
//    io_service.run();
//
//    curl_multi_cleanup(g.multi);
//
//    fprintf(MSG_OUT, "\ndone.\n");
//    return 0;
//}

} // namespace curl
