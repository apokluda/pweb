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

    class send_connection: public boost::enable_shared_from_this<send_connection>
    {
    public:
        send_connection(io_service& io_service, ip::tcp::resolver::iterator iter,
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
                    boost::bind( &send_connection::handle_connect, shared_from_this(), ph::error ));
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
                       boost::bind( &send_connection::handle_query_sent, shared_from_this(), ph::error, ph::bytes_transferred ) );
            }
            else
            {
                log4.errorStream() << "[send_connection] Unable to connect to '" << hahostname_ << "'";
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
}

hasendproxy::hasendproxy(io_service& io_service,
        string const& hahostname, boost::uint16_t const haport,
        string const& nshostname, boost::uint16_t const nsport,
        string const& suffix)
: io_service_(io_service)
, resolver_(io_service)
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
    // The send_connection will be automatically deleted when the connection is closed so there is no memory leak
    new send_connection( io_service_, iter_, hahostname_, haport_, nshostname_, nsport_, query, suffix_);
}

harecvproxy::harecvproxy(io_service& io_service, string const& nshostname, boost::uint16_t const port)
: io_service_(io_service)
{
    // TODO: stuff
}

void harecvproxy::start()
{
    // TODO: stuff
}
