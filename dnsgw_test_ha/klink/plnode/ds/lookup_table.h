/*
 * dictionary.h
 *
 *  Created on: 2012-11-21
 *      Author: sr2chowd
 */

#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include <map>
#include <vector>
#include <stdio.h>
#include <pthread.h>

using namespace std;

template<class KeyType, class ValueType>
class LookupTable {
        map<KeyType, ValueType> table;
        typename map<KeyType, ValueType>::iterator table_iterator;
        pthread_rwlock_t table_lock;

public:

        //typedef typename::vector <KeyType> vk;

        LookupTable() {
                table.clear();
                pthread_rwlock_init(&table_lock, NULL);
        }

        virtual ~LookupTable() {
                table.clear();
                table_iterator = table.begin();
                pthread_rwlock_destroy(&table_lock);
        }

        int size() {
                pthread_rwlock_rdlock(&table_lock);
                int size = table.size();
                pthread_rwlock_unlock(&table_lock);
                return size;
        }

        void clear() {
                pthread_rwlock_wrlock(&table_lock);
                table.clear();
                pthread_rwlock_unlock(&table_lock);
        }

        bool add(KeyType key, ValueType value);
        bool lookup(KeyType key, ValueType* value);
        bool update(KeyType key, ValueType value);
        bool update_strict(KeyType key, ValueType value);
        bool remove(KeyType key);

        typename map<KeyType, ValueType>::iterator begin() {
                return table.begin();
        }

        typename map<KeyType, ValueType>::iterator end() {
                return table.end();
        }

        /*	template <typename KeyType>
         vector <KeyType> getKeySet()
         {
         vector <KeyType> keySet;

         typename map <KeyType, ValueType>::iterator mapIt;

         pthread_rwlock_rdlock(&table_lock);
         for(mapIt = table.begin(); mapIt != table.end(); mapIt++)
         keySet.push_back((*mapIt).first);

         pthread_rwlock_unlock(&table_lock);

         return keySet;
         }*/

        /*void reset_iterator()
         {
         pthread_rwlock_wrlock(&table_lock);
         table_iterator = table.begin();
         pthread_rwlock_unlock(&table_lock);
         }

         KeyType getNextKey()
         {
         KeyType ret = (*table_iterator).first;
         table_iterator++;
         return ret;
         }
         bool hasMoreKey()
         {
         return table_iterator != table.end();
         }*/
};

template<typename KeyType, typename ValueType>
bool LookupTable<KeyType, ValueType>::add(KeyType key, ValueType value) {
        bool exists = false;

        pthread_rwlock_wrlock(&table_lock);
        if (table.find(key) == table.end()) {
                exists = true;
                table.insert(make_pair(key, value));
        }
        pthread_rwlock_unlock(&table_lock);
        return !exists;
}

template<typename KeyType, typename ValueType>
bool LookupTable<KeyType, ValueType>::lookup(KeyType key, ValueType* value) {
        bool success = false;

        pthread_rwlock_rdlock(&table_lock);
        if (table.find(key) != table.end()) {
                success = true;
                *value = (*table.find(key)).second;
        }
        pthread_rwlock_unlock(&table_lock);
        return success;
}

template<typename KeyType, typename ValueType>
bool LookupTable<KeyType, ValueType>::update(KeyType key, ValueType value) {
        bool exists = false;
        pthread_rwlock_wrlock(&table_lock);
        if (table.find(key) == table.end()) {
                table.insert(make_pair(key, value));
        } else {
                table[key] = value;
                exists = true;
        }
        pthread_rwlock_unlock(&table_lock);
        return exists;
}

template<typename KeyType, typename ValueType>
bool LookupTable<KeyType, ValueType>::update_strict(KeyType key, ValueType value) {
        bool exists = false;
        pthread_rwlock_wrlock(&table_lock);
        if (table.find(key) == table.end())
        {
        	exists = false;
        }
        else
        {
                table[key] = value;
                exists = true;
        }
        pthread_rwlock_unlock(&table_lock);
        return exists;
}

template<typename KeyType, typename ValueType>
bool LookupTable<KeyType, ValueType>::remove(KeyType key) {
        bool removed = false;
        pthread_rwlock_wrlock(&table_lock);
        if (table.find(key) != table.end()) {
                table.erase(key);
                removed = true;
        }
        pthread_rwlock_unlock(&table_lock);
        return false;
}
#endif /* DICTIONARY_H_ */
