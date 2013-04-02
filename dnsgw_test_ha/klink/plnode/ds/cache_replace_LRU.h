#ifndef CACHE_REPLACE_LRU_H
#define CACHE_REPLACE_LRU_H

#include <cstdlib>
#include "cache_replace_policy.h"

class CacheReplaceLRU: public CacheReplacePolicy
{
public:

	CacheReplaceLRU()
	{
	}

	void evict()
	{
		DLLNode *tail = dll->getTail();
		if (tail != NULL)
		{
			//puts("tail is not null");
                        OverlayID *key = &(tail->key);
			dll->removeTail();
			if (key != NULL)
			{
				hm->remove(*key);
			}
		}
	}

	bool processHit(OverlayID key)
	{
		DLLNode *node;
		if (hm->lookup(key, &node))
		{
			dll->move2Head(node);
                        printf("moving to head %s -- %s -- %s\n", key.toString(), node->key.toString(), node->value.toString());
                        return true;
		}
                return false;
	}

};

#endif