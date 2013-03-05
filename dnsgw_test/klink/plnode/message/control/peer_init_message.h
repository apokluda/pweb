/*
 * peer_init_message.h
 *
 *  Created on: 2012-12-06
 *      Author: sr2chowd
 */

#ifndef PEER_INIT_MESSAGE_H_
#define PEER_INIT_MESSAGE_H_

#include "../../../plnode/ds/host_address.h"
#include "../../../plnode/ds/overlay_id.h"
#include "../../../plnode/ds/lookup_table.h"
#include "../../../plnode/ds/lookup_table_iterator.h"
#include "../message.h"
#include <memory.h>

/*
 * unsigned char messageType;
 unsigned int sequence_no;
 string dest_host;
 int dest_port;
 string source_host;
 int source_port;
 unsigned char overlay_hops;
 unsigned char overlay_ttl;
 OverlayID oID;
 */
class PeerInitMessage: public ABSMessage
{
	LookupTable<OverlayID, HostAddress> routing_table;
	int n_peers, k;
	double alpha;
	int publish_name_range_start, publish_name_range_end;
	int lookup_name_range_start, lookup_name_range_end;
    int webserver_port;

    int run_sequence_no;
    string log_server_name;
    string log_server_user;
	string peer_name;
public:
	PeerInitMessage()
	{
		messageType = MSG_PEER_INIT;
	}

	void setRoutingTable(LookupTable<OverlayID, HostAddress>& r_table)
	{
		routing_table = r_table;
	}

	LookupTable<OverlayID, HostAddress>& getRoutingTable()
	{
		return routing_table;
	}

	void setNPeers(int n)
	{
		n_peers = n;
	}
	int getNPeers()
	{
		return n_peers;
	}

	void setK(int k)
	{
		this->k = k;
	}
	int getK()
	{
		return k;
	}

	void setAlpha(double alpha)
	{
		this->alpha = alpha;
	}
	double getAlpha()
	{
		return alpha;
	}

	string get_peer_name()
	{
		return peer_name;
	}

	void set_peer_name(string name)
	{
		peer_name = name;
	}

	size_t getSize()
	{
		size_t ret = getBaseSize();
		ret += sizeof(int) * 5;
		ret += sizeof(double);
		ret += sizeof(int) * 4;
		ret += sizeof(int) * 3;
		ret += sizeof(char) * (log_server_name.size() + log_server_user.size() + peer_name.size());

		LookupTableIterator<OverlayID, HostAddress> r_iterator(&routing_table);
		r_iterator.reset_iterator();

		while (r_iterator.hasMoreKey())
		{
			OverlayID key = r_iterator.getNextKey();
			HostAddress value;
			routing_table.lookup(key, &value);
			ret += sizeof(int) * 5;
			ret += sizeof(char) * value.GetHostName().size();
		}
		return ret;
	}

	virtual char* serialize(int* serialize_length)
	{
		*serialize_length = getSize();
		LookupTableIterator<OverlayID, HostAddress> rtable_iterator(
				&routing_table);

		int offset = 0;
		int parent_size = 0;

		char* parent_buffer = ABSMessage::serialize(&parent_size);
		char* buffer = new char[*serialize_length];
		memcpy(buffer + offset, parent_buffer, parent_size);
		offset += parent_size;
		//delete[] parent_buffer;

		memcpy(buffer + offset, (char*)(&n_peers), sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*)(&k), sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*)(&alpha), sizeof(double));
		offset += sizeof(double);

		memcpy(buffer + offset, (char*)(&publish_name_range_start),sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*)(&publish_name_range_end), sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*)(&lookup_name_range_start), sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*)(&lookup_name_range_end), sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*)(&webserver_port), sizeof(int));
		offset += sizeof(int);

		memcpy(buffer + offset, (char*)&run_sequence_no, sizeof(int));
		offset += sizeof(int);

		int peer_name_length = peer_name.size();
		memcpy(buffer + offset, (char*)(&peer_name_length), sizeof(int));
		offset += sizeof(int);

		const char* str_peer_name = peer_name.c_str();		
		memcpy(buffer + offset, str_peer_name, peer_name_length);
		offset += peer_name_length;

		int logServerNameLength = log_server_name.size();
		memcpy(buffer + offset, (char*)&logServerNameLength, sizeof(int));
		offset += sizeof(int);

		const char* str_lg_server = log_server_name.c_str();
		memcpy(buffer + offset, str_lg_server, logServerNameLength);
		offset += logServerNameLength;

		int logServerUserLength = log_server_user.size();
		memcpy(buffer + offset, (char*)&logServerUserLength, sizeof(int));
		offset += sizeof(int);

		const char* str_lg_user = log_server_user.c_str();
		memcpy(buffer + offset, str_lg_user, logServerUserLength);
		offset += logServerUserLength;

		int routingTableSize = routing_table.size();
		memcpy(buffer + offset, (char*)(&routingTableSize), sizeof(int));
		offset += sizeof(int);

		rtable_iterator.reset_iterator();
		OverlayID key;
		HostAddress value;
		while(rtable_iterator.hasMoreKey())
		{
			key = rtable_iterator.getNextKey();

			routing_table.lookup(key, &value);
			int hostNameLength = value.GetHostName().size();

			int o_id = key.GetOverlay_id(), p_len = key.GetPrefix_length(), m_len = key.MAX_LENGTH;

			memcpy(buffer + offset, (char*)&o_id, sizeof(int)); offset += sizeof(int);
			memcpy(buffer + offset, (char*)&p_len, sizeof(int)); offset += sizeof(int);
			memcpy(buffer + offset, (char*)&m_len, sizeof(int)); offset += sizeof(int);

			memcpy(buffer + offset, (char*) (&hostNameLength), sizeof(int));
			offset += sizeof(int);
			const char* str = value.GetHostName().c_str();

			for (int i = 0; i < hostNameLength; i++)
			{
				memcpy(buffer + offset, (char*)(str + i), sizeof(char));
				offset += sizeof(char);
			}

			int hostport = value.GetHostPort();
			memcpy(buffer + offset, (char*) (&hostport), sizeof(int));
			offset += sizeof(int);
		}
		printf("INIT serialized\n");
		return buffer;
	}

	virtual ABSMessage* deserialize(char* buffer, int buffer_length)
	{
		int offset = 0;
		routing_table.clear();

		ABSMessage::deserialize(buffer, buffer_length);
		offset = getBaseSize(); //printf("offset = %d\n", offset);

		memcpy(&n_peers, buffer + offset, sizeof(int));
		offset += sizeof(int);
		//printf("offset = %d\n", offset);

		memcpy(&k, buffer + offset, sizeof(int));
		offset += sizeof(int);
		//printf("offset = %d\n", offset);

		memcpy(&alpha, buffer + offset, sizeof(double));
		offset += sizeof(double);
		//printf("offset = %d\n", offset);

		memcpy(&publish_name_range_start, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&publish_name_range_end, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&lookup_name_range_start, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&lookup_name_range_end, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&webserver_port, buffer + offset, sizeof(int));
		offset += sizeof(int);

		memcpy(&run_sequence_no, buffer + offset, sizeof(int));
		offset += sizeof(int);
		
		int peer_name_length;
		memcpy(&peer_name_length, buffer + offset, sizeof(int));
		offset += sizeof(int);

		char* p_name = new char[peer_name_length + 1];
		memcpy(p_name, buffer + offset, peer_name_length);
		offset += peer_name_length;
		p_name[peer_name_length] = '\0';
		peer_name = string(p_name);
		delete[] p_name;

		int logNameLength, logUserLength;
		memcpy(&logNameLength, buffer + offset, sizeof(int));
		offset += sizeof(int);

		char* str_lg_name = new char[logNameLength + 1];
		memcpy(str_lg_name, buffer + offset, logNameLength);
		offset += logNameLength;
		log_server_name = string(str_lg_name);
		delete[] str_lg_name;

		memcpy(&logUserLength, buffer + offset, sizeof(int));
		offset += sizeof(int);
		char* str_lg_user = new char[logUserLength + 1];
		memcpy(str_lg_user, buffer + offset, logUserLength);
		offset += logUserLength;
		log_server_user = string(str_lg_user);
		delete[] str_lg_user;

		int routingTableSize;
		memcpy(&routingTableSize, buffer + offset, sizeof(int));
		offset += sizeof(int); printf("offset = %d\n", offset);
		//printf("%d\n", routingTableSize);

		for (int i = 0; i < routingTableSize; i++)
		{
			OverlayID key;
			HostAddress value;
			int hostNameLength;
			string hostname;
			int hostport;

			int o_id, p_len, m_len;
			memcpy(&o_id, buffer + offset, sizeof(int)); offset += sizeof(int);
			memcpy(&p_len, buffer + offset, sizeof(int)); offset += sizeof(int);
			memcpy(&m_len, buffer + offset, sizeof(int)); offset += sizeof(int);

			key.SetOverlay_id(o_id);
			key.SetPrefix_length(p_len);
			key.MAX_LENGTH = m_len;

			memcpy(&hostNameLength, buffer + offset, sizeof(int));
			offset += sizeof(int); printf("offset = %d\n", offset);
			char ch;

			for (int i = 0; i < hostNameLength; i++)
			{
				memcpy(&ch, buffer + offset, sizeof(char));
				hostname += ch;
				offset += sizeof(char); //printf("offset = %d\n", offset);
			}

			memcpy(&hostport, buffer + offset, sizeof(int));
			offset += sizeof(int); //printf("offset = %d\n", offset);
			value.SetHostName(hostname);
			value.SetHostPort(hostport);
			routing_table.add(key, value);
		}

		return this;
	}

	virtual void message_print_dump()
	{
		puts("<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>");
		printf("Init Message\n");
		ABSMessage::message_print_dump();
		LookupTableIterator<OverlayID, HostAddress> rtable_iterator(
				&routing_table);
		//routing_table.reset_iterator();
		rtable_iterator.reset_iterator();

		while (rtable_iterator.hasMoreKey())
		//while(routing_table.hasMoreKey())
		{
			OverlayID key = rtable_iterator.getNextKey();
			//OverlayID key = routing_table.getNextKey();
			HostAddress value;
			routing_table.lookup(key, &value);
			printf("Overlay ID = %d, Hostname = %s, Host Port = %d\n",
					key.GetOverlay_id(), value.GetHostName().c_str(),
					value.GetHostPort());
		}
		printf("Peer Name = %s\n", peer_name.c_str());
		printf("N Peers = %d\n", n_peers);
		printf("Alpha = %.4lf\n", alpha);
		printf("K = %d\n", k);
		printf("Publish Start = %d End = %d\n", publish_name_range_start,
				publish_name_range_end);
		printf("Lookup Start = %d End = %d\n", lookup_name_range_start,
				lookup_name_range_end);
		printf("Webserver Port = %d\n", webserver_port);
		printf("Log Server = %s\n", log_server_name.c_str());
		printf("Log Server User = %s\n", log_server_user.c_str());
		puts("<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>");
	}

	void setLookup_name_range_end(int lookup_name_range_end)
	{
		this->lookup_name_range_end = lookup_name_range_end;
	}

	int getLookup_name_range_end() const
	{
		return lookup_name_range_end;
	}

	void setLookup_name_range_start(int lookup_name_range_start)
	{
		this->lookup_name_range_start = lookup_name_range_start;
	}

	int getLookup_name_range_start() const
	{
		return lookup_name_range_start;
	}

	void setPublish_name_range_end(int publish_name_range_end)
	{
		this->publish_name_range_end = publish_name_range_end;
	}

	int getPublish_name_range_end() const
	{
		return publish_name_range_end;
	}

	void setPublish_name_range_start(int publish_name_range_start)
	{
		this->publish_name_range_start = publish_name_range_start;
	}

	int getPublish_name_range_start() const
	{
		return publish_name_range_start;
	}

	void setWebserverPort(int webserver_port) {
		this->webserver_port = webserver_port;
	}

	int getWebserverPort() const {
		return webserver_port;
	}

	string getLogServerName() const
	{
		return log_server_name;
	}

	void setLogServerName(const string& logServerName)
	{
		log_server_name = logServerName;
	}

	string getLogServerUser() const
	{
		return log_server_user;
	}

	void setLogServerUser(const string& logServerUser)
	{
		log_server_user = logServerUser;
	}

	int getRunSequenceNo() const
	{
		return run_sequence_no;
	}

	void setRunSequenceNo(int runSequenceNo)
	{
		run_sequence_no = runSequenceNo;
	}
};

#endif /* PEER_INIT_MESSAGE_H_ */
