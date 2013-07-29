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

#include "stdhdr.hpp"
#include "klink/plnode/message/control/peer_initiate_get.h"
#include "klink/plnode/message/p2p/message_get_reply.h"

using namespace boost::asio;
using namespace boost::asio::ip;

typedef std::stringstream sstream;

const int max_length = 4096;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

void session(socket_ptr sock, bool fail, bool verbose)
{
    try
    {
        char data[max_length];

        boost::system::error_code error;
        size_t const length( read( *sock, boost::asio::buffer(data), error) );
        if (error == boost::asio::error::eof)
        {
            PeerInitiateGET getmsg;
            getmsg.deserialize(data, length);
            if (verbose) getmsg.message_print_dump();
            // Close connection
            io_service& io_service = sock->get_io_service();
            sock.reset();

            //sleep(15);

            char* replybuf = 0;
            int buflen = 0;
            if ( !fail )
            {
                MessageGET_REPLY replymsg("alex.laptop.uw", 9999, "dnsgw.pwebproject.net", 8888, OverlayID(),
                        OverlayID(), 0, OverlayID(), HostAddress("6.6.6.6", 7777), getmsg.getDeviceName());
                //replymsg.setOriginSeqNo(getmsg.getSequenceNo());
                replymsg.setSequenceNo(getmsg.getSequenceNo());
                replybuf = replymsg.serialize(&buflen);
            }
            else
            {
                MessageGET_REPLY replymsg("alex.laptop.uw", 9999, "dnsgw.pwebproject.net", 8888, OverlayID(),
                        OverlayID(), 1, OverlayID(), HostAddress(), getmsg.getDeviceName());
                replymsg.setOriginSeqNo(getmsg.getSequenceNo());
                replybuf = replymsg.serialize(&buflen);
            }

            tcp::resolver resolver(io_service);
            sstream ss;
            ss << getmsg.getSourcePort();
            tcp::resolver::query query(tcp::v4(), getmsg.getSourceHost(), ss.str());
            tcp::resolver::iterator iterator = resolver.resolve(query);

            tcp::socket s(io_service);
            boost::asio::connect(s, iterator);

            boost::asio::write(s, boost::asio::buffer(replybuf, buflen));
            delete [] replybuf;
        }
        else if (error)
        {
            std::cerr << "Error reading GET message: " << error.message() << std::endl;
            throw boost::system::system_error(error); // Some other error.
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

void server( io_service& io_service, short port, bool fail, bool verbose )
{
    tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
    for (;;)
    {
        socket_ptr sock(new tcp::socket(io_service));
        a.accept(*sock);
        boost::thread t(boost::bind(session, sock, fail, verbose));
    }
}

int main(int argc, char const* argv[])
{
    if ( argc < 2 )
    {
        std::cerr << "Usage: dnsgw_test <port> [-f][-v]\n"
                "-f    reply with failure\n"
                "-v    verbose output";
        return 1;
    }

    bool fail = false;
    bool verbose = false;

    for (--argc; argc > 1; --argc)
    {
        if (strcmp(argv[argc], "-f")) fail = true;
        else if (strcmp(argv[argc], "-v")) verbose = true;
    }

    try
    {
        io_service io_service;

        using namespace std; // for atoi
        server( io_service, atoi(argv[1]), fail, verbose );
    }
    catch ( std::exception const& e )
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
