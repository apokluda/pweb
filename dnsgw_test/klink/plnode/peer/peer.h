/*
 * peer.h
 *
 *  Created on: 2012-11-21
 *      Author: sr2chowd
 */

#ifndef PEER_H_
#define PEER_H_

#include <pthread.h>
#include <string.h>
#include "../../common/util.h"
#include "../protocol/protocol.h"
#include "../ds/overlay_id.h"
#include "../ds/ip_address.h"
#include "../ds/configuration.h"
#include "../../communication/server_socket.h"
#include "../ds/lookup_table.h"
#include "../protocol/code.h"

class ABSProtocol;

class ABSProtocol;

class Peer {
        int run_sequence_no;
        int n_peers;

        int peer_id;
	string peer_name;

        int code_word;
        OverlayID overlay_id;
        int status;

        IPAddress ip_address;
        string host_name;
        int listen_port_number;

        int publish_name_range_start;
        int publish_name_range_end;
        int lookup_name_range_start;
        int lookup_name_range_end;
        int webserver_port;

        //PUT
        int put_generated, put_received, put_processed, put_forwarded, put_dropped;
        pthread_mutex_t put_generated_lock;
        pthread_mutex_t put_received_lock;
        pthread_mutex_t put_processed_lock;
        pthread_mutex_t put_forwarded_lock;
        pthread_mutex_t put_dropped_lock;
        //GET
        int get_generated, get_received, get_processed, get_forwarded, get_dropped;
        pthread_mutex_t get_generated_lock;
        pthread_mutex_t get_received_lock;
        pthread_mutex_t get_processed_lock;
        pthread_mutex_t get_forwarded_lock;
        pthread_mutex_t get_dropped_lock;

        double alpha;
        int k;

        int timeout_sec;
        int timeout_micro_sec;
        int n_retry;

        bool init_rcvd, start_gen_name_rcvd, start_lookup_name_rcvd, start_gen_name;
        int dyn_status;

        ABSProtocol* protocol;
        ABSCode *iCode;
        ServerSocket* server_socket;

        string log_server_name;
        string log_server_user;

        string cache_storage;	//end_point or path
        string cache_type;		//reactive or proactive

        Configuration* configuration;

        LookupTable <HostAddress, pair <sockaddr_in, pair <int, double> > > address_db;
public:


	void INIT()
	{
		char hostname[100];
		hostname[0] = '\0';
		gethostname(hostname, 100);
		//getdomainname(domain_name, 100);
		//struct hostent* host_info;
		//host_info = gethostbyname(hostname);
		host_name = string(hostname);

		init_rcvd = false;
		start_gen_name = false;

		put_generated, put_received, put_processed, put_forwarded, put_dropped = 0;
		get_generated, get_received, get_processed, get_forwarded, get_dropped = 0;

		pthread_mutex_init(&put_generated_lock, NULL);
		pthread_mutex_init(&put_received_lock, NULL);
		pthread_mutex_init(&put_processed_lock, NULL);
		pthread_mutex_init(&put_forwarded_lock, NULL);
		pthread_mutex_init(&put_forwarded_lock, NULL);

		pthread_mutex_init(&get_generated_lock, NULL);
		pthread_mutex_init(&get_received_lock, NULL);
		pthread_mutex_init(&get_processed_lock, NULL);
		pthread_mutex_init(&get_forwarded_lock, NULL);
		pthread_mutex_init(&get_dropped_lock, NULL);
	}

	Peer()
	{
		INIT();
		listen_port_number = 0;
	}

	Peer(int port)
	{
		INIT();
		listen_port_number = port;
		server_socket = new ServerSocket(listen_port_number);
	}

	Peer(const char* configuration_file)
	{
		INIT();
		configuration = new Configuration(configuration_file);

		alpha = configuration->getAlpha();
		k = configuration->getK();
		n_retry = configuration->getNRetry();
		timeout_sec = (int) configuration->getTimeout();
		timeout_micro_sec = (int) ((configuration->getTimeout() - (double) timeout_sec) * 1000000);
		cache_storage = configuration->getCacheStorage();
		cache_type = configuration->getCacheType();
		/*log_server_name = configuration->getLogServerHostName();
		log_server_user = configuration->getLogServerUserName();*/

		initListenSocket(configuration->getNodesFilePath().c_str());
	}

	Peer(Configuration* config)
	{
		INIT();
		this->configuration = config;
		alpha = configuration->getAlpha();
		k = configuration->getK();
		n_retry = configuration->getNRetry();
		timeout_sec = (int) configuration->getTimeout();
		timeout_micro_sec = (int) ((configuration->getTimeout() - (double) timeout_sec) * 1000000);
		cache_storage = configuration->getCacheStorage();
		cache_type = configuration->getCacheType();
	}

	string get_peer_name()
	{
		return peer_name;
	}

	void set_peer_name(string name)
	{
		peer_name = name;	
	}

	void reconfigure()
	{
		alpha = configuration->getAlpha();
		k = configuration->getK();
		n_retry = configuration->getNRetry();
		timeout_sec = (int) configuration->getTimeout();
		timeout_micro_sec = (int) ((configuration->getTimeout() - (double) timeout_sec) * 1000000);
		cache_storage = configuration->getCacheStorage();
		cache_type = configuration->getCacheType();
	}

	void initListenSocket(const char* hosts_file)
	{
		listen_port_number = -1;

		FILE* hosts_ptr = fopen(hosts_file, "r");
		if (hosts_ptr == NULL)
		{
			printf("bad hosts file %s\n", hosts_file);
			listen_port_number = -1;
			server_socket = NULL;
			return;
		}

		char host_name[200];
		int port = -1;
		int n_hosts;
		bool found = false;
		char line[400];

		fgets(line, sizeof(line), hosts_ptr);
		n_hosts = atoi(line);

		//fscanf(hosts_ptr, "%d", &n_hosts);
		printf("N hosts = %d\n", n_hosts);

		for (int i = 0; i < n_hosts; i++)
		{
			fgets(line, sizeof(line), hosts_ptr);
			//fscanf(hosts_ptr, "%s %d", host_name, &port);
			strcpy(host_name, strtok(line, " \n"));
			port = atoi(strtok(NULL, " \n"));

			printf("%s:%d\n", host_name, port);

			if (strncmp(this->getHostName().c_str(), host_name, strlen(this->getHostName().c_str()))
					== 0 || strcmp(host_name, "localhost") == 0)
			{
				listen_port_number = port;
				server_socket = new ServerSocket(listen_port_number);
				if (server_socket->init_connection() < 0)
				{
					delete server_socket;
					listen_port_number = -1;
					server_socket = NULL;
				}
				else
					break;
			}
		}
		fclose(hosts_ptr);
	}

	void populate_addressdb()
	{
		FILE* nodes_file_ptr = fopen(configuration->getNodesFilePath().c_str(),"r");
		char buffer[300];
		address_db.clear();
		fgets(buffer, sizeof(buffer), nodes_file_ptr);
		int N = atoi(buffer);
		int total = N, done = 0;

		while (N > 0)
		{
			N--;
			fgets(buffer, sizeof(buffer), nodes_file_ptr);

			addrinfo hints;
			addrinfo* result;


			HostAddress h_address;

			char* str_h_addr = strtok(buffer, " \n");
			char* str_h_port = strtok(NULL, " \n");
			char* ip_address = strtok(NULL, " \n");

			h_address.SetHostName(string(str_h_addr));
			h_address.SetHostPort(atoi(str_h_port));

			char str_port[12];
			sprintf(str_port, "%d", h_address.GetHostPort());
			pair <int, double> cost;

			if(ip_address == NULL)
			{
				memset(&hints, 0, sizeof(addrinfo));
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_flags = AI_CANONNAME;
				hints.ai_protocol = IPPROTO_TCP;

				int ret_code = getaddrinfo(str_h_addr, str_port, &hints, &result);

				if(ret_code < 0)
				{
					printf("%s\n", gai_strerror(ret_code));
					continue;
				}
				char ip[INET_ADDRSTRLEN + 1];
				inet_ntop(AF_INET, &(((sockaddr_in*)(result->ai_addr))->sin_addr), ip, INET_ADDRSTRLEN);
				string str_ip = string(ip);
				cost = getCost(str_ip);

				address_db.add(h_address, make_pair(*((sockaddr_in*)(result->ai_addr)), cost) );
			}
			else
			{
				sockaddr_in s_addr;
				s_addr.sin_family = AF_INET;
				inet_pton(AF_INET, ip_address, &(s_addr.sin_addr));
				s_addr.sin_port = htons(h_address.GetHostPort());
				string str_ip = string(ip_address);
				cost = getCost(str_ip);
				address_db.add(h_address, make_pair(s_addr, cost));
			}

			done++;
			printf("[%d/%d] resolving address = %s:%s, ip_hop = %d, latency = %.3lf (ms)\n", done, total, str_h_addr, str_port, cost.first, cost.second);

			/*addrinfo* rp;
			for(rp = result; rp != NULL; rp = result->ai_next)
			{
				int sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
				if(sfd < 0) continue;
				int res = connect(sfd, rp->ai_addr, rp->ai_addrlen);
				if(res < 0) continue;
				address_db.add(h_address, *rp);
				close(sfd);
				break;
			}*/
		}
		fclose(nodes_file_ptr);
	}

	pair <sockaddr_in, pair <int, double> > lookup_address(const HostAddress& h_address)
	{
		pair <sockaddr_in, pair <int, double> > ret;
		ret.first.sin_port = 0;
		bool found = address_db.lookup(h_address, &ret);
		return ret;
	}

	~Peer()
	{
		delete protocol;
		delete server_socket;
		if (configuration != NULL)
			delete configuration;

		address_db.clear();
                
		pthread_mutex_destroy(&put_generated_lock);
		pthread_mutex_destroy(&put_received_lock);
		pthread_mutex_destroy(&put_processed_lock);
		pthread_mutex_destroy(&put_forwarded_lock);
		pthread_mutex_destroy(&put_dropped_lock);
		pthread_mutex_destroy(&get_generated_lock);
		pthread_mutex_destroy(&get_received_lock);
		pthread_mutex_destroy(&get_processed_lock);
		pthread_mutex_destroy(&get_forwarded_lock);
		pthread_mutex_destroy(&get_dropped_lock);
                
	}

	double getAlpha()
	{
		return alpha;
	}

	void setAlpha(double alpha)
	{
		this->alpha = alpha;
	}

	int getCodeWord()
	{
		return code_word;
	}

	void setCodeWord(int codeWord)
	{
		code_word = codeWord;
	}

	IPAddress getIpAddress()
	{
		return ip_address;
	}

	int getIntIpAddress()
	{
		return ip_address.getIpAddress();
	}

	string getStrIpAddress()
	{
		return ip_address.getStrIpAddress();
	}

	void setIpAddress(char* ipAddress)
	{
		ip_address.setIp(ipAddress);
	}

	int getK()
	{
		return k;
	}

	void setK(int k)
	{
		this->k = k;
	}

	int getPeerId()
	{
		return peer_id;
	}

	void setPeerId(int peerId)
	{
		peer_id = peerId;
	}

	void setHostName(string host_name)
	{
		this->host_name = host_name;
	}

	string getHostName()
	{
		return host_name;
	}

	int getListenPortNumber()
	{
		return listen_port_number;
	}

	void setListenPortNumber(int port)
	{
		listen_port_number = port;
	}

	ABSProtocol* getProtocol()
	{
		return protocol;
	}

	void setProtocol(ABSProtocol* protocol)
	{
		this->protocol = protocol;
	}

	ServerSocket* getServerSocket()
	{
		return server_socket;
	}

	void setServerSocket(ServerSocket* socket)
	{
		server_socket = socket;
	}

	void setNPeers(int n)
	{
		n_peers = n;
	}

	int getNPeers()
	{
		return n_peers;
	}

	OverlayID getOverlayID()
	{
		return overlay_id;
	}

	void setOverlayID(OverlayID id)
	{
		overlay_id = id;
	}

	void setStatus(int status)
	{
		this->status = status;
	}

	int getStatus() const
	{
		return status;
	}

	void setLookup_name_range_end(int lookup_name_range_end)
	{
		this->lookup_name_range_end = lookup_name_range_end;
	}

	int getLookup_name_range_end() const
	{
		return lookup_name_range_end;
	}

	void setLookup_name_range_start(int lookup_name_range_start)
	{
		this->lookup_name_range_start = lookup_name_range_start;
	}

	int getLookup_name_range_start() const
	{
		return lookup_name_range_start;
	}

	void setPublish_name_range_end(int publish_name_range_end)
	{
		this->publish_name_range_end = publish_name_range_end;
	}

	int getPublish_name_range_end() const
	{
		return publish_name_range_end;
	}

	void setPublish_name_range_start(int publish_name_range_start)
	{
		this->publish_name_range_start = publish_name_range_start;
	}

	int getPublish_name_range_start() const
	{
		return publish_name_range_start;
	}

	void SetInitRcvd(bool init_rcvd)
	{
		this->init_rcvd = init_rcvd;
	}

	bool IsInitRcvd() const
	{
		return init_rcvd;
	}

	int getTimeoutSec() const
	{
		return timeout_sec;
	}
	void setTimeoutSec(int t)
	{
		timeout_sec = t;
	}

	int getTimeoutMicroSec() const
	{
		return timeout_micro_sec;
	}
	void setTimeoutMicroSec(int t)
	{
		timeout_micro_sec = t;
	}

	int getNRetry() const
	{
		return n_retry;
	}
	void setNRetry(int retry)
	{
		n_retry = retry;
	}

	void SetStart_gen_name(bool start_gen_name)
	{
		this->start_gen_name = start_gen_name;
	}

	bool IsStart_gen_name() const
	{
		return start_gen_name;
	}

	Configuration* getConfiguration() const
	{
		return configuration;
	}

	void setConfiguration(const char* config_file)
	{
		configuration = new Configuration(config_file);
	}

	string getLogServerName() const
	{
		return log_server_name;
	}

	void setLogServerName(const string& logServerName)
	{
		log_server_name = logServerName;
	}

	string getLogServerUser() const
	{
		return log_server_user;
	}

	void setLogServerUser(const string& logServerUser)
	{
		log_server_user = logServerUser;
	}

	void SetDyn_status(int dyn_status)
	{
		this->dyn_status = dyn_status;
	}

        int GetDyn_status() const {
                return dyn_status;
        }

        void SetWebserverPort(int webserver_port) {
                this->webserver_port = webserver_port;
        }

        int GetWebserverPort() const {
                return webserver_port;
        }

        int numOfGet_generated() const {
                return get_generated;
        }

        int numOfGet_forwarded() const {
                return get_forwarded;
        }

        int numOfGet_processed() const {
                return get_processed;
        }

        int numOfGet_received() const {
                return get_received;
        }

        int numOfGet_dropped() const {
                return get_dropped;
        }

        int numOfPut_generated() const {
                return put_generated;
        }

        int numOfPut_received() const {
                return put_received;
        }

        int numOfPut_forwarded() const {
                return put_forwarded;
        }

        int numOfPut_processed() const {
                return put_processed;
        }

        int numOfPut_dropped() const {
                return put_dropped;
        }

        void incrementPut_generated() {
                pthread_mutex_lock(&put_generated_lock);
                this->put_generated++;
                pthread_mutex_unlock(&put_generated_lock);
        }

        void incrementPut_received() {
                pthread_mutex_lock(&put_received_lock);
                this->put_received++;
                pthread_mutex_unlock(&put_received_lock);
        }

        void incrementPut_processed() {
                pthread_mutex_lock(&put_processed_lock);
                this->put_processed++;
                pthread_mutex_unlock(&put_processed_lock);
        }

        void incrementPut_forwarded() {
                pthread_mutex_lock(&put_forwarded_lock);
                this->put_forwarded++;
                pthread_mutex_unlock(&put_forwarded_lock);
        }

        void incrementPut_Dropped() {
                pthread_mutex_lock(&put_dropped_lock);
                this->put_dropped++;
                pthread_mutex_unlock(&put_dropped_lock);
        }

        void incrementGet_generated() {
                pthread_mutex_lock(&get_generated_lock);
                this->get_generated++;
                pthread_mutex_unlock(&get_generated_lock);
        }

        void incrementGet_received() {
                pthread_mutex_lock(&get_received_lock);
                this->get_received++;
                pthread_mutex_unlock(&get_received_lock);
        }

        void incrementGet_processed() {
                pthread_mutex_lock(&get_processed_lock);
                this->get_processed++;
                pthread_mutex_unlock(&get_processed_lock);
        }

        void incrementGet_forwarded() {
                pthread_mutex_lock(&get_forwarded_lock);
                this->get_forwarded++;
                pthread_mutex_unlock(&get_forwarded_lock);
        }

        void incrementGet_Dropped() {
                pthread_mutex_lock(&get_dropped_lock);
                this->get_dropped++;
                pthread_mutex_unlock(&get_dropped_lock);
        }

        void SetiCode(ABSCode* iCode) {
                this->iCode = iCode;
        }

        ABSCode* GetiCode() const {
                return iCode;
        }

        int getRunSequenceNo() const {
                return run_sequence_no;
        }

        void setRunSequenceNo(int seq_no) {
                run_sequence_no = seq_no;
        }

        void SetStart_gen_name_rcvd(bool start_gen_name_rcvd) {
                this->start_gen_name_rcvd = start_gen_name_rcvd;
        }

        bool IsStart_gen_name_rcvd() const {
                return start_gen_name_rcvd;
        }

        void SetStart_lookup__name_rcvd(bool start_lookup__name_rcvd) {
                this->start_lookup_name_rcvd = start_lookup__name_rcvd;
        }

        bool IsStart_lookup__name_rcvd() const {
                return start_lookup_name_rcvd;
        }

		string getCacheStorage() const
		{
			return cache_storage;
		}

		void setCacheStorage(const string& cacheStorage)
		{
			cache_storage = cacheStorage;
		}

		string getCacheType() const
		{
			return cache_type;
		}

		void setCacheType(const string& cacheType)
		{
			cache_type = cacheType;
		}
};

#endif /* PEER_H_ */
