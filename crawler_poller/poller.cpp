/*
 * scheduler.cpp
 *
 *  Created on: 2013-05-05
 *      Author: alex
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
	os(opf);
	return os;
}

typedef boost::shared_ptr< curl::AsyncHTTPRequester > reqptr_t;
void handle_getcontentlist(poller::Context const&, reqptr_t const&, std::string const& device, CURLcode const&, std::string const& body);
void handle_postcontentlist(reqptr_t const&, std::string const& device, CURLcode const&, std::string const& body);
}

namespace poller
{

parser::getall_parser< std::string::const_iterator > const poller::g_;

poller::poller(Context const& pollerctx, curl::Context& curlctx, std::string const& hostname )
: requester_( curlctx, false )
, hostname_( hostname )
, timer_( curlctx.get_io_service() )
, timestamp_( 1 ) // For some bizarre reason, the Home Agents can't handle a request with timestamp 0
, pollerctx_( pollerctx )
{
	using namespace boost::posix_time;
	boost::random::uniform_int_distribution< long > dist(0, pollerctx_.interval.total_milliseconds());
	time_duration const first_interval = milliseconds( dist( gen ) );

	log4.debugStream() << "Poller created for '" << hostname_ << "', waiting " << first_interval << " before first poll";

	timer_.expires_from_now( first_interval );
	start();
}

void poller::do_poll( bs::error_code const& ec )
{
	// Do this now so that we actually poll the Home Agent once ever 'interval' seconds
	// (or as close as possible) rather than once ever 'interval + polling + parsing' seconds.
	timer_.expires_from_now( pollerctx_.interval );

	if ( !ec )
	{
		std::ostringstream url;
		url << "http://" << hostname_ << ":20005/?method=getall&timestamp=" << timestamp_;
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
			log4.errorStream() << "Failed to parse device update from " << hostname_ << " here: \"" << OutputParseFail(content, iter) << '\"';
			start(); return;
		}

		parser::getall::halist_t& homeagents = gall.homeagents;
		parser::getall::devlist_t& devices = gall.devices;

		log4.noticeStream() << "Home Agent '" << hostname_ << "' returned list of " << homeagents.size() << " neighbours and " << devices.size() << " updated devices";

		typedef parser::getall::halist_t::const_iterator hiter_t;
		for ( hiter_t i = homeagents.begin(); i != homeagents.end(); ++i ) signals::home_agent_discovered( i->hostname );

		if ( devices.empty() ) { start(); return; }

		// We could use the Spirit Karma library here, but this is sufficient for now
		std::ostringstream out;
		out << "<add overwrite=\"true\">";
		typedef parser::getall::devlist_t::const_iterator diter_t;
		time_t newtimestamp = 0;
		for ( diter_t i = devices.begin(); i != devices.end(); ++i )
		{
			out << "<doc>"
					"<field name=\"owner\">"       << i->owner                      << "</field>"
					"<field name=\"name\">"        << i->name << '.' << gall.haname << "</field>"
					"<field name=\"home\">"        << gall.haname                   << "</field>"
					"<field name=\"port\">"        << i->port                       << "</field>"
					"<field name=\"timestamp\">"   << i->timestamp                  << "</field>"
					"<field name=\"location\">"    << i->location                   << "</field>"
					"<field name=\"description\">" << i->description                << "</field>"
					"</doc>";

			if (i->timestamp > newtimestamp) newtimestamp = i->timestamp;
		}
		out << "</add>";

		log4.debugStream() << "Sending device list update to Solr for " << gall.haname;
		requester_.fetch(pollerctx_.solr_deviceurl + "?commit=true", boost::bind(&poller::handle_post, this, _1, _2, newtimestamp), out.str());

		// Poll for content updates
		for (parser::getall::contlist_t::const_iterator i = gall.updates.begin(); i != gall.updates.end(); ++i)
		{
			std::ostringstream url;
			url << "http://" << hostname_ << ":20005/?method=getcontentlist&name=" << *i;
			std::ostringstream device;
			device << *i << '.' << gall.haname;

			boost::shared_ptr< curl::AsyncHTTPRequester > r( new curl::AsyncHTTPRequester(requester_.get_context(), false) );
			r->fetch(url.str(), boost::bind(&handle_getcontentlist, pollerctx_, r, device.str(), _1, _2) );

			log4.debugStream() << "Retrieving content metadata for " << device.str();
		}
	}
	else
	{
		log4.errorStream() << "An error occurred while polling '" << hostname_ << "', CURLcode " << code << ": " << curl_easy_strerror(code) ;
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
			log4.errorStream() << "Failed to parse content metadata for " << device << " here: \"" << OutputParseFail(body, iter) << '\"';
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
						"<field name=\"ctid\">"        << device << "-vid" << i->id << "</field>"
						"<field name=\"id\">"          << i->id                     << "</field>"
						"<field name=\"device_name\">" << device                    << "</field>"
						"<field name=\"title\">"       << i->title                  << "</field>"
						"<field name=\"filesize\">"    << i->filesize               << "</field>"
						"<field name=\"mimetype\">"    << i->mimetype               << "</field>"
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
	pollers_.push_back( new poller( pollerctx_, curlctx_, hostname ) );
	log4.noticeStream() << "Now monitoring " << pollers_.size() << " Home Agents";
}

}
