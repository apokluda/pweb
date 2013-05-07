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
6|
planetlab-02.bu.edu|
planetlab-1.cs.colostate.edu|
mtuplanetlab2.cs.mtu.edu|
planetlab1.temple.edu|
vn5.cse.wustl.edu|
jupiter.cs.brown.edu|
7|
alex|1367591509,
apokluda|1367587699,
droplet.shihab|1367587810,
nexus.alex|1367591383,
rahmed|1367587706,
src|1367587691,
srchowdhury|1367587693
</body></html>

*/

namespace parser
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    struct deviceinfo_t
    {
        std::string name;
        long timestamp;
    };

    struct getall_t
    {
        std::vector< std::string > homeagents;
        std::vector< deviceinfo_t > deviceinfos;
    };
}

// We need to tell fusion about our deviceinfo struct
// to make it a first-class fusion citizen. This has to
// be in global scope.

BOOST_FUSION_ADAPT_STRUCT(
    parser::deviceinfo_t,
    (std::string, name)
    (long, timestamp)
)

BOOST_FUSION_ADAPT_STRUCT(
    parser::getall_t,
    (std::vector< std::string >, homeagents)
    (std::vector< parser::deviceinfo_t >, deviceinfos)
)

namespace parser
{
    template <typename Iterator>
    struct getall_parser
            : qi::grammar<Iterator, getall_t()>
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
        qi::rule<Iterator, deviceinfo_t()> deviceinfo;
        qi::rule<Iterator, getall_t()> getall;
    };

    template <typename Iterator>
    struct deviceinfo_parser
            : qi::grammar<Iterator, deviceinfo_t()>
    {
        deviceinfo_parser() : deviceinfo_parser::base_type(deviceinfo)
        {
            using qi::long_;
            using ascii::char_;

            string_ %= +(char_ - '|');

            deviceinfo %= string_ >> '|' >> long_ >> -',';
        }

        qi::rule<Iterator, std::string()> string_;
        qi::rule<Iterator, deviceinfo_t()> deviceinfo;
    };

    //template <>
}


#endif /* PARSER_HPP_ */
