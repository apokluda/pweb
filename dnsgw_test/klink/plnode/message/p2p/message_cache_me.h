/* 
 * File:   message_cache_me.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 3:17 PM
 */

#ifndef MESSAGE_CACHE_ME_H
#define	MESSAGE_CACHE_ME_H

#include "../message.h"
#include "../../ds/host_address.h"
#include "../../ds/overlay_id.h"

#include <cstring>

class MessageCacheMe : public ABSMessage {
public:

    MessageCacheMe() {
    }

    MessageCacheMe(string source_host, int source_port, string dest_host,
            int dest_port, OverlayID src_oid, OverlayID dst_id) :
    ABSMessage(MSG_CACHE_ME, source_host, source_port, dest_host,
    dest_port, src_oid, dst_id) {
        ;
    }

    size_t getSize() {
        size_t ret = getBaseSize();
        return ret;
    }

    virtual char* serialize(int* serialize_length) {
        int parent_length = 0;
        char* parent_buffer = ABSMessage::serialize(&parent_length);
        *serialize_length = parent_length;
        return parent_buffer;
    }

    virtual ABSMessage* deserialize(char* buffer, int buffer_length) {
        ABSMessage::deserialize(buffer, buffer_length);
        return this;
    }

    virtual void message_print_dump() {
        puts("------------------------------<CACHE_ME Message>------------------------------");
        ABSMessage::message_print_dump();
        puts("------------------------------</CACHE_ME Message>-----------------------------");
    }
};

#endif	/* MESSAGE_PUT_H */

