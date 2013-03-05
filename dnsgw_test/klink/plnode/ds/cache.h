/* 
 * File:   Cache.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 10:06 AM
 */

#ifndef CACHE_H
#define	CACHE_H

#include <iostream>
#include <cstdlib>
using namespace std;

#include "double_linked_list.h"
#include "lookup_table.h"
#include "cache_insert_policy.h"
#include "cache_replace_policy.h"
#include "overlay_id.h"
#include "host_address.h"

class Cache
{
public:
	DoublyLinkedList *dll;
	LookupTable<OverlayID, DLLNode*> *hm;
	CacheInsertPolicy *cinPolicy;
	CacheReplacePolicy *crPolicy;
	int capacity;
	int size;
	DLLNode *current;

	pthread_rwlock_t cache_lock;

//	Cache()
//	{
//		dll = new DoublyLinkedList();
//		hm = new LookupTable<OverlayID, DLLNode*>();
//		size = 0;
//		current = dll->getHead();
//	}

	Cache(CacheInsertPolicy *cinPolicy, CacheReplacePolicy *crPolicy,
			LookupTable<OverlayID, HostAddress>* rt, LookupTable <OverlayID, HostAddress>* pc,
			OverlayID oid, int capacity)
	{
		dll = new DoublyLinkedList();
		hm = new LookupTable<OverlayID, DLLNode*>();
		this->cinPolicy = cinPolicy;
		this->cinPolicy->setup(dll, hm, rt, pc, oid, &size);
		this->crPolicy = crPolicy;
		this->crPolicy->setup(dll, hm);
		this->capacity = capacity;
		size = 0;
		current = dll->getHead();
		pthread_rwlock_init(&cache_lock, NULL);
	}

	/*void reset_iterator()
	{
		current = dll->getHead();
	}

	bool has_next()
	{
		if (current == NULL)
			return false;

		return current->next != NULL;
	}

	DLLNode* get_next()
	{
		if (current == NULL)
			return NULL;

		current = current->next;
		return current->prev;
	}*/

	void add(OverlayID &key, HostAddress &value)
	{
		pthread_rwlock_wrlock(&cache_lock);
		if (size == capacity && !crPolicy->processHit(key))
		{
			crPolicy->evict();
            size--;
		}
		cinPolicy->insert(key, value);
		puts("*******************************************************");
		printf("Cache size %d\n", size);
		puts(toString());
		puts("*******************************************************");
		pthread_rwlock_unlock(&cache_lock);
	}

	bool lookup(OverlayID key, HostAddress &hostAddress)
	{
		pthread_rwlock_wrlock(&cache_lock);
		DLLNode *node;// = new DLLNode();
		if (hm->lookup(key, &node))
		{
			crPolicy->processHit(key);
			hostAddress = node->value;
			pthread_rwlock_unlock(&cache_lock);
			return true;
		}
		pthread_rwlock_unlock(&cache_lock);
		return false;
	}

	void print()
	{
		//pthread_rwlock_rdlock(&cache_lock);
		cout << size << endl;
		dll->printNodesForward();
		//pthread_rwlock_unlock(&cache_lock);
	}
        
	int getStringSize(){
		int ret;
		//pthread_rwlock_rdlock(&cache_lock);
		ret = dll->getStringSize();
		//pthread_rwlock_unlock(&cache_lock);
		return ret;
	}

	char* toString(){
		char* ret;
		//pthread_rwlock_rdlock(&cache_lock);
		ret = dll->toString();
		//pthread_rwlock_unlock(&cache_lock);
		return ret;
	}

	void setSize(int size)
	{
		pthread_rwlock_wrlock(&cache_lock);
		this->size = size;
		pthread_rwlock_unlock(&cache_lock);
	}

	int getSize()
	{
		int ret;
		pthread_rwlock_rdlock(&cache_lock);
		ret = size;
		pthread_rwlock_unlock(&cache_lock);
		return size;
	}

	DoublyLinkedList* getDLL() const
	{
		return dll;
	}
	~Cache()
	{
		pthread_rwlock_destroy(&cache_lock);
	}
};

#endif	/* CACHE_H */

