/* 
 * File:   message_put.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 3:17 PM
 */

#ifndef MESSAGE_PUT_H
#define	MESSAGE_PUT_H

#include "../message.h"
#include "../../ds/host_address.h"

#include <cstring>

class MessagePUT: public ABSMessage
{
	OverlayID target_oid;
	string deviceName;
	HostAddress hostAddress;
public:
	MessagePUT()
	{
	}

	MessagePUT(string source_host, int source_port, string dest_host,
			int dest_port, OverlayID src_oid, OverlayID dst_id, OverlayID target_oid, 
			string &deviceName, HostAddress &hostAddress) :
			ABSMessage(MSG_PLEXUS_PUT, source_host, source_port, dest_host,
					dest_port, src_oid, dst_id), deviceName(deviceName), hostAddress(
					hostAddress)
	{
		this->target_oid = target_oid;
	}

	size_t getSize()
	{
		size_t ret = getBaseSize();
		ret += sizeof(int) * 4 + sizeof(char) * deviceName.size();
		ret += sizeof(int) * 2 + sizeof(char) * hostAddress.GetHostName().size();
		return ret;
	}

	char* serialize(int* serialize_length)
	{
		int parent_length = 0;
		char* parent_buffer = ABSMessage::serialize(&parent_length);
		*serialize_length = getSize();

		char* buffer = new char[*serialize_length];
		int offset = 0;

		memcpy(buffer + offset, parent_buffer, parent_length);
		offset += parent_length;

		int o_id = target_oid.GetOverlay_id();
		int p_len = target_oid.GetPrefix_length();
		int m_len = target_oid.MAX_LENGTH;

		memcpy(buffer + offset, (char*) &o_id, sizeof (int));
		offset += sizeof (int);
		memcpy(buffer + offset, (char*) &p_len, sizeof (int));
		offset += sizeof (int);
		memcpy(buffer + offset, (char*) &m_len, sizeof (int));
		offset += sizeof (int);
		
		
		int deviceNameLength = deviceName.size();
		memcpy(buffer + offset, (char*) (&deviceNameLength), sizeof(int));
		offset += sizeof(int);

		for (int i = 0; i < deviceNameLength; i++)
		{
			char ch = deviceName[i];
			memcpy(buffer + offset, (char*) (&ch), sizeof(char));
			offset += sizeof(char);
		}

		int hostAddrNameLength = hostAddress.GetHostName().size();
		memcpy(buffer + offset, (char*) &hostAddrNameLength, sizeof(int));
		offset += sizeof(int);
		for (int i = 0; i < hostAddrNameLength; i++)
		{
			char ch = hostAddress.GetHostName()[i];
			memcpy(buffer + offset, (char*) (&ch), sizeof(char));
			offset += sizeof(char);
		}
		int hostPort = hostAddress.GetHostPort();
		memcpy(buffer + offset, (char*) &hostPort, sizeof(int));
		offset += sizeof(int);

		return buffer;
	}

	ABSMessage* deserialize(char* buffer, int buffer_length)
	{
		ABSMessage::deserialize(buffer, buffer_length);
		int offset = getBaseSize();

		int deviceNameLength = 0, hostAddrNameLength = 0;
		int o_id, p_len, m_len;

		memcpy(&o_id, buffer + offset, sizeof (int));
		offset += sizeof (int); //printf("offset = %d\n", offset);
		memcpy(&p_len, buffer + offset, sizeof (int));
		offset += sizeof (int); //printf("offset = %d\n", offset);
		memcpy(&m_len, buffer + offset, sizeof (int));
		offset += sizeof (int); //printf("offset = %d\n", offset);

		target_oid.SetOverlay_id(o_id);
		target_oid.SetPrefix_length(p_len);
		target_oid.MAX_LENGTH = m_len;
		

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

		memcpy(&hostAddrNameLength, buffer + offset, sizeof(int));
		offset += sizeof(int);
		string hostAddrName = "";
		for (int i = 0; i < hostAddrNameLength; i++)
		{
			char ch;
			memcpy(&ch, buffer + offset, sizeof(char));
			offset += sizeof(char); //printf("offset = %d\n", offset);
			hostAddrName += ch;
		}
		hostAddress.SetHostName(hostAddrName);
		int hPort = 0;
		memcpy(&hPort, buffer + offset, sizeof(int));
		offset += sizeof(int);
		hostAddress.SetHostPort(hPort);

		return this;
	}

	void SetHostAddress(HostAddress &hostAddress)
	{
		this->hostAddress = hostAddress;
	}

	HostAddress GetHostAddress() const
	{
		return hostAddress;
	}

	void SetDeviceName(string &deviceName)
	{
		this->deviceName = deviceName;
	}

	string GetDeviceName() const
	{
		return deviceName;
	}

	OverlayID getTargetOid()
	{
		return target_oid;
	}

	void setTargetOid(OverlayID oid)
	{
		target_oid = oid;
	}

	virtual void message_print_dump()
	{
		puts(
				"------------------------------<PUT Message>------------------------------");
		ABSMessage::message_print_dump();
		printf("Device Name %s\n", deviceName.c_str());
		printf("Host Address %s:%d\n", hostAddress.GetHostName().c_str(),
				hostAddress.GetHostPort());
		puts(
				"------------------------------</PUT Message>-----------------------------");
	}
};

#endif	/* MESSAGE_PUT_H */

