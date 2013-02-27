/*
 * haspeaker.cpp
 *
 *  Created on: 2013-02-23
 *      Author: alex
 */

#include "stdhdr.hpp"
#include "haspeaker.hpp"
#include "dnsquery.hpp"

extern log4cpp::Category& log4;

typedef std::stringstream sstream;
using std::vector;
using std::string;
using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

void haspeaker::connect(bs::error_code const& ec)
{
    if ( !ec )
    {
        vector<string> address;
        boost::split( address, haaddress_, boost::is_any_of(":") );
        if ( address.size() == 2 )
        {
            ip::tcp::resolver::query query(address[0], address[1]);
            resolver_.async_resolve(query,
                    boost::bind(&haspeaker::handle_resolve, shared_from_this(), ph::error, ph::iterator));
        }
        else
        {
            log4.errorStream() << "Invalid home agent address format: " << haaddress_;
            log4.noticeStream() << "Unable to connect to home agent at address " << haaddress_;
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while the connection retry timer was running for connection to home agent at address " << haaddress_;
        log4.noticeStream() << "No longer trying to connect to home agent at address " << haaddress_;
    }
}

void haspeaker::handle_resolve(bs::error_code const& ec, ip::tcp::resolver::iterator iter)
{
    if ( !ec )
    {
        async_connect(socket_, iter,
                boost::bind(&haspeaker::handle_connect, shared_from_this(), ph::error));
    }
    else
    {
        log4.errorStream() << "Unable to resolve home agent address " << haaddress_ << ": " << ec.message();
        log4.noticeStream() << "No longer trying to connect to home agent at address " << haaddress_;
    }
}

void haspeaker::handle_connect(bs::error_code const& ec)
{
    using boost::posix_time::seconds;

    if ( !ec )
    {
        // Connected to home agent
        log4.infoStream() << "Connected to home agent at address " << haaddress_;
        connected_ = true;

        errwait_ = seconds(0); // reset connection error timer

        // TODO: Send hello message?
    }
    else
    {
        log4.errorStream() << "Unable to connect to home agent at address " << haaddress_ << ": " << ec.message();

        if (errwait_ < seconds(10)) errwait_ += seconds(2);

        log4.noticeStream() << "Retrying connection to " << haaddress_ << " in " << errwait_;

        retrytimer_.expires_from_now(errwait_);
        retrytimer_.async_wait(boost::bind( &haspeaker::connect, shared_from_this(), ph::error ));
    }
}

template < class hacontainer_iter_t, class hacontainer_diff_t >
void ha_load_balancer< hacontainer_iter_t, hacontainer_diff_t >::process_query( query_ptr query )
{
    for (hacontainer_diff_t cnt = 0; cnt < size_; ++cnt)
    {
        hacontainer_diff_t my_offset = offset_.fetch_add(1, boost::memory_order_relaxed);
        hacontainer_iter_t hs = begin_ + (my_offset % size_);
        if ( (*hs)->connected() )
        {
            (*hs)->process_query( query );
            return;
        }
    }
    log4.warnStream() << "Unable to process query from " << query->remote_address() << ": Not connected to any home agents";

    query->rcode(R_SERVER_FAILURE);
    query->send_reply();
}

typedef boost::shared_ptr< haspeaker > haspeaker_ptr;
typedef std::vector< haspeaker_ptr > haspeakers_t;
template class ha_load_balancer< haspeakers_t::iterator, haspeakers_t::difference_type >;
