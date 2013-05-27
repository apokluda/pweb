
#include "parser.hpp"

int main()
{
    typedef std::string::const_iterator iterator_type;
    typedef parser::getall_parser<iterator_type> getall_parser;

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
            "<home_agent>"
            "<hostname>cn105.cs.uwaterloo.ca</hostname>"
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
            "<device><owner>Faizul Bari</owner><name>nexus 4</name><port>9876</port><timestamp>1369087766</timestamp><location>Waterloo, ON, Canada</location><description>a;lhjsd;laskd;</description></device><device><owner>Faizul Bari</owner><name>nexus 4</name><port>9876</port><timestamp>1369087766</timestamp><location>Waterloo, ON, Canada</location><description>a;lhjsd;laskd;</description></device><device><owner>Faizul Bari</owner><name>asdasd</name><port>asda</port><timestamp>1369087107</timestamp><location>Waterloo, ON, Canada</location><description>asd</description></device><device><owner>Faizul Bari</owner><name>asdasd</name><port>asda</port><timestamp>1369087107</timestamp><location>Waterloo, ON, Canada</location><description>asd</description></device><device><owner>Faizul Bari</owner><name>asdasd34234</name><port>asdasd</port><timestamp>1369142462</timestamp><location>Waterloo, ON, Canada</location><description>asdasd</description></device><device><owner>Faizul Bari</owner><name>asdasd34234</name><port>asdasd</port><timestamp>1369142462</timestamp><location>Waterloo, ON, Canada</location><description>asdasd</description></device></devices></getall>";

    getall_parser const g; // Our grammar
    std::string str( all1 );

    parser::getall gall;
    std::string::const_iterator iter = str.begin();
    std::string::const_iterator end = str.end();
    using boost::spirit::ascii::space;
    bool r = phrase_parse(iter, end, g, space, gall);

    if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "\n-------------------------\n";

        std::cout << "Home Agents:\n";
        BOOST_FOREACH(parser::homeagent const& ha, gall.homeagents)
        {
            std::cout << ha.hostname << '\n';
        }
        std::cout << "Devices:\n";
        BOOST_FOREACH(parser::device const& dev, gall.devices)
        {
            std::cout << dev.name << ", " << dev.timestamp << '\n';
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
