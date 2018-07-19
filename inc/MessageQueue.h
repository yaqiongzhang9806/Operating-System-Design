/*
 * MessageQueue.h
 *
 *  Created on: Dec 8, 2016
 *      Author: yaqiong
 */

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include "Data.h"
#include "global.h"
#include "Message.h"

struct MessageNode
{
	struct Message_Send * message_node;
    struct MessageNode *next;
};



// The queue, head stores the head node of LL and tail stores the
// last node of LL
struct MessageQueue
{
    struct MessageNode *head, *tail;
};


struct MessageNode* Create_New_MessageNode(struct  Message_Send * datanode);
struct MessageQueue *create_MessageQueue();
void MessageQueue_enQueue(struct MessageQueue *q, struct Message_Send* datanode);
struct Message_Send * MessageQueue_deQueue(struct MessageQueue *q);
int MessageQueue_checkEmpty(struct MessageQueue *q);
int FindPIDinMessageQueue(struct MessageQueue *q, long Process_ID);
struct Message_Send * DeleteMessageNodeinQueue(struct MessageQueue *q, long index);
void MessageQueue_printQueue(struct MessageQueue *q);
#endif /* MESSAGEQUEUE_H_ */
