/*
 * parser.hpp
 *
 *  Created on: 2013-05-06
 *      Author: apokluda
 */

#ifndef PARSER_HPP_
#define PARSER_HPP_

#include "stdhdr.hpp"

/* Sample Text to Parse:

<html><body>
6
|planetlab-02.bu.edu
|planetlab-1.cs.colostate.edu
|mtuplanetlab2.cs.mtu.edu
|planetlab1.temple.edu
|vn5.cse.wustl.edu
|jupiter.cs.brown.edu
|7|
alex|1367591509,
apokluda|1367587699,
droplet.shihab|1367587810,
nexus.alex|1367591383,
rahmed|1367587706,
src|1367587691,
srchowdhury|1367587693
</body></html>

*/

// using same namespace name as in Spirit tutorial
namespace client
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    struct deviceinfo
    {
        std::string name;
        time_t timestamp;
    };
}

// We need to tell fusion about our deviceinfo struct
// to make it a first-class fusion citizen. This has to
// be in global scope.

BOOST_FUSION_ADAPT_STRUCT(
    client::deviceinfo,
    (std::string, name)
    (time_t, timestamp)
)

namespace client
{
    template <typename Iterator>
    struct deviceinfo_parser : qi::grammar<Iterator, deviceinfo()>
    {
        deviceinfo_parser() : deviceinfo_parser::base_type(start)
        {
            using qi::lit;
            using qi::long_;
            using ascii::char_;

            start %= +char_ >> '|' >> long_ >> -',';
        }

        qi::rule<Iterator, deviceinfo()> start;
    };
}

#endif /* PARSER_HPP_ */
