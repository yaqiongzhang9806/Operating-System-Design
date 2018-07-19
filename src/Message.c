/*
 * Message.c
 *
 *  Created on: Dec 8, 2016
 *      Author: yaqiong
 */

#include "global.h"
#include "Message.h"
#include <string.h>
//create pcb node
struct Message_Send * createMessageNode(struct Message_Send * datanode){
	struct Message_Send *temp = (struct Message_Send *)malloc(sizeof(struct Message_Send));
	temp->Target_ProcessID = datanode->Target_ProcessID;
	temp->Source_ProcessID = datanode->Source_ProcessID;
	temp->MessageSendLength = datanode->MessageSendLength;
	temp->MessageBuffer = strdup(datanode->MessageBuffer);
	temp->SendisMade = datanode->SendisMade;
	return temp;
}

//release the pcb node
//used during terminate process
void release_MessageNode(struct Message_Send* datanode){
	if(datanode){
		if(datanode->MessageBuffer)
			free(datanode->MessageBuffer);
		free(datanode);
	}
}
