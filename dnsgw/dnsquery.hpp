/*
 * dnsquery.hpp
 *
 *  Created on: 2013-02-21
 *      Author: alex
 */

#ifndef DNSQUERY_HPP_
#define DNSQUERY_HPP_

#include "stdhdr.hpp"
#include "metric.hpp"

class udp_dnsspeaker;

class dns_connection;
typedef boost::shared_ptr< dns_connection > dns_connection_ptr;

typedef std::pair< udp_dnsspeaker*, boost::asio::ip::udp::endpoint > udp_connection_t;

class dnsquery;
typedef boost::shared_ptr< dnsquery > query_ptr;

enum opcode_t
{
    O_QUERY,
    O_IQUERY,
    O_STATUS,
    O_RESERVED
};

enum rcode_t
{
    R_SUCCESS,
    R_FORMAT_ERROR,
    R_SERVER_FAILURE,
    R_NAME_ERROR,
    R_NOT_IMPLEMENTED,
    R_REFUSED,
    R_RESERVED
};

enum qtype_t
 {
     QTYPE_MIN   =   0,
     T_MIN       =   1,

     T_A         =   1,
     T_NS        =   2,
     T_MD        =   3,
     T_MF        =   4,
     T_CNAME     =   5,
     T_SOA       =   6,
     T_MB        =   7,
     T_MG        =   8,
     T_MR        =   9,
     T_NULL      =  10,
     T_WKS       =  11,
     T_PTR       =  12,
     T_HINFO     =  13,
     T_MINFO     =  14,
     T_MX        =  15,
     T_TXT       =  16,
     // Were missing some...
     T_AAAA      =  28,

     T_MAX       =  28,
     QT_MIN      = 252,

     QT_AXFR     = 252,
     QT_MAILB    = 253,
     QT_MAILA    = 254,
     QT_ALL      = 255,

     QT_MAX      = 255,
     QTYPE_MAX   = 65535
 };

enum qclass_t
{
    QCLASS_MIN   =   0,
    C_MIN        =   1,

    C_IN         =   1,
    C_CS         =   2,
    C_CH         =   3,
    C_HS         =   4,

    C_MAX        =   4,
    QC_MIN       = 255,

    QC_ANY       = 255,

    QC_MAX       = 255,
    QCLASS_MAX   = 65535
};

qclass_t to_qclass( unsigned val  );

struct dnsquestion
{
    std::string name;
    qtype_t qtype;
    qclass_t qclass;
};

struct dnsrr
{
    std::string name;
    qtype_t rtype;
    qclass_t rclass;
    boost::uint32_t ttl;
    boost::uint16_t rdlength;
    std::vector<boost::uint8_t> rdata;
};

class dnsquery
: public boost::enable_shared_from_this< dnsquery >,
  private boost::noncopyable
{
public:
    typedef std::vector< dnsquestion >::const_iterator question_iterator;
    typedef std::vector< dnsrr >::const_iterator answer_iterator;
    typedef std::vector< dnsrr >::const_iterator authority_iterator;
    typedef std::vector< dnsrr >::const_iterator additional_iterator;

    dnsquery(boost::asio::io_service& io_service)
    : timer_( io_service )
    , rcode_( R_SUCCESS )
    , id_( 0 )
    , rd_( false )
    {
    }

    boost::uint16_t id() const
    {
        return id_;
    }

    void id( boost::uint16_t const id )
    {
        id_ = id;
    }

    bool rd() const
    {
        return rd_;
    }

    void rd( bool const rd )
    {
        rd_ = rd;
    }

    rcode_t rcode() const
    {
        return rcode_;
    }

    void rcode(rcode_t const rcode)
    {
        rcode_ = rcode;
    }

    void sender( udp_dnsspeaker* const speaker,  boost::asio::ip::udp::endpoint const& endpoint )
    {
        sender_ = std::make_pair(speaker, endpoint);
    }

    void sender( dns_connection_ptr conn )
    {
        sender_ = conn;
    }

    boost::asio::ip::address remote_address() const;

    std::size_t num_questions() const
    {
        return questions_.size();
    }

    void num_questions( std::size_t const num )
    {
        questions_.reserve(num);
    }

    void add_question(dnsquestion const& question)
    {
        questions_.push_back(question);
    }

    question_iterator questions_begin() const
    {
        return questions_.begin();
    }

    question_iterator questions_end() const
    {
        return questions_.end();
    }

    std::size_t num_answers() const
    {
        return answers_.size();
    }

    void num_answers( std::size_t const num )
    {
        answers_.reserve(num);
    }

    void add_answer(dnsrr const& answer)
    {
        answers_.push_back(answer);
    }

    answer_iterator answers_begin() const
    {
        return answers_.begin();
    }

    answer_iterator answers_end() const
    {
        return answers_.end();
    }

    std::size_t num_authorities() const
    {
        return authorities_.size();
    }

    void num_authorities( std::size_t const num )
    {
        authorities_.reserve(num);
    }

    void add_authority(dnsrr const& authority)
    {
        authorities_.push_back(authority);
    }

    authority_iterator authorities_begin() const
    {
        return authorities_.begin();
    }

    authority_iterator authorities_end() const
    {
        return authorities_.end();
    }

    std::size_t num_additionals() const
    {
        return additionals_.size();
    }

    void num_additionals( std::size_t const num )
    {
        additionals_.reserve(num);
    }

    void add_additional(dnsrr const& additional)
    {
        additionals_.push_back(additional);
    }

    additional_iterator additionals_begin() const
    {
        return additionals_.begin();
    }

    additional_iterator additionals_end() const
    {
        return additionals_.end();
    }

    boost::asio::ip::udp::endpoint remote_udp_endpoint() const;

    void send_reply();

    boost::asio::deadline_timer& timer()
    {
        return timer_;
    }

    instrumentation::metric& metric()
    {
        return metric_;
    }

private:
    boost::variant< udp_connection_t, dns_connection_ptr > sender_;
    boost::asio::deadline_timer timer_;
    instrumentation::metric metric_;
    std::vector< dnsquestion > questions_;
    std::vector< dnsrr > answers_;
    std::vector< dnsrr > authorities_;
    std::vector< dnsrr > additionals_;

    rcode_t rcode_;
    boost::uint16_t id_;
    bool rd_;

};

void inline complete_query(dnsquery& query, rcode_t const rcode, instrumentation::result_t const result)
{
    query.rcode(rcode);
    query.metric().result(result);
    query.send_reply();
}

#endif /* DNSQUERY_HPP_ */
