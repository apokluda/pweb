/*
 * log_force_message.h
 *
 *  Created on: Jan 8, 2013
 *      Author: sr2chowd
 */

#ifndef LOG_FORCE_MESSAGE_H_
#define LOG_FORCE_MESSAGE_H_

#include "../message.h"

class LogForceMessage: public ABSMessage
{
public:

	LogForceMessage():ABSMessage(MSG_PEER_FORCE_LOG)
	{
	}

	LogForceMessage(string source_host, int source_port, string dest_host,
			int dest_port, OverlayID src_oid, OverlayID dst_id) :
			ABSMessage(MSG_PEER_FORCE_LOG, source_host, source_port, dest_host,
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


#endif /* LOG_FORCE_MESSAGE_H_ */
