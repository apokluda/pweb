/* 
 * File:   message_processor.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 2:45 PM
 */

#ifndef MESSAGE_PROCESSOR_H
#define	MESSAGE_PROCESSOR_H

#include "../ds/cache.h"
#include "../ds/lookup_table.h"
#include "../ds/overlay_id.h"
#include "../ds/host_address.h"
#include "../../plnode/protocol/protocol.h"

class ABSProtocol;

class MessageProcessor
{
protected:
	LookupTable<OverlayID, HostAddress>* routing_table;
	LookupTable<string, HostAddress>* index_table;
	Cache* cache;
	ABSProtocol* container_protocol;

public:
	void setup(LookupTable<OverlayID, HostAddress>* routing_table,
			LookupTable<string, HostAddress>* index_table, Cache *cache)
	{
		this->routing_table = routing_table;
		this->index_table = index_table;
		this->cache = cache;
	}

	void setContainerProtocol(ABSProtocol* protocol)
	{
		container_protocol = protocol;
	}
	ABSProtocol* getContainerProtocol()
	{
		return container_protocol;
	}

	virtual bool processMessage(ABSMessage* message) = 0;

	virtual ~MessageProcessor()
	{
		routing_table = NULL;
		index_table = NULL;
		cache = NULL;
		container_protocol = NULL;
	}
};

#endif	/* MESSAGE_PROCESSOR_H */

