/* 
 * File:   message_get_reply.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 4:31 PM
 */

#ifndef MESSAGE_GET_REPLY_H
#define	MESSAGE_GET_REPLY_H

#include "../message.h"
#include "../../ds/overlay_id.h"
#include "../../ds/ip_address.h"

#include <cstring>

class MessageGET_REPLY: public ABSMessage
{
	int resolution_status;
	int resolution_hops;
	int resolution_ip_hops;
	double resolution_latency;
	int origin_seq_no;
	OverlayID target_oid;
	HostAddress host_address;
	string device_name;

public:

	MessageGET_REPLY()
	{
		;
	}

	MessageGET_REPLY(string source_host, int source_port, string dest_host,
			int dest_port, OverlayID src_oid, OverlayID dst_id, int status,
			OverlayID target_oid,
			HostAddress h_address, string device_name) :
			ABSMessage(MSG_PLEXUS_GET_REPLY, source_host, source_port,
					dest_host, dest_port, src_oid, dst_id)
	{
		resolution_status = status;
		host_address = h_address;
		this->device_name = device_name;
		this->target_oid = target_oid;
	}

	size_t getSize()
	{
		int ret = getBaseSize();
		ret += sizeof(int) * 10
				+ sizeof(char) * host_address.GetHostName().size()
				+ sizeof(char) * device_name.size()
				+ sizeof(double);
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
		memcpy(buffer + offset, (char*) &resolution_status, sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*) &resolution_hops, sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*) &resolution_ip_hops, sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*) &resolution_latency, sizeof(double));
		offset += sizeof(double);

		memcpy(buffer + offset, (char*) &origin_seq_no, sizeof(int));
		offset += sizeof(int);

		int o_id = target_oid.GetOverlay_id();
		int p_len = target_oid.GetPrefix_length();
		int m_len = target_oid.MAX_LENGTH;

		memcpy(buffer + offset, (char*) &o_id, sizeof (int));
		offset += sizeof (int);
		memcpy(buffer + offset, (char*) &p_len, sizeof (int));
		offset += sizeof (int);
		memcpy(buffer + offset, (char*) &m_len, sizeof (int));
		offset += sizeof (int);

		int destHostLength = host_address.GetHostName().size();
		memcpy(buffer + offset, (char*) &destHostLength, sizeof(int));
		offset += sizeof(int);

		const char* haddr_str = host_address.GetHostName().c_str();

		for (int i = 0; i < destHostLength; i++)
		{
			memcpy(buffer + offset, (char*)(haddr_str + i), sizeof(char));
			offset += sizeof(char);
		}

		int port = host_address.GetHostPort();
		memcpy(buffer + offset, (char*) &port, sizeof(int));
		offset += sizeof(int);

		int deviceNameLength = device_name.size();
		memcpy(buffer + offset, (char*)&deviceNameLength, sizeof(int)); offset += sizeof(int);


		const char* d_name_str = device_name.c_str();
		for(int i = 0; i < deviceNameLength; i++)
		{
			memcpy(buffer + offset, (char*)(d_name_str + i), sizeof(char)); offset += sizeof(char);
		}

		//delete[] parent_buffer;
		return buffer;

	}

	ABSMessage* deserialize(char* buffer, int buffer_length)
	{
		ABSMessage::deserialize(buffer, buffer_length);
		int offset = getBaseSize();

		memcpy(&resolution_status, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&resolution_hops, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&resolution_ip_hops, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&resolution_latency, buffer + offset, sizeof(double));
		offset += sizeof(double);

		memcpy(&origin_seq_no, buffer + offset, sizeof(int));
		offset += sizeof(int);
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

		int hostLength = 0;
		memcpy(&hostLength, buffer + offset, sizeof(int));
		offset += sizeof(int);

		string host = "";
		for (int i = 0; i < hostLength; i++)
		{
			char ch;
			memcpy(&ch, buffer + offset, sizeof(int));
			offset += sizeof(char);
			host += ch;
		}

		host_address.SetHostName(host);
		int port = 0;
		memcpy(&port, buffer + offset, sizeof(int));
		offset += sizeof(int);
		host_address.SetHostPort(port);

		int deviceNameLength = 0;
		memcpy(&deviceNameLength, buffer + offset, sizeof(int)); offset += sizeof(int);

		device_name = "";
		for(int i = 0; i < deviceNameLength; i++)
		{
			char ch;
			memcpy(&ch, buffer + offset, sizeof(char)); offset += sizeof(char);
			device_name += ch;
		}

		return this;
	}

	void setHostAddress(HostAddress& h_address)
	{
		host_address = h_address;
	}

	HostAddress getHostAddress() const
	{
		return host_address;
	}

	int getStatus()
	{
		return resolution_status;
	}

	void setStatus(int status)
	{
		resolution_status = status;
	}

	int getResolutionHops() const
	{
		return resolution_hops;
	}

	void setResolutionHops(int hops)
	{
		resolution_hops = hops;
	}

	string getDeviceName()
	{
		return device_name;
	}

	void setDeviceName(string d_name)
	{
		device_name = d_name;
	}

	int getOriginSeqNo() const
	{
		return origin_seq_no;
	}

	void setOriginSeqNo(int origin_seq_no)
	{
		this->origin_seq_no = origin_seq_no;
	}

	int getResolutionIpHops() const
	{
		return resolution_ip_hops;
	}

	void setResolutionIpHops(int resolutionIpHops)
	{
		resolution_ip_hops = resolutionIpHops;
	}

	double getResolutionLatency() const
	{
		return resolution_latency;
	}

	void setResolutionLatency(double resolutionLatency)
	{
		resolution_latency = resolutionLatency;
	}

	OverlayID getTargetOid()
	{
		return target_oid;
	}

	void setTargetOid(OverlayID oid)
	{
		target_oid = oid;
	}
};

#endif	/* MESSAGE_GET_REPLY_H */

