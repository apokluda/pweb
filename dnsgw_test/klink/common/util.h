#ifndef UTIL_H_
#define UTIL_H_

#include <iostream>
#include <cstdlib>
#include "../plnode/ds/lookup_table.h"
#include "../plnode/ds/lookup_table_iterator.h"
#include "../plnode/ds/overlay_id.h"
#include "../plnode/ds/host_address.h"
#include <time.h>
#include <climits>

using namespace std;

//class OverlayID;
//class 

int getBitAtPosition(int value, int n)
{
	return (((value & (1 << n)) >> n) & 0x00000001);
}

void printBits(int value, int length)
{
	for (int i = length - 1; i >= 0; i--)
	{
		cout << getBitAtPosition(value, i);
	}
}

char* printBits2String(int value, int length)
{
	char* result = new char[length];
	for (int i = length - 1; i >= 0; i--)
	{
		result[i] = '0' + getBitAtPosition(value, i);
	}
	return result;
}

char* printRoutingTable2String(LookupTable<OverlayID, HostAddress> &rtable)
{
	int index = 0, size = 0;
	LookupTableIterator<OverlayID, HostAddress> rtable_iter(&rtable);
	rtable_iter.reset_iterator();
	while (rtable_iter.hasMoreKey())
	{
		OverlayID oid = rtable_iter.getNextKey();
		HostAddress ha;
		rtable.lookup(oid, &ha);
		size += oid.getStringSize() + 5 + ha.getStringSize() + 5;
	}
	size += 1;
	char* result = new char[size];

	rtable_iter.reset_iterator();
	while (rtable_iter.hasMoreKey())
	{
		OverlayID oid = rtable_iter.getNextKey();
		HostAddress ha;
		rtable.lookup(oid, &ha);
		sprintf((result + index), "%s --> %s<br/>", oid.toString(), ha.toString());
		index += oid.getStringSize() + 5 + ha.getStringSize() + 5;
	}
	result[size] = '\0';
	return result;
}


string& routingTable2String(LookupTable<OverlayID, HostAddress> &rtable, string &result)
{
	int count = 0;
	LookupTableIterator<OverlayID, HostAddress> rtable_iter(&rtable);
	rtable_iter.reset_iterator();
	string temp;
	while (rtable_iter.hasMoreKey())
	{
		OverlayID oid = rtable_iter.getNextKey();
		HostAddress ha;
		rtable.lookup(oid, &ha);
		temp = "";
		//result.append(oid.toString(temp));
		//result.append(",");
		//temp = "";
		result.append(ha.GetHostName());
		result.append("|");
		count++;
	}
	char buffer[20];
	sprintf(buffer, "%d", count);
	result = string(buffer).append("|").append(result);
	return result;
}

char* printIndexTable2String(LookupTable<string, HostAddress> &itable)
{
	int index = 0, size = 0;
	LookupTableIterator<string, HostAddress> itable_iter(&itable);
	itable_iter.reset_iterator();
	while (itable_iter.hasMoreKey())
	{
		string name = itable_iter.getNextKey();
		HostAddress ha;
		itable.lookup(name, &ha);
		size += name.size() + 1 + ha.getStringSize() + 5;
	}
	size += 1;
	char* result = new char[size];

	itable_iter.reset_iterator();
	while (itable_iter.hasMoreKey())
	{
		string name = itable_iter.getNextKey();
		HostAddress ha;
		itable.lookup(name, &ha);
		sprintf((result + index), "%s,%s<br/>", name.c_str(), ha.toString());
		index += name.size() + 1 + ha.getStringSize() + 5;
	}
	result[size] = '\0';
	if(size == 1) return "";
	return result;
}

int stringHash(string str)
{
	return atoi(str.c_str());
}

long urlHash(string url){
        unsigned int r_shift = 2;
        unsigned int l_shift = 4;
        unsigned long seed = 99099;
        unsigned int range = LONG_MAX;
        const char* chars = url.c_str();
        for (int i = 0; i < url.size(); i++) {
            seed ^= ((seed << l_shift) + (seed >> r_shift) + (chars[i] & 0xff));
        }
        if (seed < 0) seed = -seed;
        return seed % range;
}

unsigned GetNumberOfDigits(unsigned number)
{
	int digits = 0;
	while (number != 0)
	{
		number /= 10;
		digits++;
	}
	return digits;
}

/*
 * source: http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 */

void timeval_subtract(timeval x, timeval y, double* time_sec)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x.tv_usec < y.tv_usec)
	{
		int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
		y.tv_usec -= (1000000 * nsec);
		y.tv_sec += nsec;
	}

	if (x.tv_usec - y.tv_usec > 1000000)
	{
		int nsec = (x.tv_usec - y.tv_usec) / 1000000;
		y.tv_usec += (1000000 * nsec);
		y.tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 tv_usec is certainly positive. */

	timeval result;
	result.tv_sec = x.tv_sec - y.tv_sec;
	result.tv_usec = x.tv_usec - y.tv_usec;

	*time_sec = (double)result.tv_sec + (double)result.tv_usec / 1000000.0;
}

pair <int, double> getCost(string ip_address)
{

	string command = "ping -c 1 ";
	command += ip_address;
	command += " | grep -E 'ttl=|time=' | cut -d' ' -f 6- ";
	//puts(command.c_str());

	int total_retry = 3;
	int n_retry = 0;
	pair <int, double> ret;
	ret.first = 0;
	ret.second = 0.0;

	while(n_retry < total_retry)
	{
		FILE* pipe = popen(command.c_str(), "r");

		char buffer[300];

		fgets(buffer, sizeof(buffer), pipe);
		//puts(buffer);
		if( strlen(buffer) <= 0) { n_retry++; continue; }

		char* ttl_str = strtok(buffer, " =");
		if(ttl_str == NULL) { n_retry++; continue; }

		char* str_ttl = strtok(NULL, " =");
		if(str_ttl == NULL) { n_retry++; continue; }
		int ttl = atoi(str_ttl);

		char* time_str = strtok(NULL, " =");
		if(time_str == NULL) { n_retry++; continue; }

		char* str_rtt = strtok(NULL, " =");
		if(str_rtt == NULL) { n_retry++; continue; }
		double rtt = atof(str_rtt);

		int max_ttl = 1;
		while(max_ttl < ttl) max_ttl *= 2;
		int ip_hops = max_ttl - ttl;

		ret.first = ip_hops;
		ret.second = rtt / 2.0;

		pclose(pipe);

		break;
	}
	return ret;
}

string nameDbToString(vector < pair <string, time_t> > names)
{
	string ret = "";
	for(int i = 0; i < names.size(); i++)
	{
		if( i > 0 )
			ret += ",";
		char str_ts[20];
		sprintf(str_ts, "%ld", names[i].second);

		ret += (names[i].first + "|");
		ret += str_ts;
	}
	char buffer[20];
	sprintf(buffer, "%d", names.size());
	ret = string(buffer).append("|").append(ret);
	return ret;
}

#endif
