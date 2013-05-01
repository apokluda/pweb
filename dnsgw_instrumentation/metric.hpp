/*
 * metric.hpp
 *
 *  Created on: 2013-04-14
 *      Author: alex
 */

#ifndef METRIC_HPP_
#define METRIC_HPP_

// Note: stdhdr.hpp should be included before this header
#include <boost/archive/text_oarchive.hpp>

namespace instrumentation
{

// In the future, it may make sense to make this a class hierarchy.
// Right now, one class seems appropriate. Read the comment below on
// instrumenter for how the instrumentation system works.
namespace result_types
{
enum result_t
{
    SUCCESS,
    HA_CONNECTION_ERROR,
    HA_RETURNED_ERROR,
    TIMEOUT,
    INVALID_REQUEST,
    UNKNOWN
};
} // namespace result_types
using namespace result_types;

class metric;
const char* result_str(result_t const result);
std::ostream& operator<<(std::ostream& out, metric const& metric);

class metric
{
public:
    metric()
    : start_time_( boost::posix_time::microsec_clock::local_time() )
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

    result_t result() const
    {
        return result_;
    }

    void device_name(std::string const& device_name)
    {
        device_name_ = device_name;
    }

    std::string device_name() const
    {
        return device_name_;
    }

    boost::posix_time::ptime start_time() const
    {
        return start_time_;
    }

    boost::chrono::nanoseconds duration() const
    {
        return duration_;
    }

private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);

public:
    // NOTE: If any changes are made to these members, the serialize()
    // method should be updated and the class version should be incremented!
    static int const VERSION = 1;
private:
    std::string device_name_;
    boost::posix_time::ptime const start_time_;
    boost::chrono::high_resolution_clock::time_point start_time_point_;
    boost::chrono::nanoseconds duration_;
    result_t result_;
};

} // namespace instrumentation

BOOST_CLASS_VERSION(instrumentation::metric, instrumentation::metric::VERSION)

#endif /* METRIC_HPP_ */
