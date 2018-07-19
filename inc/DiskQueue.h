/*
 * DiskQueue.h
 *Purpose: Header file for a demonstration of a queue implemented
 *			  as a linked list structure.  Data type: PCB structure
 *  Created on: Sep 23, 2016
 *      Author: yaqiong
 */

#ifndef DISKQUEUE_H_
#define DISKQUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "Data.h"

struct diskQueuePCBNode
{
	struct Data * node;
    struct diskQueuePCBNode *next;
};

//extern struct PCBNode * pcbnode;

// The queue, head stores the head node of LL and tail stores the
// last node of LL
struct diskQueue
{
    struct diskQueuePCBNode *head, *tail;
};

//extern struct Queue * diskQueue;

// List Function Prototypes
struct diskQueuePCBNode* diskQueue_Create_New_PCBNode(struct Data * datanode);
struct diskQueue *create_diskQueue();
void diskQueue_enQueue(struct diskQueue *q, struct Data* datanode);
struct Data *diskQueue_deQueue(struct diskQueue *q);
struct Data *diskQueue_Get_Fisrt_PCBnode(struct diskQueue *q);
int diskQueue_checkEmpty(struct diskQueue *q);
void diskQueue_printQueue(struct diskQueue *q);
int diskQueue_countQueue(struct diskQueue *q);
long diskQueue_haveSamePCBName(struct diskQueue *q, char *Process_Name);
int diskQueue_findPCBNodeinQueue(struct diskQueue *q, long Process_ID);
long diskQueue_DeletePCBNodeinQueue(struct diskQueue *q, struct Data* datanode, long index);
int diskQueue_checkDiskInUse(struct diskQueue *q, long diskID);
struct Data * diskQueue_GetPCBNodeinQueue(struct diskQueue *q, long contextID);
#endif /* DISKQUEUE_H_ */
