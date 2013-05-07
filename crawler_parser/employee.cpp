/*=============================================================================
    Copyright (c) 2002-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A parser for arbitrary tuples. This example presents a parser
//  for an employee structure.
//
//  [ JDG May 9, 2007 ]
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

#include <iostream>
#include <string>
#include <complex>

namespace parser
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    ///////////////////////////////////////////////////////////////////////////
    //  Our employee struct
    ///////////////////////////////////////////////////////////////////////////
    //[tutorial_employee_struct
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
    //]
}

// We need to tell fusion about our employee struct
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
    ///////////////////////////////////////////////////////////////////////////////
    //  Our employee parser
    ///////////////////////////////////////////////////////////////////////////////
    //[tutorial_employee_parser
    template <typename Iterator>
    struct deviceinfo_parser : qi::grammar<Iterator, getall(), qi::locals<int> >
    {
        deviceinfo_parser() : deviceinfo_parser::base_type(start)
        {
            using qi::lit;
            using qi::int_;
            using qi::long_;
            using qi::_1;
            using qi::_a;
            using qi::_r1;
            using boost::spirit::omit;
            using boost::spirit::repeat;
            using ascii::char_;

            start_tag %= lit("<html><body>");

            num %= int_ >> '|';

            homeagents %= repeat(_r1)[ +(char_ - '|' ) >> '|' ];

            devices %= repeat(_r1)[ +(char_ - '|') >> '|' >> long_ >> -lit(',') ];

            end_tag   %= lit("</body></html>");

            //hanum %= int_[_a = _1] >> '|';

            start %= start_tag
                  >> omit[num[_a = _1]]
                  >> homeagents(_a)
                  >> omit[num[_a = _1]]
                  >> devices(_a)
                  >> end_tag;
        }

        //qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
        qi::rule<Iterator> start_tag;
        qi::rule<Iterator> end_tag;
        qi::rule<Iterator, int()> num;
        qi::rule<Iterator, std::vector<std::string>(int) > homeagents;
        qi::rule<Iterator, std::vector<deviceinfo>(int) > devices;
        qi::rule<Iterator, getall(), qi::locals<int> > start;
    };

    template <typename Iterator>
    struct getall_parser : qi::grammar<Iterator, std::vector<getall>()>
    {
        getall_parser() : getall_parser::base_type(start)
        {
            using qi::lit;
            using qi::long_;
            using ascii::char_;

            start_tag %= lit("<>");
            //end_tag %= lit("</>");

            start %= (start_tag >> +(char_ - '|') >> '|' >> long_ /*>> end_tag*/) % lit(',');
        }

        //qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
        qi::rule<Iterator> start_tag;
        //qi::rule<Iterator, void> end_tag;
        qi::rule<Iterator, std::vector<getall>()> start;
    };

    //]
}

////////////////////////////////////////////////////////////////////////////
//  Main program
////////////////////////////////////////////////////////////////////////////
int
main()
{
    //std::cout << "/////////////////////////////////////////////////////////\n\n";
    //std::cout << "\t\tAn employee parser for Spirit...\n\n";
    //std::cout << "/////////////////////////////////////////////////////////\n\n";

    //std::cout
    //    << "Give me an employee of the form :"
    //    << "employee{age, \"surname\", \"forename\", salary } \n";
    //std::cout << "Type [q or Q] to quit\n\n";

    using boost::spirit::ascii::space;
    typedef std::string::const_iterator iterator_type;
    typedef parser::deviceinfo_parser<iterator_type> deviceinfo_parser;

    char const* all =
    "<html><body>"
    "6|"
    "planetlab-02.bu.edu|"
    "planetlab-1.cs.colostate.edu|"
    "mtuplanetlab2.cs.mtu.edu|"
    "planetlab1.temple.edu|"
    "vn5.cse.wustl.edu|"
    "jupiter.cs.brown.edu|"
    "3|"
    "alex|1367591509,"
    "apokluda|1367587699,"
    "droplet.shihab|1367587810,"
//    "nexus.alex|1367591383,"
//    "rahmed|1367587706,"
//    "src|1367587691,"
    "</body></html>";

    deviceinfo_parser g; // Our grammar
    std::string str( all );
    //while (getline(std::cin, str))
    //{
    //    if (str.empty() || str[0] == 'q' || str[0] == 'Q')
    //        break;

        parser::getall gall;
        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = phrase_parse(iter, end, g, space, gall);

        if (r && iter == end)
        {
     //       std::cout << boost::fusion::tuple_open('[');
     //       std::cout << boost::fusion::tuple_close(']');
      //      std::cout << boost::fusion::tuple_delimiter(", ");

            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            //std::cout << "got: " << boost::fusion::as_vector(emp) << std::endl;
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
    //}

    std::cout << "Bye... :-) \n\n";
    return 0;
}
