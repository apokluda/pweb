/*
 * MessageStateIndex.h
 *
 *  Created on: Jan 3, 2013
 *      Author: sr2chowd
 */

#ifndef MESSAGESTATEINDEX_H_
#define MESSAGESTATEINDEX_H_

class MessageStateIndex
{
	int name_hash;
	int message_seq_no;

public:

	MessageStateIndex()
	{
		name_hash = message_seq_no = -1;
	}

	MessageStateIndex(int name_hash, int message_seq_no):name_hash(name_hash), message_seq_no(message_seq_no){}

	void setNameHash(int name_hash)
	{
		this->name_hash = name_hash;
	}

	int getNameHash() const
	{
		return name_hash;
	}

	void setMessageSeqNo(int message_seq_no)
	{
		this->message_seq_no = message_seq_no;
	}

	int getMessageSeqNo() const
	{
		return message_seq_no;
	}

	bool operator < (const MessageStateIndex&  index) const
	{
		if(name_hash != index.name_hash)
			return name_hash < index.name_hash;

		return message_seq_no < index.message_seq_no;
	}

	bool operator == (const MessageStateIndex& index) const
	{
		return (name_hash == index.name_hash) && (message_seq_no == index.message_seq_no);
	}
};


#endif /* MESSAGESTATEINDEX_H_ */
