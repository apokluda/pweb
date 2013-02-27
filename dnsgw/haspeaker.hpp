/*
 * haspeaker.hpp
 *
 *  Created on: 2013-02-23
 *      Author: alex
 */

#ifndef HASPEAKER_HPP_
#define HASPEAKER_HPP_

#include "stdhdr.hpp"

extern log4cpp::Category& log4;

class dnsquery;
typedef boost::shared_ptr< dnsquery > query_ptr;

class haspeaker
: private boost::noncopyable
, public boost::enable_shared_from_this<haspeaker>
{
public:
    haspeaker(boost::asio::io_service& io_service, std::string const& haaddr, std::string const& suffix)
    : haaddress_(haaddr)
    , suffix_(suffix)
    , strand_(io_service)
    , resolver_(io_service)
    , retrytimer_(io_service)
    , socket_(io_service)
    , connected_(false)
    {
    }

    bool connected() const
    {
        return connected_;
    }

    void process_query( query_ptr query )
    {
        strand_.dispatch(
                boost::bind( &haspeaker::process_query_, shared_from_this(), query ) );
    }

    void connect( boost::system::error_code const& ec = boost::system::error_code() );

private:
    void handle_resolve(boost::system::error_code const& ec, boost::asio::ip::tcp::resolver::iterator iter);
    void handle_connect(boost::system::error_code const& ec);

    void process_query_( query_ptr query );

    boost::posix_time::time_duration errwait_;
    std::string haaddress_;
    std::string suffix_;
    boost::asio::io_service::strand strand_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::deadline_timer retrytimer_;
    boost::asio::ip::tcp::socket socket_;
    bool connected_; // potentially accessed from different threads, but *should* be ok
};

// In future, the ha_load_balancer could be made into a
// class hierarchy with subclasses implementing different
// load balancing algorithms. The algorithm/subclass to
// instantiate could be selected from a config parameter.

// hacontainer_iter_t = random access iterator
// Note: right now, we're using plain but const STL containers.
// If we want to add/remove on the fly, then we will need to use
// concurrent containers, such as the ones provide by Intel TBB

// Perhaps the number of template parameters could be reduced from 2
// down to 1 with some sort of traits magic, but this is fine for now...
template < class hacontainer_iter_t, class hacontainer_diff_t >
class ha_load_balancer
{
public:
    ha_load_balancer( boost::asio::io_service& io_service, hacontainer_iter_t const haspeakers_begin, std::size_t const num_elements)
    : offset_(0)
    , begin_(haspeakers_begin)
    , size_(num_elements)
    {
    }

    void process_query( query_ptr query );

private:
    boost::atomic< hacontainer_diff_t > mutable offset_;
    hacontainer_iter_t const begin_;
    hacontainer_diff_t const size_;
};

#endif /* HASPEAKER_HPP_ */
