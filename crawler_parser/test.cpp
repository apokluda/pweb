
#include "parser.hpp"

int main()
{
    typedef std::string::const_iterator iterator_type;
    typedef parser::getall_parser<iterator_type> getall_parser;

    char const* all =
    "<html><body>"
    "6|"
    "planetlab-02.bu.edu|"
    "planetlab-1.cs.colostate.edu|"
    "mtuplanetlab2.cs.mtu.edu|"
    "planetlab1.temple.edu|"
    "vn5.cse.wustl.edu|"
    "jupiter.cs.brown.edu|"
    "5|"
    "alex|1367591509,"
    "apokluda|1367587699,"
    "droplet.shihab|1367587810,"
    "nexus.alex|1367591383,"
    "rahmed|1367587706,"
    "</body></html>";

    getall_parser const g; // Our grammar
    std::string str( all );

    parser::getall gall;
    std::string::const_iterator iter = str.begin();
    std::string::const_iterator end = str.end();
    bool r = parse(iter, end, g, gall);

    if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "\n-------------------------\n";

        std::cout << "Home Agents:\n";
        BOOST_FOREACH(std::string const& str, gall.homeagents)
        {
            std::cout << str << '\n';
        }
        std::cout << "Devices:\n";
        BOOST_FOREACH(parser::deviceinfo const& devinfo, gall.deviceinfos)
        {
            std::cout << devinfo.name << ", " << devinfo.timestamp << '\n';
        }
    }
    else
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------\n";
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}
