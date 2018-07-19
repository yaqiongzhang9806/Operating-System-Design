/*
 * Data.c
 *
 *  Created on: Sep 23, 2016
 *      Author: yaqiong
 */
#include "global.h"
#include <string.h>

//create pcb node
struct Data * createDataNode(struct Data * datanode){
	struct Data *temp = (struct Data*)malloc(sizeof(struct Data));
	temp->InitialPriority = datanode->InitialPriority;
	temp->Process_ID = datanode->Process_ID;
	temp->Process_Name = strdup(datanode->Process_Name);
	temp->Process_Status = strdup(datanode->Process_Status);
	temp->Process_Pointer = datanode->Process_Pointer;
	temp->Argument = datanode->Argument;
	temp->PageTable = datanode->PageTable;
	temp->Shadow_PageTable = datanode->Shadow_PageTable;
	return temp;
}

//release the pcb node
//used during terminate process
void release_DataNode(struct Data* datanode){
	if(datanode){
		if(datanode->Process_Name)
			free(datanode->Process_Name);
		if(datanode->Process_Status)
			free(datanode->Process_Status);
		free(datanode);
	}
}
