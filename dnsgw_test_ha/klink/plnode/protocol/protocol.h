/*
 * protocol.h
 *
 *  Created on: 2012-11-21
 *      Author: sr2chowd
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include "../ds/lookup_table_iterator.h"
#include "../message/message.h"
#include "../message/message_processor.h"
#include "../ds/lookup_table.h"
#include "../ds/cache.h"
#include "../ds/overlay_id.h"
#include "../ds/host_address.h"
#include "../peer/peer.h"
#include "../../communication/client_socket.h"

class Peer;

class ABSProtocol
{
protected:
	LookupTable<OverlayID, HostAddress>* routing_table;
	LookupTable<string, HostAddress>* index_table;
	Peer* container_peer;
	Cache* cache;
	MessageProcessor* msgProcessor;

public:

	ABSProtocol()
	{
		this->routing_table = new LookupTable <OverlayID, HostAddress>();
		this->index_table = new LookupTable<string, HostAddress>();
		this->container_peer = NULL;
		this->cache = NULL;
		this->msgProcessor = NULL;
	}

	ABSProtocol(LookupTable<OverlayID, HostAddress>* routing_table,
			LookupTable<string, HostAddress>* index_table, Cache *cache,
			MessageProcessor* msgProcessor, Peer* container)
	{
		this->routing_table = routing_table;
		this->index_table = index_table;
		this->cache = cache;
		this->container_peer = container;
		msgProcessor->setup(routing_table, index_table, cache);
	}

	ABSProtocol(Peer* container, MessageProcessor* msgProcessor)
	{
		routing_table = new LookupTable <OverlayID, HostAddress>();
		index_table = new LookupTable<string, HostAddress>();
		this->msgProcessor = msgProcessor;
		//cache = new Cache();
		this->container_peer = container;
	}

	void setContainerPeer(Peer* peer)
	{
		container_peer = peer;
	}
	Peer* getContainerPeer()
	{
		return container_peer;
	}

	void setCache(Cache* cache)
	{
		this->cache = cache;
	}
	Cache* getCache()
	{
		return cache;
	}

	void setMessageProcess(MessageProcessor* processor)
	{
		msgProcessor = processor;
	}
	MessageProcessor* getMessageProcessor()
	{
		return msgProcessor;
	}

	void setRoutingTable(LookupTable<OverlayID, HostAddress>* table)
	{
		if (routing_table != NULL)
			delete routing_table;
		routing_table = new LookupTable<OverlayID, HostAddress>();
		*routing_table = *table;
	}

	LookupTable<OverlayID, HostAddress>* getRoutingTable()
	{
		return routing_table;
	}

	LookupTable<string, HostAddress>* getIndexTable()
	{
		return index_table;
	}

	void setIndexTable(LookupTable<string, HostAddress>* table)
	{
		if (index_table != NULL)
			delete index_table;
		index_table = new LookupTable<string, HostAddress>();
		*index_table = *table;
	}

	void printRoutingTable()
	{
		LookupTableIterator<OverlayID, HostAddress> rtable_iterator(
				routing_table);
		rtable_iterator.reset_iterator();

		while (rtable_iterator.hasMoreKey())
		{
			//OverlayID key = routing_table->getNextKey();
			OverlayID key = rtable_iterator.getNextKey();
			HostAddress value;
			routing_table->lookup(key, &value);
			printf("%d %s:%d\n", key.GetOverlay_id(), value.GetHostName().c_str(), value.GetHostPort());
		}
	}

	int send_message(ABSMessage* message)
	{
		int error_code = 0;

		ClientSocket c_socket(message->getDestHost(), message->getDestPort());
		HostAddress dest_address(message->getDestHost(), message->getDestPort());

		sockaddr_in server_info = (container_peer->lookup_address(dest_address)).first;
		c_socket.setServerInfo(server_info);

		error_code = c_socket.connect_to_server();

		if (error_code < 0)
			return error_code;

		char* buffer;
		int buffer_length;
		buffer = message->serialize(&buffer_length);

		timeval timeout;
		timeout.tv_sec = container_peer->getTimeoutSec();
		timeout.tv_usec = container_peer->getTimeoutMicroSec();

		error_code = c_socket.send_data(buffer, buffer_length, &timeout);
		c_socket.close_socket();

		delete[] buffer;
		return error_code;
	}

	virtual ~ABSProtocol()
	{
		delete routing_table;
		delete index_table;
		delete cache;
		delete msgProcessor;
		container_peer = NULL;
	}

	virtual void initiate_join() = 0;
	virtual void process_join() = 0;
	virtual bool setNextHop(ABSMessage*) = 0;
	virtual void get(string name) = 0;
	virtual void put(string name, HostAddress my_address) = 0;
	virtual void rejoin() = 0;
};

#endif /* PROTOCOL_H_ */
