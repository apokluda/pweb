/*
 * stdhdr.h
 *
 *  Created on: 2013-02-19
 *      Author: alex
 */

#ifndef STDHDR_H_
#define STDHDR_H_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <queue>
#include <map>

#include <cstring>
#include <cstdlib>

// Ubuntu package dependencies
// libboost-program-options1.53-dev
// libboost-thread1.53-dev
// libboost-atomic1.53-dev
// libboost-date-time1.53-dev
// liblog4cpp5-dev

// Boost
#include <boost/program_options.hpp>
#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>
#include <boost/version.hpp>
#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/signals2.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

// Boost.Spirit
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

// Logging
#include <log4cpp/Category.hh>
#include <log4cpp/Appender.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/Layout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/Priority.hh>

// cURL
#include <curl/curl.h>

#endif /* STDHDR_H_ */
