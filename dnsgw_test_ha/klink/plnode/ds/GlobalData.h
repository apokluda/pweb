/*
 * GlobalData.h
 *
 *  Created on: 2012-12-07
 *      Author: sr2chowd
 */

#ifndef GLOBALDATA_H_
#define GLOBALDATA_H_

#include "../protocol/plexus/rm/ReadMullerCode.h"

class GlobalData
{
public:
	static string config_file_name;
	static int network_size;
};

string GlobalData::config_file_name = "config_project";
int GlobalData::network_size;
#endif /* GLOBALDATA_H_ */
