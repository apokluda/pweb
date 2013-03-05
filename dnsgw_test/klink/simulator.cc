/*
 * simulator.cc
 *
 *  Created on: 2013-01-15
 *      Author: sr2chowd
 */
#include "communication/error_code.h"

#include "communication/server_socket.h"
#include "communication/client_socket.h"

#include "plnode/protocol/protocol.h"
#include "plnode/protocol/plexus/plexus_simulation_protocol.h"
#include "plnode/protocol/plexus/plexus_sim_message_processor.h"
#include "plnode/protocol/code.h"

#include "plnode/message/message.h"
#include "plnode/message/control/peer_init_message.h"
#include "plnode/protocol/plexus/plexus_message_processor.h"
#include "plnode/message/control/peer_initiate_get.h"
#include "plnode/message/control/peer_initiate_put.h"
#include "plnode/message/control/peer_start_message.h"
#include "plnode/message/control/peer_change_status_message.h"
#include "plnode/message/p2p/message_put_reply.h"
#include "plnode/message/control/peer_start_gen_name_message.h"
#include "plnode/message/control/peer_start_lookup_name_message.h"
#include "plnode/message/control/log_force_message.h"

#include "plnode/ds/thread_parameter.h"

#include "webinterface/mongoose.h"
#include "plnode/protocol/plexus/golay/GolayCode.h"
#include "plnode/protocol/null_code.h"

#include "plnode/message/p2p/message_cache_me.h"

#include <cstdlib>
#include <cstdio>
#include <pthread.h>
#include <fcntl.h>
#include <vector>
#include <string.h>

using namespace std;

#define N_PEERS 50
#define MAX_LISTENER_THREAD 1
#define MAX_CONTROL_THREAD 1
#define MAX_PROCESSOR_THREAD 1
#define MAX_FORWARDING_THREAD 8
#define MAX_LOGGING_THREAD 1
//Globals
Peer *peer_arr[N_PEERS];
LookupTable<OverlayID, int> peer_db;
ABSProtocol *protocol_arr[N_PEERS];
ABSCode *iCode;
Configuration* configuration;
string this_host_name;
bool all_init_received = false;

int listen_port_number;
int fd_max;
fd_set connection_pool;
fd_set read_connection_fds;

//queue<ABSMessage*> received_before_init_queue;

ServerSocket* s_socket = NULL;

struct mg_context *ctx;

void system_init();
void cleanup();
void initListenSocket(const char* hosts_file);

void *listener_thread(void*);
void *forwarding_thread(void*);
void *processing_thread(void*);
void *controlling_thread(void*);
//void *web_thread(void*);
void *logging_thread(void*);
//void *storage_stat_thread(void*);
//void *pending_message_process_thread(void*);

int main(int argc, char* argv[])
{
	system_init();

	pthread_t listener, controller, web, logger, storage_stat;
	pthread_t forwarder[MAX_FORWARDING_THREAD], processor[MAX_PROCESSOR_THREAD];
	pthread_t pending_msg_processor;

	ThreadParameter f_param[MAX_FORWARDING_THREAD], p_param[MAX_PROCESSOR_THREAD];

	for (int i = 0; i < MAX_FORWARDING_THREAD; i++)
		f_param[i] = ThreadParameter(i);

	for (int i = 0; i < MAX_PROCESSOR_THREAD; i++)
		p_param[i] = ThreadParameter(i);

	pthread_create(&listener, NULL, listener_thread, NULL);

	for (int i = 0; i < MAX_FORWARDING_THREAD; i++)
	{
		pthread_create(&forwarder[i], NULL, forwarding_thread, &f_param[i]);
	}

	for (int i = 0; i < MAX_PROCESSOR_THREAD; i++)
	{
		pthread_create(&processor[i], NULL, processing_thread, &p_param);
	}

	pthread_create(&logger, NULL, logging_thread, NULL);
	pthread_create(&controller, NULL, controlling_thread, NULL);
	//pthread_create(&web, NULL, web_thread, NULL);
	//pthread_create(&storage_stat, NULL, storage_stat_thread, NULL);
	//pthread_create(&pending_msg_processor, NULL, pending_message_process_thread,NULL);

	pthread_join(listener, NULL);

	for (int i = 0; i < MAX_FORWARDING_THREAD; i++)
		pthread_join(forwarder[i], NULL);
	for (int i = 0; i < MAX_PROCESSOR_THREAD; i++)
		pthread_join(processor[i], NULL);
	pthread_join(logger, NULL);
	pthread_join(controller, NULL);
	//pthread_join(storage_stat, NULL);

	//pthread_join(web, NULL);

	cleanup();
}

void system_init()
{
	int error_code;
	puts("Initializing the System");
	char name[200];

	gethostname(name, sizeof(name));
	this_host_name = name;
	configuration = new Configuration(GlobalData::config_file_name.c_str());

	printf("hostname = %s\n", this_host_name.c_str());

	/* creating the server socket for accepting connection from other machines */
	initListenSocket(configuration->getNodesFilePath().c_str());
	if (s_socket == NULL)
	{
		printf("Socket create error");
		exit(1);
	}

	/* initializing the connection sets */
	FD_ZERO(&connection_pool);
	FD_ZERO(&read_connection_fds);

	for (int i = 0; i < N_PEERS; i++)
	{
		/* creating the peer containers */
		peer_arr[i] = new Peer(configuration);
		peer_arr[i]->setHostName("localhost");
		peer_arr[i]->setListenPortNumber(listen_port_number);

		/* creating the message processor for plexus */
		PlexusSimMessageProcessor* msg_processor = new PlexusSimMessageProcessor();

		/* creating the plexus protocol object */
		protocol_arr[i] = new PlexusSimulationProtocol(peer_arr[i], msg_processor);

		/* setting the protocol of the peer */
		peer_arr[i]->SetiCode(new NullCode(30));
		peer_arr[i]->setProtocol(protocol_arr[i]);
	}

	((PlexusSimulationProtocol*) protocol_arr[0])->initLocks();

	FD_SET(s_socket->getSocketFd(), &connection_pool);
	fd_max = s_socket->getSocketFd();
}

void cleanup()
{
	/*delete this_peer;
	 this_peer = NULL;
	 plexus = NULL;
	 s_socket->print_socket_info();
	 mg_stop(ctx);*/
}

void *forwarding_thread(void* args)
{
	ThreadParameter t_param = *((ThreadParameter*) args);
	printf("Starting forwarding thread %d\n", t_param.getThreadId());

	char* buffer = NULL;
	int buffer_length;
	ABSMessage* message = NULL;

	while (true)
	{
		message = ((PlexusSimulationProtocol*) protocol_arr[0])->getOutgoingQueueFront();
		message->incrementOverlayHops();

		printf("[Forwarding Thread %d:]\tForwarding a %d message to %s:%d\n", t_param.getThreadId(), message->getMessageType(),
				message->getDestHost().c_str(), message->getDestPort());

		int retry = 0, error_code = 0;
		int peer_index;
		peer_db.lookup(message->getDstOid(), &peer_index);

		while (retry < peer_arr[peer_index]->getNRetry())
		{
			error_code = protocol_arr[peer_index]->send_message(message);
			if (error_code < 0)
				retry++;
			else
				break;
		}

		if (error_code >= 0)
		{
			if (message->getMessageType() == MSG_PLEXUS_GET)
				peer_arr[peer_index]->incrementGet_forwarded();
			else if (message->getMessageType() == MSG_PLEXUS_PUT)
				peer_arr[peer_index]->incrementPut_forwarded();
		}
		else
		{
			if (message->getMessageType() == MSG_PLEXUS_GET)
				peer_arr[peer_index]->incrementGet_Dropped();
			else if (message->getMessageType() == MSG_PLEXUS_PUT)
				peer_arr[peer_index]->incrementPut_Dropped();
		}

		delete message;
	}
}

void *listener_thread(void* args)
{
	puts("Starting Listener Thread");

	int buffer_length;
	char* buffer;
	int last_init_received = -1;

	while (true)
	{
		read_connection_fds = connection_pool;

		int n_select = select(fd_max + 1, &read_connection_fds, NULL, NULL, NULL);

		if (n_select < 0)
		{
			puts("Select Error");
			printf("errno = %d\n", errno);

			s_socket->printActiveConnectionList();
			printf("fd_max = %d, socket_fd = %d\n", fd_max, s_socket->getSocketFd());

			for (int con = 0; con <= fd_max; con++)
			{
				if (FD_ISSET(con, &connection_pool))
				{
					int fopts = 0;
					if (fcntl(con, F_GETFL, &fopts) < 0)
					{
						FD_CLR(con, &connection_pool);
						s_socket->close_connection(con);
						fd_max = s_socket->getMaxConnectionFd();
					}
				}
			}
			//this_peer->incrementPut_Dropped();
			//exit(1);
			continue;
		}

		for (int i = 0; i <= fd_max; i++)
		{
			if (FD_ISSET(i, &read_connection_fds))
			{
				if (i == s_socket->getSocketFd())
				{
					int connection_fd = s_socket->accept_connection();
					if (connection_fd < 0)
					{
						print_error_message(connection_fd);
						continue;
//						exit(1);
					}
					FD_SET(connection_fd, &connection_pool);
					if (connection_fd > fd_max)
						fd_max = connection_fd;
					//puts("new connection");
					//s_socket->printActiveConnectionList();
				}
				else
				{
					buffer_length = s_socket->receive_data(i, &buffer);
					printf("[Listening thread]\t Received %d Bytes\n", buffer_length);

					/*for(int j = 0; j < buffer_length; j++) printf("%d ", buffer[j]);
					 putchar('\n');*/

					s_socket->close_connection(i);
					FD_CLR(i, &connection_pool);
					fd_max = s_socket->getMaxConnectionFd();

					//s_socket->printActiveConnectionList();

					if (buffer_length > 0)
					{
						char messageType = 0;
						ABSMessage* rcvd_message = NULL;

						memcpy(&messageType, buffer, sizeof(char));
						printf("[Listening thread]\t Message Type: %d\n", messageType);
						int peer_index;

						switch (messageType)
						{
							case MSG_PEER_INIT:
								rcvd_message = new PeerInitMessage();
								rcvd_message->deserialize(buffer, buffer_length);
								rcvd_message->message_print_dump();
								++last_init_received;
								peer_db.add(rcvd_message->getDstOid(), last_init_received);
								if (last_init_received == N_PEERS - 1)
									all_init_received = true;
								break;

							case MSG_PLEXUS_GET:
								rcvd_message = new MessageGET();
								rcvd_message->deserialize(buffer, buffer_length);
								peer_db.lookup(rcvd_message->getDstOid(), &peer_index);
								peer_arr[i]->incrementGet_received();
								//rcvd_message->message_print_dump();
								//rcvd_message->setDstOid(OverlayID(atoi(((MessageGET*)rcvd_message)->GetDeviceName().c_str()), iCode));
								break;

							case MSG_PLEXUS_GET_REPLY:
								rcvd_message = new MessageGET_REPLY();
								rcvd_message->deserialize(buffer, buffer_length);
								//rcvd_message->message_print_dump();
								break;

							case MSG_PLEXUS_PUT:
								rcvd_message = new MessagePUT();
								rcvd_message->deserialize(buffer, buffer_length);
								peer_db.lookup(rcvd_message->getDstOid(), &peer_index);
								peer_arr[i]->incrementGet_received();
								//rcvd_message->message_print_dump();
								//rcvd_message->setDstOid(OverlayID(atoi(((MessagePUT*)rcvd_message)->GetDeviceName().c_str()), iCode));
								break;

							case MSG_PLEXUS_PUT_REPLY:
								rcvd_message = new MessagePUT_REPLY();
								rcvd_message->deserialize(buffer, buffer_length);
								break;

							case MSG_PEER_INITIATE_GET:
								//this_peer->incrementGet_received();
								rcvd_message = new PeerInitiateGET();
								rcvd_message->deserialize(buffer, buffer_length);
								//rcvd_message->message_print_dump();
								break;
							case MSG_PEER_INITIATE_PUT:
								//this_peer->incrementPut_received();
								rcvd_message = new PeerInitiatePUT();
								rcvd_message->deserialize(buffer, buffer_length);
								break;
							case MSG_PEER_START:
								rcvd_message = new PeerStartMessage();
								rcvd_message->deserialize(buffer, buffer_length);
								break;
							case MSG_PEER_CHANGE_STATUS:
								rcvd_message = new PeerChangeStatusMessage();
								rcvd_message->deserialize(buffer, buffer_length);
								break;
							case MSG_START_GENERATE_NAME:
								rcvd_message = new PeerStartGenNameMessage();
								rcvd_message->deserialize(buffer, buffer_length);
								break;
							case MSG_START_LOOKUP_NAME:
								rcvd_message = new PeerStartLookupNameMessage();
								rcvd_message->deserialize(buffer, buffer_length);
								break;
							case MSG_DYN_CHANGE_STATUS:
								rcvd_message = new PeerDynChangeStatusMessage();
								rcvd_message->deserialize(buffer, buffer_length);
								break;
							case MSG_PEER_FORCE_LOG:
								rcvd_message = new LogForceMessage();
								rcvd_message->deserialize(buffer, buffer_length);
								break;
							case MSG_CACHE_ME:
								rcvd_message = new MessageCacheMe();
								rcvd_message->deserialize(buffer, buffer_length);
								break;
							default:
								puts("reached default case");
								//exit(1);
								continue;
						}

						if (rcvd_message != NULL)
						{
							int peer_index;
							peer_db.lookup(rcvd_message->getDstOid(), &peer_index);
							((PlexusSimulationProtocol*) protocol_arr[0])->addToIncomingQueue(rcvd_message);
//							((PlexusProtocol*)peer_arr[peer_index]->getProtocol())->addToIncomingQueue(rcvd_message);

							/*if (this_peer->IsInitRcvd() || rcvd_message->getMessageType() == MSG_PEER_INIT)
							 {
							 puts("INIT already received");
							 ((PlexusProtocol*) plexus)->addToIncomingQueue(
							 rcvd_message);
							 printf(
							 "[Listening thread]\t Added a %d message to the incoming queue\n",
							 rcvd_message->getMessageType());
							 } else
							 {
							 puts(
							 "Received a message before receiving INIT message");
							 received_before_init_queue.push(rcvd_message);
							 }*/

						}
						else
						{
							puts("received message is null");
							//exit(1);
							continue;
						}

						delete[] buffer;
					}
					else if (buffer_length < 0)
					{
						printf("buffer_length < 0: %d\n", buffer_length);
						//exit(1);
						continue;
					}
				}
			}
		}
	}
}

void *processing_thread(void* args)
{
	ThreadParameter t_param = *((ThreadParameter*) args);
	printf("Starting processing thread %d\n", t_param.getThreadId());

	ABSMessage* message = NULL;
	while (true)
	{
		message = ((PlexusSimulationProtocol*) protocol_arr[0])->getIncomingQueueFront();

		printf("[Processing Thread %d]\tpulled a %d type message from the incoming queue\n", t_param.getThreadId(), message->getMessageType());

		int peer_index = -1;
		peer_db.lookup(message->getDstOid(), &peer_index);
		printf("peer_index = %d\n", peer_index);
		bool forward = peer_arr[peer_index]->getProtocol()->getMessageProcessor()->processMessage(message);

		if (forward)
		{
			printf("[Processing Thread %d]\tpushed a %d type message for forwarding\n", t_param.getThreadId(), message->getMessageType());

			message->getDstOid().printBits();
			/*printf("[Processing Thread %d:]\thost: %s:%d TTL: %d Hops: %d\n", t_param.getThreadId(),
			 message->getDestHost().c_str(), message->getDestPort(),
			 message->getOverlayTtl(), message->getOverlayHops());*/
			//                        if(message->getMessageType() == MSG_PLEXUS_PUT){
			//                                message->setDstOid(OverlayID(atoi(((MessagePUT*)message)->GetDeviceName().c_str()), iCode));
			//                        }
			//                        if(message->getMessageType() == MSG_PLEXUS_GET){
			//                                message->setDstOid(OverlayID(atoi(((MessageGET*)message)->GetDeviceName().c_str()), iCode));
			//                        }
			//((PlexusProtocol*) plexus)->addToOutgoingQueue(message);
			((PlexusSimulationProtocol*) protocol_arr[0])->addToOutgoingQueue(message);
		}
	}
}

void *controlling_thread(void* args)
{
	puts("Starting a controlling thread");

	//sleep(20);
	//vector <int> putId, getId, idsp, idsg;

	bool publish_name_done = false, lookup_name_done = false;
	char buffer[33];

	all_init_received = false;
	bool all_gen_name_rcvd = true;
	bool all_lkp_name_rcvd = true;

	puts("waiting for everyone to receive init");

	while (!all_init_received)
	{
		all_init_received = true;
		for (int i = 0; i < N_PEERS; i++)
		{
			if (!peer_arr[i]->IsInitRcvd())
			{
				all_init_received = false;
				break;
			}
		}

	}

	puts("Everyone received init\nLoading data");
	vector<unsigned int> names;
	unsigned int name;
	char filename[100];
	strcpy(filename, peer_arr[0]->getConfiguration()->getInputFilePath().c_str());
	FILE* name_file = fopen(filename, "r");
	while (fscanf(name_file, "%u", &name) != EOF)
		names.push_back(name);
	fclose(name_file);
	puts("Data Loading complete\nWatiting "
			"for everyone to receive gen_name");
	all_gen_name_rcvd = false;
	while (!all_gen_name_rcvd)
	{
		all_gen_name_rcvd = true;
		int cnt = 0;
		for (int i = 0; i < N_PEERS; i++)
		{
			if (!peer_arr[i]->IsStart_gen_name_rcvd())
			{
				all_gen_name_rcvd = false;
			}
			else cnt++;
		}
		//printf("%d\n", cnt);
	}
	puts("Everyone received gen_name\nPublising name");
	sleep(5);

	for (int k = 0; k < N_PEERS; k++)
	{
		Peer* this_peer = peer_arr[k];
		printf("Peer index, oid = %d %d\n", k, this_peer->getOverlayID().GetOverlay_id());
		//publish names
		printf("[Controlling Thread]\tPublishing name in range %d %d\n", this_peer->getPublish_name_range_start(),
				this_peer->getPublish_name_range_end());

		for (int i = this_peer->getPublish_name_range_start(); i <= this_peer->getPublish_name_range_end(); i++)
		{
			HostAddress ha("dummyhost", i);
			//itoa(i, buffer, 10);
			sprintf(buffer, "%u", names[i]);
			printf("[Controlling Thread]\tPublishing name: %d\n", i);

			//putId.push_back(OverlayID(i, iCode).GetOverlay_id());
			//idsp.push_back(i);

			this_peer->getProtocol()->put(string(buffer), ha);
			if (i % 3 == 0)
				pthread_yield();
		}
		publish_name_done = true;
	}
	puts("All names published\nWaiting to receive lkp");
	all_lkp_name_rcvd = false;
	while (!all_lkp_name_rcvd)
	{
		all_lkp_name_rcvd = true;
		for (int i = 0; i < N_PEERS; i++)
		{
			if (!peer_arr[i]->IsStart_lookup__name_rcvd())
			{
				all_lkp_name_rcvd = false;
				break;
			}
		}
	}
	puts("Everyone received lkp\nLooking up names");
	for (int k = 0; k < N_PEERS; k++)
	{
		Peer* this_peer = peer_arr[k];
		printf("Peer index, oid = %d %d\n", k, this_peer->getOverlayID().GetOverlay_id());
		//lookup names
		printf("[Controlling Thread:]\tLooking up name ...\n");
		//usleep(8000000);
		//sleep(60);
		for (int i = this_peer->getLookup_name_range_start(); i <= this_peer->getLookup_name_range_end(); i++)
		{
			HostAddress ha("dummyhost", i);
			//itoa(i, buffer, 10);
			sprintf(buffer, "%u", names[i]);
			printf("[Controlling Thread:]\tLooking up name: %d\n", i);

			//getId.push_back(OverlayID(i, iCode).GetOverlay_id());
			//idsg.push_back(i);

			this_peer->getProtocol()->get(string(buffer));
			if (i % 3 == 0)
				pthread_yield();
		}
		lookup_name_done = true;
	}

	pthread_yield();
}

void *logging_thread(void*)
{
	puts("Starting a logging thread");
	LogEntry *entry;
	while (true)
	{
		/*if (!this_peer->IsInitRcvd())
		 {
		 //puts("[Logging Thread]\tnot init received");
		 continue;
		 }*/
		entry = ((PlexusSimulationProtocol*) protocol_arr[0])->getLoggingQueueFront();

		if (entry->getType() == ALL_LOGS)
		{
			((PlexusSimulationProtocol*) protocol_arr[0])->flushAllLog();
			continue;
		}

		Log* log = ((PlexusSimulationProtocol*) protocol_arr[0])->getLog(entry->getType());
		log->write(entry->getKeyString().c_str(), entry->getValueString().c_str());

		delete entry;
		//printf("Index table size = %d\n", plexus->getIndexTable()->size());
	}
}

/*
 static void *callback(enum mg_event event, struct mg_connection * conn)
 {
 const struct mg_request_info *request_info = mg_get_request_info(conn);

 if (event == MG_NEW_REQUEST)
 {
 if (!this_peer->IsInitRcvd())
 {
 char content[1024];
 int content_length = snprintf(content, sizeof(content),
 "Peer Status Report<br/><br/> \
                        peer oid %s<br/>\
                        INIT not received",
 this_peer->getOverlayID().toString());
 mg_printf(conn, "HTTP/1.1 200 OK\r\n"
 "Content-Type: text/html\r\n"
 "Content-Length: %d\r\n" // Always set Content-Length
 "\r\n"
 "%s", content_length, content);
 //delete[] content;
 // Mark as processed
 }
 else
 {
 char content[16384];
 PlexusProtocol* plexus = (PlexusProtocol*) this_peer->getProtocol();
 //<meta http-equiv=\"refresh\" content=\"10\">
 int content_length = snprintf(content, sizeof(content), "<html><head></head><body>"
 "<h1>Peer Status Report</h1><br/><br/><strong>peer oid = </strong>%s<br/><strong>Routing Table</strong><br/>size = %d<br/>%s<br/>"
 "<strong>Proactive Cache</strong><br/>size = %d<br/>%s<br/>"
 "<strong>Cache</strong><br/>size = %d<br/>%s<br/>"
 "<strong>Queue Stats</strong><br/>Incoming Queue<br/>pushed = %d<br/>popped = %d<br/>"
 "Outgoing Queue<br/>pushed = %d<br/>popped = %d<br/><br/>"
 "Logging Queue<br/>pushed = %d<br/>popped = %d<br/><br/>"
 "<strong>PUT Message</strong><br/>Generated = %d<br/>Received = %d<br/>Generated/Received = %d<br/><br/>"
 "Locally processed = %d<br/>Forwarded = %d<br/>Dropped = %d<br/>Processed/Forwarded = %d<br/><br/><br/>"
 "<strong>GET Message</strong><br/>Generated = %d<br/>Received = %d<br/>Generated/Received = %d<br/><br/>"
 "Locally processed = %d<br/>Forwarded = %d<br/>Dropped = %d<br/>Processed/Forwarded = %d<br/><br/><br/>"
 "<strong>Index Table</strong><br/>size = %d<br/></body></html>", this_peer->getOverlayID().toString(),

 this_peer->getProtocol()->getRoutingTable()->size(), printRoutingTable2String(*this_peer->getProtocol()->getRoutingTable()),

 ((PlexusProtocol*) plexus)->getProactiveCache()->size(), printRoutingTable2String(*(((PlexusProtocol*) plexus)->getProactiveCache())),

 this_peer->getProtocol()->getCache()->getSize(), this_peer->getProtocol()->getCache()->toString(),

 plexus->incoming_queue_pushed, plexus->incoming_queue_popped, plexus->outgoing_queue_pushed, plexus->outgoing_queue_popped,
 plexus->logging_queue_pushed, plexus->logging_queue_popped,

 this_peer->numOfPut_generated(), this_peer->numOfPut_received(), this_peer->numOfPut_generated() + this_peer->numOfPut_received(),
 this_peer->numOfPut_processed(), this_peer->numOfPut_forwarded(), this_peer->numOfPut_dropped(),
 this_peer->numOfPut_processed() + this_peer->numOfPut_forwarded(),

 this_peer->numOfGet_generated(), this_peer->numOfGet_received(), this_peer->numOfGet_generated() + this_peer->numOfGet_received(),
 this_peer->numOfGet_processed(), this_peer->numOfGet_forwarded(), this_peer->numOfGet_dropped(),
 this_peer->numOfGet_processed() + this_peer->numOfGet_forwarded(),

 this_peer->getProtocol()->getIndexTable()->size() //,
 //printIndexTable2String(*this_peer->getProtocol()->getIndexTable())
 );
 //printf("html content: %d::%s\n", content_length, content);
 mg_printf(conn, "HTTP/1.1 200 OK\r\n"
 "Content-Type: text/html\r\n"
 "Content-Length: %d\r\n" // Always set Content-Length
 "\r\n"
 "%s", content_length, content);
 }
 }
 return NULL;
 }

 void *web_thread(void*)
 {
 printf("Starting a web thread on port ");
 char buffer[33];
 int port = 20002;
 while (true)
 {
 sprintf(buffer, "%d", port++);
 const char *options[] = { "listening_ports", buffer, NULL };
 ctx = mg_start(&callback, NULL, options);
 if (ctx != NULL)
 break;
 }
 puts(buffer);
 }

 void *storage_stat_thread(void*)
 {
 puts("Starting a storage stat thread");
 int cache_size = 0, index_size = 0, get_process_count = 0, put_process_count = 0;
 double get_hit = 0.0, put_hit = 0.0;
 bool loggable = false;
 PlexusProtocol* p_protocol = (PlexusProtocol*) plexus;
 const double EPS = 1e-3;

 while (true)
 {
 for (int i = 0; i < N_PEERS; i++)
 if (!peer_arr[i]->IsInitRcvd())
 continue;

 sleep(60);

 double temp_get_hit = 0.0;
 if (this_peer->numOfGet_processed() + this_peer->numOfGet_forwarded() != 0)
 temp_get_hit = (double) p_protocol->getGetCacheHitCounter()
 / (double) (this_peer->numOfGet_processed() + this_peer->numOfGet_forwarded());

 double temp_put_hit = 0;
 if (this_peer->numOfPut_processed() + this_peer->numOfPut_forwarded() != 0)
 temp_put_hit = (double) p_protocol->getPutCacheHitCounter()
 / (double) (this_peer->numOfPut_processed() + this_peer->numOfPut_forwarded());

 if (fabs(get_hit - temp_get_hit) > EPS)
 {
 get_hit = temp_get_hit;
 loggable = true;
 }

 if (fabs(put_hit - temp_put_hit) > EPS)
 {
 put_hit = temp_put_hit;
 loggable = true;

 }

 if (cache_size != p_protocol->getCache()->getSize())
 {
 cache_size = p_protocol->getCache()->getSize();
 loggable = true;
 }

 if (index_size != p_protocol->getIndexTable()->size())
 {
 index_size = p_protocol->getIndexTable()->size();
 loggable = true;
 }

 if (get_process_count != (this_peer->numOfGet_processed() + this_peer->numOfGet_forwarded()))
 {
 get_process_count = this_peer->numOfGet_processed() + this_peer->numOfGet_forwarded();
 loggable = true;
 }

 if (put_process_count != (this_peer->numOfPut_processed() + this_peer->numOfPut_forwarded()))
 {
 put_process_count = this_peer->numOfPut_processed() + this_peer->numOfPut_forwarded();
 loggable = true;
 }

 if (loggable)
 {
 string key = this_peer->getHostName();
 LogEntry* entry = new LogEntry(LOG_STORAGE, key.c_str(), "ddiiii", get_hit, put_hit, cache_size, index_size, get_process_count,
 put_process_count);
 p_protocol->addToLogQueue(entry);
 }
 loggable = false;
 }

 }
 */

/*void *pending_message_process_thread(void*)
 {
 puts("Starting a Pending Message Processing Thread");

 while (!this_peer->IsInitRcvd() || received_before_init_queue.empty())
 ;

 while (!received_before_init_queue.empty())
 {
 ((PlexusProtocol*) plexus)->addToIncomingQueue(
 received_before_init_queue.front());
 received_before_init_queue.pop();
 }

 puts("All Pending Messages Processed");
 }*/

void initListenSocket(const char* hosts_file)
{
	listen_port_number = -1;

	FILE* hosts_ptr = fopen(hosts_file, "r");
	if (hosts_ptr == NULL)
	{
		printf("bad hosts file %s\n", hosts_file);
		listen_port_number = -1;
		s_socket = NULL;
		return;
	}

	char host_name[200];
	int port = -1;
	int n_hosts;
	bool found = false;
	char line[400];

	fgets(line, sizeof(line), hosts_ptr);
	n_hosts = atoi(line);

//fscanf(hosts_ptr, "%d", &n_hosts);
	printf("N hosts = %d\n", n_hosts);

	for (int i = 0; i < n_hosts; i++)
	{
		fgets(line, sizeof(line), hosts_ptr);
		//fscanf(hosts_ptr, "%s %d", host_name, &port);
		strcpy(host_name, strtok(line, " \n"));
		port = atoi(strtok(NULL, " \n"));

		//printf("%s:%d\n", host_name, port);

		if (strncmp(this_host_name.c_str(), host_name, strlen(this_host_name.c_str())) == 0 || strcmp(host_name, "localhost") == 0)
		{
			listen_port_number = port;
			s_socket = new ServerSocket(listen_port_number);
			if (s_socket->init_connection() < 0)
			{
				delete s_socket;
				listen_port_number = -1;
				s_socket = NULL;
			}
			else
				break;
		}
	}
	fclose(hosts_ptr);
}

