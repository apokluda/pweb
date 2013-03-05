/*
 * ip_address.h
 *
 *  Created on: 2012-11-22
 *      Author: sr2chowd
 */

#ifndef IP_ADDRESS_H_
#define IP_ADDRESS_H_

#include <cstring>
#include <vector>
#include <cstdlib>
#include <cstdio>

using namespace std;

class IPAddress
{
	int ip_address;
	string str_ip_address;
public:
	IPAddress();
	IPAddress(int a, int b, int c, int d);
	IPAddress(int* a);
	IPAddress(const vector<int>& a);
	IPAddress(char* ip);
	IPAddress(const string& ip);

	void setIp(int a, int b, int c, int d);
	void setIp(int* a);
	void setIp(const vector<int>& a);
	void setIp(char* ip);
	void setIp(const string& ip);

	string getStrIpAddress()
	{
		return str_ip_address;
	}
	int getIpAddress()
	{
		return ip_address;
	}
};

IPAddress::IPAddress()
{
	ip_address = 0;
	str_ip_address = "0.0.0.0";
}

IPAddress::IPAddress(int a, int b, int c, int d)
{
	ip_address = (a << 24) | (b << 16) | (c << 8) | d;
	char temp_a[10], temp_b[10], temp_c[10], temp_d[10];

	sprintf(temp_a, "%d", a);
	sprintf(temp_b, "%d", b);
	sprintf(temp_c, "%d", c);
	sprintf(temp_d, "%d", d);

	str_ip_address = temp_a;
	str_ip_address += ".";
	str_ip_address = temp_b;
	str_ip_address += ".";
	str_ip_address = temp_c;
	str_ip_address += ".";
	str_ip_address = temp_d;
}

IPAddress::IPAddress(const string& str_ip)
{
	str_ip_address = str_ip;
	char temp[20];
	int toShift = 24;

	strcpy(temp, str_ip.c_str());
	char* ip_component = strtok(temp, ".");

	ip_address = atoi(ip_component) << toShift;

	while ((ip_component = strtok(NULL, ".")) != NULL)
	{
		toShift -= 8;
		ip_address |= (atoi(ip_component) << toShift);
	}
}

IPAddress::IPAddress(char *ip)
{
	str_ip_address = ip;
	int toShift = 24;

	char* ip_component = strtok(ip, ".");

	ip_address = atoi(ip_component) << toShift;

	while ((ip_component = strtok(NULL, ".")) != NULL)
	{
		toShift -= 8;
		ip_address |= (atoi(ip_component) << toShift);
	}
}

IPAddress::IPAddress(int* a)
{
	ip_address = (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
	char temp_a[10], temp_b[10], temp_c[10], temp_d[10];

	sprintf(temp_a, "%d", a[0]);
	sprintf(temp_b, "%d", a[1]);
	sprintf(temp_c, "%d", a[2]);
	sprintf(temp_d, "%d", a[3]);

	str_ip_address = temp_a;
	str_ip_address += ".";
	str_ip_address = temp_b;
	str_ip_address += ".";
	str_ip_address = temp_c;
	str_ip_address += ".";
	str_ip_address = temp_d;
}

IPAddress::IPAddress(const vector<int>& a)
{
	ip_address = (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
	char temp_a[10], temp_b[10], temp_c[10], temp_d[10];

	sprintf(temp_a, "%d", a[0]);
	sprintf(temp_b, "%d", a[1]);
	sprintf(temp_c, "%d", a[2]);
	sprintf(temp_d, "%d", a[3]);

	str_ip_address = temp_a;
	str_ip_address += ".";
	str_ip_address = temp_b;
	str_ip_address += ".";
	str_ip_address = temp_c;
	str_ip_address += ".";
	str_ip_address = temp_d;
}

void IPAddress::setIp(int a, int b, int c, int d)
{
	ip_address = (a << 24) | (b << 16) | (c << 8) | d;
	char temp_a[10], temp_b[10], temp_c[10], temp_d[10];

	sprintf(temp_a, "%d", a);
	sprintf(temp_b, "%d", b);
	sprintf(temp_c, "%d", c);
	sprintf(temp_d, "%d", d);

	str_ip_address = temp_a;
	str_ip_address += ".";
	str_ip_address = temp_b;
	str_ip_address += ".";
	str_ip_address = temp_c;
	str_ip_address += ".";
	str_ip_address = temp_d;
}

void IPAddress::setIp(const string& str_ip)
{
	str_ip_address = str_ip;
	char temp[20];
	int toShift = 24;

	strcpy(temp, str_ip.c_str());
	char* ip_component = strtok(temp, ".");

	ip_address = atoi(ip_component) << toShift;

	while ((ip_component = strtok(NULL, ".")) != NULL)
	{
		toShift -= 8;
		ip_address |= (atoi(ip_component) << toShift);
	}
}

void IPAddress::setIp(char *ip)
{
	str_ip_address = ip;
	int toShift = 24;

	char* ip_component = strtok(ip, ".");

	ip_address = atoi(ip_component) << toShift;

	while ((ip_component = strtok(NULL, ".")) != NULL)
	{
		toShift -= 8;
		ip_address |= (atoi(ip_component) << toShift);
	}
}

void IPAddress::setIp(int* a)
{
	ip_address = (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
	char temp_a[10], temp_b[10], temp_c[10], temp_d[10];

	sprintf(temp_a, "%d", a[0]);
	sprintf(temp_b, "%d", a[1]);
	sprintf(temp_c, "%d", a[2]);
	sprintf(temp_d, "%d", a[3]);

	str_ip_address = temp_a;
	str_ip_address += ".";
	str_ip_address = temp_b;
	str_ip_address += ".";
	str_ip_address = temp_c;
	str_ip_address += ".";
	str_ip_address = temp_d;
}

void IPAddress::setIp(const vector<int>& a)
{
	ip_address = (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
	char temp_a[10], temp_b[10], temp_c[10], temp_d[10];

	sprintf(temp_a, "%d", a[0]);
	sprintf(temp_b, "%d", a[1]);
	sprintf(temp_c, "%d", a[2]);
	sprintf(temp_d, "%d", a[3]);

	str_ip_address = temp_a;
	str_ip_address += ".";
	str_ip_address = temp_b;
	str_ip_address += ".";
	str_ip_address = temp_c;
	str_ip_address += ".";
	str_ip_address = temp_d;
}
#endif /* IP_ADDRESS_H_ */
