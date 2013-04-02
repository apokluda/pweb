/*
 * ha_config.hpp
 *
 *  Created on: 2013-04-02
 *      Author: alex
 */

#include "stdhdr.hpp"

enum hastate_t
{
    GOOD,
    DNS_LOOKUP_FAILED,
    URL_IP_MISMATCH,
    UNABLE_TO_CONNECT,
    UNKNOWN
};

char const* hastate_to_string( hastate_t const state );

struct haconfig
{
    std::string url;
    std::string port;
    std::string haname;
    boost::asio::ip::address ip;
    hastate_t status;
};
typedef std::vector< haconfig > halist_t;

void load_halist(halist_t& halist, std::istream& is);

template < typename iter_t, typename const_iter_t >
class ha_checker : private boost::noncopyable
{
public:
    ha_checker( iter_t begin, const_iter_t const end );
    void sync_run( std::size_t maxconn );

private:
    void start_check();

    boost::asio::io_service io_service_;
    iter_t iter_;
    const_iter_t const end_;
};
