/*
 * lookup_table_iterator.h
 *
 *  Created on: 2012-12-14
 *      Author: sr2chowd
 */
#ifndef LOOKUP_ITERATOR_H
#define LOOKUP_ITERATOR_H

#include "lookup_table.h"
#include <pthread.h>
#include <map>

template<class KeyType, class ValueType>
class LookupTableIterator
{
	LookupTable<KeyType, ValueType>* table_ptr;
	typename map<KeyType, ValueType>::iterator table_iterator;

public:

	LookupTableIterator()
	{
		table_ptr = NULL;
	}
	
    LookupTableIterator(LookupTable<KeyType, ValueType>* ptr)
	{
		table_ptr = ptr;
		table_iterator = table_ptr->begin();
		//puts("iterator created");
	}

	void reset_iterator()
	{
		table_iterator = table_ptr->begin();
	}

	KeyType getNextKey()
	{
		KeyType ret = (*table_iterator).first;
		table_iterator++;
		return ret;
	}

	bool hasMoreKey()
	{
		return table_iterator != (table_ptr->end());
	}
};

#endif

