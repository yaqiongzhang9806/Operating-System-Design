/*
 * DiskManager.h
 *
 *  Created on: Nov 8, 2016
 *      Author: yaqiong
 */

#ifndef DISKMANAGER_H_
#define DISKMANAGER_H_

#include "global.h"

typedef struct DiskManager{
	//int disk_in_use[MAX_NUMBER_OF_DISKS];
	int disk_in_use[8];
	int is_All_used;
	int current_available_Disk;
}DiskManager;


#endif /* DISKMANAGER_H_ */
