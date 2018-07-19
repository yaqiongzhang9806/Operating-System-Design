/*
 * TimerQueue.h
 *Purpose: Header file for a demonstration of a queue implemented
 *			  as a linked list structure.  Data type: PCB structure
 *  Created on: Sep 23, 2016
 *      Author: yaqiong
 */

#ifndef TIMERQUEUE_H_
#define TIMERQUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include "Data.h"
#include "global.h"

struct timerQueuePCBNode
{
	struct Data * node;
    struct timerQueuePCBNode *next;
};

//extern struct PCBNode * pcbnode;

// The queue, head stores the head node of LL and tail stores the
// last node of LL
struct timerQueue
{
    struct timerQueuePCBNode *tq_head, *tq_tail;
};

//extern struct Queue * timerQueue;

// List Function Prototypes
struct timerQueuePCBNode* timerQueue_Create_New_PCBNode(struct Data * datanode);
struct timerQueue *create_TimerQueue();
void timerQueue_enQueue(struct timerQueue *q, struct Data* datanode);
struct Data *timerQueue_deQueue(struct timerQueue *q);
struct Data *timerQueue_Get_Fisrt_PCBnode(struct timerQueue *q);
int timerQueue_checkEmpty(struct timerQueue *q);
void timerQueue_insertNode(struct timerQueue *q, struct Data* datanode);
void timerQueue_printQueue(struct timerQueue *q);
int timerQueue_countQueue(struct timerQueue *q);
long timerQueue_haveSamePCBName(struct timerQueue *q, char *Process_Name);
int timerQueue_findPCBNodeinQueue(struct timerQueue *q, long Process_ID);
long timerQueue_DeletePCBNodeinQueue(struct timerQueue *q, struct Data* datanode, long index);
int timerQueue_haveSamePCBID(struct timerQueue *q, long Process_ID);
void TimerEnqueue(struct timerQueue *q, struct Data *data);
struct Data * timerQueue_GetPCBNodeinQueue(struct timerQueue *q, long contextID);
#endif /* TIMERQUEUE_H_ */
