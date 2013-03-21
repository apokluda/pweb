/*
 * haproxy.hpp
 *
 *  Created on: 2013-03-04
 *      Author: apokluda
 */

#ifndef HAPROXY_HPP_
#define HAPROXY_HPP_

#include "stdhdr.hpp"

class dnsquery;
typedef boost::shared_ptr< dnsquery > query_ptr;

namespace haproxy
{
    void timeout( boost::posix_time::time_duration const& );
}

class hasendproxy :
        private boost::noncopyable,
        public boost::enable_shared_from_this<hasendproxy>
{
public:
    hasendproxy(boost::asio::io_service& io_service,
            std::string const& hahostname, boost::uint16_t const haport,
            std::string const& nshostname, boost::uint16_t const nsport,
            std::string const& suffix);

    bool enabled() const
    {
        return enabled_.load(boost::memory_order_acquire);
    }

    void start();
    void process_query( query_ptr query );

    std::string hostname() const
    {
        return hahostname_;
    }

private:
    void handle_resolve(boost::system::error_code const& ec, boost::asio::ip::tcp::resolver::iterator iter);
    void handle_query_sent(boost::system::error_code const& ec, std::size_t const bytes_tranferred);

    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::resolver::iterator iter_;
    std::string const hahostname_;
    std::string const nshostname_;
    std::string const suffix_;
    boost::uint16_t const haport_;
    boost::uint16_t const nsport_;
    boost::atomic< bool > enabled_;
};

class harecvconnection;

class harecvproxy
{
public:
    harecvproxy(boost::asio::io_service& io_service, std::string const& nshostname, const boost::uint16_t nsport, boost::uint16_t const ttl);

    void start();

private:
    void handle_accept(boost::system::error_code const& ec);

    typedef boost::shared_ptr< harecvconnection > recv_connection_ptr;

    recv_connection_ptr new_connection_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::string const nshostname_;
    boost::uint16_t const ttl_;
};

#endif /* HAPROXY_HPP_ */
