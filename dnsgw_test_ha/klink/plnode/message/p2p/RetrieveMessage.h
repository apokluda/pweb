/*
 * RetrieveMessage.h
 *
 *  Created on: 2013-03-11
 *      Author: sr2chowd
 */

#ifndef RETRIEVEMESSAGE_H_
#define RETRIEVEMESSAGE_H_

#include <string>
#include "../message.h"

using namespace std;

class RetrieveMessage: public ABSMessage
{
	string deviceName;

public:

	RetrieveMessage()
	{
		;
	}

	RetrieveMessage(string source_host, int source_port, string dest_host,
			int dest_port, OverlayID src_oid, OverlayID dst_id,
			OverlayID target_oid, string &deviceName) :
			ABSMessage(MSG_RETRIEVE, source_host, source_port, dest_host,
					dest_port, src_oid, dst_id), deviceName(deviceName)
	{
		;
	}

	size_t getSize()
	{
		int ret = getBaseSize();
		ret += sizeof(int) + deviceName.size() * sizeof(char);
		return ret;
	}

	virtual char* serialize(int* serialize_length)
	{
		*serialize_length = getSize();

		int parent_size = 0;
		char* buffer = new char[*serialize_length];
		char* parent_buffer = ABSMessage::serialize(&parent_size);

		int offset = 0;
		memcpy(buffer + offset, parent_buffer, parent_size);
		offset += parent_size;

		int deviceNameLength = deviceName.size();
		memcpy(buffer + offset, (char*) (&deviceNameLength), sizeof(int));
		offset += sizeof(int);

		for (int i = 0; i < deviceNameLength; i++)
		{
			char ch = deviceName[i];
			memcpy(buffer + offset, (char*) (&ch), sizeof(char));
			offset += sizeof(char);
		}

		delete[] parent_buffer;
		return buffer;
	}

	ABSMessage* deserialize(char* buffer, int buffer_length)
	{
		int offset = 0;
		int destHostLength, sourceHostLength, deviceNameLength,
				hostAddrNameLength;

		ABSMessage::deserialize(buffer, buffer_length);
		offset += getBaseSize();

		memcpy(&deviceNameLength, buffer + offset, sizeof(int));
		offset += sizeof(int);
		deviceName = "";
		for (int i = 0; i < deviceNameLength; i++)
		{
			char ch;
			memcpy(&ch, buffer + offset, sizeof(char));
			offset += sizeof(char); //printf("offset = %d\n", offset);
			deviceName += ch;
		}

		return this;
	}

	void SetDeviceName(string &deviceName)
	{
		this->deviceName = deviceName;
	}

	string GetDeviceName() const
	{
		return deviceName;
	}

};

#endif /* RETRIEVEMESSAGE_H_ */
