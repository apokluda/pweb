/* 
 * File:   cache_replace_policy.h
 * Author: mfbari
 *
 * Created on November 29, 2012, 10:11 AM
 */

#ifndef CACHE_REPLACE_POLICY_H
#define	CACHE_REPLACE_POLICY_H

#include "overlay_id.h"
#include "lookup_table.h"
#include "double_linked_list.h"

class CacheReplacePolicy
{
protected:
	DoublyLinkedList *dll;
	LookupTable<OverlayID, DLLNode*> *hm;
public:
	CacheReplacePolicy()
	{
	}
	void setup(DoublyLinkedList *dll, LookupTable<OverlayID, DLLNode*> *hm)
	{
		this->dll = dll;
		this->hm = hm;
	}
	virtual void evict() = 0;
	virtual bool processHit(OverlayID key) = 0;
};

#endif	/* CACHE_REPLACE_POLICY_H */

