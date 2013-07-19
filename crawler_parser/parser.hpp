
#include "stdhdr.hpp"

namespace parser
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    struct homeagent
    {
        std::string hostname;
        std::string port;
    };

    struct device
    {
        std::string owner;
        std::string name;
        std::string port;
        time_t      timestamp;
        std::string location;
        std::string description;
    };

    struct getall
    {
        typedef std::vector< homeagent >   halist_t;
        typedef std::vector< device >      devlist_t;
        typedef std::vector< std::string > content_t;

        std::string   haname;
        halist_t      homeagents;
        devlist_t     devices;
        content_t     updates;
    };
}

BOOST_FUSION_ADAPT_STRUCT( parser::homeagent,
    (std::string, hostname)
    (std::string, port)
)

BOOST_FUSION_ADAPT_STRUCT( parser::device,
    (std::string, owner)
    (std::string, name)
    (std::string, port)
    (time_t,      timestamp)
    (std::string, location)
    (std::string, description)
)

BOOST_FUSION_ADAPT_STRUCT( parser::getall,
    (std::string,               haname)
    (parser::getall::halist_t,  homeagents)
    (parser::getall::devlist_t, devices)
    (parser::getall::content_t, updates)
)

namespace parser
{
    template <typename Iterator>
    struct getall_parser : qi::grammar<Iterator, getall(), ascii::space_type>
    {
        getall_parser() : getall_parser::base_type(getall_)
        {
            using qi::lit;
            using qi::long_;
            using qi::lexeme;
            using qi::omit;
            using ascii::char_;

            str_        %= lexeme[*(char_ - '<')];

            haname_     %= "<name>" >> str_ >> "</name>";

            neighbours_ %= "<neighbours>" >>
                             *homeagent_ >>
                          "</neighbours>";

            // need '>>' between tags so that whitespace formatting will be skipped
            homeagent_  %= lit("<home_agent>") >>
                              "<hostname>" >> str_ >> "</hostname>" >>
                              "<port>"     >> str_ >> "</port>" >>
                          "</home_agent>";

            devices_    %= "<devices>" >>
                              *device_ >>
                          "</devices>";

            device_     %= lit("<device>") >>
                              "<owner>"       >> str_        >> "</owner>" >>
                              "<name>"        >> str_        >> "</name>" >>
                              "<port>"        >> str_        >> "</port>" >>
                              "<timestamp>"   >> long_       >> "</timestamp>" >>
                              "<location>"    >> str_        >> "</location>" >>
                              "<description>" >> str_        >> "</description>" >>
                              "<content_meta>">> omit[str_]  >> "</content_meta>" >>
                          "</device>";

            content_    %= "<content updates>" >> str_ % ',' >> "</content updates>";

            getall_     %= "<getall>"
                          >> haname_
                          >> neighbours_
                          >> devices_
                          >> content_
                          >> "</getall>";
        }

        qi::rule<Iterator, std::string(),              ascii::space_type> str_;
        qi::rule<Iterator, std::string(),              ascii::space_type> haname_;
        qi::rule<Iterator, homeagent(),                ascii::space_type> homeagent_;
        qi::rule<Iterator, device(),                   ascii::space_type> device_;
        qi::rule<Iterator, std::vector<homeagent>(),   ascii::space_type> neighbours_;
        qi::rule<Iterator, std::vector<device>(),      ascii::space_type> devices_;
        qi::rule<Iterator, std::vector<std::string>(), ascii::space_type> content_;
        qi::rule<Iterator, getall(),                   ascii::space_type> getall_;
    };
}
