/*
 * cache_iterator.h
 *
 *  Created on: Jan 9, 2013
 *      Author: sr2chowd
 */

#ifndef CACHE_ITERATOR_H_
#define CACHE_ITERATOR_H_

#include "cache.h"
#include "double_linked_list.h"

class CacheIterator
{
	Cache* cache_ptr;
	DLLNode* current;

public:
	CacheIterator()
	{
		cache_ptr = NULL;
		current = NULL;
	}

	CacheIterator(Cache* c)
	{
		cache_ptr = c;
		current = c->getDLL()->getHead();
	}

	void reset_iterator()
	{
		current = cache_ptr->getDLL()->getHead();
	}

	bool hasMore()
	{
		return current != NULL;
	}

	DLLNode* getNext()
	{
		if (current == NULL)
			return NULL;

		DLLNode* ret = current;
		current = current->next;
		return ret;
	}
};


#endif /* CACHE_ITERATOR_H_ */
