/*
 *  Copyright (C) 2013 Alexander Pokluda
 *
 *  This file is part of pWeb.
 *
 *  pWeb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pWeb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pWeb.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PARSER_HPP
#define PARSER_HPP

#include "stdhdr.hpp"

namespace parser
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    struct homeagent
    {
        std::string hostname;
        std::string description;
        std::string port;
    };

    struct device
    {
        std::string owner;
        std::string name;
        std::string port;
        std::string type;
        time_t      timestamp;
        std::string location;
        std::string description;
    };

    struct getall
    {
        typedef std::vector< homeagent >   halist_t;
        typedef std::vector< device >      devlist_t;
        typedef std::vector< std::string > contlist_t;

        std::string   haname;
        halist_t      homeagents;
        devlist_t     devices;
        contlist_t    updates;
    };

    enum ACCESS_LEVEL
    {
    	PUBLIC,
    	PRIVATE,
    	NOT_SHARED
    };

    struct video
    {
        int          id;
        std::string  title;
        long         filesize;
        std::string  mimetype;
        std::string  description;
        ACCESS_LEVEL access;
    };

    struct contmeta
    {
        typedef std::vector< video >        videolist_t;

        videolist_t   videos;
    };
}

BOOST_FUSION_ADAPT_STRUCT( parser::homeagent,
    (std::string, hostname)
    (std::string, description)
    (std::string, port)
)

BOOST_FUSION_ADAPT_STRUCT( parser::device,
    (std::string, owner)
    (std::string, name)
    (std::string, port)
    (std::string, type)
    (time_t,      timestamp)
    (std::string, location)
    (std::string, description)
)

BOOST_FUSION_ADAPT_STRUCT( parser::getall,
    (std::string,                haname)
    (parser::getall::halist_t,   homeagents)
    (parser::getall::devlist_t,  devices)
    (parser::getall::contlist_t, updates)
)

BOOST_FUSION_ADAPT_STRUCT( parser::video,
    (int,                  id)
    (std::string,          title)
    (long,                 filesize)
    (std::string,          mimetype)
    (std::string,          description)
    (parser::ACCESS_LEVEL, access)
)

BOOST_FUSION_ADAPT_STRUCT( parser::contmeta,
    (parser::contmeta::videolist_t, videos)
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
            str2_       %= lexeme[*(char_ - ',' - '<')];

            haname_     %= "<name>" >> str_ >> "</name>";

            neighbours_ %= "<neighbours>" >>
                             *homeagent_ >>
                          "</neighbours>";

            // need '>>' between tags so that whitespace formatting will be skipped
            homeagent_  %= lit("<home agent>") >>
                              "<hostname>" >> str_ >> "</hostname>" >>
                              "<description>" >> str_ >> "</description>" >>
                              "<port>"     >> str_ >> "</port>" >>
                          "</home agent>";

            devices_    %= "<devices>" >>
                              *device_ >>
                          "</devices>";

            device_     %= lit("<device>") >>
                              "<owner>"       >> str_        >> "</owner>" >>
                              "<name>"        >> str_        >> "</name>" >>
                              "<port>"        >> str_        >> "</port>" >>
                              "<type>"        >> str_        >> "</type>" >>
                              "<timestamp>"   >> long_       >> "</timestamp>" >>
                              "<location>"    >> str_        >> "</location>" >>
                              "<description>" >> str_        >> "</description>" >>
                          "</device>";

            content_    %= "<content updates>" >> (str2_ % ',') >> "</content updates>";

            getall_     %= "<getall>"
                          >> haname_
                          >> neighbours_
                          >> devices_
                          >> content_
                          >> "</getall>";
        }

        qi::rule<Iterator, std::string(),              ascii::space_type> str_;
        qi::rule<Iterator, std::string(),              ascii::space_type> str2_;
        qi::rule<Iterator, std::string(),              ascii::space_type> haname_;
        qi::rule<Iterator, homeagent(),                ascii::space_type> homeagent_;
        qi::rule<Iterator, device(),                   ascii::space_type> device_;
        qi::rule<Iterator, std::vector<homeagent>(),   ascii::space_type> neighbours_;
        qi::rule<Iterator, std::vector<device>(),      ascii::space_type> devices_;
        qi::rule<Iterator, std::vector<std::string>(), ascii::space_type> content_;
        qi::rule<Iterator, getall(),                   ascii::space_type> getall_;
    };

    struct access_level_ : qi::symbols<char, ACCESS_LEVEL>
    {
    	access_level_()
        {
            add
                ("Public" ,    PUBLIC)
                ("Private",    PRIVATE)
                ("Not Shared", NOT_SHARED)
            ;
        }

    };

    extern access_level_ access_level;

    template <typename Iterator>
    struct contmeta_parser : qi::grammar<Iterator, contmeta(), ascii::space_type>
    {
        contmeta_parser() : contmeta_parser::base_type(contmeta_)
        {
            using qi::lit;
            using qi::int_;
            using qi::long_;
            using qi::lexeme;
            using qi::matches;
            using ascii::char_;

            str_        %= lexeme[*(char_ - '<')];

            videos_    %= "<videos>" >>
                            *video_ >>
                          "</videos>";

            video_     %= lit("<video>") >>
                              "<id>"          >> int_         >> "</id>" >>
                              "<title>"       >> str_         >> "</title>" >>
                              "<filesize>"    >> long_        >> "</filesize>" >>
                              "<mimetype>"    >> str_         >> "</mimetype>" >>
                              "<description>" >> str_         >> "</description>" >>
                              "<shared>"      >> access_level >> "</shared>" >>
                          "</video>";

            contmeta_     %= lit("<html>") >>
                            "<multimedia>" >>
                            videos_ >>
                            "</multimedia>" >>
                            "</html>";
        }

        qi::rule<Iterator, std::string(),              ascii::space_type> str_;
        qi::rule<Iterator, video(),                    ascii::space_type> video_;
        qi::rule<Iterator, std::vector<video>(),       ascii::space_type> videos_;
        qi::rule<Iterator, contmeta(),                 ascii::space_type> contmeta_;
    };
}

#endif
