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
namespace parser
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    struct deviceinfo
    {
        std::string name;
        time_t timestamp;
    };

    struct getall
    {
        std::vector< std::string > homeagents;
        std::vector< deviceinfo > deviceinfos;
    };
}

// We need to tell fusion about our deviceinfo struct
// to make it a first-class fusion citizen. This has to
// be in global scope.

BOOST_FUSION_ADAPT_STRUCT(
    parser::deviceinfo,
    (std::string, name)
    (time_t, timestamp)
)

BOOST_FUSION_ADAPT_STRUCT(
    parser::getall,
    (std::vector< std::string >, homeagents)
    (std::vector< parser::deviceinfo >, deviceinfos)
)

namespace parser
{
    template <typename Iterator>
    struct getall_parser : qi::grammar<Iterator, getall_parser()>
    {
        getall_parser() : getall_parser::base_type(getall)
        {
            using qi::lit;
            using qi::int_;
            using qi::long_;
            using ascii::char_;
            using boost::spirit::omit;

            start_tag %= lit("<html><body>");

            homeagent %= '|' >> +(char_ - '|');

            deviceinfo %= +(char_ - '|') >> '|' >> long_ >> -',';

            end_tag = lit("</html></body>");

            getall %= start_tag >> omit[int_] >> *homeagent >> '|' >> omit[int_] >> '|' >> *deviceinfo >> end_tag;
        }

        qi::rule<Iterator, void(std::string)> start_tag;
        qi::rule<Iterator, void(std::string)> end_tag;
        qi::rule<Iterator, std::string()> homeagent;
        qi::rule<Iterator, deviceinfo()> deviceinfo;
        qi::rule<Iterator, getall()> getall;
    };

    //template <>
}


#endif /* PARSER_HPP_ */
