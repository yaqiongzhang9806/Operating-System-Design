/*
 * Data.h
 *Purpose: Header file for a demonstration of a queue implemented
 *			  as a linked list structure.  Data type: PCB structure
 *  Created on: Sep 23, 2016
 *      Author: yaqiong
 */

#ifndef DATA_H_
#define DATA_H_

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "ShadowPageTable.h"
#include "shared_manager.h"
#include "Message.h"

struct Directory
{
	long DiskID;
	long Sector;
	char* DirName;
};

struct Data
{
	long InitialPriority;
	long Process_ID;
	char *Process_Name;
	char *Process_Status;
	long *Process_Pointer;
	long Argument;
	short * PageTable;
	struct Shadow_PageTable * Shadow_PageTable;
	struct Shared_Manager Shared_Manager;
	struct Directory current_Dir;
	struct Directory parent_Dir;
	struct Message_Send send_message;
	struct Message_Receive receive_message;
};



// List Function Prototypes
struct Data * createDataNode(struct Data * datanode);
void release_DataNode(struct Data* datanode);

#endif /* DATA_H_ */
