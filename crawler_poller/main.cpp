/*
 * main.cpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
 */

#include "stdhdr.hpp"
#include "config.h"
#include "service.hpp"
#include "manconnection.hpp"
#include "signals.hpp"
#include "poller.hpp"
#include "asynchttprequester.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;

namespace po = boost::program_options;

log4cpp::Category& log4 = log4cpp::Category::getRoot();

int main(int argc, char const* argv[])
{
    int exit_code = 0;

    try
    {
        typedef std::vector< string > halist_t;
        halist_t home_agents;
        std::string log_file;
        std::string log_level;

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
                    ("log_file,l",    po::value< string >         (&log_file)->default_value("poller.log"),   "Log file path")
                    ("log_level,L",   po::value< string >         (&log_level)->default_value("WARN"),        "Log level\n"
                                                                                                              "    Only log messages with a level less than or equal to the specified severity will be logged. "
                                                                                                              "The log levels are NOTSET < DEBUG < INFO < NOTICE < WARN < ERROR < CRIT  < ALERT < FATAL = EMERG")
                    ("manager,M",     po::value< string >         (),                                         "The hostname or IP address of the Crawler Manager")
                    ("manport,P",     po::value< string >         ()    ->default_value(string("1141")),      "The port number to use when connecting to the Crawler Manager" )
                    ("solrurl,S",     po::value< string >         ()            ->required(),                 "The URL of the Solr HTTP interface"
                                                                                                              "    Device information is POST'ed to Solr at this URL")
                    ("home_agent,H",  po::value< halist_t >       (&home_agents),                             "List of well-known Home Agent web interface addresses\n"
                                                                                                              "    Any number of Home Agent addresses may be specified, separated by commas. "
                                                                                                              "Each address should have the form '<hostname or IP address>:<port>'. These "
                                                                                                              "addresses are used to 'seed' the crawler.")
                    ("threads",       po::value< std::size_t >    ()            ->default_value(1),           "Number of application threads\n"
                                                                                                              "    Set to 0 to use one thread per hardware core")
                    ;

        // Hidden options, will be allowed both on command line
        // and in config file, but will not be shown to the user
        po::options_description hidden("Hidden options");
        hidden.add_options()
                    ("debug,d", po::value< bool >()->implicit_value(true), "Enable debugging output")
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
        // Initialize logging
        {
            log4cpp::Appender* app = new log4cpp::FileAppender("file", log_file.c_str());
            log4.addAppender(app); // ownership of appender passed to category
            log4cpp::PatternLayout* lay = new log4cpp::PatternLayout();
            lay->setConversionPattern("%d [%p] %m%n");
            app->setLayout(lay); // ownership of layout passed to appender
        }

        if ( vm["debug"].as< bool >() )
        {
            log4.setAdditivity(true);
            log4cpp::Appender* capp = new log4cpp::OstreamAppender("console", &std::cout);
            log4.addAppender(capp);
            log4cpp::PatternLayout* clay = new log4cpp::PatternLayout();
            clay->setConversionPattern("%d [%p] %m%n");
            capp->setLayout(clay);
        }

        log4.setPriority(log4cpp::Priority::getPriorityValue(log_level));


        boost::asio::io_service io_service;

        typedef signals::duplicate_filter< std::string, boost::function< void(std::string const&) > > filter_t;
        filter_t filter;
        signals::home_agent_discovered.connect( boost::ref( filter ) );

        std::auto_ptr< manconnection > conn;
        if ( vm.count("manager") )
        {
            conn.reset( new manconnection( io_service, vm["manager"].as< string >(), vm["manport"].as< string >() ) );
            filter.connect( boost::bind( &manconnection::home_agent_discovered, conn.get(), _1 ) );
        }
        else
        {
            filter.connect( boost::ref( signals::home_agent_assigned ) );
        }

        poller::Context pollerctx( vm["solrurl"].as< string >(), boost::posix_time::seconds( vm["interval"].as< long >() ) );
        curl::Context curlctx( io_service );
        poller::pollercreator pc( pollerctx, curlctx );
        signals::home_agent_assigned.connect( boost::bind( &poller::pollercreator::create_poller, &pc, _1 ) );

        // I tried for a looong time to do this with std::for_each and I couldn't get it to work
        for ( halist_t::iterator i = home_agents.begin(); i != home_agents.end(); ++i) signals::home_agent_assigned(*i);

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


