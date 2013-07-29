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
#include "haproxy.hpp"
#include "dnsquery.hpp"
#include "protocol_helper.hpp"

typedef std::stringstream sstream;
using std::vector;
using std::string;
using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

using namespace instrumentation::result_types;

extern log4cpp::Category& log4;

namespace haproxy
{
    static boost::posix_time::time_duration _timeout;

    void timeout( boost::posix_time::time_duration const& timeout )
    {
        _timeout = timeout;
    }
}

namespace
{
    using namespace boost::asio::ip;

    std::string remove_suffix(std::string const& name, std::string const& suffix)
    {
        std::string name_suffix = name.substr(name.length() - suffix.length());
        std::transform( name_suffix.begin(), name_suffix.end(), name_suffix.begin(), ::tolower );
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
        ABS_GET = 5,
        ABS_REPLY = 43
    };

    using namespace protocol_helper;

    typedef boost::int32_t absint_t;
    typedef boost::uint32_t absuint_t;
    typedef double absdbl_t; // IEEE 754 Double Precision Floating Point format (always 64 bits)

    template < typename IntType >
    boost::uint8_t* read_abs_int(IntType& val, boost::uint8_t* const buf, boost::uint8_t const* const end)
    {
        check_end( sizeof( IntType), buf, end );
        memcpy(&val, buf, sizeof(  IntType ));
        return buf + sizeof( IntType );
    }

    template < typename IntType >
    boost::uint8_t* write_abs_int(IntType const val, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        check_end( sizeof( IntType ), buf, end );
        memcpy(buf, &val, sizeof( IntType ));
        return buf + sizeof( IntType );
    }

    boost::uint8_t* write_abs_chars(string const& str, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        check_end(str.length(), buf, end);
        memcpy(buf, str.c_str(), str.length());
        return buf + str.length();
    }

    template < typename IntType >
    boost::uint8_t* write_abs_string(string const& str, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        buf = write_abs_int< IntType >(str.length(), buf, end);
        return write_abs_chars(str, buf, end);
    }

    boost::uint8_t* write_abs_zero(std::size_t const len, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        check_end(len, buf, end);
        memset(buf, 0, len);
        return buf + len;
    }

    static std::size_t const unused_abs_header_len =
              1                // overlay hops
            + 1                // overlay ttl
            + sizeof(absint_t) // IP hops
            + sizeof(absdbl_t) // latency
            + sizeof(absint_t) // destination overlay ID
            + sizeof(absint_t) // destination prefix length
            + sizeof(absint_t) // destination max length
            + sizeof(absint_t) // source overlay ID
            + sizeof(absint_t) // source prefix length
            + sizeof(absint_t);// source max length

    static std::size_t const abs_reply_len_before_hostname =
              sizeof(absint_t) // resolution status
            + sizeof(absint_t) // resolution hops
            + sizeof(absint_t) // resolution ip hops
            + sizeof(absdbl_t) // resolution latency
            + sizeof(absint_t) // sequence number
            + sizeof(absint_t) // target overlay ID
            + sizeof(absint_t) // target prefix length
            + sizeof(absint_t) // target max length
            + sizeof(absint_t);// destination hostname length

//    static std::size_t const unused_abs_get_len =
//              sizeof(absint_t) // target overlay ID
//            + sizeof(absint_t) // target prefix length
//            + sizeof(absint_t);// target max length

    boost::uint8_t* write_abs_header(absmsgid_t const msgid, boost::uint32_t const sequence, string const& hahostname, boost::uint16_t const haport, string const& nshostname, boost::uint16_t const nsport, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        check_end(buf, end);

        *(buf++) = static_cast< boost::uint8_t >( msgid );
        buf = write_abs_int   < absuint_t >(sequence,   buf, end);
        buf = write_abs_string< absint_t  >(hahostname, buf, end);
        buf = write_abs_int   < absint_t  >(haport,     buf, end);
        buf = write_abs_string< absint_t  >(nshostname, buf, end);
        buf = write_abs_int   < absint_t  >(nsport,     buf, end);
        buf = write_abs_zero(unused_abs_header_len, buf, end);

        return buf;
    }

    boost::uint8_t* write_abs_get(dnsquery& query, string const& suffix, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        if (query.num_questions() < 1) return buf;

        // NOTE: We only look up the name in the first question!
        // (In the current implementation, we expect only one)

        dnsquestion const& question = *query.questions_begin();
        string const devicename( remove_suffix( question.name, suffix ) );
        query.metric().device_name(devicename);

        //buf = write_abs_zero(unused_abs_get_len, buf, end);
        return write_abs_string< absint_t >( devicename, buf, end );
    }

    class querymap
    {
        typedef boost::posix_time::seconds seconds;
        typedef boost::posix_time::minutes minutes;

    public:
        bool insert(query_ptr query)
        {
            bool inserted = false;
            {
                boost::lock_guard<boost::mutex> m(mtx_);
                inserted = map_.insert(std::make_pair(query->id(), query)).second;
            }
            if (inserted)
            {
                deadline_timer& timer = query->timer();
                timer.expires_from_now( haproxy::_timeout );
                timer.async_wait(boost::bind( &querymap::expire, this, query->id() ));
            }
            return inserted;
        }

        query_ptr remove(boost::uint16_t const sequence)
        {
            query_ptr query;
            boost::lock_guard<boost::mutex> m(mtx_);
            map_t::iterator i( map_.find( sequence ) );
            if ( i != map_.end() )
            {
                query = i->second;
                map_.erase(i);

                query->timer().cancel();
            }
            return query;
        }

    private:
        void expire( boost::uint16_t const sequence )
        {
            query_ptr query( remove(sequence) );
            if ( query )
            {
                log4.warnStream() << "Query for '" << query->questions_begin()->name << "' timed out";
                complete_query(*query, R_NAME_ERROR, TIMEOUT);
            }
        }

        typedef std::map< boost::uint16_t, query_ptr > map_t;
        map_t map_;
        boost::mutex mtx_;
    };
}

querymap queries; // Note: constructor run at global scope

class hasendconnection: public boost::enable_shared_from_this<hasendconnection>
{
public:
    hasendconnection(io_service& io_service,
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
        try
        {
            // We compose the message in the buffer here so that if there is an error, we never attempt to open
            // the connection to the home agent. We also do this before inserting the query into the querymap
            // so that we don't need to remove it.
            boost::uint8_t* buf = buf_.data();
            boost::uint8_t const* const end = buf + buf_.size();

            buf = write_abs_header(ABS_GET, query_->id(), hahostname_, haport_, nshostname_, nsport_, buf, end);
            buf = write_abs_get(*query_, suffix_, buf, end);

            send_buf_ = buffer(buf_,  buf - buf_.data());

            log4.debugStream() << "Created new home agent send connection object for '" << hahostname_ << '\'';
        }
        catch ( std::exception const& )
        {
            // In this particular case, we want to send and error back to the client. We do this here because at
            // a higher level it's harder to know if we want to return an error or not. (For example, we don't
            // return an error on duplicate sequence number because that may have the effect of canceling a
            // query in progress).

            complete_query(*query, R_NAME_ERROR, INVALID_REQUEST);
            throw;
        }

        if ( !queries.insert(query_) )
        {
             // NOTE: We don't send an error back here, because if the sequence number already exists in the map,
             // the client is most likely sending us a duplicate UDP request incase the first on was lost (but
             // we are really just being slow)
             throw std::runtime_error("Duplicate sequence number");
         }
    }

    void start(ip::tcp::resolver::iterator iter)
    {
        log4.infoStream() << "Attempting to connect to '" << hahostname_ << '\'';

        async_connect(socket_, iter,
            boost::bind( &hasendconnection::handle_connect, shared_from_this(), ph::error ));
    }

private:
    void handle_connect( bs::error_code const& ec )
    {
        if ( !ec )
        {
            log4.infoStream() << "Connection established to '" << hahostname_ << "'; sending query";

            async_write(socket_, buffer(send_buf_),
                    boost::bind( &hasendconnection::handle_query_sent, shared_from_this(), ph::error, ph::bytes_transferred ) );
        }
        else
        {
            log4.errorStream() << "Unable to connect to '" << hahostname_ << "'";
            queries.remove(query_->id());
            complete_query(*query_, R_SERVER_FAILURE, HA_CONNECTION_ERROR);
        }
    }

    void handle_query_sent(bs::error_code const& ec, std::size_t bytes_transferred)
    {
        if ( !ec )
        {
            // This will work, we know that there is at least one question because we sent it
            log4.infoStream() << "Sent query for '" << query_->questions_begin()->name << "' to '" << hahostname_ << "' with sequence number " << query_->id();
        }
        else
        {
            log4.errorStream() << "An error occurred while sending query to '" << hahostname_ << "': " << ec.message();
            queries.remove(query_->id());
            complete_query(*query_, R_SERVER_FAILURE, HA_CONNECTION_ERROR);
        }
    }

    tcp::socket socket_;
    string const hahostname_;
    string const nshostname_;
    string const suffix_;
    query_ptr query_;
    boost::asio::const_buffer send_buf_;
    boost::array< boost::uint8_t, 256 > buf_;
    boost::uint16_t const haport_;
    boost::uint16_t const nsport_;
};

class harecvconnection : private boost::noncopyable, public boost::enable_shared_from_this< harecvconnection >
{
public:
    harecvconnection( io_service& io_service, string const& nshostname, boost::uint16_t const ttl )
    : socket_( io_service )
    , nshostname_( nshostname )
    , hostlen_(0)
    , sequence_(0)
    , ttl_(ttl)
    {
    }

    void start()
    {
        async_read( socket_, buffer(buf_, 1 /* messageType*/ + sizeof(absint_t) /*sequence_no*/ + sizeof(absint_t) /*destHostLength*/),
               boost::bind( &harecvconnection::handle_read_to_desthostlen, shared_from_this(), ph::error, ph::bytes_transferred ) );
    }

private:
    void handle_read_to_desthostlen( bs::error_code const& ec, std::size_t const bytes_transferred )
    {
        if ( !ec )
        {
            boost::uint8_t msgtype;
            read_abs_int< boost::uint8_t >(msgtype, buf_.data(), buf_.data() + bytes_transferred);
            if ( msgtype ==  ABS_REPLY )
            {
                absint_t sequence;
                read_abs_int< absint_t >(sequence, buf_.data() + 1, buf_.data() + bytes_transferred );
                if ( sequence <= std::numeric_limits< boost::uint16_t >::max() )
                {
                    sequence_ = static_cast< boost::uint16_t >( sequence );
                    log4.infoStream() << "Receiving a reply message from home agent with sequence number " << sequence;
                }
                else
                {
                    log4.errorStream() << "Receiving a reply message from home agent with invalid sequence number " << sequence;
                    // This means that we will try to reply to a query with sequence number 0
                    // There is a very small probability that we may send the wrong IP address to an outstanding query with sequence number 0
                    // But we'll just put our faith in the home agent for now and trust that this won't happen
                    // (This and other problems could be fixed by using our own unique hash rather than the DNS transaction IDs for sequence numbers
                    // in messages to home agents)
                }

                absint_t desthostlen;
                read_abs_int< absint_t >(desthostlen, buf_.data() + bytes_transferred - sizeof(absint_t), buf_.data() + bytes_transferred );

                async_read( socket_, buffer(buf_, desthostlen + sizeof(absint_t) + sizeof(absint_t)),
                        boost::bind( &harecvconnection::handle_read_to_srchostlen, shared_from_this(), ph::error, ph::bytes_transferred ) );
            }
            else
            {
                log4.errorStream() << "Received invalid message type from home agent: " << msgtype;
            }
        }
        else
        {
            log4.errorStream() << "An error occurred while reading ABS message header: " << ec.message();
        }
    }

    void handle_read_to_srchostlen( bs::error_code const& ec, std::size_t const bytes_transferred )
    {
        if ( !ec )
        {
            //log4.debugStream() << "Trace: handle_read_to_srchostlen()";

            absint_t srchostlen;
            read_abs_int< absint_t >(srchostlen, buf_.data() + bytes_transferred - sizeof(absint_t), buf_.data() + bytes_transferred );
            async_read( socket_, buffer(buf_, srchostlen + sizeof(absint_t) + unused_abs_header_len),
                    boost::bind( &harecvconnection::handle_absheader_read, shared_from_this(), ph::error, ph::bytes_transferred ) );
        }
        else
        {
            log4.errorStream() << "An error occurred while reading ABS message header: " << ec.message();
        }
    }

    void handle_absheader_read( bs::error_code const& ec, std::size_t const bytes_transferred )
    {
        if ( !ec )
        {
            //log4.debugStream() << "Trace: handle_absheader_read()";

            async_read( socket_, buffer( buf_, abs_reply_len_before_hostname ),
                    boost::bind( &harecvconnection::handle_read_to_replyhostlen, shared_from_this(), ph::error, ph::bytes_transferred ) );
        }
        else
        {
            log4.errorStream() << "An error occurred while reading ABS message header: " << ec.message();
        }
    }

    void handle_read_to_replyhostlen( bs::error_code const& ec, std::size_t const bytes_transferred )
    {
        if ( !ec )
        {
            //log4.debugStream() << "Trace: handle_read_to_replyhostlen()";

            absint_t hostlen;
            read_abs_int< absint_t >(hostlen, buf_.data() + bytes_transferred - sizeof(absint_t), buf_.data() + bytes_transferred );
            hostlen_ = hostlen;

            async_read(socket_, buffer(buf_, hostlen + sizeof(absint_t) + sizeof(absint_t)),
                    boost::bind( &harecvconnection::handle_read_to_devicenamelen, shared_from_this(), ph::error, ph::bytes_transferred ) );
        }
        else
        {
            log4.errorStream() << "An error occurred while reading GET REPLY message body: " << ec.message();
        }
    }

    void handle_read_to_devicenamelen( bs::error_code const& ec, std::size_t const bytes_transferred )
    {
        if ( !ec )
        {
            //log4.debugStream() << "Trace: handle_read_to_devicenamelen()";

            absint_t devicenamelen;

            boost::uint8_t* const beg = buf_.data() + bytes_transferred - sizeof(absint_t);
            boost::uint8_t const* const end = buf_.data() + bytes_transferred;
            //log4.debugStream() << "Trace: received " << bytes_transferred << ", looking at bytes " << beg - buf_.data() << " to " << end - buf_.data();

            read_abs_int< absint_t >(devicenamelen, beg, end);
            //log4.debugStream() << "Trace: devicenamelen = " << devicenamelen;

            query_ptr query( queries.remove( sequence_ ) );
            //log4.debugStream() << "Trace: removed query from map";

            if ( query )
            {
                std::string const& name = query->questions_begin()->name;
                if (hostlen_ == 0)
                {
                    // Device name not found
                    log4.infoStream() << "No IP address found for '" << name << '\'';
                    complete_query(*query, R_NAME_ERROR, HA_RETURNED_ERROR);
                }
                else if ( buf_.size() > hostlen_ )
                {
                    //log4.debugStream() << "Trace: about to parse IP address; buf_.size() = " << buf_.size() << ", hostlen_ = " << hostlen_;
                    buf_[hostlen_] = '\0'; // overwrites device name length in buffer
                    //log4.debugStream() << "Trace: instered nul character";
                    bs::error_code ec;
                    ip::address addr = ip::address::from_string(reinterpret_cast< char* >( buf_.data() ), ec);
                    //log4.debugStream() << "Trace: parsed IP address";
                    if ( !ec )
                    {
                        dnsrr rr;
                        rr.name = name;
                        rr.rclass = C_IN;
                        rr.ttl = ttl_;
                        if ( addr.is_v4() )
                        {
                            rr.rdlength = 4;
                            address_v4::bytes_type addr_bytes = addr.to_v4().to_bytes();
                            assert( sizeof(addr_bytes) == 4 );
                            rr.rdata.resize( sizeof( addr_bytes ) );
                            std::copy( addr_bytes.begin(), addr_bytes.end(), rr.rdata.begin() );
                            rr.rtype = T_A;
                        }
                        else // addr.is_v6()
                        {
                            rr.rdlength = 16;
                            address_v6::bytes_type addr_bytes = addr.to_v6().to_bytes();
                            assert( sizeof(addr_bytes) == 16 );
                            rr.rdata.resize( sizeof( addr_bytes ) );
                            std::copy( addr_bytes.begin(), addr_bytes.end(), rr.rdata.begin() );
                            rr.rtype = T_AAAA;
                        }

                        log4.infoStream() << "Home agent returned IP address " << addr << " for '" << name << "'; Sending reply to DNS client";

                        query->add_answer(rr);
                        complete_query(*query, R_SUCCESS, SUCCESS);
                    }
                    else
                    {
                        log4.errorStream() << "An error occurred while parsing IP address '" << reinterpret_cast< char* >( buf_.data() ) << "'  returned from home agent: " << ec.message();
                        complete_query(*query, R_SERVER_FAILURE, UNKNOWN);
                    }
                }
                else
                {
                    log4.errorStream() << "IP address length " << hostlen_ << " received from home agent is too long!";
                    complete_query(*query, R_SERVER_FAILURE, UNKNOWN);
                }
            }
            else
            {
                log4.warnStream() << "Unable to find an outstanding query with sequence number " << sequence_;
            }

            async_read(socket_, buffer(buf_, devicenamelen),
                    boost::bind( &harecvconnection::handle_devicename_read, shared_from_this(), ph::error, ph::bytes_transferred ) );
        }
        else
        {
            log4.errorStream() << "An error occurred while reading GET REPLY message body: " << ec.message();
            query_ptr query( queries.remove( sequence_ ) );
            if ( query )
                complete_query(*query, R_SERVER_FAILURE, UNKNOWN);
        }
    }

    void handle_devicename_read(boost::system::error_code const& ec, std::size_t const bytes_transferred)
    {
        if ( !ec )
        {
            // Done! We can let the connection close.
            log4.debugStream() << "Successfully received message from home agent";
        }
        else
        {
            log4.errorStream() << "An error occurred while reading GET REPLY message body: " << ec.message();
        }
    }

    friend class harecvproxy;

    tcp::socket& socket()
    {
        return socket_;
    }

    tcp::socket socket_;
    std::size_t hostlen_;
    std::string nshostname_;
    boost::array< boost::uint8_t, 256 > buf_;
    boost::uint16_t sequence_;
    boost::uint16_t ttl_;
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
}

void hasendproxy::start()
{
    sstream ss;
    ss << haport_;
    ip::tcp::resolver::query q(hahostname_, ss.str());

    log4.debugStream() << "Created home agent proxy object for '" << hahostname_ << "'; resolving hostname";

    resolver_.async_resolve(q,
            boost::bind( &hasendproxy::handle_resolve, shared_from_this(), ph::error, ph::iterator ));
}

void hasendproxy::handle_resolve(bs::error_code const& ec, ip::tcp::resolver::iterator iter)
{
    if( !ec )
    {
        log4.debugStream() << "Successfully resolved hostname '" << hahostname_ << "'; enabling home agent proxy object";

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
    try
    {
        log4.debugStream() << "Proxy object for '" << hahostname_ << "' processing query";

        boost::shared_ptr< hasendconnection > conn(new hasendconnection( resolver_.get_io_service(), hahostname_, haport_, nshostname_, nsport_, query, suffix_));
        conn->start(iter_);
    }
    catch ( std::exception const& e)
    {
        log4.errorStream() << "Error processing query: " << e.what();
    }
}

harecvproxy::harecvproxy(io_service& io_service, string const& nshostname, boost::uint16_t const port, boost::uint16_t ttl)
: acceptor_(io_service)
, nshostname_(nshostname)
, ttl_(ttl)
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
    new_connection_.reset( new harecvconnection( acceptor_.get_io_service(), nshostname_, ttl_ ) );
    acceptor_.async_accept( new_connection_->socket(),
            boost::bind( &harecvproxy::handle_accept, this, ph::error ) );
}

void harecvproxy::handle_accept(bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.debugStream() << "Received a new home agent connection from " << new_connection_->socket().remote_endpoint();
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
