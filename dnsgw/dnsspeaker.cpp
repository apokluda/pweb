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
#include "dnsspeaker.hpp"
#include "dnsquery.hpp"
#include "protocol_helper.hpp"
#include "instrumenter.hpp"

using namespace boost::asio;
using std::string;
typedef std::stringstream sstream;
namespace b = boost;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

typedef instrumentation::instrumenter instrumenter_t;

extern log4cpp::Category& log4;
extern std::auto_ptr< instrumenter_t > instrumenter;

namespace dns_query_parser
{
    using namespace protocol_helper;

    boost::uint8_t const* parse_label_len(std::size_t& len, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        check_end(buf, end);

        len = *(buf++);
        if ( len > 63 ) // max defined label length (RFC 1035 S2.3.4)
        {
            sstream ss;
            ss << "Invalid label length: " << len;
            throw parse_error(ss.str());
        }
        return buf;
    }

    boost::uint8_t const* parse_label(sstream& name, std::size_t len, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        while ( len-- > 0 )
        {
            check_end(buf, end);

            name << static_cast< char >( *(buf++) );
        }
        name << '.';
        return buf;
    }

    boost::uint8_t const* parse_name(string& name, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        sstream sname;
        std::size_t name_len = 0;

        std::size_t label_len;
        buf = parse_label_len(label_len, buf, end);
        while ( label_len )
        {
            if ( (name_len += label_len) > 255 ) // max defined name length (RFC 1035 S2.3.4)
            {
                throw parse_error("Name exceeds 255 octets");
            }

            buf = parse_label(sname, label_len, buf, end);
            buf = parse_label_len(label_len, buf, end);
        }
        name = sname.str();

        return buf;
    }

    inline boost::uint8_t* write_label_len(std::size_t const len, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        check_end(buf, end);
        if ( len > 63 ) // max defined label length (RFC 1035 S2.3.4)
        {
            sstream ss;
            ss << "Invalid label length: " << len;
            throw parse_error(ss.str());
        }
        *buf = static_cast< boost::uint8_t >( len );
        return ++buf;
    }

    boost::uint8_t* write_label(std::string const& label, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        std::size_t const len = label.length();
        buf = write_label_len( len, buf, end );
        check_end(len, buf, end);
        memcpy(buf, label.c_str(), len);
        return buf + len;
    }

    boost::uint8_t* write_name(std::string const& name, boost::uint8_t* buf, boost::uint8_t const* const end)
    { // Writes a string in DNS "name format" (ie. a series of length-prefixed strings)
        std::size_t const len = name.length();
        if ( len > 255 ) // max defined name length (RFC 1035 S2.3.4)
        {
            sstream ss;
            ss << "Invalid name length: " << len;
            throw parse_error(ss.str());
        }

        using boost::tokenizer;

        tokenizer<> tok(name);
        for( tokenizer<>::iterator beg = tok.begin(); beg != tok.end(); ++beg )
        {
            buf = write_label(*beg, buf, end);
        }

        check_end(buf, end);
        *buf = 0;
        return ++buf;
    }

    boost::uint8_t const* parse_qtype(qtype_t& qtype, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        boost::uint16_t val;
        buf = parse_short(val, buf, end);
        // Changed condition to allow new types that have been added since 1987
        //if (  (val >= T_MIN && /*val <= T_MAX) || (val >= QT_MIN &&*/ val <= QT_MAX ) )
        //{
            qtype = static_cast< qtype_t >( val );
            return buf;
        //}
        //sstream ss;
        //ss << "Invalid QTYPE value: " << val;
        //throw parse_error(ss.str());
    }

    boost::uint8_t const* parse_qclass(qclass_t& qclass, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        boost::uint16_t val;
        buf = parse_short(val, buf, end);
        // Change condition to allow new types that have been added since 1987
        //if ( (val >= C_MIN && /*val <= C_MAX) || (val >= QC_MIN &&*/ val <= QC_MAX) )
        //{
            qclass = static_cast< qclass_t >( val );
            return buf;
        //}
        //sstream ss;
        //ss << "Invalid QCLASS value: " << val;
        //throw parse_error(ss.str());
    }

    boost::uint8_t const* parse_rr(dnsrr& rr, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {

        buf = parse_name( rr.name, buf, end );
        buf = parse_qtype( rr.rtype, buf, end );
        buf = parse_qclass( rr.rclass, buf, end );
        buf = parse_ulong( rr.ttl, buf, end);
        buf = parse_short( rr.rdlength, buf, end );

        static std::size_t const MAX_RDATA = 1024; // A kilobyte ought to be enough...
        if ( rr.rdlength > MAX_RDATA ) throw parse_error("Error parsing DNS RR: rdlength > MAX_RDATA");
        check_end(rr.rdlength, buf, end);
        rr.rdata.resize( rr.rdlength );
        std::copy( buf, buf + rr.rdlength, rr.rdata.begin() );
        return buf + rr.rdlength;
    }

    boost::uint8_t const* parse_question(dnsquery& query, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        dnsquestion question;

        buf = parse_name(question.name, buf, end);
        buf = parse_qtype(question.qtype, buf, end);
        buf = parse_qclass(question.qclass, buf, end);

        query.add_question(question);
        return buf;
    }

    boost::uint8_t const* parse_answer(dnsquery& query, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        dnsrr rr;
        buf = parse_rr(rr, buf, end);
        query.add_answer(rr);
        return buf;
    }

    boost::uint8_t const* parse_authority(dnsquery& query, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        dnsrr rr;
        buf = parse_rr(rr, buf, end);
        query.add_authority(rr);
        return buf;
    }

    boost::uint8_t const* parse_additional(dnsquery& query, boost::uint8_t const* buf, boost::uint8_t const* const end)
    {
        dnsrr rr;
        buf = parse_rr(rr, buf, end);

        // Added log line and commented out query.add_additonal().
        // Many recursive DNS servers are sending is requests with DNSSEC
        // flags and additional sections that the DNS Gateway doesn't handle.
        log4.warnStream() << "Ignoring additional section in DNS query: ID=" << query.id() << ", name=" << rr.name << ", rtype=" << rr.rtype << ", rclass=" << rr.rclass << ", ttl=" << rr.ttl << ", rdlength=" << rr.rdlength << ", rdata omitted";
        //query.add_additional(rr);

        return buf;
    }

    boost::uint8_t* write_rr(dnsquestion const& question, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        buf = write_name( question.name, buf, end );
        buf = write_short( question.qtype, buf, end );
        buf = write_short( question.qclass, buf, end );

        return buf;
    }

    boost::uint8_t* write_rr(dnsrr const& rr, boost::uint8_t* buf, boost::uint8_t const* const end)
    {
        buf = write_name( rr.name, buf, end );
        buf = write_short( rr.rtype, buf, end );
        buf = write_short( rr.rclass, buf, end );
        buf = write_ulong( rr.ttl, buf, end);
        buf = write_short( rr.rdlength, buf, end );

        if ( rr.rdlength > rr.rdata.size() ) throw parse_error("Error writing DNS RR: rdlength > rdata.size()");
        check_end(rr.rdlength, buf, end);
        std::copy( rr.rdata.begin(), rr.rdata.begin() + rr.rdlength, buf );
        return buf + rr.rdlength;
    }
};

void parse_dns_query(dnsquery& query, dns_query_header const& header, boost::uint8_t const* buf, boost::uint8_t const* const end)
{
    using namespace dns_query_parser;

    boost::uint16_t const id( header.id() );
    query.id( id );
    query.rd( header.rd() );

    // Warning: calling query.num_questions() may cause it to allocate memory for
    // num_questions question objects. Obviously this means a mallicious client could
    // potentially cause us to allocate a large amount of memory in a DoS attack; however,
    // Unwarning: the number of questions is limited by the query message size that
    // has already been checked
    // The same applies for query.num_answers/authority/additional().

    std::size_t num_questions = header.qdcount();
    int num_answers = header.ancount();
    int num_authorities = header.nscount();
    int num_additionals = header.arcount();

    log4.debugStream() << "Parsing DNS query from " << query.remote_address() << ": ID=" << id << ", QDCOUNT=" << num_questions << ", ANCOUNT=" << num_answers << ", NSCOUNT=" << num_authorities << ", ARCOUNT=" << num_additionals;

    query.num_questions(num_questions);
    for ( std::size_t i = 0; i < num_questions; ++i )
    {
        buf = parse_question(query, buf, end);
    }

    query.num_answers(num_answers);
    for ( int i = 0; i < num_answers; ++i )
    {
        buf = parse_answer(query, buf, end);
    }

    query.num_authorities(num_authorities);
    for ( int i = 0; i < num_authorities; ++i )
    {
        buf = parse_authority(query, buf, end);
    }

    query.num_additionals(num_additionals);
    for ( int i = 0; i < num_additionals; ++i )
    {
        buf = parse_additional(query, buf, end);
    }

    if ( buf != end )
    {
        throw parse_error("Additional data at end of query message");
    }
}

void compose_dns_header(dns_query_header& header, dnsquery const& query)
{
    header.clear();
    header.id( query.id() );                    // set transaction id
    header.qr( true );                          // set response flag
    header.aa( true );                          // set authoritative answer flag
    header.rd( query.rd() );                    // set recursion desired flag
    header.rcode( query.rcode() );              // set result code
    header.qdcount( query.num_questions() );    // set number of question sections
    header.ancount( query.num_answers() );      // set number of answer resource records
    header.nscount( query.num_authorities() );  // set number of authority resource records
    header.arcount( query.num_additionals() );  // set number of additional resource records
}

boost::uint8_t* compose_dns_response(dnsquery const& query, dns_query_header const& header, boost::uint8_t* buf, boost::uint8_t const* const end)
{
    using namespace dns_query_parser;



    for ( dnsquery::question_iterator i = query.questions_begin(); i != query.questions_end(); ++i )
    {
        buf = write_rr(*i, buf, end);
    }

    for ( dnsquery::answer_iterator i = query.answers_begin(); i != query.answers_end(); ++i )
    {
        buf = write_rr(*i, buf, end);
    }

    for ( dnsquery::authority_iterator i = query.authorities_begin(); i != query.authorities_end(); ++i )
    {
        buf = write_rr(*i, buf, end);
    }

    for ( dnsquery::additional_iterator i = query.additionals_begin(); i != query.additionals_end(); ++i )
    {
        buf = write_rr(*i, buf, end);
    }

    return buf;
}

udp_dnsspeaker::udp_dnsspeaker(io_service& io_service, b::function<void (query_ptr)> processor, string const& iface, boost::uint16_t port)
: processor_(processor)
, socket_( io_service )
, strand_( io_service )
, reply_buf_( 50 )
, send_in_progress_( false )
{
    recv_buf_arr_[0] = buffer(recv_header_.buffer());
    recv_buf_arr_[1] = buffer(recv_buf_);

    send_buf_arr_[0] = buffer(send_header_.buffer());
    try
    {
    	// Note: changed from ip::udp::v6() to ip::udp::v4()
    	// for deployment on PlanetLab
        ip::udp::endpoint endpoint(ip::udp::v4(), port);
        if ( !iface.empty() )
        {
            ip::address bind_addr(ip::address::from_string(iface));
            endpoint = ip::udp::endpoint(bind_addr, port);
        }

        socket_.open(endpoint.protocol());
        socket_.set_option( ip::udp::socket::reuse_address( true ) );
        socket_.bind(endpoint);

        log4.infoStream() << "Listening on " << (iface.empty() ? "all interfaces" : iface.c_str()) << " port " << port << " for UDP connections";
    }
    catch ( boost::system::system_error const& )
    {
        log4.fatalStream() << "Unable to bind to interface";
        if (port < 1024 )
            log4.fatalStream() << "Does the program have sufficient privileges to bind to port " << port << '?';
        throw;
    }
}

void udp_dnsspeaker::start()
{
    // Note: Handler not wrapped in strand. All read operations occur in an implicit strand. Write operations are synchronized
    // using the strand_ member. It is possible that a read an write operation may execute concurrently. According
    // to the ASIO documentation, socket objects are not thread safe, but it does say that you can have an async_read
    // and async_write operation executing in parallel...
    socket_.async_receive_from( recv_buf_arr_, sender_endpoint_,
            boost::bind( &udp_dnsspeaker::handle_datagram_received, this, ph::error, ph::bytes_transferred ) );
}

void udp_dnsspeaker::handle_datagram_received( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        // Validate header
        if ( bytes_transferred >= recv_header_.length() &&
                !recv_header_.qr() &&
                recv_header_.opcode() == O_QUERY &&
                recv_header_.z() == 0 )
        {
            // Header format OK
            log4.debugStream() << "Received UDP DNS query from " << sender_endpoint_;
            if ( recv_header_.ad_cd() != 0 )
                log4.warnStream() << "Ignoring security extension flags in DNS query from " << sender_endpoint_;


            boost::shared_ptr< dnsquery > query( new dnsquery(socket_.get_io_service()) );
            query->metric().start_timer();
            query->sender( this, sender_endpoint_ );
            try
            {
                parse_dns_query( *query, recv_header_, recv_buf_.data(), recv_buf_.data() + bytes_transferred - recv_header_.length());
                processor_( query );

                // Fall through to start() below
            }
            catch ( dns_query_parser::parse_error const& e )
            {
                log4.noticeStream() << "An error occurred while parsing UDP DNS query from " << sender_endpoint_ << ": " << e.what();

                // Fall through to start() below
            }
        }
        else
        {
            // Malformed header
            log4.noticeStream() << "Received UDP DNS query with invalid header from "
                    << sender_endpoint_
                    << ": length=" << recv_header_.length()
                    << "; qr=" << recv_header_.qr()
                    << "; opcode=" << recv_header_.opcode()
                    << "; z=" << recv_header_.z();

            // Fall through to start() below
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while reading UDP DNS query header: " << ec.message();

        // Fall through to start() below
    }
    // Receive another datagram
    start();
}

void udp_dnsspeaker::send_reply( query_ptr query )
{
    strand_.dispatch( boost::bind( &udp_dnsspeaker::send_reply_, this, query ) );
}

void udp_dnsspeaker::send_reply_( query_ptr query )
{
    if ( send_in_progress_ )
    {
        if ( reply_buf_.full() )
        {
            log4.warnStream() << "UDP send buffer full. Dropping reply to " << query->remote_udp_endpoint();
            return;
        }
        reply_buf_.push_back( query );
    }
    else
    {
        // Send reply immediately.
        send_in_progress_ = true;

        // Ok, I agree that having the header separate from the body ("query") seems unnecessarily
        // cumbersome. In a lot of other protocols, the size of a message body can be determined from
        // the content in a fixed size header, and in this case it makes sense to separate the header
        // from the body. (For DNS over TCP, an extra "meta-header" field is send that includes the
        // size of the header + body). I'm going to leave it as it is for now. I might change it later.

        compose_dns_header(send_header_, *query);
        boost::uint8_t const* const end = compose_dns_response( *query, send_header_, send_buf_.data(), send_buf_.data() + send_buf_.size());

        send_buf_arr_[1] = buffer( send_buf_, end - send_buf_.data() );

        socket_.async_send_to( send_buf_arr_, query->remote_udp_endpoint(), strand_.wrap(
                boost::bind( &udp_dnsspeaker::handle_send_reply, this, ph::error ) ) );

        query->metric().stop_timer();
        instrumenter->add_metric(query->metric());
    }
}

void udp_dnsspeaker::handle_send_reply( bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.info("Successfully sent UDP DNS reply");
        // Fall through
    }
    else
    {
        log4.errorStream() << "An error occurred while sending UDP DNS response: " << ec.message();
        // Fall through
    }

    send_in_progress_ = false;

    if ( !reply_buf_.empty() )
    {
        query_ptr query = reply_buf_.front();
        reply_buf_.pop_front();
        send_reply_( query );
    }
}

tcp_dnsspeaker::tcp_dnsspeaker(io_service& io_service, b::function<void (query_ptr)> processor, string const& iface, boost::uint16_t port)
: processor_(processor)
, acceptor_(io_service)
{
    try
    {
    	// Changed from ip::tcp::v6() to ip::tcp::v4()
    	// in order to support PlanetLab
        ip::tcp::endpoint endpoint(ip::tcp::v4(), port);
        if ( !iface.empty() )
        {
            ip::address bind_addr(ip::address::from_string(iface));
            endpoint = ip::tcp::endpoint(bind_addr, port);
        }

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option( ip::tcp::acceptor::reuse_address( true ) );
        acceptor_.bind(endpoint);
        acceptor_.listen();

        log4.infoStream() << "Listening on " << (iface.empty() ? "all interfaces" : iface.c_str()) << " port " << port << " for TCP connections";
    }
    catch ( boost::system::system_error const& )
    {
        log4.fatalStream() << "Unable to bind to interface";
        if (port < 1024 )
            log4.fatalStream() << "Does the program have sufficient privileges to bind to port " << port << '?';
        throw;
    }
}

void tcp_dnsspeaker::start()
{
    new_connection_.reset( new dns_connection(acceptor_.get_io_service(), processor_) );
    acceptor_.async_accept( new_connection_->socket(),
            boost::bind( &tcp_dnsspeaker::handle_accept, this, ph::error ) );
}

void tcp_dnsspeaker::handle_accept( bs::error_code const& ec )
{
    if ( !ec )
    {
        log4.debugStream() << "Established a new TCP DNS connection with " << new_connection_->socket().remote_endpoint();
        new_connection_->start();
    }
    else
    {
        log4.errorStream() << "An error occurred while accepting a new TCP DNS connection: " << ec.message();
    }
    // Accept additional connections
    start();
}

void dns_connection::start()
{
    async_read( socket_, buffer(&recv_msg_len_, 2),
            boost::bind( &dns_connection::handle_msg_len_read, shared_from_this(),
                    ph::error, ph::bytes_transferred));
}

void dns_connection::handle_msg_len_read( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    if ( !ec )
    {
        recv_msg_len_ = ntohs(recv_msg_len_);
        bs::error_code ec;
        log4.debugStream() << "Received DNS message length of " << recv_msg_len_ << " from " << socket_.remote_endpoint(ec);

        if (recv_msg_len_ > recv_header_.length() + recv_buf_.size())
        {
            log4.errorStream() << "TCP DNS message length " << recv_msg_len_ << " from " << socket_.remote_endpoint(ec) << " is too large! Closing connection.";
            return;
        }

        recv_buf_arr_[1] = buffer(recv_buf_, recv_msg_len_ - recv_header_.length());
        async_read( socket_, recv_buf_arr_, boost::bind(
                &dns_connection::handle_query_read,
                shared_from_this(), ph::error, ph::bytes_transferred ) );
    }
    else
    {
        if( ec != error::eof ) // eof means client closed connection
        {
            bs::error_code ec;
            log4.errorStream() << "An error occurred while reading TCP DNS message length from " << socket_.remote_endpoint(ec) << ": " << ec.message();
        }
    }
}

void dns_connection::handle_query_read( bs::error_code const& ec, std::size_t const bytes_transferred )
{
    bs::error_code endpec;
    boost::asio::ip::tcp::endpoint remote_endpoint( socket_.remote_endpoint(endpec) );

    if ( !ec )
    {
        // Validate header
        if ( bytes_transferred >= recv_header_.length() &&
                !recv_header_.qr() &&
                recv_header_.opcode() == O_QUERY &&
                recv_header_.z() == 0 )
        {
            // Header format OK
            log4.debugStream() << "Received TCP DNS query from " << remote_endpoint;
            if ( recv_header_.ad_cd() != 0 )
                log4.warnStream() << "Ignoring security extension flags in DNS query from " << remote_endpoint;

            boost::shared_ptr< dnsquery > query( new dnsquery(socket_.get_io_service()) );
            query->metric().start_timer();
            query->sender( shared_from_this() );
            try
            {
                parse_dns_query( *query, recv_header_, recv_buf_.data(), recv_buf_.data() + bytes_transferred - recv_header_.length());
                processor_( query );

                start();
            }
            catch ( dns_query_parser::parse_error const& )
            {
                log4.noticeStream() << "An error occurred while parsing TCP DNS query from " << remote_endpoint;
            }
        }
        else
        {
            // Malformed header
            log4.noticeStream() << "Received TCP DNS query with invalid header from " << remote_endpoint;
        }
    }
    else
    {
        log4.errorStream() << "An error occurred while receiving TCP DNS query from " << remote_endpoint << ": " << ec.message();
    }
}

void dns_connection::send_reply( query_ptr query )
{
    strand_.dispatch(
            boost::bind( &dns_connection::send_reply_, shared_from_this(), query) );
}


void dns_connection::send_reply_( query_ptr query )
{
    if ( send_in_progress_ )
    {
        if ( reply_buf_.full() )
        {
            bs::error_code ec;
            log4.warnStream() << "TCP send buffer full. Dropping reply to " << socket_.remote_endpoint(ec);
            return;
        }
        reply_buf_.push_back( query );
    }
    else
    {
        // Send reply immediately.
        send_in_progress_ = true;

        compose_dns_header(send_header_, *query);
        boost::uint8_t const* const end = compose_dns_response( *query, send_header_, send_buf_.data(), send_buf_.data() + send_buf_.size());

        std::ptrdiff_t const body_len = end - send_buf_.data();
        send_msg_len_ = htons( static_cast< boost::uint16_t >( body_len + send_header_.length() ) );

        send_buf_arr_[2] = buffer( send_buf_, end - send_buf_.data() );
        async_write(socket_, send_buf_arr_, strand_.wrap(
                boost::bind( &dns_connection::handle_send_reply, shared_from_this(), ph::error ) ) );

        query->metric().stop_timer();
        instrumenter->add_metric(query->metric());
    }
}

void dns_connection::handle_send_reply( bs::error_code const& ec )
{
    if ( !ec )
    {
        bs::error_code ec;
        log4.debugStream() << "Successfully sent TCP DNS reply to " << socket_.remote_endpoint(ec);
        // Fall through
    }
    else
    {
        bs::error_code ec;
        log4.errorStream() << "An error occurred while sending TCP DNS response to " << socket_.remote_endpoint(ec) << ": " << ec.message();
        // Fall through
    }

    send_in_progress_ = false;

    if ( !reply_buf_.empty() )
    {
        query_ptr query = reply_buf_.front();
        reply_buf_.pop_front();
        send_reply_( query );
    }
}
