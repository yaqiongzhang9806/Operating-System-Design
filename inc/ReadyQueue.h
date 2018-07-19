/*
 * ReadyQueue.h
 *Purpose: Header file for a demonstration of a queue implemented
 *			  as a linked list structure.  Data type: PCB structure
 *  Created on: Sep 23, 2016
 *      Author: yaqiong
 */

#ifndef READYQUEUE_H_
#define READYQUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "Data.h"

struct readyQueuePCBNode
{
	struct Data * node;
    struct readyQueuePCBNode *next;
};

//extern struct PCBNode * pcbnode;

// The queue, head stores the head node of LL and tail stores the
// last node of LL
struct readyQueue
{
    struct readyQueuePCBNode *head, *tail;
};

//extern struct Queue * readyQueue;

// List Function Prototypes
struct readyQueuePCBNode* readyQueue_Create_New_PCBNode(struct Data * datanode);
struct readyQueue *create_readyQueue();
void readyQueue_enQueue(struct readyQueue *q, struct Data* datanode);
struct Data *readyQueue_deQueue(struct readyQueue *q);
struct Data *readyQueue_Get_Fisrt_PCBnode(struct readyQueue *q);
int readyQueue_checkEmpty(struct readyQueue *q);
void readyQueue_printQueue(struct readyQueue *q);
int readyQueue_countQueue(struct readyQueue *q);
long readyQueue_haveSamePCBName(struct readyQueue *q, char *Process_Name);
int readyQueue_findPCBNodeinQueue(struct readyQueue *q, long Process_ID);
long readyQueue_DeletePCBNodeinQueue(struct readyQueue *q, struct Data* datanode, long index);
struct Data * readyQueue_GetPCBNodeinQueue(struct readyQueue *q, long contextID);
#endif /* READYQUEUE_H_ */
