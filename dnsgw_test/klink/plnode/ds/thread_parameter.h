/*
 * thread_parameter.h
 *
 *  Created on: Dec 20, 2012
 *      Author: sr2chowd
 */

#ifndef THREAD_PARAMETER_H_
#define THREAD_PARAMETER_H_

class ThreadParameter
{
	int thread_id;

public:
	ThreadParameter(){}
	ThreadParameter(int id){ this->thread_id = id; }

	int getThreadId(){ return thread_id; }
	void setThreadId(int id){ this->thread_id = id; }
};


#endif /* THREAD_PARAMETER_H_ */
