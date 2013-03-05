#ifndef PEER_START_MESSAGE_H
#define PEER_START_MESSAGE_H

#include "../message.h"

#include <cstring>

class PeerStartMessage: public ABSMessage
{
public:

	PeerStartMessage():ABSMessage(MSG_PEER_START)
	{
	}

	PeerStartMessage(string source_host, int source_port, string dest_host,
			int dest_port, OverlayID src_oid, OverlayID dst_id) :
			ABSMessage(MSG_PEER_START, source_host, source_port, dest_host,
					dest_port, src_oid, dst_id)
	{
	}

	size_t getSize()
	{
		size_t ret = getBaseSize();
		return ret;
	}

	virtual char* serialize(int* serialize_length)
	{
		int parent_length = 0;
		char* parent_buffer = ABSMessage::serialize(&parent_length);
		*serialize_length = parent_length;
		return parent_buffer;
	}

	virtual ABSMessage* deserialize(char* buffer, int buffer_length)
	{
		ABSMessage::deserialize(buffer, buffer_length);
		return this;
	}
};

#endif
