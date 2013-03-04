/*
 * haproxy.cpp
 *
 *  Created on: 2013-03-04
 *      Author: apokluda
 */

#include "stdhdr.hpp"
#include "haproxy.hpp"
#include "dnsquery.hpp"
#include "protocol_helper.hpp"

typedef std::stringstream sstream;
using std::vector;
using std::string;
using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

extern log4cpp::Category& log4;

namespace
{
    using namespace boost::asio::ip;

    std::string remove_suffix(std::string const& name, std::string const& suffix)
    {
        std::string const name_suffix = name.substr(name.length() - suffix.length());
        if ( name_suffix != suffix)
        {
            sstream ss;
            ss << "Name '" << name  << "' contains an invalid suffix: " << name_suffix;
            throw std::runtime_error(ss.str());
        }
        return name.substr(0, name.length() - suffix.length());
    }

    enum absmsgid_t
    {
        ABS_GET = 5
    };

    using namespace protocol_helper;

    typedef boost::int32_t absint_t;
    typedef boost::uint32_t absuint_t;

    template < typename IntType >
    uint8_t* write_abs_int(IntType const val, uint8_t* buf, uint8_t const* const end)
    {
        check_end( sizeof( IntType ), buf, end );
        *reinterpret_cast< IntType* >( buf ) = static_cast< IntType >( val );
        return buf + sizeof( IntType );
    }

    uint8_t* write_abs_chars(string const& str, uint8_t* buf, uint8_t const* const end)
    {
        check_end(str.length(), buf, end);
        memcpy(buf, str.c_str(), str.length());
        return buf + str.length();
    }

    template < typename IntType >
    uint8_t* write_abs_string(string const& str, uint8_t* buf, uint8_t const* const end)
    {
        buf = write_abs_int< IntType >(str.length(), buf, end);
        return write_abs_chars(str, buf, end);
    }

    uint8_t* write_abs_header(absmsgid_t const msgid, uint32_t const sequence, string const& hahostname, boost::uint16_t const haport, string const& nshostname, boost::uint16_t const nsport, uint8_t* buf, uint8_t const* const end)
    {
        check_end(buf, end);
        *(buf++) = static_cast< boost::uint8_t >( msgid );

        buf = write_abs_int   < absuint_t >(sequence, buf, end);
        buf = write_abs_string< absuint_t >(hahostname, buf, end);
        buf = write_abs_int   < absint_t > (haport, buf, end);
        buf = write_abs_string< absuint_t >(nshostname, buf, end);
        buf = write_abs_int   < absint_t > (nsport, buf, end);

        return buf;
    }

    boost::uint8_t* write_abs_get(dnsquery const& query, string const& suffix, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        if (query.num_questions() < 1) return buf;

        // NOTE: We only look up the name in the first question!
        // (In the current implementation, we expect only one)

        dnsquestion const& question = *query.questions_begin();
        string const devicename( remove_suffix( question.name, suffix ) );

        return write_abs_string< absuint_t >( devicename, buf, end );
    }
}

class hasendconnection: public boost::enable_shared_from_this<hasendconnection>
{
public:
    hasendconnection(io_service& io_service, ip::tcp::resolver::iterator iter,
            string const& hahostname, boost::uint16_t const haport,
            string const& nshostname, boost::uint16_t const nsport,
            query_ptr query, std::string const& suffix)
    : socket_(io_service)
    , hahostname_(hahostname)
    , nshostname_(nshostname)
    , suffix_(suffix)
    , query_(query)
    , haport_(haport)
    , nsport_(nsport)
    {
            async_connect(socket_, iter,
                    boost::bind( &hasendconnection::handle_connect, shared_from_this(), ph::error ));
}

private:
    void handle_connect( bs::error_code const& ec )
    {
        if ( !ec )
        {
            uint8_t* buf = buf_.data();
            uint8_t const* const end = buf + buf_.size();

            buf = write_abs_header(ABS_GET, query_->id(), hahostname_, haport_, nshostname_, nsport_, buf, end);
            buf = write_abs_get(*query_, suffix_, buf, end);

            async_write(socket_, buffer(buf_, buf - buf_.data()),
                    boost::bind( &hasendconnection::handle_query_sent, shared_from_this(), ph::error, ph::bytes_transferred ) );
        }
        else
        {
            log4.errorStream() << "[hasendconnection] Unable to connect to '" << hahostname_ << "'";
            query_->rcode(R_SERVER_FAILURE);
            query_->send_reply();
        }
    }

    void handle_query_sent(bs::error_code const& ec, std::size_t bytes_transferred)
    {
        if ( !ec )
        {
            // This will work, we know that there is at least one question because we sent it
            log4.infoStream() << "Send query for '" << query_->questions_begin()->name << "' to '" << hahostname_ << '\'';
        }
        else
        {
            log4.errorStream() << "An error occurred while sending query to '" << hahostname_ << "': " << ec.message();
            query_->rcode(R_SERVER_FAILURE);
            query_->send_reply();
        }
    }

    tcp::socket socket_;
    string const hahostname_;
    string const nshostname_;
    string const suffix_;
    query_ptr query_;
    boost::array< boost::uint8_t, 256 > buf_;
    boost::uint16_t const haport_;
    boost::uint16_t const nsport_;
};

class harecvconnection
{
public:
    harecvconnection( io_service& io_service )
    : socket_( io_service )
    {

    }

    void start()
    {

    }

private:
    friend class harecvproxy;

    tcp::socket& socket()
    {
        return socket_;
    }

    tcp::socket socket_;
};


hasendproxy::hasendproxy(io_service& io_service,
        string const& hahostname, boost::uint16_t const haport,
        string const& nshostname, boost::uint16_t const nsport,
        string const& suffix)
: resolver_(io_service)
, hahostname_(hahostname)
, nshostname_(nshostname)
, suffix_(suffix)
, haport_(haport)
, nsport_(nsport)
{
    sstream ss;
    ss << haport;
    ip::tcp::resolver::query q(hahostname, ss.str());
    resolver_.async_resolve(q,
            boost::bind( &hasendproxy::handle_resolve, shared_from_this(), ph::error, ph::iterator ));
}

void hasendproxy::handle_resolve(bs::error_code const& ec, ip::tcp::resolver::iterator iter)
{
    if( !ec )
    {
        iter_ = iter;
        enabled_.store(true, boost::memory_order_release);
    }
    else
    {
        log4.errorStream() << "An error occurred while resolving home agent hostname '" << hahostname_ << "': " << ec.message();
    }
}

void hasendproxy::process_query( query_ptr query )
{
    // The hasendconnection will be automatically deleted when the connection is closed so there is no memory leak
    new hasendconnection( resolver_.get_io_service(), iter_, hahostname_, haport_, nshostname_, nsport_, query, suffix_);
}

harecvproxy::harecvproxy(io_service& io_service, boost::uint16_t const port)
: acceptor_(io_service)
{
    try
    {
        ip::tcp::endpoint endpoint(ip::tcp::v6(), port);

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option( ip::tcp::acceptor::reuse_address( true ) );
        acceptor_.bind(endpoint);
        acceptor_.listen();

        log4.infoStream() << "Listening on all interfaces port " << port << " for home agent connections";
    }
    catch ( boost::system::system_error const& ec )
    {
        log4.fatalStream() << "Unable to bind to port " << port << " to accept home agent connections: " << ec.what();
        throw;
    }
}

void harecvproxy::start()
{
    new_connection_.reset( new harecvconnection( acceptor_.get_io_service() ) );
    acceptor_.async_accept( new_connection_->socket(),
            boost::bind( &harecvproxy::handle_accept, this, ph::error ) );
}

void harecvproxy::handle_accept(bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.infoStream() << "Received a new home agent connection from " << new_connection_->socket().remote_endpoint();
        new_connection_->start();
        // Fall through
    }
    else
    {
        log4.errorStream() << "An error occurred while accepting a new home agent connection: " << ec.message();
        // Fall through
    }
    start();
}
