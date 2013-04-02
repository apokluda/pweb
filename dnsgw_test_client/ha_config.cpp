/*
 * ha_config.cpp
 *
 *  Created on: 2013-04-02
 *      Author: alex
 */

#include "ha_config.hpp"

using std::string;
using boost::lexical_cast;
using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

extern log4cpp::Category& log4;

char const* hastate_to_string( hastate_t const state )
{
    switch ( state )
    {
    case GOOD:
        return "GOOD";
    case URL_IP_MISMATCH:
        return "URL_IP_MISMATCH";
    case UNABLE_TO_CONNECT:
        return "UNABLE TO CONNECT";
    default:
        return "UNKNOWN";
    }
}

void load_halist(halist_t& halist, std::istream& is)
{
    // Estimate max possible line length:
    // url + space + port + space  + ip address + haname
    // 255 +   1   +  5   +   1    +      39    +  128   = 429 (round up)-> 512
    char buf[512];

    is.getline(buf, sizeof(buf));
    int num = lexical_cast< int >( buf );
    halist.reserve(num);

    // Done here and at end of while loop so that we can easily detect EOF
    is.getline(buf, sizeof(buf), ' ');

    while ( is.good() )
    {
        haconfig ha;

        // Read URL
        ha.url = buf;

        // Read port
        is.getline(buf, sizeof(buf), ' ');
        ha.port = buf;

        // Read IP address
        is.getline(buf, sizeof(buf), ' ');
        ha.ip = ip::address::from_string(buf);

        // Read home agent name
        is.getline(buf, sizeof(buf));
        ha.haname = buf;

        ha.status = UNKNOWN;

        halist.push_back( ha );

        // Attempt to read next URL; break out of loop if EOF
        is.getline(buf, sizeof(buf), ' ');
    }
}

class ha_checker_impl : boost::noncopyable, public boost::enable_shared_from_this< ha_checker_impl >
{
public:
    ha_checker_impl( boost::asio::io_service& io_service, haconfig& haconfig)
    : resolver_( io_service )
    , socket_( io_service )
    , haconfig_( haconfig )
    {
    }

    void start( boost::function< void(void) > cb )
    {
        cb_ = cb;

        // Resolve host name to IP address and check that it matches
        log4.infoStream() << "Performing DNS lookup for " << haconfig_.url;

        ip::tcp::resolver::query query( haconfig_.url, string(haconfig_.port) );
        resolver_.async_resolve(query, boost::bind( &ha_checker_impl::handle_resolve, shared_from_this(), ph::error, ph::iterator ) );
    }

private:
    void handle_resolve( bs::error_code const& ec, ip::tcp::resolver::iterator endpoint_iterator )
    {
        if ( !ec )
        {
            ip::tcp::resolver::iterator end;
            ip::tcp::resolver::iterator begin = endpoint_iterator;
            while ( begin != end ) if ( begin->endpoint().address() == haconfig_.ip ) goto success;

            log4.errorStream() << "DNS lookup did not return a matching IP for " << haconfig_.url;
            haconfig_.status = URL_IP_MISMATCH;
            done();
            return;

            success:
            // Now try to connect
            async_connect(socket_, endpoint_iterator, boost::bind( &ha_checker_impl::handle_connect, shared_from_this(), ph::error ) );
            return;
        }
        else
        {
            log4.errorStream() << "DNS lookup failed for " << haconfig_.url;
            haconfig_.status = DNS_LOOKUP_FAILED;
            done();
        }
    }

    void handle_connect( bs::error_code const& ec )
    {
        if ( !ec )
        {
            log4.infoStream() << "Successfully connected to " << haconfig_.url;
            haconfig_.status = GOOD;
            // Fall through to done()
        }
        else
        {
            log4.errorStream() << "Unable to connect to " << haconfig_.url;
            haconfig_.status = UNABLE_TO_CONNECT;
            // Fall through to done()
        }
        done();
    }

    void done()
    {
        cb_();
    }

    boost::function< void(void) > cb_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket socket_;
    haconfig& haconfig_;
};

template < typename iter_t, typename const_iter_t >
ha_checker<iter_t, const_iter_t>::ha_checker( iter_t begin, const_iter_t const end )
: iter_( begin )
, end_( end )
{
}

template < typename iter_t, typename const_iter_t >
void ha_checker< iter_t, const_iter_t >::sync_run( std::size_t maxconn )
{
    std::size_t activeconn = 0;
    while ( ++activeconn <= maxconn ) start_check();

    io_service_.run();
}

template < typename iter_t, typename const_iter_t >
void ha_checker< iter_t, const_iter_t >::start_check()
{
    // Steps (but done in a async manner!):
    // 1. Check that iter != end
    // 2. Do a DNS lookup and make sure that the DNS result matches haconfig
    // 3. Establish a TCP connection to IP/Port
    // 4. Advance iter and start next check

    if ( iter_ != end_ )
    {
        boost::shared_ptr< ha_checker_impl > checker( new ha_checker_impl( io_service_, *iter_ ) );
        checker->start( boost::bind( &ha_checker< iter_t, const_iter_t >::start_check, this ) );
        ++iter_;
    }
}

template class ha_checker< halist_t::iterator, halist_t::const_iterator >;
