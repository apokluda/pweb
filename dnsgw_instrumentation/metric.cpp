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
#include "metric.hpp"

static const int NANOSECONDS_VERSION = 1;
BOOST_CLASS_VERSION(boost::chrono::nanoseconds, NANOSECONDS_VERSION)
BOOST_SERIALIZATION_SPLIT_FREE(boost::chrono::nanoseconds)

namespace boost {
namespace serialization {

template < class Archive >
void save(Archive& ar, boost::chrono::nanoseconds const& nanos, unsigned const version)
{
    boost::chrono::nanoseconds::rep count = nanos.count();
    ar << count;
}

template < class Archive >
void load(Archive& ar, boost::chrono::nanoseconds& nanos, unsigned const version)
{
    if ( version != NANOSECONDS_VERSION ) throw std::runtime_error("invalid version for boost::chrono::nanoseconds");
    boost::chrono::nanoseconds::rep count;
    ar >> count;
    nanos = boost::chrono::nanoseconds( count );
}

} // namespace serialization
} // namespace boost

using namespace instrumentation;

template <class Archive>
void metric::serialize(Archive& ar, const unsigned int version)
{
    if ( version != VERSION ) throw std::runtime_error("metric: invalid class version");
    ar & device_name_;
    ar & start_time_;
    ar & duration_;
    ar & result_;
}

template void metric::serialize<boost::archive::text_oarchive>(boost::archive::text_oarchive&, unsigned int);

const char* instrumentation::result_str(result_t const result)
{
    switch ( result )
    {
    case SUCCESS:
        return "SUCCESS";
    case HA_CONNECTION_ERROR:
        return "HA_CONNECTION_ERROR";
    case HA_RETURNED_ERROR:
        return "HA_RETURNED_ERROR";
    case TIMEOUT:
        return "TIMEOUT";
    case INVALID_REQUEST:
        return "INVALID_REQUEST";
    default:
        return "UNKNOWN";
    }
}

std::ostream& instrumentation::operator<<(std::ostream& out, metric const& metric)
{
    out << "device_name=" << metric.device_name()
            << "; start_time=" << metric.start_time()
            << "; duration=" << metric.duration()
            << "; result=" << result_str( metric.result() );
    return out;
}
