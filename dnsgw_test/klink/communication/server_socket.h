/*
 * server_socket.h
 *
 *  Created on: 2012-11-24
 *      Author: sr2chowd
 */

#ifndef SERVER_SOCKET_H_
#define SERVER_SOCKET_H_

#include "socket.h"
#include "error_code.h"
#include <set>

using namespace std;

class ServerSocket: public ABSSocket
{
	string server_host_name;
	int server_port_number;
	set<int> active_connections;

protected:

	int create_server_socket();
	int bind_port_to_socket();
	int add_listener_to_socket();

public:
	ServerSocket()
	{
		char hostname[100];
		gethostname(hostname, 100);
		server_host_name = hostname;
		server_port_number = 0;
		active_connections.clear();
	}

	ServerSocket(string hostname, int port) :
			server_host_name(hostname), server_port_number(port)
	{
		active_connections.clear();
	}

	ServerSocket(int port) :
			server_port_number(port)
	{
		char hostname[100];
		gethostname(hostname, 100);
		server_host_name = hostname;
		active_connections.clear();
	}
	/* Initiates the socket (TCP) and prepares it to accept any incoming connections */
	int init_connection();

	/* Accepts a connection and returns the connection handler for that connection */
	int accept_connection();

	void setServerHostName(const string& hostname)
	{
		server_host_name = hostname;
	}
	void setServerPortNumber(int port)
	{
		server_port_number = port;
	}
	string getServerHostName()
	{
		return server_host_name;
	}
	int getServerPortNumber()
	{
		return server_port_number;
	}
	int getMaxConnectionFd();
	void printActiveConnectionList();

	int receive_data(int connection_fd, char** buffer);
	int send_data(int connection_fd, char* buffer, int n_bytes);
	void close_connection(int connection_fd);
	~ServerSocket();
};

int ServerSocket::create_server_socket()
{
	int ret_code = SUCCESS;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
		ret_code = ERROR_SOCKET_CREATE_FAIL;
	return ret_code;
}

int ServerSocket::bind_port_to_socket()
{
	int ret_code = SUCCESS;
	sockaddr_in local_socket_address;
	local_socket_address.sin_family = AF_INET;
	local_socket_address.sin_addr.s_addr = INADDR_ANY;
	local_socket_address.sin_port = htons(server_port_number);
	memset(local_socket_address.sin_zero, 0, sizeof(local_socket_address.sin_zero));

	if (bind(socket_fd, (sockaddr*) &local_socket_address, sizeof(sockaddr)) < 0)
	{
		close_socket();
		ret_code = ERROR_SOCKET_BIND_FAIL;
	}
	return ret_code;
}

int ServerSocket::add_listener_to_socket()
{
	int ret_code = SUCCESS;

	if (listen(socket_fd, MAX_CONNECTIONS) < 0)
	{
		close_socket();
		ret_code = ERROR_SOCKET_LISTEN_FAIL;
	}
	return ret_code;
}

int ServerSocket::init_connection()
{
	int ret_code = SUCCESS;
	ret_code = create_server_socket();
	if (ret_code < 0)
	{
		//print_error_message(ret_code);
		close_socket();
		return ret_code;
	}

	ret_code = bind_port_to_socket();
	if (ret_code < 0)
	{
		//	print_error_message(ret_code);
		close_socket();
		return ret_code;
	}

	ret_code = add_listener_to_socket();
	if (ret_code < 0)
	{
		//	print_error_message(ret_code);
		close_socket();
		return ret_code;
	}

	return ret_code;
}

int ServerSocket::accept_connection()
{
	int connection_fd = accept(socket_fd, NULL, NULL);
	if (connection_fd < 0)
	{
		//print_error_message(ERROR_CONNECTION_ACCEPT_FAIL);
		return ERROR_CONNECTION_ACCEPT_FAIL;
	}
	active_connections.insert(connection_fd);
	return connection_fd;
}

void ServerSocket::close_connection(int connection_fd)
{
	active_connections.erase(connection_fd);
	close(connection_fd);
}

ServerSocket::~ServerSocket()
{
	set<int>::iterator active_con_it;
	for (active_con_it = active_connections.begin();
			active_con_it != active_connections.end(); active_con_it++)
	{
		close(*active_con_it);
	}
	active_connections.clear();
	close(socket_fd);
}

int ServerSocket::receive_data(int connection_fd, char** buffer)
{
	*buffer = new char[MAX_INCOMING_DATA_LENGTH];
	int n_bytes = recv(connection_fd, *buffer, MAX_INCOMING_DATA_LENGTH, 0);
	return n_bytes;
}

int ServerSocket::send_data(int connection_fd, char* buffer, int n_bytes)
{
	int sent_bytes = send(connection_fd, buffer, n_bytes, 0);
	return sent_bytes;
}

int ServerSocket::getMaxConnectionFd()
{
	int ret = socket_fd;
	set<int>::iterator active_con_itr;

	for (active_con_itr = active_connections.begin();
			active_con_itr != active_connections.end(); active_con_itr++)
		if (ret < *active_con_itr)
			ret = *active_con_itr;

	return ret;
}

void ServerSocket::printActiveConnectionList()
{
	set<int>::iterator active_con_itr;
	printf("active connection set <");
	for (active_con_itr = active_connections.begin();
			active_con_itr != active_connections.end(); active_con_itr++)
	{
		printf(" %d", *active_con_itr);
	}
	printf(" >\n");
}
#endif /* SERVER_SOCKET_H_ */
