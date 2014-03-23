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
#include "config.h"
#include "service.hpp"
#include "manconnection.hpp"
#include "signals.hpp"
#include "poller.hpp"
#include "asynchttprequester.hpp"
#include "instrumentation.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;

namespace po = boost::program_options;

log4cpp::Category& log4 = log4cpp::Category::getRoot();
namespace curl { extern bool debug; }

// This needs to go somewhere, and right now it's going here!
parser::access_level_ parser::access_level;

int main(int argc, char const* argv[])
{
    int exit_code = 0;

    try
    {
        typedef std::vector< string > halist_t;
        halist_t home_agents;
        std::string log_file;
        std::string log_level;
        std::string hadevurl;
        std::string haconurl;
        std::string manager;
        std::string manport;
        std::string solr_deviceurl;
        std::string solr_contenturl;
        std::size_t threads;
        long interval;
        boost::uint16_t instport;
        bool verify_ssl_certs;
        bool debug = false;

        // Declare a group of options that will be available only
        // on the command line
        po::options_description generic("Generic options");
        generic.add_options()
                    ("version,v", po::value< bool >  ()->implicit_value(true), "Print program version and exit")
                    ("help,h",    po::value< bool >  ()->implicit_value(true), "Print summary of configuration options and exit")
                    ("config,c",  po::value< string >(),                       "Path to configuration file")
                    ;

        // Default value for the haurl (specified here to make the formatting nice below)
        char const * const dhadevurl = "https://%1%/?method=getall&timestamp=%2%";
        char const * const dhaconurl = "https://%1%/?method=getcontentlist&name=%2%";

        // Declare a group of options that will be allowed both
        // on the command line and in the config file
        po::options_description config("Configuration");
        config.add_options()
                    ("log_file,l",    po::value< string >         (&log_file)->default_value("poller.log"),   "Log file path")
                    ("log_level,L",   po::value< string >         (&log_level)->default_value("WARN"),        "Log level\n"
                                                                                                              "    Only log messages with a level less than or equal to the specified severity will be logged. "
                                                                                                              "Log levels are NOTSET < DEBUG < INFO < NOTICE < WARN < ERROR < CRIT  < ALERT < FATAL = EMERG")
                    ("hadevurl",      po::value< string >         (&hadevurl)->default_value(dhadevurl),      "The URL used to query the Home Agent for device updates.\n"
                                                                                                              "    The string may contain the placeholders %1% and %2% for the hostname and timestamp respectively.")
                    ("haconurl",      po::value< string >         (&haconurl)->default_value(dhaconurl),      "The URL used to query the Home Agent for content updates.\n"
                                                                                                              "    The string may contain the placeholders %1% and %2% for the hostname and device name respectively.")
                    ("verifysslcerts",po::value< bool >           (&verify_ssl_certs)->default_value(true),   "Whether or not to verify the SSL certificates for the Home Agents and Solr database")
                    ("manager,M",     po::value< string >         (&manager),                                 "Hostname or IP address of the Crawler Manager")
                    ("manport,P",     po::value< string >         (&manport)->default_value("1141"),          "Port number to use when connecting to the Crawler Manager" )
                    ("instport",      po::value< boost::uint16_t >(&instport)->default_value(8889),           "Port number for the instrumentation service or 0 to disable")
                    ("solrdevurl,D",  po::value< string >         (&solr_deviceurl)->required(),              "URL of the HTTP interface for the pweb_devices Solr core")
                    ("solrconurl,C",  po::value< string >         (&solr_contenturl)->required(),             "URL of the HTTP interface for the pweb_content Solr core")
                    ("interval,I",    po::value< long >           (&interval)->default_value(60),             "Number of seconds between polls to individual Home Agents")
                    ("home_agent,H",  po::value< halist_t >       (&home_agents),                             "List of well-known Home Agent web interface addresses\n"
                                                                                                              "    Any number of Home Agent addresses may be specified, separated by commas. "
                                                                                                              "Each address should have the form '<hostname or IP address>:<port>'. These "
                                                                                                              "addresses are used to 'seed' the crawler.")
                    ("threads",       po::value< std::size_t >    (&threads)->default_value(1),               "Number of application threads\n"
                                                                                                              "    Set to 0 to use one thread per hardware core")
                    ;

        // Hidden options, will be allowed both on command line
        // and in config file, but will not be shown to the user
        po::options_description hidden("Hidden options");
        hidden.add_options()
                    ("debug,d", po::value< bool >( &debug )->implicit_value(true), "Enable debugging output")
                    ("debug_curl", po::value< bool >( &curl::debug )->implicit_value(true), "Enable debugging output from cURL wrapper")
                    ;

        // Combine options
        po::options_description cmdline_options;
        cmdline_options.add(generic).add(config).add(hidden);

        po::options_description conffile_options;
        conffile_options.add(config).add(hidden);

        po::options_description visible_options("Allowed options");
        visible_options.add(generic).add(config);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).run(), vm);

        cout << "pWeb Crawler Poller Process " PWEB_VERSION_STR << endl;
        if ( vm.count("version") ) return EXIT_SUCCESS; // Exit after printing version

        if ( vm.count("help") )
        {
            cout << visible_options << endl;
            return EXIT_SUCCESS;
        }

        if ( vm.count("config") )
        {
            string config_file( vm["config"].as< string >() );
            if ( !config_file.empty() )
            {
                ifstream ifs(config_file.c_str());
                if ( !ifs )
                {
                    cerr << "Unable to open config file: " << config_file << endl;
                    return EXIT_FAILURE;
                }
                else
                {
                    po::store(po::parse_config_file(ifs, conffile_options), vm);
                }
            }
        }
        po::notify(vm);

        if ( manager.empty() && home_agents.empty() )
        {
            cerr << "You must set at least one of --manager and --home_agent";
            return EXIT_FAILURE;
        }

        // Initialize logging
        {
            log4cpp::Appender* app = new log4cpp::FileAppender("file", log_file.c_str());
            log4.addAppender(app); // ownership of appender passed to category
            log4cpp::PatternLayout* lay = new log4cpp::PatternLayout();
            lay->setConversionPattern("%d [%p] %m%n");
            app->setLayout(lay); // ownership of layout passed to appender
        }

        if ( debug )
        {
            log4.setAdditivity(true);
            log4cpp::Appender* capp = new log4cpp::OstreamAppender("console", &std::cout);
            log4.addAppender(capp);
            log4cpp::PatternLayout* clay = new log4cpp::PatternLayout();
            clay->setConversionPattern("%d [%p] %m%n");
            capp->setLayout(clay);
        }

        log4.setPriority(log4cpp::Priority::getPriorityValue(log_level));

        log4.infoStream() << "--- pWeb Crawler Poller Process " << PWEB_VERSION_STR << " started ---" ;

        boost::asio::io_service io_service;

        typedef signals::duplicate_filter< std::string, boost::function< void(std::string const&) > > filter_t;
        filter_t filter;
        signals::home_agent_discovered.connect( boost::ref( filter ) );

        std::auto_ptr< manconnection > conn;
        if ( vm.count("manager") )
        {
            conn.reset( new manconnection( io_service, manager, manport ) );
            filter.connect( boost::bind( &manconnection::home_agent_discovered, conn.get(), _1 ) );
        }
        else
        {
            filter.connect( boost::ref( signals::home_agent_assigned ) );
        }

        using boost::posix_time::seconds;
        instrumentation::instrumenter instrumenter( io_service, instport );
        poller::Context pollerctx( instrumenter, hadevurl, haconurl, solr_deviceurl, solr_contenturl, verify_ssl_certs, seconds(interval) );
        poller::pollercreator pc( pollerctx );
        signals::home_agent_assigned.connect( boost::bind( &poller::pollercreator::create_poller, &pc, _1 ) );

        // I tried for a looong time to do this with std::for_each and I couldn't get it to work
        for ( halist_t::iterator i = home_agents.begin(); i != home_agents.end(); ++i) signals::home_agent_discovered(*i);

        run( io_service, threads );
        exit_code = EXIT_SUCCESS;

        // Fall through to shutdown logging
    }
    catch ( std::exception const& e )
    {
        cerr << e.what() << endl;
        exit_code = EXIT_FAILURE;
    }

    // Shutdown logging
    log4cpp::Category::shutdown();

    return exit_code;
}


