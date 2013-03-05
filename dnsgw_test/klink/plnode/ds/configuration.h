/*
 * configuration.h
 *
 *  Created on: 2012-12-18
 *      Author: sr2chowd
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <map>

using namespace std;

class Configuration
{
	string config_file_path;

	string nodes_file_path;
	string monitors_file_path;

	string log_server_host_name;
	string log_server_user_name;

	string seq_file_path;
	string input_file_path;

	int K;
	double alpha;
	int name_count;
	int check_point_row;
	double timeout;
	int n_retry;
	string cache_storage;
	string cache_type;

	map <string, string> config_map;

	bool isNumeric(string str)
	{
		int dot_count = 0;
		for(int i = 0; i < str.size(); i++)
		{
			if(str[i] == '.')
			{
				dot_count++;
				if(dot_count > 1)
					return false;
			}
			else if(str[i] < '0' || str[i] > '9')
				return false;
		}

		return true;
	}

	void stripNewline(char* str)
	{
		int ptr = (int)strlen(str) - 1;
		if(str[ptr] == '\n' || str[ptr] == '\r') str[ptr] = '\0';
	}

public:

	Configuration()
	{
		config_map.clear();
	}

	Configuration(string config_file_path)
	{
		this->config_file_path = config_file_path;
		config_map.clear();
		load_data();
	}

	void load_data()
	{
		FILE* config_file_ptr = fopen(config_file_path.c_str(), "r");
		char line[200];
		config_map.clear();

		while (fgets(line, sizeof(line), config_file_ptr) != NULL)
		{
			if(line[0] == '#' || strlen(line) == 0 || !isalnum(line[0])) continue;

			char* key = strtok(line, "="); stripNewline(key);
			char* value = strtok(NULL, "="); stripNewline(value);
			config_map.insert(make_pair(key, value));

			if (strcmp(key, "node_file") == 0 || strcmp(key, "nodes_file") == 0)
			{
				nodes_file_path = value;
			}
			else if(strcmp(key, "monitor_file") == 0 || strcmp(key, "monitors_file") == 0
					|| strcmp(key, "monitors_file_path") == 0 || strcmp(key, "monitor_file_path") == 0)
			{
				monitors_file_path = value;
			}
			else if(strcmp(key, "log_server_name") == 0 || strcmp(key, "log_server_host_name") == 0
					|| strcmp(key, "log_server") == 0)
			{
				log_server_host_name = value;
			}
			else if(strcmp(key, "log_server_user") == 0 || strcmp(key, "log_server_user_name") == 0)
			{
				log_server_user_name = value;
			}
			else if(strcmp(key, "seq_file") == 0)
			{
				seq_file_path = value;
			}
			else if(strcasecmp(key, "k") == 0)
			{
				K = atoi(value);
			}
			else if(strcmp(key, "check_point_row") == 0)
			{
				check_point_row = atoi(value);
			}
			else if(strcmp(key, "alpha") == 0)
			{
				alpha = atof(value);
			}
			else if(strcmp(key, "name_count") == 0)
			{
				name_count = atoi(value);
			}
			else if (strcmp(key, "timeout") == 0)
			{
				timeout = atof(value);
			}
			else if (strcmp(key, "retry") == 0)
			{
				n_retry = atoi(value);
			}
			else if(strcmp(key, "cache_storage") == 0)
			{
				cache_storage = value;
			}
			else if(strcmp(key, "cache_type") == 0)
			{
				cache_type = value;
			}
			else if(strcmp(key, "input") == 0 || strcmp(key, "input_file") == 0)
			{
				input_file_path = value;
			}
		}
		fclose(config_file_ptr);
	}

	string getConfigFilePath() const
	{
		return this->config_file_path;
	}

	void setConfigFilePath(string& config_file_path)
	{
		this->config_file_path = config_file_path;
	}

	double getTimeout() const
	{
		return this->timeout;
	}

	int getNRetry() const
	{
		return this->n_retry;
	}

	double getAlpha() const
	{
		return alpha;
	}

	int getK() const
	{
		return K;
	}

	int getCheckPointRow() const
	{
		return check_point_row;
	}

	string getLogServerHostName() const
	{
		return log_server_host_name;
	}

	string getLogServerUserName() const
	{
		return log_server_user_name;
	}

	string getMonitorsFilePath() const
	{
		return monitors_file_path;
	}

	int getNameCount() const
	{
		return name_count;
	}

	string getNodesFilePath() const
	{
		return nodes_file_path;
	}

	string getSeqFilePath() const
	{
		return seq_file_path;
	}

	string getCacheStorage() const
	{
		return cache_storage;
	}

	string getCacheType() const
	{
		return cache_type;
	}

	string getInputFilePath() const
	{
		return input_file_path;
	}

	int getInt(string key)
	{
		int ret = -1;
		if(!isNumeric(key))
			return ret;

		if(config_map.find(key) != config_map.end())
		{
			ret = atoi(config_map[key].c_str());
		}
		return ret;
	}

	double getDouble(string key)
	{
		double ret = -1.00;
		if(!isNumeric(key))
			return ret;
		if(config_map.find(key) != config_map.end())
		{
			ret = atof(config_map[key].c_str());
		}
		return ret;
	}

	string getString(string key)
	{
		string ret = "";
		if(config_map.find(key) != config_map.end())
		{
			ret = config_map[key];
		}
		return ret;
	}


	void print_configuration()
	{
		map <string, string>::iterator config_itr;
		puts("--------------------Configuration Parameters--------------------");
		for(config_itr = config_map.begin(); config_itr != config_map.end(); config_itr++)
		{
			printf("%s = %s\n", config_itr->first.c_str(), config_itr->second.c_str());
		}
		puts("----------------------------------------------------------------");
	}
};

#endif /* CONFIGURATION_H_ */
