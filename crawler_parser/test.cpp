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

#include "parser.hpp"

parser::access_level_ parser::access_level;

int main()
{
    typedef std::string::const_iterator iterator_type;
    typedef parser::getall_parser<iterator_type> getall_parser;
    typedef parser::contmeta_parser<iterator_type> contmeta_parser;

    char const* all1 =
            "<getall>\
            <name>mypc0</name>\
            <neighbours>\
                <home agent>\
                    <hostname>localhost</hostname>\
                    <port>20005</port>\
                </home agent>\
            </neighbours>\
            <devices>\
                <device>\
                    <owner>Faizul Bari</owner>\
                    <name>nexus2</name>\
                    <home>mypc0</home>\
                    <port>12345</port>\
                    <timestamp>1368462463</timestamp>\
                    <location>Waterloo, ON, Canada</location>\
                    <description>my first android phone...</description>\
                </device>\
                <device>\
                    <owner>Faizul Bari</owner>\
                    <name>asdasd</name>\
                    <home>mypc0</home>\
                    <port>12345</port>\
                    <timestamp>1369087107</timestamp>\
                    <location>Waterloo, ON, Canada</location>\
                    <description>asd</description>\
                </device>\
                <device>\
                    <owner>Faizul Bari</owner>\
                    <name>asdasd34234</name>\
                    <home>mypc0</home>\
                    <port>12345</port>\
                    <timestamp>1369142462</timestamp>\
                    <location>Waterloo, ON, Canada</location>\
                    <description>asdasd</description>\
                </device>\
            </devices>\
            </getall>";

    char const* all2 =
            "<getall>\
            <name>mypc0</name>\
            <neighbours>\
            </neighbours>\
            <devices>\
            </devices>\
            </getall>";

    char const* all3 =
            "<getall>\
            <name>mypc0</name>\
            <neighbours>\
                <home agent>\
                    <hostname>localhost</hostname>\
                    <port>20005</port>\
                </home agent>\
            </neighbours>\
            <devices>\
            </devices>\
            </getall>";

    char const* all4 =
            "<getall>\
            <name>mypc0</name>\
            <neighbours>\
                <home agent>\
                    <hostname>localhost</hostname>\
                    <port>20005</port>\
                </home agent>\
            </neighbours>\
            <devices>\
                <device>\
                    <owner>Faizul Bari</owner>\
                    <name>nexus2</name>\
                    <home>mypc0</home>\
                    <port>12345</port>\
                    <timestamp>1368462463</timestamp>\
                    <location>Waterloo, ON, Canada</location>\
                    <description>my first android phone...</description>\
                </device>\
            </devices>\
            </getall>";

    char const* all5 = "<getall>"
            "<name>uw02</name>"
            "<neighbours>"
            "<home_agent>"
            "<hostname>cn101.cs.uwaterloo.ca</hostname>"
            "<port>20000</port>"
            "</home_agent>"
            "<home_agent>"
            "<hostname>cn104.cs.uwaterloo.ca</hostname>"
            "<port>20000</port>"
            "</home_agent>"
            "</neighbours>"
            "<devices>"
            "<device>"
            "<owner>Faizul Bari</owner>"
            "<name>nexus2</name>"
            "<port>3128</port>"
            "<timestamp>1368462463</timestamp>"
            "<location>Waterloo, ON, Canada</location>"
            "<description>my first android phone...</description>"
            "</device>"
            "<device>"
            "<owner>Faizul Bari</owner>"
            "<name>nexus 4</name>"
            "<port>9876</port>"
            "<timestamp>1369087766</timestamp>"
            "<location>Waterloo, ON, Canada</location>"
            "<description>a;lhjsd;laskd;</description>"
            "</device>"
            "</devices>"
            "</getall>";

    char const* all6 =
            "<getall>"
            "<name>uw01</name>"
            "<neighbours>"
            "<home_agent><hostname>cn102.cs.uwaterloo.ca</hostname><port>20000</port></home_agent>"
            "<home_agent><hostname>cn103.cs.uwaterloo.ca</hostname><port>20000</port></home_agent>"
            "</neighbours>"
            "<devices>"
            "<device>"
            "<owner>faiz</owner>"
            "<name>nexus.faiz</name>"
            "<port>1111</port>"
            "<timestamp>1373903603</timestamp>"
            "<location>bd</location>"
            "<description>111</description>"
            "<content_meta>1</content_meta>"
            "</device>"
            "</devices>"
            "<content updates>nexus.faiz</content updates>"
            "</getall>";

    char const* all7 =
    		"<getall>"
    		"	<name>ct</name>"
    		"	<neighbours>"
    		"		<home_agent>"
    		"			<hostname>mg</hostname>"
    		"			<port>53</port>"
    		"		</home_agent>"
    		"		<home_agent>"
    		"			<hostname>ct</hostname>"
    		"			<port>53</port>"
    		"		</home_agent>"
    		"	</neighbours>"
    		"	<devices>"
    		"	</devices>"
    		"	<content updates>"
    		"	</content updates>"
    		"</getall>";

    char const* all8 =
    		"<getall>"
    		"	<name>mg</name>"
    		"	<neighbours>"
    		"		<home agent>"
    		"			<hostname>mg</hostname>"
    		"			<port>53</port>"
    		"		</home agent>"
    		"		<home agent>"
    		"			<hostname>ct</hostname>"
    		"			<port>53</port>"
    		"		</home agent>"
    		"	</neighbours>"
    		"	<devices>"
    		"		<device>"
    		"			<owner>faiz</owner>"
    		"			<name>asasd.faiz</name>"
    		"			<port>1234</port>"
    		"			<timestamp>1392753891</timestamp>"
    		"			<location></location>"
    		"			<description></description>"
    		"		</device>"
    		"		<device>"
    		"			<owner>faiz</owner>"
    		"			<name>n5.faiz</name>"
    		"			<port>1234</port>"
    		"			<timestamp>1392753506</timestamp>"
    		"			<location></location>"
    		"			<description></description>"
    		"		</device>"
    		"		<device>"
    		"			<owner>faiz</owner>"
    		"			<name>pc.faiz</name>"
    		"			<port>1234</port>"
    		"			<timestamp>1392753741</timestamp>"
    		"			<location></location>"
    		"			<description>shihabs pc</description>"
    		"		</device>"
    		"	</devices>"
    		"	<content updates>asasd.faiz,n5.faiz,pc.faiz</content updates>"
    		"</getall>";

    char const* all9 =
    		"<getall>"
    		"	<name>mg</name>"
    		"	<neighbours>"
    		"		<home agent>"
    		"			<hostname>mg</hostname>"
    		"			<port>53</port>"
    		"		</home agent>"
    		"		<home agent>"
    		"			<hostname>ct</hostname>"
    		"			<port>53</port>"
    		"		</home agent>"
    		"	</neighbours>"
    		"	<devices>"
    		"		<device>"
    		"			<owner>faiz</owner>"
    		"			<name>asasd.faiz</name>"
    		"			<port>3256</port>"
    		"			<type>phone</type>"
    		"			<timestamp>1392753891</timestamp>"
    		"			<location></location>"
    		"			<description></description>"
    		"		</device>"
    		"		<device>"
    		"			<owner>faiz</owner>"
    		"			<name>n5.faiz</name>"
    		"			<port>253</port>"
    		"			<type>phone</type>"
    		"			<timestamp>1392753506</timestamp>"
    		"			<location></location>"
    		"			<description></description>"
    		"		</device>"
    		"		<device>"
    		"			<owner>faiz</owner>"
    		"			<name>pc.faiz</name>"
    		"			<port>5644</port>"
    		"			<type>pc</type>"
    		"			<timestamp>1392753741</timestamp>"
    		"			<location></location>"
    		"			<description>shihabs pc</description>"
    		"		</device>"
    		"		<device>"
    		"			<owner>faiz</owner>"
    		"			<name>pv3.faiz</name>"
    		"			<port>123</port>"
    		"			<type>none</type>"
    		"			<timestamp>1392826476</timestamp>"
    		"			<location></location>"
    		"			<description></description>"
    		"		</device>"
    		"	</devices>"
    		"	<content updates>asasd.faiz,n5.faiz,pc.faiz,pv3.faiz</content updates>"
    		"</getall>";


    char const* meta1 =
            "<multimedia>"
            " <videos>"
            "  <video>"
            "   <id>1</id>"
            "   <title>Ed Sheeran - The A Team Lyrics (On Screen).mp4</title>"
            "   <filesize>40258480</filesize>"
            "   <mimetype>video/mp4</mimetype>"
            "   <description>nice melodic song</description>"
            "   <shared>Public</shared>"
            "  </video>"
            "  <video>"
            "   <id>2</id>"
            "   <title>Fireworks.mp4</title>"
            "   <filesize>44378752</filesize>"
            "   <mimetype>video/mp4</mimetype>"
            "   <description>fireworks event on the 1st of july</description>"
            "   <shared>Public</shared>"
            "  </video>"
            " </videos>"
            "</multimedia>";

    char const* meta2 =
    		"<multimedia>"
    		"<videos>"
    		" <video>"
    		"  <id>1</id>"
    		"  <title>Ed Sheeran - The A Team Lyrics (On Screen)</title>"
    		"  <filesize>40258480</filesize>"
    		"  <mimetype>video/mp4</mimetype>"
    		"  <description>nice song</description>"
    		"  <shared>Public</shared>"
    		" </video>"
    		" <video>"
    		"  <id>2</id>"
    		"  <title>Mirror's Edge Theme Song - Still Alive</title>"
    		"  <filesize>87816808</filesize>"
    		"  <mimetype>video/mp4</mimetype>"
    		"  <description>great</description>"
    		"  <shared>Private</shared>"
    		" </video>"
    		" <video>"
    		"  <id>3</id>"
    		"  <title>Dragonette - Easy (Fabian Remix)</title>"
    		"  <filesize>31674093</filesize>"
    		"  <mimetype>video/mp4</mimetype>"
    		"  <description>great</description>"
    		"  <shared>Public</shared>"
    		" </video>"
    		"</videos>"
    		"</multimedia>";

    getall_parser const g; // Our grammar
    std::string str( all9 );

    parser::getall getall;
    std::string::const_iterator iter = str.begin();
    std::string::const_iterator end = str.end();
    using boost::spirit::ascii::space;
    bool r = phrase_parse(iter, end, g, space, getall);

    if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "\n-------------------------\n";

        //std::cout << "Home Agents:\n";
        //BOOST_FOREACH(parser::homeagent const& ha, gall.homeagents)
        //{
        //    std::cout << ha.hostname << '\n';
        //}
        //std::cout << "Devices:\n";
        //BOOST_FOREACH(parser::device const& dev, gall.devices)
        //{
        //    std::cout << dev.name << ", " << dev.timestamp << '\n';
        //}
        std::cout << "Updates:\n";
        BOOST_FOREACH(std::string const& upd, getall.updates)
        {
            std::cout << upd << '\n';
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
