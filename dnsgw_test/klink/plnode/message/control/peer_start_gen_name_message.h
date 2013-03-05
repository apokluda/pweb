#ifndef PEER_GEN_NAME_MESSAGE_H
#define PEER_GEN_NAME_MESSAGE_H

#include "../message.h"

#include <cstring>

class PeerStartGenNameMessage: public ABSMessage
{
public:

	PeerStartGenNameMessage() : ABSMessage(MSG_START_GENERATE_NAME)
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
