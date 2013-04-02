/* 
 * File:   cache_insert_policy.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 10:10 AM
 */

#ifndef CACHE_INSERT_POLICY_H
#define	CACHE_INSERT_POLICY_H

#include "overlay_id.h"
#include "host_address.h"
#include "double_linked_list.h"
#include "lookup_table.h"

class CacheInsertPolicy
{
protected:
	DoublyLinkedList *dll;
	LookupTable<OverlayID, DLLNode*> *hm;
	LookupTable<OverlayID, HostAddress>* rt;
	LookupTable<OverlayID, HostAddress> *pc;
	OverlayID myOID;
	int *cache_size;
public:

	CacheInsertPolicy()
	{

	}

	void setup(DoublyLinkedList *dll, LookupTable<OverlayID, DLLNode*> *hm,
			LookupTable<OverlayID, HostAddress>* rt,
			LookupTable<OverlayID, HostAddress> *pc,
			OverlayID myOID,
			int* cache_size)
	{
		this->dll = dll;
		this->hm = hm;
		this->rt = rt;
		this->pc = pc;
		this->myOID = myOID; printf("\n[Cache] myOID = %s\n", myOID.toString());
		this->cache_size = cache_size;
	}

	virtual void insert(OverlayID &key, HostAddress &value) = 0;
};

#endif	/* CACHE_INSERT_POLICY_H */

