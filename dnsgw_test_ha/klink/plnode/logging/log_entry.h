/*
 * log_entry.h
 *
 *  Created on: 2012-12-19
 *      Author: sr2chowd
 */

#ifndef LOG_ENTRY_H_
#define LOG_ENTRY_H_

#include <string>
#include <vector>
#include <stdarg.h>

using namespace std;

class LogEntry
{
	string key_str;
	string value_str;
	int log_type;

public:

	LogEntry()
	{
		key_str = "";
		value_str = "";
		log_type = -1;
	}

	LogEntry(int log_type, const char* key, const char* format, ...)
	{
		int argc = strlen(format);
		char str[20];
		void* arg = NULL;

		this->log_type = log_type;
		key_str = key;
		value_str = "";

		va_list values;
		va_start(values, format);

		for (int i = 0; i < argc; i++)
		{
			if(i != 0) value_str += "\t";
			switch (format[i])
			{
			case 'i':
				arg = new int;
				*((int*) arg) = va_arg(values, int);
				sprintf(str, "%d", *((int*) arg));
				value_str += str;
				delete ((int*) arg);
				break;
			case 'd':
				arg = new double;
				*((double*) arg) = va_arg(values, double);
				sprintf(str, "%lf", *((double*) arg));
				value_str += str;
				delete ((double*) arg);
				break;
			case 's':
				arg = new char*;
				*((char**) arg) = va_arg(values, char*);
				sprintf(str, "%s", *((char**) arg));
				value_str += str;
				delete ((char**) arg);
				break;
			}
		}
		va_end(values);
	}


	string getKeyString()
	{
		return key_str;
	}

	string getValueString()
	{
		return value_str;
	}

	int getType()
	{
		return log_type;
	}

	~LogEntry(){}
};

#endif /* LOG_ENTRY_H_ */
