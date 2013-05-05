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
                    ("manager,M",     po::value< string >         ()            ->required(),                 "The hostname or IP address of the Crawler Manager")
                    ("manport,P",     po::value< string >         ()            ->default_value("1141"),      "The port number to use when connecting to the Crawler Manager" )
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
            log4cpp::Appender* app = new log4cpp::FileAppender("file", vm["log_file"].as< string >().c_str());
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

        log4.setPriority(log4cpp::Priority::getPriorityValue(vm["log_level"].as< string >()));

        boost::asio::io_service io_service;

//        std::string filename = vm["halist"].as< string >();
//        log4.infoStream() << "Reading list of home agents from file '" << filename << '\'';
//        std::vector< haconfig > halist;
//        std::ifstream is;
//        is.open( filename.c_str() );
//        if ( is.fail() ) throw std::runtime_error("Unable to open file");
//        load_halist(halist, is);
//        log4.infoStream() << "Read list of " << halist.size() << " home agents";
//
//        std::size_t maxconn = vm["maxconn"].as< std::size_t >();
//        if ( vm["precheck"].as< bool >() )
//        {
//            log4.infoStream() << "Checking home agent state";
//            ha_checker< halist_t::iterator > hac( halist.begin(), halist.end() );
//            hac.sync_run( maxconn );
//
//            log4.errorStream() << "-- The Following Home Agents are Inaccessible --";
//            std::size_t inaccessible_count = 0;
//            for (halist_t::iterator iter = halist.begin(); iter != halist.end(); ++iter)
//            {
//                if ( iter->status != GOOD ) { log4.errorStream() << iter->url; ++inaccessible_count; }
//            }
//            log4.errorStream() << "-- " << inaccessible_count << " Home Agents are Inaccessible --";
//        }
//        else
//        {
//            for (halist_t::iterator begin = halist.begin(); begin != halist.end(); ++begin) begin->status = GOOD;
//        }
//
//        boost::asio::io_service io_service;
//        curl::Context c( io_service );
//        //boost::shared_ptr< curl::AsyncHTTPRequester > r( new curl::AsyncHTTPRequester(c) );
//        //r->fetch("http://pwebproject.net", &handle_fetch);
//
//        token_counter tc(maxconn);
//        std::size_t maxconnha = vm["maxconnha"].as< std::size_t >();
//        register_names(halist.begin(), halist.end(), c, tc, vm["owner"].as< string >(), vm["numnames"].as< std::size_t >(), maxconnha);

        manconnection conn( io_service, vm["manager"].as< string >(), vm["manport"].as< string >() );
        //scheduler seched;

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


