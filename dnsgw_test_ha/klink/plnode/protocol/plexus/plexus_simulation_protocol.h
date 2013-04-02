/* 
 * File:   plexus_protocol.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 3:10 PM
 */

#ifndef PLEXUS_SIMULATION_PROTOCOL_H
#define	PLEXUS_SIMULATION_PROTOCOL_H

#include "../plexus/plexus_protocol.h"
#include "../../logging/log.h"

using namespace std;

class ABSProtocol;

class PlexusSimulationProtocol: public PlexusProtocol
{
	static queue<ABSMessage*> incoming_message_queue;
	static queue<ABSMessage*> outgoing_message_queue;
	static queue<LogEntry*> logging_queue;

	static pthread_mutex_t incoming_queue_lock;
	static pthread_mutex_t outgoing_queue_lock;
	static pthread_mutex_t log_queue_lock;

	static pthread_cond_t cond_incoming_queue_empty;
	static pthread_cond_t cond_outgoing_queue_empty;
	static pthread_cond_t cond_log_queue_empty;

	static Log *log[MAX_LOGS];
public:

	PlexusSimulationProtocol() :
			PlexusProtocol()
	{
		//this->routing_table = new LookupTable<OverlayID, HostAddress > ();
		//this->msgProcessor = new PlexusMessageProcessor();
		/*pthread_mutex_init(&incoming_queue_lock, NULL);
		 pthread_mutex_init(&outgoing_queue_lock, NULL);
		 pthread_mutex_init(&log_queue_lock, NULL);
		 pthread_mutex_init(&get_cache_hit_counter_lock, NULL);
		 pthread_mutex_init(&put_cache_hit_counter_lock, NULL);

		 pthread_cond_init(&cond_incoming_queue_empty, NULL);
		 pthread_cond_init(&cond_outgoing_queue_empty, NULL);
		 pthread_cond_init(&cond_log_queue_empty, NULL);

		 incoming_queue_pushed = incoming_queue_popped = 0;
		 outgoing_queue_pushed = outgoing_queue_popped = 0;
		 logging_queue_pushed = logging_queue_popped = 0;
		 get_cache_hit_count = put_cache_hit_count = 0;

		 proactive_cache = new LookupTable <OverlayID, HostAddress>();*/
		//this->msgProcessor->setContainerProtocol(this);
	}

	PlexusSimulationProtocol(LookupTable<OverlayID, HostAddress>* routing_table,
			LookupTable<string, HostAddress>* index_table, Cache *cache,
			MessageProcessor* msgProcessor, Peer* container) :
			PlexusProtocol(routing_table, index_table, cache, msgProcessor,
					container)
	{
		/*this->msgProcessor->setContainerProtocol(this);

		 pthread_mutex_init(&incoming_queue_lock, NULL);
		 pthread_mutex_init(&outgoing_queue_lock, NULL);
		 pthread_mutex_init(&log_queue_lock, NULL);
		 pthread_mutex_init(&get_cache_hit_counter_lock, NULL);
		 pthread_mutex_init(&put_cache_hit_counter_lock, NULL);

		 pthread_cond_init(&cond_incoming_queue_empty, NULL);
		 pthread_cond_init(&cond_outgoing_queue_empty, NULL);
		 pthread_cond_init(&cond_log_queue_empty, NULL);

		 incoming_queue_pushed = incoming_queue_popped = 0;
		 outgoing_queue_pushed = outgoing_queue_popped = 0;
		 logging_queue_pushed = logging_queue_popped = 0;
		 get_cache_hit_count = put_cache_hit_count = 0;

		 proactive_cache = new LookupTable <OverlayID, HostAddress>();*/
		//initLogs(container->getLogServerName().c_str(), container->getLogServerUser().c_str());
	}

	PlexusSimulationProtocol(Peer* container, MessageProcessor* msgProcessor) :
			PlexusProtocol(container, msgProcessor)
	{
		/*this->msgProcessor->setContainerProtocol(this);

		 pthread_mutex_init(&incoming_queue_lock, NULL);
		 pthread_mutex_init(&outgoing_queue_lock, NULL);
		 pthread_mutex_init(&log_queue_lock, NULL);
		 pthread_mutex_init(&get_cache_hit_counter_lock, NULL);
		 pthread_mutex_init(&put_cache_hit_counter_lock, NULL);

		 pthread_cond_init(&cond_incoming_queue_empty, NULL);
		 pthread_cond_init(&cond_outgoing_queue_empty, NULL);
		 pthread_cond_init(&cond_log_queue_empty, NULL);

		 incoming_queue_pushed = incoming_queue_popped = 0;
		 outgoing_queue_pushed = outgoing_queue_popped = 0;
		 logging_queue_pushed = logging_queue_popped = 0;
		 get_cache_hit_count = put_cache_hit_count = 0;

		 proactive_cache = new LookupTable <OverlayID, HostAddress>();*/
		//initLogs(container->getLogServerName().c_str(), container->getLogServerUser().c_str());
	}

	void initLogs(int log_seq_no, const char* log_server_name,
			const char* log_server_user)
	{
		if (log[LOG_GET] == NULL)
		{
			log[LOG_GET] = new Log(log_seq_no, "get",
					container_peer->getCacheType().c_str(),
					container_peer->getCacheStorage().c_str(),
					container_peer->getK(), log_server_name, log_server_user);

			log[LOG_GET]->setCheckPointRowCount(
					container_peer->getConfiguration()->getCheckPointRow());
			log[LOG_GET]->open("a");
		}

		if (log[LOG_PUT] == NULL)
		{
			log[LOG_PUT] = new Log(log_seq_no, "put",
					container_peer->getCacheType().c_str(),
					container_peer->getCacheStorage().c_str(),
					container_peer->getK(), log_server_name, log_server_user);

			log[LOG_PUT]->setCheckPointRowCount(
					container_peer->getConfiguration()->getCheckPointRow());
			log[LOG_PUT]->open("a");
		}

		if (log[LOG_STORAGE] == NULL)
		{
			log[LOG_STORAGE] = new Log(log_seq_no, "storage",
					container_peer->getCacheType().c_str(),
					container_peer->getCacheStorage().c_str(),
					container_peer->getK(), log_server_name, log_server_user);

			log[LOG_STORAGE]->setCheckPointRowCount(1);
			log[LOG_STORAGE]->open("a");
		}
	}

	void initLocks()
	{
		pthread_mutex_init(&incoming_queue_lock, NULL);
		pthread_mutex_init(&outgoing_queue_lock, NULL);
		pthread_mutex_init(&log_queue_lock, NULL);

		pthread_cond_init(&cond_incoming_queue_empty, NULL);
		pthread_cond_init(&cond_outgoing_queue_empty, NULL);
		pthread_cond_init(&cond_log_queue_empty, NULL);
	}

	void addToIncomingQueue(ABSMessage* message)
	{
		pthread_mutex_lock(&incoming_queue_lock);
		incoming_message_queue.push(message);
		incoming_queue_pushed++;
		pthread_cond_broadcast(&cond_incoming_queue_empty);
		pthread_mutex_unlock(&incoming_queue_lock);
	}

	bool isIncomingQueueEmpty()
	{
		bool status;
		pthread_mutex_lock(&incoming_queue_lock);
		status = incoming_message_queue.empty();
		pthread_mutex_unlock(&incoming_queue_lock);
		return status;
	}

	ABSMessage* getIncomingQueueFront()
	{
		pthread_mutex_lock(&incoming_queue_lock);

		while (incoming_message_queue.empty())
			pthread_cond_wait(&cond_incoming_queue_empty, &incoming_queue_lock);

		ABSMessage* ret = incoming_message_queue.front();

		incoming_message_queue.pop();
		incoming_queue_popped++;
		pthread_mutex_unlock(&incoming_queue_lock);
		return ret;
	}

	void addToOutgoingQueue(ABSMessage* message)
	{
		pthread_mutex_lock(&outgoing_queue_lock);

		outgoing_message_queue.push(message);
		outgoing_queue_pushed++;
		pthread_cond_broadcast(&cond_outgoing_queue_empty);
		pthread_mutex_unlock(&outgoing_queue_lock);
	}

	bool isOutgoingQueueEmpty()
	{
		bool status;
		pthread_mutex_lock(&outgoing_queue_lock);
		status = outgoing_message_queue.empty();
		pthread_mutex_unlock(&outgoing_queue_lock);
		return status;
	}

	ABSMessage* getOutgoingQueueFront()
	{
		pthread_mutex_lock(&outgoing_queue_lock);

		while (outgoing_message_queue.empty())
		{
			//printf("Waiting for a message in outgoing queue");
			pthread_cond_wait(&cond_outgoing_queue_empty, &outgoing_queue_lock);
		}

		ABSMessage* ret = outgoing_message_queue.front();

		//printf("Got a messge from the outgoing queue");
		outgoing_message_queue.pop();
		outgoing_queue_popped++;
		pthread_mutex_unlock(&outgoing_queue_lock);
		return ret;
	}

	void addToLogQueue(LogEntry* log_entry)
	{
		pthread_mutex_lock(&log_queue_lock);
		logging_queue.push(log_entry);
		logging_queue_pushed++;
		pthread_cond_signal(&cond_log_queue_empty);
		pthread_mutex_unlock(&log_queue_lock);
	}

	bool isLogQueueEmpty()
	{
		bool status;
		pthread_mutex_lock(&log_queue_lock);
		status = logging_queue.empty();
		pthread_mutex_unlock(&log_queue_lock);
		return status;
	}

	LogEntry* getLoggingQueueFront()
	{
		pthread_mutex_lock(&log_queue_lock);
		while (logging_queue.empty())
		{
			pthread_cond_wait(&cond_log_queue_empty, &log_queue_lock);
		}
		LogEntry* ret = logging_queue.front();
		logging_queue.pop();
		logging_queue_popped++;
		pthread_mutex_unlock(&log_queue_lock);
		return ret;
	}

	Log* getGetLog()
	{
		return log[LOG_GET];
	}

	Log* getPutLog()
	{
		return log[LOG_PUT];
	}

	Log* getLog(int type)
	{
		if (type >= MAX_LOGS)
			return NULL;
		return log[type];
	}

	void flushAllLog()
	{
		int i;
		for (i = 0; i < MAX_LOGS; i++)
		{
			log[i]->flush();
		}
	}

	bool setNextHop(ABSMessage* msg)
	{
		printf("Setting next hop, Message type = %d\n", msg->getMessageType());
		int maxLengthMatch = 0, currentMatchLength = 0, currentNodeMathLength =
				0;
		HostAddress next_hop("", -1);
		OverlayID next_oid;

		switch (msg->getMessageType())
		{
		case MSG_PEER_INIT:
		case MSG_PEER_CONFIG:
		case MSG_PEER_CHANGE_STATUS:
		case MSG_PEER_START:
		case MSG_START_GENERATE_NAME:
		case MSG_START_LOOKUP_NAME:
		case MSG_DYN_CHANGE_STATUS:
		case MSG_PEER_INITIATE_GET:
		case MSG_PEER_INITIATE_PUT:
		case MSG_PEER_FORCE_LOG:
		case MSG_CACHE_ME:
			puts("returning false");
			return false;
			break;
		}

		OverlayID target_oid;

		switch (msg->getMessageType())
		{
		case MSG_PLEXUS_GET:
			target_oid = ((MessageGET*) msg)->getTargetOid();
			break;
		case MSG_PLEXUS_GET_REPLY:
			target_oid = ((MessageGET_REPLY*) msg)->getTargetOid();
			break;
		case MSG_PLEXUS_PUT:
			target_oid = ((MessagePUT*) msg)->getTargetOid();
			break;
		case MSG_PLEXUS_PUT_REPLY:
			target_oid = ((MessagePUT_REPLY*) msg)->getTargetOid();
			break;
		}

		if (container_peer->getCacheStorage() == "end_point")
		{
			if (msg->getMessageType() == MSG_PLEXUS_GET_REPLY
					|| msg->getMessageType() == MSG_PLEXUS_PUT_REPLY)
				return false;
		}

		if (msg->getOverlayTtl() == 0)
			return false;

		if (msg->getMessageType() == MSG_PLEXUS_GET)
		{
			MessageGET *get_msg = (MessageGET*) msg;
			HostAddress h;
			if (index_table->lookup(get_msg->GetDeviceName(), &h))
			{
				return false;
			}
		}
		currentNodeMathLength =
				container_peer->getOverlayID().GetMatchedPrefixLength(
						msg->getDstOid());
		printf("Current match length = %d\n", currentNodeMathLength);
		printf("Message oid = %d\n", msg->getDstOid().GetOverlay_id());
		target_oid.printBits();
		putchar('\n');

		//cout << endl << "current node match : ";
		//container_peer->getOverlayID().printBits();
		//cout << " <> ";
		//msg->getOID().printBits();
		//cout << " = " << currentNodeMathLength << endl;

		//search in the RT
		//        OverlayID::MAX_LENGTH = GlobalData::rm->rm->k;
		//cout << "S OID M LEN " << OverlayID::MAX_LENGTH << endl;
		//puts("creating iterator");
		LookupTableIterator<OverlayID, HostAddress> rtable_iterator(
				routing_table);
		rtable_iterator.reset_iterator();

		//puts("looking up in routing table");
		OverlayID maxMatchOid;
		//routing_table->reset_iterator();
		while (rtable_iterator.hasMoreKey())
		{
			//while (routing_table->hasMoreKey()) {
			//   OverlayID oid = routing_table->getNextKey();
			OverlayID oid = rtable_iterator.getNextKey();
			//printf("next key = %d My id = ", oid.GetOverlay_id());
			//msg->getDstOid().printBits();
			//putchar('\n');

			//cout << endl << "current match ";
			//oid.printBits();
			currentMatchLength = target_oid.GetMatchedPrefixLength(oid);
			//cout << " ==== " << currentMatchLength << endl;
			//printf(">current match length = %d\n", currentMatchLength);

			if (currentMatchLength > maxLengthMatch)
			{
				maxLengthMatch = currentMatchLength;
				maxMatchOid = oid;

				/*printf("next host %s, next port %d\n",
				 next_hop.GetHostName().c_str(), next_hop.GetHostPort());*/
			}
		}
		routing_table->lookup(maxMatchOid, &next_hop);
		next_oid = maxMatchOid;

		//search in proactive cache
		LookupTableIterator<OverlayID, HostAddress> pcache_iterator(
				proactive_cache);
		pcache_iterator.reset_iterator();

		//puts("looking up in routing table");
		//OverlayID maxMatchOid;
		//routing_table->reset_iterator();
		bool cache_hit = false;

		while (pcache_iterator.hasMoreKey())
		{
			//while (routing_table->hasMoreKey()) {
			//   OverlayID oid = routing_table->getNextKey();
			OverlayID oid = pcache_iterator.getNextKey();
			//printf("next key = %d My id = ", oid.GetOverlay_id());
			//msg->getDstOid().printBits();
			//putchar('\n');

			//cout << endl << "current match ";
			//oid.printBits();
			currentMatchLength = target_oid.GetMatchedPrefixLength(oid);
			//cout << " ==== " << currentMatchLength << endl;
			//printf(">current match length = %d\n", currentMatchLength);

			if (currentMatchLength > maxLengthMatch)
			{
				maxLengthMatch = currentMatchLength;
				maxMatchOid = oid;
				cache_hit = true;
				/*printf("next host %s, next port %d\n",
				 next_hop.GetHostName().c_str(), next_hop.GetHostPort());*/
			}
		}
		if (cache_hit)
			proactive_cache->lookup(maxMatchOid, &next_hop);
		next_oid = maxMatchOid;

		//search in the Cache
		CacheIterator cache_iterator(cache);
		cache_iterator.reset_iterator();

		//printf("**********************************\n%s\n**********************************\n", cache->toString());

		while (cache_iterator.hasMore())
		{
			DLLNode *node = cache_iterator.getNext();
			OverlayID id = node->key;
			currentMatchLength = target_oid.GetMatchedPrefixLength(id);
			if (currentMatchLength > maxLengthMatch)
			{
				maxLengthMatch = currentMatchLength;
				/*if(cache->lookup(msg->getDstOid(), next_hop))
				 cache_hit = true;*/
				if (cache->lookup(id, next_hop))
					cache_hit = true;
				next_oid = id;
				//printf("[From Cache] -> next host %s, next port %d\n",next_hop.GetHostName().c_str(), next_hop.GetHostPort());
			}
			printf("[From Cache] -> oid = %s\n", id.toString());
		}

		if (cache_hit)
		{
			if (msg->getMessageType() == MSG_PLEXUS_GET)
				incrementGetCacheHitCounter();
			else if (msg->getMessageType() == MSG_PLEXUS_PUT)
				incrementPutCacheHitCounter();
		}

		cout << endl << "max match : = " << maxLengthMatch << endl;

		if (maxLengthMatch == 0 || maxLengthMatch < currentNodeMathLength)
		{
			//puts("returning false");
			//msg->setDestHost("localhost");
			//msg->setDestPort(container_peer->getListenPortNumber());
			return false;
		} else
		{
			//puts("returning true");
			msg->setDestHost(next_hop.GetHostName().c_str());
			msg->setDestPort(next_hop.GetHostPort());
			msg->setDstOid(next_oid);
			return true;
		}
	}

	void get(string name)
	{
		int hash_name_to_get = atoi(name.c_str());
		OverlayID targetID(hash_name_to_get, getContainerPeer()->GetiCode());

		//printf("h_name = %d, oid = %d\n", hash_name_to_get, targetID.GetOverlay_id());

		MessageGET *msg = new MessageGET(container_peer->getHostName(),
				container_peer->getListenPortNumber(), "", -1,
				container_peer->getOverlayID(), OverlayID(), targetID, name);

		/*printf("Constructed Get Message");
		 msg->message_print_dump();*/

		MessageStateIndex msg_index(hash_name_to_get, msg->getSequenceNo());
		timeval timestamp;
		gettimeofday(&timestamp, NULL);
		double timestamp_t = (double) (timestamp.tv_sec) * 1000.0
				+ (double) (timestamp.tv_usec) / 1000.0;
		unresolved_get.add(msg_index, timestamp_t);

		if (msgProcessor->processMessage(msg))
		{
			addToOutgoingQueue(msg);
		}
		getContainerPeer()->incrementGet_generated();
	}

	void get_from_client(string name, HostAddress destination)
	{
		int hash_name_to_get = atoi(name.c_str());
		OverlayID destID(hash_name_to_get, getContainerPeer()->GetiCode());

		cout << "id = " << hash_name_to_get << " oid = ";
		destID.printBits();
		cout << endl;

		PeerInitiateGET *msg = new PeerInitiateGET(
				container_peer->getHostName(),
				container_peer->getListenPortNumber(),
				destination.GetHostName(), destination.GetHostPort(),
				container_peer->getOverlayID(), destID, name);
		msg->calculateOverlayTTL(getContainerPeer()->getNPeers());
		//msg->message_print_dump();
		send_message(msg);
	}

	void put(string name, HostAddress hostAddress)
	{
		unsigned int hash_name_to_publish = atoi(name.c_str());
		printf("Publish name hash = %d\n", hash_name_to_publish);

		OverlayID targetID(hash_name_to_publish,
				getContainerPeer()->GetiCode());

		printf("target id = ");
		targetID.printBits();
		putchar('\n');

		MessagePUT *msg = new MessagePUT(container_peer->getHostName(),
				container_peer->getListenPortNumber(), "", -1,
				container_peer->getOverlayID(), OverlayID(), targetID, name,
				hostAddress);

		msg->message_print_dump();

		/*MessageStateIndex msg_index(hash_name_to_publish, msg->getSequenceNo());
		 timeval timestamp;
		 gettimeofday(&timestamp, NULL);
		 double timestamp_t = (double) (timestamp.tv_sec) * 1000.0
		 + (double) (timestamp.tv_usec) / 1000.0;
		 unresolved_put.add(msg_index, timestamp_t);*/

		puts("Processing newly created put message");
		if (msgProcessor->processMessage(msg))
		{
			puts("Adding to outgoing queue");
			addToOutgoingQueue(msg);
		}
		getContainerPeer()->incrementPut_generated();
	}

	void put_from_client(string name, HostAddress hostAddress,
			HostAddress destination)
	{
		int hash_name_to_publish = atoi(name.c_str());
		OverlayID destID(hash_name_to_publish, getContainerPeer()->GetiCode());

		cout << "id = " << hash_name_to_publish << " oid = ";
		destID.printBits();
		cout << endl;

		PeerInitiatePUT *msg = new PeerInitiatePUT(
				container_peer->getHostName(),
				container_peer->getListenPortNumber(),
				destination.GetHostName(), destination.GetHostPort(),
				container_peer->getOverlayID(), destID, name, hostAddress);
		msg->calculateOverlayTTL(getContainerPeer()->getNPeers());

		msg->message_print_dump();
		send_message(msg);
	}
};

queue<ABSMessage*> PlexusSimulationProtocol::incoming_message_queue;
queue<ABSMessage*> PlexusSimulationProtocol::outgoing_message_queue;
queue<LogEntry*> PlexusSimulationProtocol::logging_queue;

pthread_mutex_t PlexusSimulationProtocol::incoming_queue_lock;
pthread_mutex_t PlexusSimulationProtocol::outgoing_queue_lock;
pthread_mutex_t PlexusSimulationProtocol::log_queue_lock;

pthread_cond_t PlexusSimulationProtocol::cond_incoming_queue_empty;
pthread_cond_t PlexusSimulationProtocol::cond_outgoing_queue_empty;
pthread_cond_t PlexusSimulationProtocol::cond_log_queue_empty;

Log* PlexusSimulationProtocol::log[MAX_LOGS] = { NULL, NULL, NULL };
#endif	/* PLEXUS_PROTOCOL_H */

