/*
 * test.cpp
 *
 *  Created on: 2013-05-06
 *      Author: apokluda
 */

#include "parser.hpp"

char const* str =
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

int main(int argc, char const* argv)
{
    parser::getall gall;
    std::string::const_iterator iter = str;
    std::string::const_iterator end = str + strlen(str) + 1;
    parser::getall_parser<const char*> g;
    bool r = phrase_parse(iter, end, g, gall);
    if (r && iter == end)
        std::cout << "success\n";
    else
        std::cout << "falure\n";

    return 0;
}
