/* 
 * File:   plexus_sim_message_processor.h
 * Author: mfbari
 *
 * Created on January 17, 2012, 7:09 PM
 */

#ifndef PLEXUS_SIM_MESSAGE_PROCESSOR_H
#define	PLEXUS_SIM_MESSAGE_PROCESSOR_H

#include "../../message/message_processor.h"

#include "../../message/message.h"
#include "../../message/p2p/message_get.h"
#include "../../message/p2p/message_put.h"
#include "../../message/p2p/message_get_reply.h"
#include "../../message/p2p/message_put_reply.h"
#include "../../message/p2p/message_cache_me.h"

#include "../../message/control/peer_init_message.h"
#include "../../message/control/peer_initiate_get.h"
#include "../../message/control/peer_initiate_put.h"
#include "../../message/control/peer_config_msg.h"
#include "../../message/control/peer_change_status_message.h"
#include "../../message/control/peer_start_gen_name_message.h"
#include "../../message/control/peer_dyn_change_status_message.h"
#include "../../message/control/log_force_message.h"

#include "../protocol.h"
#include "../plexus/plexus_protocol.h"

#include "../../../common/util.h"

#include "../../ds/cache.h"
#include "../../ds/cache_insert_endpoint.h"
#include "../../ds/cache_replace_LRU.h"

#include "../../ds/overlay_id.h"

#include "../../peer/peer_status.h"

#include "../../logging/log_entry.h"

class PlexusSimMessageProcessor : public MessageProcessor {
public:

        void setup(LookupTable<OverlayID, HostAddress>* routing_table,
                LookupTable<string, HostAddress>* index_table, Cache *cache) {
                MessageProcessor::setup(routing_table, index_table, cache);
        }

        bool processMessage(ABSMessage* message)
        {
                message->decrementOverlayTtl();

                PlexusSimulationProtocol* plexus = (PlexusSimulationProtocol*) container_protocol;
                Peer* container_peer = container_protocol->getContainerPeer();

                bool forward = plexus->setNextHop(message);

                if (forward)
                {
                	if(container_peer->getCacheStorage() == "path")
                	{
						if(message->getMessageType() == MSG_PLEXUS_GET_REPLY || message->getMessageType() == MSG_PLEXUS_PUT_REPLY)
						{
							OverlayID id(message->getSrcOid().GetOverlay_id(), message->getSrcOid().GetPrefix_length(), message->getSrcOid().MAX_LENGTH);
							HostAddress ha(message->getSourceHost(), message->getSourcePort());
							cache->add(id, ha);
						}
                	}
                	return true;
                }
                //PUT
                if (message->getMessageType() == MSG_PLEXUS_PUT) {
                        container_peer->incrementPut_processed();
                        MessagePUT* putMsg = (MessagePUT*) message;
                        puts("Adding to index table");
                        index_table->add(putMsg->GetDeviceName(), putMsg->GetHostAddress());

                        //puts("PUT Successful");
                        MessagePUT_REPLY *reply;

						reply = new MessagePUT_REPLY(container_peer->getHostName(),
										container_peer->getListenPortNumber(), putMsg->getSourceHost(),
										putMsg->getSourcePort(), container_peer->getOverlayID(), OverlayID(),
										SUCCESS, putMsg->getSrcOid(), putMsg->GetDeviceName());

						reply->setResolutionHops(putMsg->getOverlayHops());
								reply->setResolutionIpHops(putMsg->getIpHops());
								reply->setResolutionLatency(putMsg->getLatency());
								reply->setOriginSeqNo(putMsg->getSequenceNo());

						if(container_peer->getCacheStorage() == "path")
						{
							plexus->setNextHop(reply);
						}
                        plexus->addToOutgoingQueue(reply);
                }//GET
                else if (message->getMessageType() == MSG_PLEXUS_GET) {
                        container_peer->incrementGet_processed();
                        MessageGET *msg = ((MessageGET*) message);
                        HostAddress hostAddress;
                        if (index_table->lookup(msg->GetDeviceName(), &hostAddress)) {
                                puts("Got it :)");
                                MessageGET_REPLY *reply = new MessageGET_REPLY(container_peer->getHostName(),
                                        container_peer->getListenPortNumber(), msg->getSourceHost(),
                                        msg->getSourcePort(), container_peer->getOverlayID(), OverlayID(),
                                        SUCCESS, msg->getSrcOid(), hostAddress, msg->GetDeviceName());

                                reply->setResolutionHops(msg->getOverlayHops());
                                reply->setResolutionIpHops(msg->getIpHops());
                                reply->setResolutionLatency(msg->getLatency());
                                reply->setOriginSeqNo(msg->getSequenceNo());

                                if(container_peer->getCacheStorage() == "path")
                                {
                                	plexus->setNextHop(reply);
                                }
                                plexus->addToOutgoingQueue(reply);
                        } else {

                                puts("GET Failed");
                                MessageGET_REPLY *reply = new MessageGET_REPLY(container_peer->getHostName(),
                                        container_peer->getListenPortNumber(), msg->getSourceHost(),
                                        msg->getSourcePort(), container_peer->getOverlayID(), OverlayID(),
                                        ERROR_GET_FAILED, msg->getSrcOid(), hostAddress, msg->GetDeviceName());

                                reply->setResolutionHops(msg->getOverlayHops());
                                reply->setResolutionIpHops(msg->getIpHops());
                                reply->setResolutionLatency(msg->getLatency());
                                reply->setOriginSeqNo(msg->getSequenceNo());

                                plexus->addToOutgoingQueue(reply);
                        }
                }//GET_REPLY
        else if (message->getMessageType() == MSG_PLEXUS_GET_REPLY) {
            MessageGET_REPLY *msg = ((MessageGET_REPLY*) message);
            OverlayID srcID(msg->getSrcOid().GetOverlay_id(), msg->getSrcOid().GetPrefix_length(), msg->getSrcOid().MAX_LENGTH);

            HostAddress ha(msg->getSourceHost(), msg->getSourcePort());
            cache->add(srcID, ha);
            Log* g_log = plexus->getGetLog();

            char i_str[300];
            sprintf(i_str, "%s_%s_%d", msg->getSourceHost().c_str(), msg->getDestHost().c_str(),
                    msg->getSequenceNo());
            string key = i_str;

            int hash_name_to_get = atoi(msg->getDeviceName().c_str());
            MessageStateIndex msg_index(hash_name_to_get, msg->getOriginSeqNo());

            //timeval start_t;
            double start;
            plexus->getUnresolvedGet().lookup(msg_index, &start);
            plexus->getUnresolvedGet().remove(msg_index);

            //timeval total;
            //timersub(&end_t, &start_t, &total);
            //double total_t =((double)total.tv_sec * 1000.0) + ((double)total.tv_usec / 1000.0);
            double latency = msg->getResolutionLatency();
            int ip_hops = msg->getResolutionIpHops();

            string status = "S";
            if (msg->getStatus() == ERROR_GET_FAILED)
                status = "F";

            LogEntry *entry = new LogEntry(LOG_GET, key.c_str(), "iidssi",
                    msg->getResolutionHops(), ip_hops, latency, status.c_str(), msg->getDeviceName().c_str(),
                    msg->getSrcOid().GetOverlay_id());

            printf("[Processing Thread:]\tNew log entry created: %s %s\n", entry->getKeyString().c_str(), entry->getValueString().c_str());
            plexus->addToLogQueue(entry);
            //cache->print();
        } else if (message->getMessageType() == MSG_PLEXUS_PUT_REPLY) {

            MessagePUT_REPLY *msg = (MessagePUT_REPLY*) message;
            //msg->message_print_dump();
            OverlayID srcID(msg->getSrcOid().GetOverlay_id(), msg->getSrcOid().GetPrefix_length(), msg->getSrcOid().MAX_LENGTH);

            HostAddress ha(msg->getSourceHost(), msg->getSourcePort());
            cache->add(srcID, ha);
            Log* p_log = plexus->getPutLog();

            char i_str[300];
            sprintf(i_str, "%s_%s_%d", msg->getSourceHost().c_str(), msg->getDestHost().c_str(),
                    msg->getSequenceNo());

            string key = i_str;

            int hash_name_to_publish = atoi(msg->getDeviceName().c_str());
            MessageStateIndex msg_index(hash_name_to_publish, msg->getOriginSeqNo());

            double start = 0.0;
            plexus->getUnresolvedPut().lookup(msg_index, &start);
            plexus->getUnresolvedPut().remove(msg_index);

            double latency = msg->getResolutionLatency();
            int ip_hops = msg->getResolutionIpHops();

            LogEntry *entry = new LogEntry(LOG_PUT, key.c_str(), "iidsi",
                    msg->getResolutionHops(), ip_hops, latency, msg->getDeviceName().c_str(),
                    msg->getSrcOid().GetOverlay_id());

            printf("[Processing Thread:]\tNew log entry created: %s %s\n", entry->getKeyString().c_str(), entry->getValueString().c_str());
            plexus->addToLogQueue(entry);
            //cache->print();
        }//INIT Message
                else if (message->getMessageType() == MSG_PEER_INIT) {
                        PeerInitMessage* pInitMsg = (PeerInitMessage*) message;


                        container_peer->setNPeers(pInitMsg->getNPeers());
                        GlobalData::network_size = pInitMsg->getNPeers();
                        container_peer->setOverlayID(pInitMsg->getDstOid());
                        container_peer->setLogServerName(pInitMsg->getLogServerName());
                        container_peer->setLogServerUser(pInitMsg->getLogServerUser());
                        container_peer->setRunSequenceNo(pInitMsg->getRunSequenceNo());
                        container_peer->setK(pInitMsg->getK());
                        container_peer->setAlpha(pInitMsg->getAlpha());
                        container_peer->populate_addressdb();

                        container_peer->setPublish_name_range_start(pInitMsg->getPublish_name_range_start());
                        container_peer->setPublish_name_range_end(pInitMsg->getPublish_name_range_end());
                        container_peer->setLookup_name_range_start(pInitMsg->getLookup_name_range_start());
                        container_peer->setLookup_name_range_end(pInitMsg->getLookup_name_range_end());
                        container_peer->SetWebserverPort(pInitMsg->getWebserverPort());

                        container_protocol->setRoutingTable(&pInitMsg->getRoutingTable());
                        container_protocol->setIndexTable(new LookupTable<string, HostAddress > ());
                        container_protocol->setCache(new Cache(new CacheInsertEndpoint(), new CacheReplaceLRU(),
                                container_protocol->getRoutingTable(), plexus->getProactiveCache(), container_protocol->getContainerPeer()->getOverlayID(),
                                container_protocol->getContainerPeer()->getK()));

                        plexus->initLogs(container_peer->getRunSequenceNo(), container_peer->getLogServerName().c_str(), container_peer->getLogServerUser().c_str());

                        this->setup(container_protocol->getRoutingTable(), container_protocol->getIndexTable(), container_protocol->getCache());
                        plexus->reset_counters();

                        container_peer->SetInitRcvd(true);

                        /////////////////send cache_me message to all nbr/////

                        if (strcmp(container_peer->getCacheType().c_str(), "proactive") == 0) {
                                LookupTableIterator<OverlayID, HostAddress> rtable_iterator(container_protocol->getRoutingTable());
                                rtable_iterator.reset_iterator();
                                while (rtable_iterator.hasMoreKey()) {
                                        OverlayID dst_oid = rtable_iterator.getNextKey();
                                        HostAddress dst_ha;
                                        container_protocol->getRoutingTable()->lookup(dst_oid, &dst_ha);
                                        MessageCacheMe *msg = new MessageCacheMe(container_peer->getHostName(), container_peer->getListenPortNumber(),
                                                dst_ha.GetHostName(), dst_ha.GetHostPort(), container_peer->getOverlayID(), dst_oid);
                                        msg->setOverlayTtl(2);
                                        plexus->addToOutgoingQueue(msg);
                                }
                        }

                } else if (message->getMessageType() == MSG_PEER_CONFIG) {
                        PeerConfigMessage* pConfMsg = (PeerConfigMessage*) message;
                        container_peer->setAlpha(pConfMsg->getAlpha());
                        container_peer->setK(pConfMsg->getK());
                } else if (message->getMessageType() == MSG_PEER_INITIATE_GET) {
                        PeerInitiateGET* pInitGet = (PeerInitiateGET*) message;
                        printf("Processing peer init get msg, oid = %d\n",
                                pInitGet->getDstOid().GetOverlay_id());
                        container_protocol->get(pInitGet->getDeviceName());
                } else if (message->getMessageType() == MSG_PEER_INITIATE_PUT) {
                        PeerInitiatePUT* pInitPut = (PeerInitiatePUT*) message;
                        container_protocol->put(pInitPut->getDeviceName(), pInitPut->GetHostAddress());
                } else if (message->getMessageType() == MSG_PEER_START) {
                        container_peer->setStatus(PEER_RUNNING);
                } else if (message->getMessageType() == MSG_PEER_CHANGE_STATUS) {
                        PeerChangeStatusMessage* changeStatusMSG = (PeerChangeStatusMessage*) message;
                        container_protocol->getContainerPeer()->setStatus(changeStatusMSG->getPeer_status());
                } else if (message->getMessageType() == MSG_START_GENERATE_NAME) {
                		puts("Gen name start");
                        container_peer->SetStart_gen_name_rcvd(true);
                } else if (message->getMessageType() == MSG_START_LOOKUP_NAME) {
                        container_peer->SetStart_lookup__name_rcvd(true);
                } else if (message->getMessageType() == MSG_DYN_CHANGE_STATUS) {
                        PeerDynChangeStatusMessage* dcsMsg = (PeerDynChangeStatusMessage*) message;
                        container_peer->SetDyn_status(dcsMsg->getDynStatus());
                } else if (message->getMessageType() == MSG_CACHE_ME) {
					if (strcmp(container_peer->getCacheType().c_str(), "proactive") == 0) {
						printf("Cache me received from %s:%d\n", message->getSourceHost().c_str(), message->getSourcePort());

						MessageCacheMe* cache_msg = (MessageCacheMe*) message;
						OverlayID oid = cache_msg->getSrcOid();
						HostAddress ha(cache_msg->getSourceHost(), cache_msg->getSourcePort());
						HostAddress dummy;
						//cache->add(oid, ha);
						if(!(oid == container_peer->getOverlayID()) && !plexus->getRoutingTable()->lookup(oid, &dummy))
							plexus->getProactiveCache()->add(oid, ha);

						message->message_print_dump();

						if(message->getOverlayTtl() > 0)
						{
							LookupTableIterator<OverlayID, HostAddress> rtable_iterator(container_protocol->getRoutingTable());
							rtable_iterator.reset_iterator();
							while (rtable_iterator.hasMoreKey()) {
									OverlayID dst_oid = rtable_iterator.getNextKey();
									HostAddress dst_ha;
									container_protocol->getRoutingTable()->lookup(dst_oid, &dst_ha);

									MessageCacheMe *msg = new MessageCacheMe(message->getSourceHost(), message->getSourcePort(),
											dst_ha.GetHostName(), dst_ha.GetHostPort(), message->getSrcOid(), dst_oid);
									msg->setOverlayTtl(message->getOverlayTtl());
									plexus->addToOutgoingQueue(msg);
							}
						}
					}
                } else if (message->getMessageType() == MSG_PEER_FORCE_LOG) {
                        LogEntry* entry = new LogEntry(ALL_LOGS, "flush", "");
                        plexus->addToLogQueue(entry);
                } else {
                        puts("unknown message type in processMessage");
                        exit(1);
                }

                return false;
        }

};

#endif	/* PLEXUS_MESSAGE_PROCESSOR_H */

