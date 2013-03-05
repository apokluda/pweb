/*
 * error_code.h
 *
 *  Created on: 2012-11-24
 *      Author: sr2chowd
 */

#ifndef ERROR_CODE_H_
#define ERROR_CODE_H_
#define DEBUG

#include <stdio.h>

#define SUCCESS							0
#define ERROR_SOCKET_CREATE_FAIL		-100
#define ERROR_SOCKET_BIND_FAIL			-101
#define ERROR_SOCKET_LISTEN_FAIL		-102
#define ERROR_CONNECTION_ACCEPT_FAIL 	-103
#define ERROR_SERVER_CONNECTION_FAIL	-104
#define ERROR_OPEN_FILE_FAIL			-105
#define ERROR_FILE_NOT_OPEN				-106
#define ERROR_NOT_IN_HOSTS_FILE			-107
#define ERROR_GET_FAILED				-108
#define ERROR_CONNECTION_TIMEOUT		-109
#define ERROR_DATA_SEND_FAILED			-110

char ERROR_MESSAGES[][100] = { "Socket Create Fail",
		"Socket Bind Fail", "Socket Listener Add Fail",
		"Cannot Create a Connection", "Cannot Connect to Server",
		"Cannot Open File", "File Not Open", "Not in Hosts File",
		"Request Not Resolved", "Connection Timed Out",
		"Failed to Send Data"
};

void print_error_message(int error_code)
{
	puts(ERROR_MESSAGES[-error_code - 100]);
}

#endif /* ERROR_CODE_H_ */

