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

#include "poller.hpp"
#include "asynchttprequester.hpp"
#include "signals.hpp"

using namespace boost::asio;
namespace ph = boost::asio::placeholders;
namespace bs = boost::system;

static boost::random::mt19937 gen;

extern log4cpp::Category& log4;


namespace
{

class OutputParseFail
{
public:
	OutputParseFail(std::string const& str, std::string::const_iterator const iter)
	: str_(str), iter_(iter) {}

	void operator()(std::ostream& out) const
	{
		// Find position where fail occurred and back up a few characters
		std::string::size_type const goodchars = std::min(iter_ - str_.begin(), std::string::difference_type( 5 ) );
		std::ostream_iterator<std::string::value_type> oiter(out);
		std::copy(iter_ - goodchars, iter_, oiter);

		// Print marker
		out << '|';

		// Print some more output after the fail
		std::string::size_type const badchars = std::min(str_.end() - iter_, std::string::difference_type( 9 ) );
		std::copy(iter_, iter_ + badchars, oiter);
	}

private:
	std::string::const_iterator const iter_;
	std::string const& str_;
};

OutputParseFail outputparsefail(std::string const& str, std::string::const_iterator const iter)
{
	return OutputParseFail(str, iter);
}

std::ostream& operator<<(std::ostream& os, OutputParseFail const& opf)
{
	opf(os);
	return os;
}

typedef boost::shared_ptr< curl::AsyncHTTPRequester > reqptr_t;
void handle_getcontentlist(poller::Context const&, reqptr_t const&, std::string const& device, CURLcode const&, std::string const& body);
void handle_postcontentlist(reqptr_t const&, std::string const& device, CURLcode const&, std::string const& body);
}

namespace poller
{

parser::getall_parser< std::string::const_iterator > const poller::g_;

boost::atomic< std::size_t > poller::num_pollers_(0);
boost::atomic< std::size_t > poller::num_falling_behind_(0);

poller::poller(Context const& pollerctx, std::string const& hostname )
: curlctx_( pollerctx.io_service, pollerctx.verify_ssl_certs )
, requester_( curlctx_, false )
, hostname_( hostname )
, timer_( pollerctx.io_service )
, timestamp_( 1 ) // For some bizarre reason, the Home Agents can't handle a request with timestamp 0
, pollerctx_( pollerctx )
, falling_behind_(false)
{
    num_pollers_.fetch_add(1, boost::memory_order_relaxed);

    pollerctx_.instrumenter.home_agent_discovered( hostname_ );

	using namespace boost::posix_time;
	boost::random::uniform_int_distribution< long > dist(0, pollerctx_.interval.total_milliseconds());
	time_duration const first_interval = milliseconds( dist( gen ) );

	log4.debugStream() << "Poller created for '" << hostname_ << "', waiting " << first_interval << " before first poll";

	timer_.expires_from_now( first_interval );
	start();
}

void poller::start()
{
    if (timer_.expires_from_now() < boost::posix_time::time_duration(0, 0 ,0 ,0))
    {
        if (!falling_behind_)
        {
            falling_behind_ = true;
            double const num_falling_behind = num_falling_behind_.fetch_add(1, boost::memory_order_consume);
            double const num_pollers = num_pollers_.load(boost::memory_order_consume);
            if (num_falling_behind / num_pollers > 0.25)
            {
                log4.noticeStream() << "More than 25% of pollers are falling behind";
            }
        }
    }
    else
    {
        if (falling_behind_)
        {
            falling_behind_ = false;
            num_falling_behind_.fetch_sub(1, boost::memory_order_relaxed);
        }
    }

    timer_.async_wait( boost::bind( &poller::do_poll, this, boost::asio::placeholders::error ) );
}

void poller::do_poll( bs::error_code const& ec )
{
	// Do this now so that we actually poll the Home Agent once ever 'interval' seconds
	// (or as close as possible) rather than once ever 'interval + polling + parsing' seconds.
	timer_.expires_from_now( pollerctx_.interval );

	if ( !ec )
	{
	    boost::format url( pollerctx_.hadevurlfmt );
        url % hostname_ % timestamp_;
		std::string const& urlstr = url.str();

		log4.infoStream() << "Polling Home Agent at '" << hostname_ << "' with URL: " << urlstr;

		requester_.fetch(urlstr, boost::bind(&poller::handle_poll, this, _1, _2));
	}
	else
	{
		// We will get operation_aborted on shutdown
		if ( ec != error::operation_aborted ) log4.errorStream() << "An error occurred while waiting for timer: " << ec.message();
	}
}

inline instrumentation::query_result curlcode_to_query_result(CURLcode const code)
{
    switch (code)
    {
    case CURLE_OPERATION_TIMEDOUT:
        return instrumentation::TIMEOUT;
    default:
        return instrumentation::NETWORK_ERROR;
    }
}

void poller::handle_poll( CURLcode const code, std::string const& content )
{
	// check that code is CURLE_OK, log result, and start a new request
	if ( code == CURLE_OK )
	{
		log4.infoStream() << "Successfully polled '" << hostname_ << '\'';
		log4.debugStream() << "Content received:\n" << content;

		parser::getall gall;
		std::string::const_iterator iter = content.begin();
		std::string::const_iterator end = content.end();
		using boost::spirit::ascii::space;
		bool const r = boost::spirit::qi::phrase_parse(iter, end, g_, space, gall);

		if ( !r || iter != end)
		{
			log4.errorStream() << "Failed to parse device update from " << hostname_ << " here: \"" << outputparsefail(content, iter) << '\"';
			pollerctx_.instrumenter.query_result( hostname_, instrumentation::PARSE_ERROR );
			start(); return;
		}
		pollerctx_.instrumenter.query_result( hostname_, instrumentation::SUCCESS, gall.haname );

		parser::getall::halist_t& homeagents = gall.homeagents;
		parser::getall::devlist_t& devices = gall.devices;

		log4.infoStream() << "Home Agent '" << hostname_ << "' returned list of " << homeagents.size() << " neighbours and " << devices.size() << " updated devices";

		typedef parser::getall::halist_t::const_iterator hiter_t;
		for ( hiter_t i = homeagents.begin(); i != homeagents.end(); ++i )
	    {
		    signals::home_agent_discovered( i->hostname );
		    // This is a bit of a hack and I don't really like it. The
		    // instrumentation interface should really be just for getting
		    // information about the state of the poller, but the other guys
		    // want to use it for the registration interface and other things
		    // (so that these things can find out which Home Agents are up).
		    // And, they want these things to be able to get the Home Agent
		    // description, so we have to pass it through to the instrumentation
		    // "module" here. Ideally, there should be another component that
		    // uses the poller instrumentation interface as one of its sources
		    // to make an API that the registration system can use.
		    pollerctx_.instrumenter.set_description( i->hostname, i->description );
	    }

		time_t newtimestamp = 0;
		if ( !devices.empty() )
		{
			// We could use the Spirit Karma library here, but this is sufficient for now
			std::ostringstream out;
			out << "<add overwrite=\"true\">";
			typedef parser::getall::devlist_t::const_iterator diter_t;
			for ( diter_t i = devices.begin(); i != devices.end(); ++i )
			{
				out <<  "<doc>"
					    "<field name=\"r_type\">"           "device"                        "</field>"
						"<field name=\"r_key\">"         << i->name                       << "</field>"
						"<field name=\"d_owner\">"       << i->owner                      << "</field>"
						"<field name=\"d_name\">"        << i->name << '.' << gall.haname << "</field>"
						"<field name=\"d_home\">"        << gall.haname                   << "</field>";
				if ( !i->port.empty() )
				out <<  "<field name=\"d_port\">"        << i->port                       << "</field>";
				out <<  "<field name=\"d_type\">"        << i->type                       << "</field>"
						"<field name=\"d_timestamp\">"   << i->timestamp                  << "</field>"
						"<field name=\"d_location\">"    << i->location                   << "</field>"
						"<field name=\"description\">" << i->description                << "</field>"
						"</doc>";

				if (i->timestamp > newtimestamp) newtimestamp = i->timestamp;
			}
			out << "</add>";

			log4.debugStream() << "Sending device list update to Solr for " << gall.haname;
			requester_.fetch(pollerctx_.solr_deviceurl + "?commit=true", boost::bind(&poller::handle_post, this, _1, _2, newtimestamp), out.str());
		}
		else
		{
			// NOTE!!!! We need to update the timestamp here!!!!!!!! How is the question.
			start();
		}

		// Poll for content updates
		for (parser::getall::contlist_t::const_iterator i = gall.updates.begin(); i != gall.updates.end(); ++i)
		{
			if ( i->empty() ) continue;

			boost::format url( pollerctx_.haconurlfmt );
			url % hostname_ % *i;
			std::string const& urlstr = url.str();

			std::ostringstream device;
			device << *i << '.' << gall.haname;

			boost::shared_ptr< curl::AsyncHTTPRequester > r( new curl::AsyncHTTPRequester(requester_.get_context(), false) );
			r->fetch(urlstr, boost::bind(&handle_getcontentlist, pollerctx_, r, device.str(), _1, _2) );

			log4.debugStream() << "Retrieving content metadata for " << device.str() << " with URL " << urlstr;
		}
	}
	else
	{
		log4.errorStream() << "An error occurred while polling '" << hostname_ << "', CURLcode " << code << ": " << curl_easy_strerror(code) ;
		pollerctx_.instrumenter.query_result( hostname_, curlcode_to_query_result(code), curl_easy_strerror(code) );
		start();
	}
}

void poller::handle_post( CURLcode const code, std::string const& content, time_t const newtimestamp  )
{
	if ( code == CURLE_OK )
	{
		if ( content.find("<int name=\"status\">0</int>") != std::string::npos )
		{
			log4.infoStream() << "Successfully sent device info update to Solr";
		}
		else
		{
			log4.errorStream() << "Unable to find success status for device info update in Solr output:\n" << content;
		}
		// We update the timestamp used for polling only if we successfully contacted Solr
		// This ensures that we don't loose any device updates if the update fails.
		// (Previously, this was only if the results were successfully committed to the Solr database,
		// but if some bozo puts garbage in their device info, we will try to commit it
		// over and over and over and over again! So we update the timestamp even if
		// the update fails, so that we can forget about the junk).
		timestamp_ = newtimestamp + 1;
		log4.debugStream() << "New timestamp is " << timestamp_;
	}
	else
	{
		log4.errorStream() << "An error occurred while POST'ing data to Solr, CURLcode " << code << ": " << curl_easy_strerror(code);
	}
	start();
}

}

namespace
{

static parser::contmeta_parser< std::string::const_iterator > const cmparser;

void handle_getcontentlist(poller::Context const& pollerctx, reqptr_t const& requester, std::string const& device, CURLcode const& code, std::string const& body)
{
	// Note: This method may be called multiple times simultaneously. It must be thread-safe.

	if ( code == CURLE_OK )
	{
		// parse content metadata and send update to solr
		log4.infoStream() << "Successfully retrieved content metadata for " << device;
		log4.debugStream() << "Content received:\n" << body;

		parser::contmeta contmeta;
		std::string::const_iterator iter = body.begin();
		std::string::const_iterator end = body.end();
		using boost::spirit::ascii::space;
		bool const r = boost::spirit::qi::phrase_parse(iter, end, cmparser, space, contmeta);

		if ( !r || iter != end)
		{
			log4.errorStream() << "Failed to parse content metadata for " << device << " here: \"" << outputparsefail(body, iter) << '\"';
			return;
		}

		log4.noticeStream() << "Received content metadata for " << device << " containing " << contmeta.videos.size() << " videos";

		parser::contmeta::videolist_t& videos = contmeta.videos;

		std::ostringstream out;
		out << "<add overwrite=\"true\">";
		int pubcount = 0;
		typedef parser::contmeta::videolist_t::const_iterator viter_t;
		for ( viter_t i = videos.begin(); i != videos.end(); ++i )
		{
			if ( i->access == parser::PUBLIC )
			{
				++pubcount;
				out << "<doc>"
						"<field name=\"r_type\">"           "content"                    "</field>"
						"<field name=\"r_key\">"         << device << "-vid" << i->id << "</field>"
						"<field name=\"c_ctid\">"        << device << "-vid" << i->id << "</field>"
						"<field name=\"c_id\">"          << i->id                     << "</field>"
						"<field name=\"c_device_name\">" << device                    << "</field>"
						"<field name=\"c_title\">"       << i->title                  << "</field>"
						"<field name=\"c_filesize\">"    << i->filesize               << "</field>"
						"<field name=\"c_mimetype\">"    << i->mimetype               << "</field>"
						"<field name=\"description\">" << i->description            << "</field>"
						"</doc>";
			}
		}
		out << "</add>";

		log4.debugStream() << "Sending content metadata update to Solr for " << device << " with " << pubcount << " public videos";
		if ( pubcount > 0 ) requester->fetch(pollerctx.solr_contenturl + "?softCommit=true", boost::bind(&handle_postcontentlist, requester, device, _1, _2), out.str());
	}
	else
	{
		log4.errorStream() << "An error occurred while retrieving content metadata from home agent for " << device << ": " << curl_easy_strerror(code);
	}
}

void handle_postcontentlist(reqptr_t const&, std::string const& device, CURLcode const& code, std::string const& body)
{
	if ( code == CURLE_OK )
	{
		if ( body.find("<int name=\"status\">0</int>") != std::string::npos )
		{
			log4.infoStream() << "Successfully sent content metadata update to Solr for " << device;
		}
		else
		{
			log4.errorStream() << "Unable to find success status for content metadata update for " << device << " in Solr output:\n" << body;
		}
	}
	else
	{
		log4.errorStream() << "An error occurred while sending content metadata update to Solr for " << device;
	}
}

}

namespace poller
{

void pollercreator::create_poller( std::string const& hostname )
{
	log4.infoStream() << "Home Agent '" << hostname << "' has been assigned for monitoring";
	strand_.dispatch( boost::bind( &pollercreator::create_poller_, this, hostname ) );
}

void pollercreator::create_poller_( std::string const& hostname )
{
	// The pointer must be stored in the container immediately for exception safety
	pollers_.push_back( new poller( pollerctx_, hostname ) );
	log4.noticeStream() << "Now monitoring " << pollers_.size() << " Home Agents";
}

}
