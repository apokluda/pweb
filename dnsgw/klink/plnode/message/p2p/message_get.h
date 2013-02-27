/* 
 * File:   message_get.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 4:31 PM
 */

#ifndef MESSAGE_GET_H
#define	MESSAGE_GET_H

#include "../message.h"

#include <cstring>

class MessageGET: public ABSMessage
{
	OverlayID target_oid;
	string deviceName;

public:
	MessageGET()
	{
	}

	MessageGET(string source_host, int source_port, string dest_host,
			int dest_port, OverlayID src_oid, OverlayID dst_id,
			OverlayID target_oid,
			string &deviceName) :
			ABSMessage(MSG_PLEXUS_GET, source_host, source_port, dest_host,
					dest_port, src_oid, dst_id), deviceName(deviceName)
	{
		this->target_oid = target_oid;
	}

	size_t getSize()
	{
		int ret = getBaseSize();
		ret += sizeof(int) * 4;
		ret += sizeof(char) * deviceName.size();
	}

	virtual char* serialize(int* serialize_length)
	{
		*serialize_length = getSize();

		//*serialize_length = getBaseSize();
		//*serialize_length += (sizeof (int) + sizeof (char) * deviceName.size());
		int parent_size = 0;
		char* buffer = new char[*serialize_length];
		char* parent_buffer = ABSMessage::serialize(&parent_size);

		int offset = 0;
		memcpy(buffer + offset, parent_buffer, parent_size);
		offset += parent_size;

		/*int destHostLength = dest_host.size();
		 int sourceHostLength = source_host.size();

		 memcpy(buffer + offset, (char*) (&messageType), sizeof (char));
		 offset += sizeof (char);
		 memcpy(buffer + offset, (char*) (&sequence_no), sizeof (int));
		 offset += sizeof (int);

		 memcpy(buffer + offset, (char*) (&destHostLength), sizeof (int));
		 offset += sizeof (int);
		 for (int i = 0; i < destHostLength; i++) {
		 char ch = dest_host[i];
		 memcpy(buffer + offset, (char*) (&ch), sizeof (char));
		 offset += sizeof (char);
		 }
		 memcpy(buffer + offset, (char*) (&dest_port), sizeof (int));
		 offset += sizeof (int);

		 memcpy(buffer + offset, (char*) (&sourceHostLength), sizeof (int));
		 offset += sizeof (int);
		 for (int i = 0; i < sourceHostLength; i++) {
		 char ch = source_host[i];
		 memcpy(buffer + offset, (char*) (&ch), sizeof (char));
		 offset += sizeof (char);
		 }
		 memcpy(buffer + offset, (char*) (&source_port), sizeof (int));
		 offset += sizeof (int);

		 memcpy(buffer + offset, (char*) (&overlay_hops), sizeof (char));
		 offset += sizeof (char);
		 memcpy(buffer + offset, (char*) (&overlay_ttl), sizeof (char));
		 offset += sizeof (char);

		 memcpy(buffer + offset, (char*) (&dst_oid), sizeof (OverlayID));
		 offset += sizeof (OverlayID);

		 memcpy(buffer + offset, (char*) (&src_oid), sizeof (OverlayID));
		 offset += sizeof (OverlayID);*/
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

		/*memcpy(&messageType, buffer + offset, sizeof (char));
		 offset += sizeof (char); //printf("offset = %d\n", offset);
		 memcpy(&sequence_no, buffer + offset, sizeof (int));
		 offset += sizeof (int); //printf("offset = %d\n", offset);
		 memcpy(&destHostLength, buffer + offset, sizeof (int));
		 offset += sizeof (int); //printf("offset = %d\n", offset);
		 dest_host = "";
		 //printf("DH Length : %d\n", destHostLength);
		 for (int i = 0; i < destHostLength; i++) {
		 char ch;
		 memcpy(&ch, buffer + offset, sizeof (char));
		 offset += sizeof (char); //printf("offset = %d\n", offset);
		 dest_host += ch;
		 }
		 memcpy(&dest_port, buffer + offset, sizeof (int));
		 offset += sizeof (int); //printf("offset = %d\n", offset);
		 memcpy(&sourceHostLength, buffer + offset, sizeof (int));
		 offset += sizeof (int); //printf("offset = %d\n", offset);
		 source_host = "";
		 for (int i = 0; i < sourceHostLength; i++) {
		 char ch;
		 memcpy(&ch, buffer + offset, sizeof (char));
		 offset += sizeof (char); //printf("offset = %d\n", offset);
		 source_host += ch;
		 }
		 memcpy(&source_port, buffer + offset, sizeof (int));
		 offset += sizeof (int); //printf("offset = %d\n", offset);
		 memcpy(&overlay_hops, buffer + offset, sizeof (char));
		 offset += sizeof (char); //printf("offset = %d\n", offset);
		 memcpy(&overlay_ttl, buffer + offset, sizeof (char));
		 offset += sizeof (char); //printf("offset = %d\n", offset);

		 memcpy(&dst_oid, buffer + offset, sizeof (OverlayID));
		 offset += sizeof (OverlayID); //printf("offset = %d\n", offset);

		 memcpy(&src_oid, buffer + offset, sizeof (OverlayID));
		 offset += sizeof (OverlayID); //printf("offset = %d\n", offset);*/
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
		printf("Get Message");
		ABSMessage::message_print_dump();
		printf("Device Name %s\n", deviceName.c_str());
	}
};

#endif	/* MESSAGE_GET_H */

