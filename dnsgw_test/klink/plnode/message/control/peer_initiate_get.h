/*
 * peer_initiate_get.h
 *
 *  Created on: 2012-12-17
 *      Author: sr2chowd
 */

#ifndef PEER_INIT_GET_H
#define PEER_INIT_GET_H

#include "../message.h"

class PeerInitiateGET: public ABSMessage
{
	string device_name;

public:
	PeerInitiateGET()
	{
	}

	PeerInitiateGET(string source_host, int source_port, string dest_host,
			int dest_port, OverlayID src_oid, OverlayID dst_id,
			string device_name) :
			ABSMessage(MSG_PEER_INITIATE_GET, source_host, source_port,
					dest_host, dest_port, src_oid, dst_id)
	{
		this->device_name = device_name;
	}

	void setDeviceName(string device_name)
	{
		this->device_name = device_name;
	}

	string getDeviceName()
	{
		return device_name;
	}

	size_t getSize()
	{
		size_t ret = getBaseSize() + sizeof(int)
				+ sizeof(char) * device_name.size();
		return ret;
	}

	virtual char* serialize(int* serialize_length)
	{

		*serialize_length = getSize();
		int parent_length;
		char* parent_buffer = ABSMessage::serialize(&parent_length);

		char* buffer = new char[*serialize_length];

		int offset = 0;
		memcpy(buffer + offset, (char*)parent_buffer, parent_length);
		offset += parent_length;

		int deviceNameLength = device_name.size();

		memcpy(buffer + offset, (char*) &deviceNameLength, sizeof(int));
		offset += sizeof(int);
		const char* str = device_name.c_str();

		for (int i = 0; i < deviceNameLength; i++)
		{
			memcpy(buffer + offset, (char*) (str + i), sizeof(char));
			offset += sizeof(char);
		}
		//delete[] parent_buffer;
		return buffer;
	}

	virtual ABSMessage* deserialize(char* buffer, int buffer_length)
	{
		ABSMessage::deserialize(buffer, buffer_length);
		int offset = getBaseSize();

		int deviceNameLength = 0;
		memcpy(&deviceNameLength, buffer + offset, sizeof(int));
		offset += sizeof(int);
		device_name = "";

		for (int i = 0; i < deviceNameLength; i++)
		{
			char ch;
			memcpy(&ch, buffer + offset, sizeof(char));
			offset += sizeof(char);
			device_name += ch;
		}

		return this;
	}

	virtual void message_print_dump()
	{
		puts(">>>>>>>>Peer Init Get<<<<<<<<<");
		ABSMessage::message_print_dump();
		puts(device_name.c_str());
		puts(">>>>>>>>>>>>>>><<<<<<<<<<<<<<<<");
	}
};

#endif

