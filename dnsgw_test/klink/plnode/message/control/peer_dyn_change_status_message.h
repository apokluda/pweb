#ifndef PEER_DYN_CHANGE_STATUS_MESSAGE_H
#define PEER_DYN_CHANGE_STATUS_MESSAGE_H

#include "../message.h"

#include <cstring>

class PeerDynChangeStatusMessage: public ABSMessage
{
	int dyn_status;
public:

	PeerDynChangeStatusMessage()
	{
	}

	PeerDynChangeStatusMessage(string source_host, int source_port,
			string dest_host, int dest_port, OverlayID src_oid,
			OverlayID dst_id) :
			ABSMessage(MSG_DYN_CHANGE_STATUS, source_host, source_port,
					dest_host, dest_port, src_oid, dst_id)
	{
	}

	size_t getSize()
	{
		size_t ret = getBaseSize();
		return ret;
	}

	virtual char* serialize(int* serialize_length)
	{
		*serialize_length = getSize();
		int parent_length;
		char* parent_buffer = ABSMessage::serialize(&parent_length);
		char* buffer = new char[*serialize_length];

		int offset = 0;
		memcpy(buffer + offset, parent_buffer, parent_length);
		offset += parent_length;

		memcpy(buffer + offset, (char*) &dyn_status, sizeof(int));
		offset += sizeof(int);

		delete[] parent_buffer;
		return parent_buffer;
	}

	virtual ABSMessage* deserialize(char* buffer, int buffer_length)
	{
		ABSMessage::deserialize(buffer, buffer_length);
		int offset = getBaseSize();

		memcpy(&dyn_status, buffer + offset, sizeof(int));
		offset += sizeof(int);

		return this;
	}

	void setDynStatus(int peer_status)
	{
		this->dyn_status = dyn_status;
	}

	int getDynStatus() const
	{
		return dyn_status;
	}
};

#endif
