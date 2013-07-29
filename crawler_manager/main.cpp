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
#include "pollerconnector.hpp"
#include "signals.hpp"
#include "homeagentdb.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;

namespace po = boost::program_options;
namespace ptime = boost::posix_time;
using namespace boost::asio;

log4cpp::Category& log4 = log4cpp::Category::getRoot();
bool debug = false;

int main(int argc, char const* argv[] )
{
    int exit_code = 0;

    try
    {
        typedef std::vector< string > halist_t;
        halist_t home_agents;

        // Declare a group of options that will be available only
        // on the command line
        po::options_description generic("Generic options");
        generic.add_options()
                    ("version,v", po::value< bool >  ()->implicit_value(true), "Print program version and exit")
                    ("help,h",    po::value< bool >  ()->implicit_value(true), "Print summary of configuration options and exit")
                    ("config,c",  po::value< string >(),                       "Path to configuration file")
                    ;

        // Declare a group of options that will be allowed both
        // on the command line and in the config file
        po::options_description config("Configuration");
        config.add_options()
                    ("log_file,l",    po::value< string >         ()            ->default_value("dnsgw.log"), "Log file path")
                    ("log_level,L",   po::value< string >         ()            ->default_value("WARN"),      "Log level\n"
                                                                                                              "    Only log messages with a level less than or equal to the specified severity will be logged. "
                                                                                                              "The log levels are NOTSET < DEBUG < INFO < NOTICE < WARN < ERROR < CRIT  < ALERT < FATAL = EMERG")
                    ("iface,i",       po::value< string >         (),                                         "IP v4 or v6 address of interface to listen on for connections from poller processes")
                    ("port,p",        po::value< boost::uint16_t >()            ->default_value(1141),        "TCP port to listen on for poller processes queries")
                    ("home_agent,H",  po::value< halist_t >       (&home_agents)->required(),                 "List of well-known Home Agent web interface addresses\n"
                                                                                                              "    Any number of Home Agent addresses may be specified, separated by commas. "
                                                                                                              "Each address should have the form '<hostname or IP address>:<port>'. These "
                                                                                                              "addresses are used to 'seed' the crawlers.")
                    ("threads",       po::value< std::size_t >    ()            ->default_value(1),           "Number of application threads\n"
                                                                                                              "    Set to 0 to use one thread per hardware core")
                    ;

        // Hidden options, will be allowed both on command line
        // and in config file, but will not be shown to the user
        po::options_description hidden("Hidden options");
        hidden.add_options()
                    ("debug,d", po::value< bool >(&debug)->implicit_value(true), "Enable debugging output")
                    ;

        // Combine options
        po::options_description cmdline_options;
        cmdline_options.add(generic).add(config).add(hidden);

        po::options_description conffile_options;
        conffile_options.add(config).add(hidden);

        po::options_description visible_options("Allowed options");
        visible_options.add(generic).add(config);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(generic).run(), vm);

        cout << "pWeb Crawler Manager Process " PWEB_VERSION_STR << endl;
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

        // Initialize logging
        {
            log4cpp::Appender* app = new log4cpp::FileAppender("file", vm["log_file"].as< string >().c_str());
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

        log4.setPriority(log4cpp::Priority::getPriorityValue( vm["log_level"].as< string >()) );

        boost::asio::io_service io_service;

        pollerconnector pconn( io_service, vm["iface"].as< string >(), vm["port"].as< boost::uint16_t >() );
        homeagentdb hadb( io_service );
        signals::poller_connected.connect( boost::bind( &homeagentdb::poller_connected, &hadb, _1 ) );
        signals::poller_disconnected.connect( boost::bind( &homeagentdb::poller_disconnected, &hadb, _1 ) );
        signals::home_agent_discovered.connect( boost::bind( &homeagentdb::add_home_agent, &hadb, _1 ) );

        for ( halist_t::const_iterator i = home_agents.begin(); i != home_agents.end(); ++i ) hadb.add_home_agent( *i );

        run( io_service, vm["num_threads"].as< std::size_t >() );

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
