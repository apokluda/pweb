/* 
 * File:   plexus_protocol.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 3:10 PM
 */

#ifndef PLEXUS_PROTOCOL_H
#define	PLEXUS_PROTOCOL_H

#include <pthread.h>
#include <cstring>
#include <queue>
#include "../protocol.h"
#include "../../ds/cache.h"
#include "../../ds/cache_iterator.h"
#include "../../message/message_processor.h"
//#include "plexus_message_processor.h"
#include "../../message/p2p/message_get.h"
#include "../../message/p2p/message_put.h"
#include "../../message/p2p/message_get_reply.h"
#include "../../message/p2p/message_put_reply.h"

#include "../../message/control/peer_initiate_get.h"
#include "../../message/control/peer_initiate_put.h"

#include "../../ds/host_address.h"
#include "../../ds/message_state_index.h"

#include "../../message/message.h"
#include "../../logging/log.h"
#include "../../logging/log_entry.h"
#include <cmath>

using namespace std;

class ABSProtocol;

#define MAX_LOGS 3
#define LOG_GET 0
#define LOG_PUT 1
#define LOG_STORAGE 2
#define ALL_LOGS 3

class PlexusProtocol : public ABSProtocol {
protected:
        Log *log[MAX_LOGS];
        LookupTable <MessageStateIndex, double> unresolved_put;
        LookupTable <MessageStateIndex, pair <HostAddress, string> > unresolved_get;

        LookupTable <OverlayID, HostAddress>* proactive_cache;

        queue<ABSMessage*> incoming_message_queue;
        queue<ABSMessage*> outgoing_message_queue;
        queue<LogEntry*> logging_queue;
        
        pthread_mutex_t incoming_queue_lock;
        pthread_mutex_t outgoing_queue_lock;
        pthread_mutex_t log_queue_lock;
        pthread_mutex_t get_cache_hit_counter_lock;
        pthread_mutex_t put_cache_hit_counter_lock;

        pthread_cond_t cond_incoming_queue_empty;
        pthread_cond_t cond_outgoing_queue_empty;
        pthread_cond_t cond_log_queue_empty;

        int get_cache_hit_count, put_cache_hit_count;
public:
        int incoming_queue_pushed, incoming_queue_popped;
        int outgoing_queue_pushed, outgoing_queue_popped;
        int logging_queue_pushed, logging_queue_popped;


        PlexusProtocol() :
        ABSProtocol() {
                //this->routing_table = new LookupTable<OverlayID, HostAddress > ();

                //this->msgProcessor = new PlexusMessageProcessor();
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

                proactive_cache = new LookupTable <OverlayID, HostAddress>();
                //this->msgProcessor->setContainerProtocol(this);
        }

        PlexusProtocol(LookupTable<OverlayID, HostAddress>* routing_table,
                LookupTable<string, HostAddress>* index_table, Cache *cache,
                MessageProcessor* msgProcessor, Peer* container) :
        ABSProtocol(routing_table, index_table, cache, msgProcessor,
        container) {
                this->msgProcessor->setContainerProtocol(this);

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

                proactive_cache = new LookupTable <OverlayID, HostAddress>();
                //initLogs(container->getLogServerName().c_str(), container->getLogServerUser().c_str());
        }

        PlexusProtocol(Peer* container, MessageProcessor* msgProcessor) :
        ABSProtocol(container, msgProcessor) {
                this->msgProcessor->setContainerProtocol(this);

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

                proactive_cache = new LookupTable <OverlayID, HostAddress>();
                //initLogs(container->getLogServerName().c_str(), container->getLogServerUser().c_str());
        }

        void reset_counters()
        {
        	incoming_queue_pushed = incoming_queue_popped = 0;
			outgoing_queue_pushed = outgoing_queue_popped = 0;
			logging_queue_pushed = logging_queue_popped = 0;
			get_cache_hit_count = put_cache_hit_count = 0;
        }

        void initLogs(int log_seq_no, const char* log_server_name, const char* log_server_user) {
                log[LOG_GET] = new Log(log_seq_no, "get",
                		container_peer->getCacheType().c_str(), container_peer->getCacheStorage().c_str(), container_peer->getK(),
                		log_server_name,
                        log_server_user);
                log[LOG_PUT] = new Log(log_seq_no, "put",
                		container_peer->getCacheType().c_str(), container_peer->getCacheStorage().c_str(), container_peer->getK(),
                		log_server_name,
                        log_server_user);

                log[LOG_STORAGE] = new Log(log_seq_no, "storage",
                		container_peer->getCacheType().c_str(), container_peer->getCacheStorage().c_str(), container_peer->getK(),
                		log_server_name, log_server_user);

                log[LOG_GET]->setCheckPointRowCount(container_peer->getConfiguration()->getCheckPointRow());
                log[LOG_PUT]->setCheckPointRowCount(container_peer->getConfiguration()->getCheckPointRow());
                log[LOG_STORAGE]->setCheckPointRowCount(1);

                log[LOG_GET]->open("a");
                log[LOG_PUT]->open("a");
                log[LOG_STORAGE]->open("a");
        }

        void processMessage(ABSMessage *message) {
                msgProcessor->processMessage(message);
        }

        void initiate_join() {
        }

        void process_join() {
        }
        
        bool setNextHop(ABSMessage* msg) {
                printf("Setting next hop, Message type = %d\n", msg->getMessageType());
                int maxLengthMatch = 0, currentMatchLength = 0, currentNodeMathLength = 0;
                HostAddress next_hop("", -1);
                OverlayID next_oid;

                switch (msg->getMessageType()) {
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
                        case MSG_RETRIEVE:
                        case MSG_RETRIEVE_REPLY:
                        		puts("returning false");
                                return false;
                                break;
                }

                OverlayID target_oid;

                switch(msg->getMessageType())
                {
                case MSG_PLEXUS_GET:
                	target_oid = ((MessageGET*)msg)->getTargetOid();
                	break;
                case MSG_PLEXUS_GET_REPLY:
                	target_oid = ((MessageGET_REPLY*)msg)->getTargetOid();
                	break;
                case MSG_PLEXUS_PUT:
                	target_oid = ((MessagePUT*)msg)->getTargetOid();
                	break;
                case MSG_PLEXUS_PUT_REPLY:
                	target_oid = ((MessagePUT_REPLY*)msg)->getTargetOid();
                	break;
                }

                if(container_peer->getCacheStorage() == "end_point")
                {
                	if(msg->getMessageType() == MSG_PLEXUS_GET_REPLY || msg->getMessageType() == MSG_PLEXUS_PUT_REPLY)
                		return false;
                }

                if (msg->getOverlayTtl() == 0)
                        return false;

                if(msg->getMessageType() == MSG_PLEXUS_GET){
                        MessageGET *get_msg = (MessageGET*)msg;
                        HostAddress h;
                        if(index_table->lookup(get_msg->GetDeviceName(), &h)){
                                return false;
                        }
                }
                currentNodeMathLength = container_peer->getOverlayID().GetMatchedPrefixLength(msg->getDstOid());
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
                LookupTableIterator<OverlayID, HostAddress> rtable_iterator(routing_table);
                rtable_iterator.reset_iterator();

                //puts("looking up in routing table");
                OverlayID maxMatchOid;
                //routing_table->reset_iterator();
                while (rtable_iterator.hasMoreKey()) {
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

                        if (currentMatchLength > maxLengthMatch) {
                                maxLengthMatch = currentMatchLength;
                                maxMatchOid = oid;

                                /*printf("next host %s, next port %d\n",
                                                next_hop.GetHostName().c_str(), next_hop.GetHostPort());*/
                        }
                }
                routing_table->lookup(maxMatchOid, &next_hop);
                next_oid = maxMatchOid;

                //search in proactive cache
                LookupTableIterator<OverlayID, HostAddress> pcache_iterator(proactive_cache);
                pcache_iterator.reset_iterator();

				//puts("looking up in routing table");
				//OverlayID maxMatchOid;
				//routing_table->reset_iterator();
                bool cache_hit = false;

				while (pcache_iterator.hasMoreKey()) {
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

						if (currentMatchLength > maxLengthMatch) {
								maxLengthMatch = currentMatchLength;
								maxMatchOid = oid;
								cache_hit = true;
								/*printf("next host %s, next port %d\n",
												next_hop.GetHostName().c_str(), next_hop.GetHostPort());*/
						}
				}
				if(cache_hit) proactive_cache->lookup(maxMatchOid, &next_hop);
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
                                if(cache->lookup(id, next_hop))
                                	cache_hit = true;
                                next_oid = id;
                                //printf("[From Cache] -> next host %s, next port %d\n",next_hop.GetHostName().c_str(), next_hop.GetHostPort());
                        }
                        printf("[From Cache] -> oid = %s\n", id.toString());
                }

                if(cache_hit)
                {
                	if(msg->getMessageType() == MSG_PLEXUS_GET)
                		incrementGetCacheHitCounter();
                	else if(msg->getMessageType() == MSG_PLEXUS_PUT)
                		incrementPutCacheHitCounter();
                }

                cout << endl << "max match : = " << maxLengthMatch << endl;

                if (maxLengthMatch == 0 || maxLengthMatch < currentNodeMathLength) {
                        //puts("returning false");
                        //msg->setDestHost("localhost");
                        //msg->setDestPort(container_peer->getListenPortNumber());
                        return false;
                } else {
                        //puts("returning true");
                        msg->setDestHost(next_hop.GetHostName().c_str());
                        msg->setDestPort(next_hop.GetHostPort());
                        msg->setDstOid(next_oid);
                        return true;
                }
        }

        void get(string name) {
                //int hash_name_to_get = (int)urlHash(name) & 0x003FFFFF;
		//int hash_name_to_get = (int)urlHash(name);
		int hash_name_to_get = atoi(name.c_str());
                OverlayID targetID(hash_name_to_get, getContainerPeer()->GetiCode());

                //printf("h_name = %d, oid = %d\n", hash_name_to_get, targetID.GetOverlay_id());

                MessageGET *msg = new MessageGET(container_peer->getHostName(),
                        container_peer->getListenPortNumber(), "", -1,
                        container_peer->getOverlayID(), OverlayID(), targetID, name);

                /*printf("Constructed Get Message");
                msg->message_print_dump();*/

                if (msgProcessor->processMessage(msg))
                {
                        addToOutgoingQueue(msg);
                }
                getContainerPeer()->incrementGet_generated();
        }

        void get_for_client(PeerInitiateGET* message)
        {
        	int last_dot;
        	string str = message->getDeviceName();
        	printf("str = %s\n", str.c_str());

        	for(last_dot = (int)str.size() - 1; last_dot >= 0; last_dot--)
        		if(str[last_dot] == '.')
        			break;

        	string ha_name = str.substr(last_dot + 1);
        	string d_name = str.substr(0, last_dot);

        	printf("Home agent name = %s, Device name = %s\n", ha_name.c_str(), d_name.c_str());
        	int hash_name_to_get = (int)urlHash(ha_name) & 0x003FFFFF;
        	//string name = message->getDeviceName();
			OverlayID targetID(hash_name_to_get, getContainerPeer()->GetiCode());

			MessageGET *msg = new MessageGET(container_peer->getHostName(),
			                        container_peer->getListenPortNumber(), "", -1,
			                        container_peer->getOverlayID(), OverlayID(), targetID, ha_name);

			msg->setSequenceNo(message->getSequenceNo());
			MessageStateIndex ind(hash_name_to_get, message->getSequenceNo());

			printf("msg_state_index = %d_%d\n", ind.getMessageSeqNo(), ind.getNameHash());
			unresolved_get.add(ind, make_pair(HostAddress(message->getSourceHost(), message->getSourcePort()), d_name));

			if (msgProcessor->processMessage(msg))
			{
					addToOutgoingQueue(msg);
			}
			getContainerPeer()->incrementGet_generated();
        }

        void get_from_client(string name, HostAddress destination) {
                //int hash_name_to_get = (int)urlHash(name) & 0x003FFFFF;
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

        void put(string name, HostAddress hostAddress) {
                //int hash_name_to_publish = (int)urlHash(name) & 0x003FFFFF;
		//int hash_name_to_publish = (int)urlHash(name);
		int hash_name_to_publish = atoi(name.c_str());
                OverlayID targetID(hash_name_to_publish, getContainerPeer()->GetiCode());
		
                targetID.printBits();

                MessagePUT *msg = new MessagePUT(container_peer->getHostName(),
                        container_peer->getListenPortNumber(), "", -1,
                        container_peer->getOverlayID(), OverlayID(), targetID, name, hostAddress);
		printf("put msg created.\n");
                MessageStateIndex msg_index(hash_name_to_publish, msg->getSequenceNo());
                
		timeval timestamp;
                gettimeofday(&timestamp, NULL);
                double timestamp_t = (double)(timestamp.tv_sec) * 1000.0 + (double)(timestamp.tv_usec) / 1000.0;
                unresolved_put.add(msg_index, timestamp_t);

                if (msgProcessor->processMessage(msg)) {
                        addToOutgoingQueue(msg);
                }
		printf("put msg add q.\n");
                getContainerPeer()->incrementPut_generated();
        }

        void put_from_client(string name, HostAddress hostAddress,
                HostAddress destination) {
                //int hash_name_to_publish = (int)urlHash(name) & 0x003FFFFF;
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

        void rejoin() {
        }

        void addToIncomingQueue(ABSMessage* message) {
                pthread_mutex_lock(&incoming_queue_lock);
                incoming_message_queue.push(message);
                incoming_queue_pushed++;
                pthread_cond_broadcast(&cond_incoming_queue_empty);
                pthread_mutex_unlock(&incoming_queue_lock);
        }

        bool isIncomingQueueEmpty() {
                bool status;
                pthread_mutex_lock(&incoming_queue_lock);
                status = incoming_message_queue.empty();
                pthread_mutex_unlock(&incoming_queue_lock);
                return status;
        }

        ABSMessage* getIncomingQueueFront() {
                pthread_mutex_lock(&incoming_queue_lock);

                while (incoming_message_queue.empty())
                        pthread_cond_wait(&cond_incoming_queue_empty, &incoming_queue_lock);

                ABSMessage* ret = incoming_message_queue.front();

                incoming_message_queue.pop();
                incoming_queue_popped++;
                pthread_mutex_unlock(&incoming_queue_lock);
                return ret;
        }

        void addToOutgoingQueue(ABSMessage* message) {
                pthread_mutex_lock(&outgoing_queue_lock);

                outgoing_message_queue.push(message);
                outgoing_queue_pushed++;
                pthread_cond_broadcast(&cond_outgoing_queue_empty);
                pthread_mutex_unlock(&outgoing_queue_lock);
        }

        bool isOutgoingQueueEmpty() {
                bool status;
                pthread_mutex_lock(&outgoing_queue_lock);
                status = outgoing_message_queue.empty();
                pthread_mutex_unlock(&outgoing_queue_lock);
                return status;
        }

        ABSMessage* getOutgoingQueueFront() {
                pthread_mutex_lock(&outgoing_queue_lock);

                while (outgoing_message_queue.empty()) {
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

        void addToLogQueue(LogEntry* log_entry) {
                pthread_mutex_lock(&log_queue_lock);
                logging_queue.push(log_entry);
                logging_queue_pushed++;
                pthread_cond_signal(&cond_log_queue_empty);
                pthread_mutex_unlock(&log_queue_lock);
        }

        bool isLogQueueEmpty() {
                bool status;
                pthread_mutex_lock(&log_queue_lock);
                status = logging_queue.empty();
                pthread_mutex_unlock(&log_queue_lock);
                return status;
        }

        LogEntry* getLoggingQueueFront() {
                pthread_mutex_lock(&log_queue_lock);
                while (logging_queue.empty()) {
                        pthread_cond_wait(&cond_log_queue_empty, &log_queue_lock);
                }
                LogEntry* ret = logging_queue.front();
                logging_queue.pop();
                logging_queue_popped++;
                pthread_mutex_unlock(&log_queue_lock);
                return ret;
        }

        void incrementGetCacheHitCounter()
        {
        	pthread_mutex_lock(&get_cache_hit_counter_lock);
        	get_cache_hit_count++;
        	pthread_mutex_unlock(&get_cache_hit_counter_lock);
        }

        void incrementPutCacheHitCounter()
        {
        	pthread_mutex_lock(&put_cache_hit_counter_lock);
        	put_cache_hit_count++;
        	pthread_mutex_unlock(&put_cache_hit_counter_lock);
        }

        int getGetCacheHitCounter()
        {
        	int ret;
        	pthread_mutex_lock(&get_cache_hit_counter_lock);
			ret = get_cache_hit_count;
			pthread_mutex_unlock(&get_cache_hit_counter_lock);
			return ret;
        }

        int getPutCacheHitCounter()
		{
			int ret;
			pthread_mutex_lock(&put_cache_hit_counter_lock);
			ret = put_cache_hit_count;
			pthread_mutex_unlock(&put_cache_hit_counter_lock);
			return ret;
		}

        Log* getGetLog() {
                return log[LOG_GET];
        }

        Log* getPutLog() {
                return log[LOG_PUT];
        }

        Log* getLog(int type) {
                if (type >= MAX_LOGS)
                        return NULL;
                return log[type];
        }

        void flushAllLog()
        {
        	int i;
        	for(i = 0; i < MAX_LOGS; i++)
        	{
        		log[i]->flush();
        	}
        }

        ~PlexusProtocol() {
                pthread_mutex_destroy(&incoming_queue_lock);
                pthread_mutex_destroy(&outgoing_queue_lock);
                pthread_mutex_destroy(&log_queue_lock);
                pthread_mutex_destroy(&get_cache_hit_counter_lock);
                pthread_mutex_destroy(&put_cache_hit_counter_lock);

                pthread_cond_destroy(&cond_incoming_queue_empty);
                pthread_cond_destroy(&cond_outgoing_queue_empty);
                pthread_cond_destroy(&cond_log_queue_empty);
        }

		LookupTable<MessageStateIndex, pair <HostAddress, string> >& getUnresolvedGet()
		{
			return unresolved_get;
		}

		void setUnresolvedGet(const LookupTable<MessageStateIndex, pair <HostAddress, string> >& unresolvedGet)
		{
			unresolved_get = unresolvedGet;
		}

		LookupTable<MessageStateIndex, double>& getUnresolvedPut()
		{
			return unresolved_put;
		}

		void setUnresolvedPut(const LookupTable<MessageStateIndex, double>& unresolvedPut)
		{
			unresolved_put = unresolvedPut;
		}

		LookupTable <OverlayID, HostAddress>* getProactiveCache()
		{
			return proactive_cache;
		}

		void setProactiveCache(LookupTable <OverlayID, HostAddress>* p_cache)
		{
			this->proactive_cache = p_cache;
		}
};

#endif	/* PLEXUS_PROTOCOL_H */

