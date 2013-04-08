/*
 * instrumenter.hpp
 *
 *  Created on: 2013-04-08
 *      Author: apokluda
 */

#ifndef INSTRUMENTER_HPP_
#define INSTRUMENTER_HPP_

#include "stdhdr.hpp"

// In the future, it may make sense to make this a class hierarchy.
// Right now, one class seems appropriate. Read the comment below on
// instrumenter for how the instrumentation system works.
class metric
{
    friend class boost::serialization::access;

public:
    enum result_t
    {
        SUCCESS,
        HA_CONNECTION_ERROR,
        HA_RETURNED_ERROR,
        TIMEOUT,
        UNKNOWN
    };

    metric(std::string const& device_name)
    : device_name_( device_name )
    , result_( UNKNOWN )
    {
    }

    void start_timer()
    {
        start_time_point_ = boost::chrono::high_resolution_clock::now();
    }

    void stop_timer()
    {
        duration_ = boost::chrono::high_resolution_clock::now() - start_time_point_;
    }

    void result(result_t const result)
    {
        result_ = result;
    }

    std::string device_name() const
    {
        return device_name_;
    }

    result_t result() const
    {
        return result_;
    }

    boost::posix_time::ptime start_time() const
    {
        return start_time_;
    }

    boost::chrono::duration duration() const
    {
        return duration_;
    }

private:
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    { // class version defined at end of file
        ar & device_name_;
        ar & start_time_;
        ar & duration_;
        ar & result_;
    }

    std::string const device_name_;
    boost::posix_time::ptime start_time_;
    union // Note: There is no check to ensure that this class is used correctly!
    {     // start_timer() and stop_timer() must be called exactly once each in that order
          // before duration() is called.
        boost::chrono::high_resolution_clock::time_point start_time_point_;
        boost::chrono::nanoseconds duration_;
    };
    result_t result_;
};

// Instances of "metric" will be added to the instrumenter, which will
// package one or more serialzied metric instances into a UDP packet
// and send them to an instrumentation daemon (possibly using a Nagle timer)
// to reduce the number of datagrams/packets sent). The
class instrumenter
{
public:
    instrumenter( boost::asio::io_service& io_service, std::string const& server, std::string const& port);

    void add_metric(boost::shared_ptr<metric> pmetric);

private:
    void handle_resolve(boost::system::error_code const& ec, boost::asio::ip::udp::resolver::iterator iter);

    void start_nagle_timer();
    void send();

    boost::asio::ip::udp::socket socket_;
    std::size_t buf_size_; // the number of bytes currently in the buffer
    boost::array<boost::uint8_t, 65507> buf_;
};

BOOST_CLASS_VERSION(metric, 1)

#endif /* INSTRUMENTER_HPP_ */
