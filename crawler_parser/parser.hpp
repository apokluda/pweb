
#include "stdhdr.hpp"

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
        typedef std::vector< std::string > halist_t;
        typedef std::vector< deviceinfo > devinfolist_t;

        halist_t homeagents;
        devinfolist_t deviceinfos;
    };
}

BOOST_FUSION_ADAPT_STRUCT(
    parser::deviceinfo,
    (std::string, name)
    (time_t, timestamp)
)

BOOST_FUSION_ADAPT_STRUCT(
    parser::getall,
    (parser::getall::halist_t, homeagents)
    (parser::getall::devinfolist_t, deviceinfos)
)

namespace parser
{
    template <typename Iterator>
    struct getall_parser : qi::grammar<Iterator, getall(), qi::locals<int> >
    {
        getall_parser() : getall_parser::base_type(start)
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

            start %= start_tag
                  >> omit[num[_a = _1]]
                  >> homeagents(_a)
                  >> omit[num[_a = _1]]
                  >> devices(_a)
                  >> end_tag;
        }

        qi::rule<Iterator> start_tag;
        qi::rule<Iterator> end_tag;
        qi::rule<Iterator, int()> num;
        qi::rule<Iterator, std::vector<std::string>(int) > homeagents;
        qi::rule<Iterator, std::vector<deviceinfo>(int) > devices;
        qi::rule<Iterator, getall(), qi::locals<int> > start;
    };
}
