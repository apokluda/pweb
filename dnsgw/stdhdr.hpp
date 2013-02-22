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
#include <stdexcept>

#include <cstring>
#include <cstdlib>

// Boost
#include <boost/program_options.hpp>
#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp> // not included by asio.hpp on 1.48
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>

// Logging
#include <log4cpp/Category.hh>
#include <log4cpp/Appender.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/Layout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/Priority.hh>

#endif /* STDHDR_H_ */
