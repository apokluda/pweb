/*
 * peer_initiate_put.h
 *
 *  Created on: 2012-12-17
 *      Author: sr2chowd
 */

#ifndef PEER_INIT_PUT_H
#define PEER_INIT_PUT_H

#include "../message.h"

#include <cstring>

using namespace std;

class PeerInitiatePUT: public ABSMessage
{
	string device_name;
	HostAddress hostAddress;
public:

	PeerInitiatePUT()
	{
	}

	PeerInitiatePUT(string source_host, int source_port, string dest_host,
			int dest_port, OverlayID src_oid, OverlayID dst_id,
			string &device_name, HostAddress &hostAddress) :
			ABSMessage(MSG_PEER_INITIATE_PUT, source_host, source_port,
					dest_host, dest_port, src_oid, dst_id)
	{
		this->device_name = device_name;
		this->hostAddress = hostAddress;
	}

	void setDeviceName(string device_name)
	{
		this->device_name = device_name;
	}

	string getDeviceName()
	{
		return device_name;
	}

	void SetHostAddress(HostAddress &hostAddress)
	{
		this->hostAddress = hostAddress;
	}

	HostAddress GetHostAddress() const
	{
		return hostAddress;
	}

	size_t getSize()
	{
		size_t ret = getBaseSize();
		ret += sizeof(int) + sizeof(char) * device_name.size();
		ret += sizeof(int) + sizeof(char) * hostAddress.GetHostName().size();
		ret += sizeof(int);
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

		int deviceNameLength = device_name.size();
		memcpy(buffer + offset, (char*) &deviceNameLength, sizeof(int));
		offset += sizeof(int);
		for (int i = 0; i < device_name.size(); i++)
		{
			char ch = device_name[i];
			memcpy(buffer + offset, (char*) &ch, sizeof(char));
			offset += sizeof(char);
		}

		int hostAddressLength = hostAddress.GetHostName().size();
		memcpy(buffer + offset, (char*) &hostAddressLength, sizeof(int));
		offset += sizeof(int);
		for (int i = 0; i < hostAddressLength; i++)
		{
			char ch = hostAddress.GetHostName()[i];
			memcpy(buffer + offset, (char*) &ch, sizeof(char));
			offset += sizeof(char);
		}

		int hostPort = hostAddress.GetHostPort();
		memcpy(buffer + offset, (char*) &hostPort, sizeof(int));
		offset += sizeof(int);

		delete[] parent_buffer;
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

		int hostNameLength = 0;
		memcpy(&hostNameLength, buffer + offset, sizeof(int));
		offset += sizeof(int);
		string hostName = "";
		for (int i = 0; i < hostNameLength; i++)
		{
			char ch;
			memcpy(&ch, buffer + offset, sizeof(char));
			offset += sizeof(char);
			hostName += ch;
		}

		int hostPort = 0;
		memcpy(&hostPort, buffer + offset, sizeof(int));
		offset += sizeof(int);

		hostAddress = HostAddress(hostName, hostPort);

		return this;
	}
};

#endif

