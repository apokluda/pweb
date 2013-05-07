/*
 * test.cpp
 *
 *  Created on: 2013-05-06
 *      Author: apokluda
 */

#include "parser.hpp"

char const* all =
"<html><body>"
"6"
"|planetlab-02.bu.edu"
"|planetlab-1.cs.colostate.edu"
"|mtuplanetlab2.cs.mtu.edu"
"|planetlab1.temple.edu"
"|vn5.cse.wustl.edu"
"|jupiter.cs.brown.edu"
"|7|"
"alex|1367591509,"
"apokluda|1367587699,"
"droplet.shihab|1367587810,"
"nexus.alex|1367591383,"
"rahmed|1367587706,"
"src|1367587691,"
"srchowdhury|1367587693"
"</body></html>";

char const* devinfo =
"alex|1367591509,";

int main(int argc, char const* argv[])
{
    parser::deviceinfo_t devinfo;
    parser::deviceinfo_parser< const char* > dinfogram;
    char const* iter = all;
    char const* end = all + strlen(all) + 1;
    bool r = parse(iter, end, dinfogram, devinfo);
    if (r && iter == end)
        std::cout << "success\n";
    else
        std::cout << "failure\n";

    return 0;
}
