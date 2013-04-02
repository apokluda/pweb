/*
 * peer_config_msg.h
 *
 *  Created on: 2012-11-22
 *      Author: sr2chowd
 */

#ifndef PEER_CONFIG_MSG_H_
#define PEER_CONFIG_MSG_H_

#include "../message.h"
#include <memory.h>

class PeerConfigMessage: public ABSMessage
{
	int parameter_k;
	double parameter_alpha;

public:

	PeerConfigMessage() :
			ABSMessage()
	{
		messageType = MSG_PEER_CONFIG;
	}

	void setK(int k)
	{
		parameter_k = k;
	}

	int getK()
	{
		return parameter_k;
	}

	void setAlpha(double alpha)
	{
		parameter_alpha = alpha;
	}

	double getAlpha()
	{
		return parameter_alpha;
	}

	size_t getSize()
	{
		int ret = getBaseSize();
		ret += sizeof(int);
		ret += sizeof(double);
		return ret;
	}

	virtual char* serialize(int* serialize_length)
	{
		*serialize_length = getSize();

		int offset = 0;
		int parent_size = 0;

		char* parent_buffer = ABSMessage::serialize(&parent_size);
		char* buffer = new char[*serialize_length];

		memcpy(buffer + offset, parent_buffer, parent_size);
		offset += parent_size;
		memcpy(buffer + offset, (char*) (&parameter_k), sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, (char*) (&parameter_alpha), sizeof(double));
		offset += sizeof(double);

		delete[] parent_buffer;

		return buffer;
	}

	virtual ABSMessage* deserialize(char* buffer, int buffer_length)
	{
		int offset = 0;

		ABSMessage::deserialize(buffer, buffer_length);
		offset = getBaseSize();

		memcpy(&parameter_k, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&parameter_alpha, buffer + offset, sizeof(double));
		offset += sizeof(double);

		return this;
	}

	virtual void message_print_dump()
	{
		puts("<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>");
		printf("Configuration Message\n");
		ABSMessage::message_print_dump();
		printf("Parameter K = %d\n", parameter_k);
		printf("Parameter alpha = %.2lf\n", parameter_alpha);
		puts("<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>");
	}
};

#endif /* PEER_CONFIG_MSG_H_ */
