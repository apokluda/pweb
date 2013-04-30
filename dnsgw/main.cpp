/*
 * This is the main entry point for the dnsgw program that
 * is part of the pWeb system. The dynsgw program provides
 * a DNS-interface to the Plexus network overlay. DNS queries
 * are translated to requests sent to Plexus home agents.
 *
 *  Created on: 2013-02-19
 *      Author: Alexander Pokluda <apokluda@uwaterloo.ca>
 *
 * Implementation/Deployment Notes:
 * - The name to IP bindings retrieved from the Plexus
 *   overlay are not cached. It is assumed that this program
 *   will be run behind name servers configured that
 *   will cache names (a configurable TTL value is returned
 *   in the DNS response). If it it determined that caching
 *   is necessary in the future, a local cache could easily
 *   be added to this program or a distributed cache based
 *   on something like memcached could be accessed by a
 *   dnsgw server farm.
 *
 * Known Issues:
 *  - The home agent protocol is crap:
 *      - security issue: send out going connections to "trusted" servers, receive connections from anyone
 *      - don't know if server is up until we try to contact them
 *      - inefficient (multiple TCP handshakes, unused fields)
 *  - The serial number used for messages to the home agents is copied from the DNS queries (this is also used internally)
 *      - a better approach would be to hash the DNS ID with connections specific information
 *  - The Gateway should return SOA records, NS records pointing to itself, and A/AAAA records for the NS servers
 *  - Log messages could be improved
 *  - Newer DNS features (such as compression and extensions) are not implemented
 */

#include "stdhdr.hpp"
#include "config.h"
#include "haproxy.hpp"
#include "dnsspeaker.hpp"
#include "haloadbalancer.hpp"
#include "instrumenter.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
namespace po = boost::program_options;

typedef instrumentation::instrumenter instrumenter_t;
using namespace instrumentation::result_types;
using instrumentation::udp_instrumenter;
using instrumentation::null_instrumenter;

log4cpp::Category& log4 = log4cpp::Category::getRoot();
std::auto_ptr< instrumenter_t > instrumenter;
bool debug = false;

static void checked_io_service_run(boost::asio::io_service& io_service)
{
    for (;;)
    {
        try
        {
            // Is it a good idea to re-run the service after an uncaught exception?
            io_service.run();
            break;
        }
        catch ( std::exception const& e )
        {
            log4.alertStream() << "UNCAUGHT EXCEPTION: " << e.what();
        }
    }
}

static void run( boost::asio::io_service& io_service, std::size_t const num_threads )
{
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through asio.
    boost::asio::signal_set sig_set( io_service, SIGINT, SIGTERM );
    sig_set.async_wait( boost::bind( &boost::asio::io_service::stop, &io_service ) );

    if ( num_threads > 1 )
    {
        // Create a pool of threads to run all of the io_services.
        std::vector<boost::shared_ptr<boost::thread> > threads;
        threads.reserve( num_threads );
        for (std::size_t i = 0; i < num_threads; ++i)
        {
            boost::shared_ptr<boost::thread> thread( new boost::thread(
                    boost::bind( checked_io_service_run, boost::ref( io_service ) ) ) );
            threads.push_back( thread );

            log4.debugStream() << "Started io_service thread " << i << " with id " << thread->get_id();
        }

        // Wait for all threads in the pool to exit.
        for (std::size_t i = 0; i < threads.size(); ++i)
            threads[i]->join();
    }
    else
    {
        // Run one io_service in current thread
        log4.debug("Starting io_service event loop");
        checked_io_service_run( io_service );
    }
}

void split_hostname(string& host, string& port, string const& hostname)
{
    std::vector<string> address;
    boost::split(address, hostname, boost::is_any_of(":"));
    if ( address.size() == 2 )
    {
        host = address[0];
        port = address[1];
    }
    else
    {
        std::ostringstream oss;
        oss << "Invalid hostname '" << hostname << "'; Format should be 'hostname:port'";
        throw std::runtime_error(oss.str());
    }
}

int main(int argc, char const* argv[])
{
    int exit_code = EXIT_SUCCESS;

    try
    {
        typedef std::vector< string > haaddr_list_t;

        string log_file;
        string log_level;
        string interface;
        string nshostname;
        string suffix;
        string instsrv;
        haaddr_list_t home_agents;
        std::size_t num_threads;
        unsigned timeout;
        boost::uint16_t ttl;
        boost::uint16_t port;
        boost::uint16_t nsport;

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
                    ("ttl",           po::value< boost::uint16_t >(&ttl)        ->default_value(3600),        "Number of seconds DNS clients will be instructed cache name to IP mappings")
                    ("timeout,t",     po::value< unsigned >       (&timeout)    ->default_value(12),          "Maximum number of seconds to wait for response from a Home Agent before returning an error to the DNS client")
                    ("log_file,l",    po::value< string >         (&log_file)   ->default_value("dnsgw.log"), "Log file path")
                    ("log_level,L",   po::value< string >         (&log_level)  ->default_value("WARN"),      "Log level\n"
                                                                                                              "    Only log messages with a level less than or equal to the specified severity will be logged."
                                                                                                              "The log levels are NOTSET < DEBUG < INFO < NOTICE < WARN < ERROR < CRIT  < ALERT < FATAL = EMERG")
                    ("iface,i",       po::value< string >         (&interface),                               "IP v4 or v6 address of interface to listen on for DNS queries")
                    ("port,p",        po::value< boost::uint16_t >(&port)       ->default_value(53),          "TCP and UDP port to listen on for DNS queries")
                    ("home_agent,H",  po::value< haaddr_list_t >  (&home_agents)->required(),                 "List of Home Agent addresses to connect to\n"
                                                                                                              "    Any number of Home Agent addresses may be speficed, separated by commas."
                                                                                                              "Each address should have the form '<hostname or IP address>:<port>'. The DNS"
                                                                                                              "Gateway will use all the Home Agent addresses specified in a round-robin manner.")
                    ("nshostname,N",  po::value< string >         (&nshostname) ->required(),                 "The hostname of the DNS gateway\n"
                                                                                                              "    This hostname is included in DNS replies and in messages sent to to Home Agents."
                                                                                                              "Home Agents will send repiles to this hostname using separate TCP connections." )
                    ("nsport,P",      po::value< boost::uint16_t >(&nsport)     ->required(),                 "TCP port used to receive replies from Home Agents")
                    ("suffix,s",      po::value< string >         (&suffix)     ->default_value(".dht."),     "Suffix to be removed from domain names in order to obtain pWeb device names")
                    ("threads",       po::value< std::size_t >    (&num_threads)->default_value(1),           "Number of application threads\n"
                                                                                                              "    Set to 0 to use one thread per hardware core")
                    ("instsrv",       po::value< string >         (&instsrv),                                 "Address of instrumentation server\n"
                                                                                                              "    The address should have the form '<hostname or IP address>:<port>'")
                    ;

        // Hidden options, will be allowed both on command line
        // and in config file, but will not be shown to the user
        po::options_description hidden("Hidden options");
        hidden.add_options()
                    ("debug,d", po::value< bool >(&debug)->implicit_value(true), "don't daemonize and enable debugging output")
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

        cout << "pWeb DNS Gateway " PWEB_VERSION_STR << endl;
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

        // Set timeout
        haproxy::timeout( boost::posix_time::seconds( timeout ) );

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

        if ( num_threads == 0 )
        {
            num_threads = boost::thread::hardware_concurrency();
        }

        boost::asio::io_service io_service;
        // Initialize instrumentation
        if ( !instsrv.empty() )
        {
            string hostname, port;
            split_hostname(hostname, port, instsrv);
            instrumenter.reset( new udp_instrumenter(io_service, hostname, port) );
        }
        else
        {
            instrumenter.reset( new null_instrumenter );
        }

        // These typedefs must match the typedefs at the end of haspeaker.cpp
        // in order to avoid linker errors
        typedef boost::shared_ptr< hasendproxy > hasendproxy_ptr;
        typedef std::vector< hasendproxy_ptr > haproxies_t;

        haproxies_t haproxies;
        haproxies.reserve( home_agents.size() );
        for (haaddr_list_t::const_iterator i = home_agents.begin(); i != home_agents.end(); ++i)
        {
            string hostname, port;
            split_hostname(hostname, port, *i);
            boost::uint16_t iport = boost::lexical_cast<boost::uint16_t>(port);
            hasendproxy_ptr hptr(new hasendproxy(io_service, hostname, iport, nshostname, nsport, suffix) );
            hptr->start();
            haproxies.push_back(hptr);
        }

        // Ownership of haproxies vector remains with caller
        typedef ha_load_balancer< haproxies_t::iterator, haproxies_t::difference_type > balancer_t;
        balancer_t halb( io_service, haproxies.begin(), haproxies.size() );

        harecvproxy harecvproxy(io_service, nshostname, nsport, ttl);
        harecvproxy.start();

        udp_dnsspeaker udp_dnsspeaker(io_service, boost::bind(&balancer_t::process_query, &halb, _1), interface, port);
        tcp_dnsspeaker tcp_dnsspeaker(io_service, boost::bind(&balancer_t::process_query, &halb, _1), interface, port);

        udp_dnsspeaker.start();
        tcp_dnsspeaker.start();

        run( io_service, num_threads );

        // Fall through to shutdown logging
    }
    catch ( std::exception const& e )
    {
        cerr << e.what() << endl;

        // Set exit code and fall through to shutdown logging
        exit_code = EXIT_FAILURE;
    }

    // Shutdown logging
    log4cpp::Category::shutdown();
    // Shutdown instrumentation
    instrumenter.reset();

    return exit_code;
}
