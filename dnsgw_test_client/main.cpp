/*
 * main.cpp
 *
 *  Created on: 2013-04-02
 *      Author: alex
 */

#include "stdhdr.hpp"
#include "ha_config.hpp"
#include "ha_register.hpp"
#include "token_counter.hpp"
#include "asynchttprequester.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using boost::lexical_cast;
namespace po = boost::program_options;
using namespace boost::asio;

log4cpp::Category& log4 = log4cpp::Category::getRoot();

int main( int argc, char const* argv[] )
{
    int exit_code = 0;

    try
    {
        // Declare a group of options that will be available only
        // on the command line
        po::options_description generic("Generic options");
        generic.add_options()
                    ("halist,f",    po::value< string >()->required(),              "file containing list of home agents")
                    ("help,h",      po::value< bool >  ()->implicit_value(true),    "produce help message")
                    ("maxconn,M",   po::value< std::size_t >()->default_value(100), "maximum number of simultaneous connections overall")
                    ("maxconnha,m", po::value< std::size_t >()->default_value(20),  "maximum number of simultaneous connections to one home agent")
                    ("numnames,n",  po::value< std::size_t >()->default_value(1000),"the number of device names to be registered at each home agent")
                    ("owner,o",     po::value< string >()->default_value("test"),   "the owner name to be used in device names")
                    ("precheck,c",  po::value< bool >()->default_value(true),      "check that the IP addresses in the file match DNS entries and TCP connections can be established to the home agents before registering names")
                    ;

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(generic).run(), vm);

        if ( vm.count("help") )
        {
            cout << generic << "\n"
                      "The first line of the home agent list file should contain the\n"
                      "number of home agents listed in the file. Each subsequent\n"
                      "line should have the following form:\n"
                      "\n"
                      "url port ip_address haname\n"
                      "\n"
                      "where haname is the name of the home agent in the Plexus network.";
        }

        po::notify(vm);

        // Initialize logging
        log4cpp::Appender* app = new log4cpp::OstreamAppender("console", &std::cout);
        log4.addAppender(app); // ownership of appender passed to category
        log4cpp::PatternLayout* lay = new log4cpp::PatternLayout();
        lay->setConversionPattern("%d [%p] %m%n");
        app->setLayout(lay);
        app->setLayout(lay); // ownership of layout passed to appender

        std::string filename = vm["halist"].as< string >();
        log4.infoStream() << "Reading list of home agents from file '" << filename << '\'';
        std::vector< haconfig > halist;
        std::ifstream is;
        is.open( filename.c_str() );
        if ( is.fail() ) throw std::runtime_error("Unable to open file");
        load_halist(halist, is);
        log4.infoStream() << "Read list of " << halist.size() << " home agents";

        std::size_t maxconn = vm["maxconn"].as< std::size_t >();
        if ( vm["precheck"].as< bool >() )
        {
            log4.infoStream() << "Checking home agent state";
            ha_checker< halist_t::iterator > hac( halist.begin(), halist.end() );
            hac.sync_run( maxconn );

            log4.errorStream() << "-- The Following Home Agents are Inaccessible --";
            std::size_t inaccessible_count = 0;
            for (halist_t::iterator iter = halist.begin(); iter != halist.end(); ++iter)
            {
                if ( iter->status != GOOD ) { log4.errorStream() << iter->url; ++inaccessible_count; }
            }
            log4.errorStream() << "-- " << inaccessible_count << " Home Agents are Inaccessible --";
        }
        else
        {
            for (halist_t::iterator begin = halist.begin(); begin != halist.end(); ++begin) begin->status = GOOD;
        }

        boost::asio::io_service io_service;
        curl::Context c( io_service );
        //boost::shared_ptr< curl::AsyncHTTPRequester > r( new curl::AsyncHTTPRequester(c) );
        //r->fetch("http://pwebproject.net", &handle_fetch);

        token_counter tc(maxconn);
        std::size_t maxconnha = vm["maxconnha"].as< std::size_t >();
        register_names(halist.begin(), halist.end(), c, tc, vm["owner"].as< string >(), vm["numnames"].as< std::size_t >(), maxconnha);

        io_service.run();

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
