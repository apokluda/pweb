/*
 * client_system_test.cc
 *
 *  Created on: 2012-12-05
 *      Author: sr2chowd
 */
#include "communication/client_socket.h"
#include "plnode/message/p2p/message_get.h"
#include "communication/error_code.h"
#include "monitor/tree/BuildTree.h"
#include "plnode/message/control/peer_init_message.h"
#include "plnode/message/control/peer_start_gen_name_message.h"
#include "plnode/message/control/peer_start_lookup_name_message.h"
#include "plnode/message/control/log_force_message.h"
#include "plnode/message/p2p/message_put.h"
#include <stdlib.h>
#include "plnode/ds/GlobalData.h"
#include "plnode/protocol/plexus/plexus_protocol.h"
#include "plnode/protocol/plexus/golay/GolayCode.h"
#include "plnode/protocol/null_code.h"

Peer* this_peer;
vector <string> log_servers;
ServerSocket* s_socket;
void* listener_thread(void*);

void loadMonitors(const char* monitor_file) {
        FILE* fp = fopen(monitor_file, "r");
        char name[300];
        int N;
        fscanf(fp, "%d", &N);
        while (N--) {
                fscanf(fp, "%s", name);
                log_servers.push_back(name);
        }
        fclose(fp);
}

string getLogServer(int node_index) {
        return log_servers[node_index % log_servers.size()];
}

int getRunSequenceNo(string seq_file_name) {
        int seq_no = 0;
        FILE* seq_file_ptr = fopen(seq_file_name.c_str(), "r+");

        if (seq_file_ptr == NULL) {
                seq_no = 0;
                seq_file_ptr = fopen(seq_file_name.c_str(), "w");
                fprintf(seq_file_ptr, "%d\n", seq_no);
                fclose(seq_file_ptr);
        } else {
                fscanf(seq_file_ptr, "%d", &seq_no);
                seq_no++;

                fseek(seq_file_ptr, 0, SEEK_SET);
                fprintf(seq_file_ptr, "%d", seq_no);
                fclose(seq_file_ptr);
        }
        return seq_no;
}

void send_init_message(BuildTree &tree, int name_count) {
        int n = tree.getTreeSize();
        printf("Tree size = %d\n", n);
        int retCode = 0;

        ClientSocket* c_socket;
        int webserver_port_start = 8080;
        int seq_no = getRunSequenceNo(this_peer->getConfiguration()->getSeqFilePath());
        string log_server_user = this_peer->getConfiguration()->getLogServerUserName();

        loadMonitors(this_peer->getConfiguration()->getMonitorsFilePath().c_str());

	//string peer_name_prefix = "uw";
	char p_name[20];

        for (int i = 0; i < n; i++) {
                puts("----------------------------------------");

                HostAddress address = tree.getHostAddress(i);
                printf("%s %d\n", address.GetHostName().c_str(), address.GetHostPort());

                c_socket = new ClientSocket(address.GetHostName(), address.GetHostPort());
                sockaddr_in s_info = (this_peer->lookup_address(address)).first;
                /*if (s_info.ai_addr != NULL) {
                        puts("not null");
                }*/

                c_socket->setServerInfo(s_info);
                retCode = c_socket->connect_to_server();
                printf("Connected to %s:%d\n", address.GetHostName().c_str(), address.GetHostPort());

                if (retCode < 0) {
                        print_error_message(retCode);
                        continue;
                }

                PeerInitMessage* pInit = new PeerInitMessage();
                LookupTable<OverlayID, HostAddress> rt = tree.getRoutingTablePtr(i);

                //rt.reset_iterator();
                /*while(rt.hasMoreKey())
                 {
                         OverlayID key = rt.getNextKey();
                         HostAddress val;
                         rt.lookup(key, &val);
                         printf("%d %s %d\n", key.GetOverlay_id(), val.GetHostName().c_str(), val.GetHostPort());
                 }*/

                string log_server = getLogServer(i);

                pInit->setDestHost(address.GetHostName().c_str());
                pInit->setDestPort(address.GetHostPort());
                pInit->setDstOid(tree.getOverlayID(i));
                pInit->setNPeers(n);
                pInit->setK(this_peer->getConfiguration()->getK());
                pInit->setAlpha(this_peer->getConfiguration()->getAlpha());
                pInit->setRoutingTable(tree.getRoutingTablePtr(i));
                pInit->setRunSequenceNo(seq_no);
                pInit->setLogServerName(log_server);
                pInit->setLogServerUser(log_server_user);
		//sprintf(p_name, "%s%d", peer_name_prefix.c_str(), i+1);
		pInit->set_peer_name(tree.aliasArray[i]);

                int name_interval = name_count / n;

                pInit->setPublish_name_range_start(name_interval * i);
                pInit->setLookup_name_range_start(name_interval * i);

                if (i < n - 1) {
                        pInit->setPublish_name_range_end(name_interval * (i + 1) - 1);
                        pInit->setLookup_name_range_end(name_interval * (i + 1) - 1);
                } else {
                        pInit->setPublish_name_range_end(name_count - 1);
                        pInit->setLookup_name_range_end(name_count - 1);
                }
                pInit->setWebserverPort(webserver_port_start++);

                char* buffer;
                int buffer_length = 0;
                pInit->message_print_dump();

                puts("Serializing INIT packet");
                buffer = pInit->serialize(&buffer_length);
                printf("Serialized Length = %d bytes\n", buffer_length);

                /*PeerInitMessage* a = new PeerInitMessage();
                a->deserialize(buffer, buffer_length);
                puts("deserialized message");
                a->message_print_dump();*/

                //puts("Sending Init Packet");
                timeval timeout;
                timeout.tv_sec = 5;
                timeout.tv_usec = 500;

                retCode = c_socket->send_data(buffer, buffer_length, &timeout);
                if (retCode < 0)
                        print_error_message(retCode);

                //printf("ret_code = %d\n", retCode);
                //puts("data sent");
                //else printf("Sent %d Bytes", retCode);

                c_socket->close_socket();
                //puts("socket closed");
                //puts("Init Packet Sent");
                puts("----------------------------------------");
                delete c_socket;
                delete pInit;
        }
}

int send_message_to_all_peers(ABSMessage* msg, BuildTree &tree)
{
        int n = tree.getTreeSize();
        int retCode = 0;

        ClientSocket* c_socket;

        for (int i = 0; i < n; i++) {
                puts("----------------------------------------");

                HostAddress address = tree.getHostAddress(i);
                msg->setDstOid(tree.getOverlayID(i));

                printf("%s %d\n", address.GetHostName().c_str(), address.GetHostPort());

                c_socket = new ClientSocket(address.GetHostName(), address.GetHostPort());
                sockaddr_in s_info = (this_peer->lookup_address(address)).first;
                /*if (s_info.ai_addr != NULL) {
                        puts("not null");
                }*/

                c_socket->setServerInfo(s_info);
                retCode = c_socket->connect_to_server();
                printf("Connected to %s:%d\n", address.GetHostName().c_str(), address.GetHostPort());

                if (retCode < 0) {
                        print_error_message(retCode);
                        continue;
                }


                char* buffer;
                int buffer_length = 0;
                msg->message_print_dump();

                puts("Serializing msg packet");
                buffer = msg->serialize(&buffer_length);
                printf("Serialized Length = %d bytes\n", buffer_length);

                /*PeerInitMessage* a = new PeerInitMessage();
                a->deserialize(buffer, buffer_length);
                puts("deserialized message");
                a->message_print_dump();*/

                //puts("Sending Init Packet");
                timeval timeout;
                timeout.tv_sec = 5;
                timeout.tv_usec = 500;

                retCode = c_socket->send_data(buffer, buffer_length, &timeout);
                if (retCode < 0)
                        print_error_message(retCode);

                //printf("ret_code = %d\n", retCode);
                //puts("data sent");
                //else printf("Sent %d Bytes", retCode);

                c_socket->close_socket();
                //puts("socket closed");
                //puts("Init Packet Sent");
                puts("----------------------------------------");
                delete c_socket;
        }

}

int main(int argc, char* argv[]) {
        this_peer = new Peer();
        this_peer->setTimeoutSec(5);
        this_peer->setConfiguration(GlobalData::config_file_name.c_str());
        //this_peer->populate_addressdb();

        PlexusProtocol* plexus = new PlexusProtocol();
        plexus->setContainerPeer(this_peer);
        //ABSCode *iCode = new ReedMuller(2, 4);
        ABSCode *iCode = new GolayCode();
        this_peer->SetiCode(iCode);
        this_peer->setListenPortNumber(this_peer->getConfiguration()->getClientListenPort());

        Configuration config(GlobalData::config_file_name);
        int name_count = config.getNameCount();

        BuildTree tree(config.getNodesFilePath(), iCode);
        tree.execute();
        tree.print();

        pthread_t listener;
        pthread_create(&listener, NULL, listener_thread, NULL);

        send_init_message(tree, name_count);

        char input[1000];
        srand(time(NULL));

        while (true) {
                printf("$");
                gets(input);

                char* command = strtok(input, " ");

                if (strcmp(command, "exit") == 0 || strcmp(command, "bye") == 0
                        || strcmp(command, "quit") == 0) {
                        exit(0);
                } else if (strcmp(command, "put") == 0) {
                        char* p = strtok(NULL, " ");
                        if (p == NULL) {
                                puts("usage: put <name> <hostname:port>");
                                continue;
                        }
                        string name = p;
                        char* address = strtok(NULL, " ");
                        string hostname;
                        int port;

                        if (address == NULL) {
                                hostname = "dummy_host";
                                port = 11111;
                                /*puts("usage: put <name> <hostname:port>");
                                continue;*/
                        } else {
                                int nTokens = 0;

                                p = strtok(address, ":");
                                if (p == NULL) {
                                        puts("usage: put <name> <hostname:port>");
                                        continue;
                                }
                                hostname = p;
                                //puts(hostname.c_str());
                                p = strtok(NULL, ":");
                                if (p == NULL) {
                                        puts("usage: put <name> <hostname:port>");
                                        continue;
                                }

                                port = atoi(p);
                        }

                        HostAddress h_address;
                        h_address.SetHostName(hostname);
                        h_address.SetHostPort(port);

                        HostAddress destination = tree.getHostAddress(rand() % tree.getTreeSize());
                        //printf("destination = %s:%d\n",destination.GetHostName().c_str(), destination.GetHostPort());
                        plexus->put_from_client(name, h_address, destination);
                } else if (strcmp(command, "get") == 0) {
                        char* p = strtok(NULL, " ");
                        if (p == NULL)
                                puts("usage get <name>");

                        else {
                                string name = p;
                                HostAddress destination = tree.getHostAddress(rand() % tree.getTreeSize());
                                plexus->get_from_client(name, destination);
                        }
                } else if (strcmp(command, "init") == 0 || strcmp(command, "INIT") == 0) {
                        send_init_message(tree, name_count);
                } else if (strcmp(command, "clear") == 0 || strcmp(command, "cls") == 0) {
                        for (int line = 0; line < 55; line++)
                                putchar('\n');
                } else if (strcmp(command, "publish") == 0 || strcmp(command, "pub") == 0) {
                        PeerStartGenNameMessage *msg = new PeerStartGenNameMessage();
                        send_message_to_all_peers(msg, tree);
                        delete msg;
                } else if (strcmp(command, "lookup") == 0 || strcmp(command, "lkp") == 0) {
                        PeerStartLookupNameMessage *msg = new PeerStartLookupNameMessage();
                        send_message_to_all_peers(msg, tree);
                        delete msg;
                }
                else if(strcmp(command, "force") == 0 || strcmp(command, "flush") == 0)
				{
                	LogForceMessage *msg = new LogForceMessage();
                	send_message_to_all_peers(msg, tree);
                	delete msg;
				}
                else puts("invalid command");
        }

        pthread_join(listener, NULL);

        /*string name_to_publish = "1378410";
         int hash_name_to_publish = atoi(name_to_publish.c_str());
         HostAddress my_address;
         my_address.SetHostName("localhost");
         my_address.SetHostPort(100);

         MessagePUT* put_msg = new MessagePUT();
         put_msg->SetDeviceName(name_to_publish);
         put_msg->SetHostAddress(my_address);


         //    cout << "pattern = " << hash_name_to_publish << endl;
         //    int id = GlobalData::rm->decode(hash_name_to_publish);
         //    cout << "decoded id = " << id << endl;
         //    OverlayID oID(id);
         put_msg->setDstOid(hash_name_to_publish);

         //int randHost = rand() % n;
         int randHost = 5;
         int ttl = ceil(log2(n)) + 2;//(int) floor(log10(n) / log(2.0)) + 2;

         put_msg->setDestHost(tree.getHostAddress(randHost).GetHostName().c_str());
         put_msg->setDestPort(tree.getHostAddress(randHost).GetHostPort());
         put_msg->setOverlayTtl(ttl);
         //put_msg->message_print_dump();

         ClientSocket cSocket(tree.getHostAddress(randHost).GetHostName(), tree.getHostAddress(randHost).GetHostPort());
         cSocket.connect_to_server();
         int buffer_len = 0;
         char* buffer = put_msg->serialize(&buffer_len);

         for(int k = 0; k < buffer_len; k++) printf(" %x", buffer[k]);
         putchar('\n');
         cSocket.send_data(buffer, buffer_len);
         cSocket.close_socket();




         name_to_publish = "4000";
         hash_name_to_publish = atoi(name_to_publish.c_str());

         put_msg->SetDeviceName(name_to_publish);
         oID.SetOverlay_id(hash_name_to_publish);
         put_msg->setOID(oID);
         delete[] buffer;
         buffer = put_msg->serialize(&buffer_len);
         cSocket.connect_to_server();
         cSocket.send_data(buffer, buffer_len);
         cSocket.close_socket();

         MessageGET* get_msg = new MessageGET();
         get_msg->SetDeviceName(name_to_publish);
         OverlayID id(hash_name_to_publish);
         get_msg->setOID(GlobalData::rm->decode(id.GetOverlay_id()));
         randHost = 2;
         ttl = (int)floor(log10(n) / log(2.0)) + 2;
         get_msg->setDestHost(tree.getHostAddress(randHost).GetHostName().c_str());
         get_msg->setDestPort(tree.getHostAddress(randHost).GetHostPort());
         get_msg->setOverlayTtl(ttl);
         cSocket.setServerHostName(tree.getHostAddress(randHost).GetHostName());
         cSocket.setServerPortNumber(tree.getHostAddress(randHost).GetHostPort());
         cSocket.connect_to_server();
         buffer_len = 0;
         buffer = get_msg->serialize(&buffer_len);
         cSocket.send_data(buffer, buffer_len);
         cSocket.close_socket();

         //plexus->put("name");
         //plexus->get("name");

         /*	ClientSocket* c_socket = new ClientSocket(server_name, server_port);
         int error_code = c_socket->connect_to_server();

         if(error_code < 0)
         {
         print_error_message(error_code);
         exit(1);
         }
         puts("Connected to server");
         MessageGET* msg_get = new MessageGET();
         //set member variables
         int buf_length = 20;
         char* buffer = new char[20];
         int bytes_sent = c_socket->send_data(buffer, buf_length);
         printf("%d Bytes Sent\n", bytes_sent);
         c_socket->close_socket();

         delete msg_get;
         delete c_socket;*/
}

void* listener_thread(void*)
{
	int buffer_length;
	char* buffer;

	s_socket = new ServerSocket(this_peer->getConfiguration()->getClientListenPort());
	s_socket->init_connection();

	fd_set read_connection_fds, connection_pool;
	FD_ZERO(&read_connection_fds);
	FD_ZERO(&connection_pool);
	FD_SET(s_socket->getSocketFd(), &connection_pool);
	int fd_max = s_socket->getSocketFd();

	while (true) {
		read_connection_fds = connection_pool;

		int n_select = select(fd_max + 1, &read_connection_fds, NULL, NULL, NULL);

		if (n_select < 0) {
			puts("Select Error");
			printf("errno = %d\n", errno);

			s_socket->printActiveConnectionList();
			printf("fd_max = %d, socket_fd = %d\n", fd_max, s_socket->getSocketFd());

			for (int con = 0; con <= fd_max; con++) {
				if (FD_ISSET(con, &connection_pool)) {
					int fopts = 0;
					if (fcntl(con, F_GETFL, &fopts) < 0) {
						FD_CLR(con, &connection_pool);
						s_socket->close_connection(con);
						fd_max = s_socket->getMaxConnectionFd();
					}
				}
			}
			this_peer->incrementPut_Dropped();
			//exit(1);
			continue;
		}
		for (int i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &read_connection_fds)) {
				if (i == s_socket->getSocketFd()) {
					int connection_fd = s_socket->accept_connection();

					if (connection_fd < 0) {
						print_error_message(connection_fd);
						exit(1);
					}

					FD_SET(connection_fd, &connection_pool);
					if (connection_fd > fd_max)
						fd_max = connection_fd;
					//puts("new connection");
					//s_socket->printActiveConnectionList();
				} else {
					buffer_length = s_socket->receive_data(i, &buffer);
					printf("[Listening thread]\t Received %d Bytes\n", buffer_length);

					/*for(int j = 0; j < buffer_length; j++) printf("%d ", buffer[j]);
					putchar('\n');*/

					s_socket->close_connection(i);
					FD_CLR(i, &connection_pool);
					fd_max = s_socket->getMaxConnectionFd();

					s_socket->printActiveConnectionList();

					if (buffer_length > 0) {
						char messageType = 0;
						ABSMessage* rcvd_message = NULL;

						memcpy(&messageType, buffer, sizeof (char));
						printf("[Listening thread]\t Message Type: %d\n", messageType);

						switch (messageType) {
							case MSG_PLEXUS_GET_REPLY:
								rcvd_message = new MessageGET_REPLY();
								rcvd_message->deserialize(buffer, buffer_length);
								printf("%s:%d\n", ((MessageGET_REPLY*)rcvd_message)->getHostAddress().GetHostName().c_str()
										, ((MessageGET_REPLY*)rcvd_message)->getHostAddress().GetHostPort());
								//rcvd_message->message_print_dump();
								break;
							default:
								puts("reached default case");
								//exit(1);
								continue;
						}

						delete[] buffer;
					} else if (buffer_length < 0) {
						printf("buffer_length < 0: %d\n", buffer_length);
						continue;
					}
				}
			}
		}
	}
}

