/*
 * This is the main entry point for the dnsgw program that
 * is part of the pWeb system. The dynsgw program provides
 * a DNS-interface to the Plexus network overlay. DNS queries
 * are translated to requests sent to Plexus home agents.
 *
 *  Created on: 2013-02-19
 *      Author: Alexander Pokluda <apokluda@uwaterloo.ca>
 */

#include "stdhdr.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
namespace po = boost::program_options;

int main(int argc, char const* argv[])
{
    try
    {
        string config_file;
        string interface;
        uint32_t ttl;
        uint16_t port;

        // Declare a group of options that will be available only
        // on the command line
        po::options_description generic("Generic options");
        generic.add_options()
            ("version,v", "print version string")
            ("help,h", "produce help message")
            ("config,c", po::value< string >(&config_file)->implicit_value("/etc/dnsgw.cfg"), "config file name")
            ("iface,i", po::value< string >(&interface), "IP address of interface to listen on")
            ("port,p", po::value< uint16_t >(&port)->default_value(53), "port to listen on")
        ;

        // Declare a group of options that will be allowed both
        // on the command line and in the config file
        po::options_description config("Configuration");
        config.add_options()
            ("ttl", po::value< uint32_t >(&ttl)->default_value( 3600 ), "number of seconds to cache name to IP mappings")
        ;

        // Hidden options, will be allowed both on command line
        // and in config file, but will not be shown to the user
        po::options_description hidden("Hidden options");
        hidden.add_options()
            ("debug,d", "enable debugging output")
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
        po::notify(vm);

        if ( !config_file.empty() )
        {
            ifstream ifs(config_file.c_str());
            if (!ifs)
            {
                cerr << "Unable to open config file: " << config_file << endl;
                return EXIT_FAILURE;
            }
            else
            {
                po::store(po::parse_config_file(ifs, conffile_options), vm);
                po::notify(vm);
            }
        }

        if ( vm.count("version") )
        {
            cout << "Development version" << endl;
            return EXIT_SUCCESS;
        }
        if ( vm.count("help") )
        {
            cout << visible_options << endl;
            return EXIT_SUCCESS;
        }

        // TODO: Create and initialize DNS protocol handler

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e )
    {
        cerr << e.what() << endl;

        return EXIT_FAILURE;
    }
}
