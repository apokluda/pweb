/*
 * log.h
 *
 *  Created on: 2012-11-26
 *      Author: sr2chowd
 */

#ifndef LOG_H_
#define LOG_H_

#include <string>
#include <stdarg.h>
#include <stdio.h>
#include "../../communication/error_code.h"
#include <unistd.h>

using namespace std;


class Log
{
	string log_type;
	string cache_type;
	string cache_storage;
	string K;
	string host_name;
	string monitor_host_name;
	string monitor_user_name;
	string remote_ftp_directory;

	string log_file_name;
	string mode;
	string seq_file_name;
	string archive_name;

	int log_sequence_no;
	int current_segment_no;
	int current_row_count;
	int check_point_row_count;
	FILE* log_file_ptr;

public:

	Log();
	Log(const char* seq_file, const char* type, const char* monitor_host,
			const char* monitor_user, const char* hostname = NULL);

	Log(int seq_no, const char* type, const char* cache_type, const char* cache_storage, int K, const char* monitor_host,
				const char* monitor_user, const char* hostname = NULL);

	void setLogType(const string& type)
	{
		log_type = type;
	}
	string getLogType()
	{
		return log_type;
	}

	void setHostName(const string& hostname)
	{
		host_name = hostname;
	}
	string getHostName()
	{
		return host_name;
	}

	void setMonitorHostName(const string& hostname)
	{
		monitor_host_name = hostname;
	}
	string getMonitorHostName()
	{
		return monitor_host_name;
	}

	void setMonitorUserName(const string& user)
	{
		monitor_user_name = user;
	}
	string getMonitorUserName()
	{
		return monitor_user_name;
	}

	void setLogFileName(const char* name);
	string getLogFileName()
	{
		return log_file_name;
	}

	string getArchiveName()
	{
		return archive_name;
	}
	void setArchiveName(const char* name);

	void setSeqFileName(const string& seq)
	{
		seq_file_name = seq;
	}
	string getSeqFileName()
	{
		return seq_file_name;
	}

	void setCheckPointRowCount(int row_count = 100)
	{
		check_point_row_count = row_count;
	}
	int getCheckPointRowCount()
	{
		return check_point_row_count;
	}

	void setRemoteFtpDirectory(const char* name)
	{
		remote_ftp_directory = name;
	}
	string getRemoteFtpDirectory()
	{
		return remote_ftp_directory;
	}

	int getCurrentRowCount() const
	{
		return current_row_count;
	}

	int open(const char* mode = "w");
	int write(const char* key, const char* value);
	void flush();
	void ftpUploadLog();
	void ftpUploadArchive();
	bool sshUploadLog();
	bool sshUploadArchive();
	void archiveCurrentLog();
	void close();

	~Log();
};

void Log::setLogFileName(const char* name = NULL)
{
	if (name != NULL)
		log_file_name = name;
	else
	{
		char i_str[10];
		sprintf(i_str, "%d", log_sequence_no);
		log_file_name = i_str;
		log_file_name += "_";
		log_file_name += host_name;
		log_file_name += "_";
		log_file_name += log_type;
		log_file_name += "_";
		log_file_name += cache_type;
		log_file_name += "_";
		log_file_name += cache_storage;
		log_file_name += "_k";
		log_file_name += K;
		log_file_name += ".txt";
	}
}

void Log::setArchiveName(const char* name = NULL)
{
	if (name != NULL)
		archive_name = name;
	else
	{
		char i_str[10];
		sprintf(i_str, "%d", log_sequence_no);
		archive_name = i_str;
		archive_name += "_";
		archive_name += host_name;
		archive_name += "_";
		archive_name += log_type;
		archive_name += ".tar";
	}
}

Log::Log()
{
	log_type = "";
	mode = "";
	char hostname[100];
	gethostname(hostname, 100);
	host_name = hostname;
	monitor_host_name = "";
	monitor_user_name = "";

	log_file_name = "";
	seq_file_name = "";
	archive_name = "";

	log_sequence_no = -1;
	current_segment_no = -1;
	current_row_count = 0;
	check_point_row_count = 3;
	log_file_ptr = NULL;
}

Log::Log(const char* seq_file, const char* type, const char* monitor_host,
		const char* monitor_user, const char* hostname)
{
	seq_file_name = seq_file;

	FILE* seq_file_ptr = fopen(seq_file_name.c_str(), "r+");

	if (seq_file_ptr == NULL)
	{
		log_sequence_no = 0;
		seq_file_ptr = fopen(seq_file_name.c_str(), "w");
		fprintf(seq_file_ptr, "%d\n", log_sequence_no);
		fclose(seq_file_ptr);
	} else
	{
		fscanf(seq_file_ptr, "%d", &log_sequence_no);
		log_sequence_no++;

		fseek(seq_file_ptr, 0, SEEK_SET);
		fprintf(seq_file_ptr, "%d", log_sequence_no);
		fclose(seq_file_ptr);
	}

	log_type = type;

	if (hostname != NULL)
		host_name = hostname;
	else
	{
		char h_name[100];
		gethostname(h_name, 100);
		host_name = h_name;
	}

	this->setLogFileName(NULL);
	this->setArchiveName(NULL);

	mode = "w";
	monitor_host_name = monitor_host;
	monitor_user_name = monitor_user;
	remote_ftp_directory = "/var/ftp/logs";
	check_point_row_count = 3;
	current_row_count = 0;
	current_segment_no = 1;
	log_file_ptr = NULL;
}

Log::Log(int seq_no, const char* type, const char* cache_type, const char* cache_storage, int K, const char* monitor_host,
		const char* monitor_user, const char* hostname)
{
	log_sequence_no = seq_no;
	log_type = type;

	if (hostname != NULL)
		host_name = hostname;
	else
	{
		char h_name[100];
		gethostname(h_name, 100);
		host_name = h_name;
	}

	this->cache_storage = cache_storage;
	this->cache_type = cache_type;

	char i_str[12];
	sprintf(i_str, "%d", K);
	this->K = string(i_str);

	this->setLogFileName(NULL);
	this->setArchiveName(NULL);

	mode = "w";
	monitor_host_name = monitor_host;
	monitor_user_name = monitor_user;
	remote_ftp_directory = "/var/ftp/logs";
	check_point_row_count = 10;
	current_row_count = 0;
	current_segment_no = 1;
	log_file_ptr = NULL;
}

int Log::open(const char* mode)
{
	this->mode = mode;
	if (log_file_ptr != NULL)
		return 0;

	setLogFileName();
	log_file_ptr = fopen(log_file_name.c_str(), mode);

	if (log_file_ptr == NULL)
		return ERROR_OPEN_FILE_FAIL;

	current_row_count = 0;
	return 1;
}

int Log::write(const char* key, const char* value)
{

	if (log_file_ptr == NULL)
		return ERROR_FILE_NOT_OPEN;

	//puts("writing log");

	string text = key;
	text += " ";
	text += value;
	//puts(text.c_str());

	int ret = fprintf(log_file_ptr, "%s\n", text.c_str());
//	printf("Bytes written: %d\n", ret);
//	printf("Line written %s\n", text.c_str());
//	printf("Log file name %s\n", log_file_name.c_str());

//	fflush(log_file_ptr);

	current_row_count++;
//	printf("%d %d\n", current_row_count, check_point_row_count);
	if (current_row_count == check_point_row_count)
	{
		fflush(log_file_ptr);
		fclose(log_file_ptr);
		printf("[Logging Thread:]\tlog flushed to the disk\n");

		log_file_ptr = NULL;

		this->sshUploadLog();
		this->archiveCurrentLog();
		this->open("w+");
	}

	return ret;
}

void Log::flush()
{
	fflush(log_file_ptr);
	fclose(log_file_ptr);
	printf("[Logging Thread:]\tlog flushed to the disk\n");

	log_file_ptr = NULL;

	this->sshUploadLog();
	this->archiveCurrentLog();
	this->open("w+");
}

void Log::archiveCurrentLog()
{
	FILE* shell_pipe = NULL;
	string command = "";
	string new_name = log_file_name;

	char i_str[10];
	sprintf(i_str, "%d", current_segment_no);

	new_name += "_";
	new_name += i_str;

//	rename the log file, mv <log_file_name> <new_name>
	command = "mv ";
	command += log_file_name;
	command += " ";
	command += new_name;

	shell_pipe = popen(command.c_str(), "w");
	pclose(shell_pipe);

//	if the tar file doesn't exist then create it, tar -cf <archive_name> <new_name>
	if (current_segment_no <= 1)
	{
		command = "tar -cf ";
		command += archive_name;
		command += " ";
		command += new_name;
	}
//	otherwise add the current log file in the tarball, tar -rf <archive_name> <new_name>
	else
	{
		command = "tar -rf ";
		command += archive_name;
		command += " ";
		command += new_name;
	}
	puts(command.c_str());
	shell_pipe = popen(command.c_str(), "w");
	pclose(shell_pipe);

	command = "rm ";
	command += new_name;
	shell_pipe = popen(command.c_str(), "w");
	pclose(shell_pipe);
	current_segment_no++;
}
void Log::ftpUploadLog()
{
	string command = "ftp ";
	command += monitor_host_name.c_str();

	FILE* shell_pipe = popen(command.c_str(), "w");

	command = "append ";
	command += log_file_name;
	command += " ";
	command += remote_ftp_directory;
	command += "/";
	command += log_file_name;
	fputs(command.c_str(), shell_pipe);
	pclose(shell_pipe);
}

bool Log::sshUploadLog()
{
	//cat <log)file_name> | ssh <monitor_user_name>@<monitor_host_name> "cat >> <remote_ftp_directory>/<log_file_name>"
	string command = "cat ";
	command += log_file_name;
	command += " | ";
	command += "ssh -o StrictHostKeyChecking=no ";
	command += monitor_user_name;
	command += "@";
	command += monitor_host_name;
	command += " ";
	command += "\" cat >> ";
	command += remote_ftp_directory;
	command += "/";
	command += log_file_name;
	command += "\"";
	//puts(command.c_str());

	FILE* shell_pipe = popen(command.c_str(), "w");
	if (shell_pipe == NULL)
		return false;
	pclose(shell_pipe);
	return true;
}

bool Log::sshUploadArchive()
{
	FILE* shell_pipe = NULL;

	/*sftp <monitor_user_name>@<monitor_host_name>:<remote_ftp_directory>/
	 then put <archive_name> */

	string command = "sftp -o StrictHostKeyChecking=no ";
	command += monitor_user_name;
	command += "@";
	command += monitor_host_name;
	command += ":";
	command += remote_ftp_directory;
	command += "/";

	shell_pipe = popen(command.c_str(), "w");
	if (shell_pipe == NULL)
		return false;

	command = "put ";
	command += archive_name;

	fprintf(shell_pipe, command.c_str());
	fflush(shell_pipe);

	fclose(shell_pipe);

	return true;
}
void Log::ftpUploadArchive()
{
	string command = "ftp ";
	command += monitor_host_name.c_str();

	FILE* shell_pipe = popen(command.c_str(), "w");

	command = "append ";
	command += archive_name;
	command += " ";
	command += remote_ftp_directory;
	command += "/";
	command += archive_name;

	fputs(command.c_str(), shell_pipe);
	pclose(shell_pipe);
}

void Log::close()
{
	if (log_file_ptr != NULL)
	{
		fclose(log_file_ptr);
		log_file_ptr = NULL;
	}

	if (current_row_count > 0)
	{
		this->sshUploadLog();
		this->archiveCurrentLog();
		current_row_count = 0;
	}
	current_segment_no++;
}

Log::~Log()
{
	if (log_file_ptr != NULL)
	{
		fclose(log_file_ptr);
		log_file_ptr = NULL;
	}

	if (current_row_count > 0)
	{
		this->sshUploadLog();
		this->archiveCurrentLog();
		current_row_count = 0;
	}
}
#endif /* LOG_H_ */
