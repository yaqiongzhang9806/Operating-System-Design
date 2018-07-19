/*
 * MessageQueue.c
 *
 *  Created on: Dec 8, 2016
 *      Author: yaqiong
 */

#include "MessageQueue.h"
#include "global.h"
#include "Data.h"
#include <string.h>

// A utility function to create a new linked list node.
struct MessageNode* Create_New_MessageNode(struct  Message_Send * datanode)
{
    struct MessageNode *temp = (struct MessageNode*)malloc(sizeof(struct MessageNode));
    temp->message_node = datanode;
    temp->next = NULL;
    return temp;
}


//create an empty queue
struct MessageQueue *create_MessageQueue()
{
    struct MessageQueue *q = (struct MessageQueue *)malloc(sizeof(struct MessageQueue));
    q->head = q->tail = NULL;
    return q;
}

void MessageQueue_enQueue(struct MessageQueue *q, struct Message_Send* datanode)
{
    // Create a new LL node
    struct MessageNode *temp = Create_New_MessageNode(datanode);

    // If queue is empty, then new node is tq_head and tq_tail both
    if (q->tail == NULL)
    {
       q->head = q->tail = temp;
       return;
    }
    // If queue is not empty, add the new node at the end of queue and change tq_tail
    q->tail->next = temp;
    q->tail = temp;
}

struct Message_Send * MessageQueue_deQueue(struct MessageQueue *q)
{
    // If queue is empty, return NULL.
    if (q->head == NULL)
       return NULL;

    // Store previous tq_head and move tq_head one node atq_head
    struct  MessageNode *temp = q->head;
 //   q->tq_head = q->tq_head->next;
    q->head = temp->next;

    // If tq_head becomes NULL, then change tq_tail also as NULL
    if (q->head == NULL)
       q->tail = NULL;

    struct Message_Send * datanode = temp->message_node;
    free(temp); //free the first node in the queue
    temp = NULL;
    return datanode;  //return the first pcb datanode
}

int MessageQueue_checkEmpty(struct MessageQueue *q)
{
    // If queue is empty
    if ((q->head == NULL) && (q->tail == NULL))
    {
       //printf("Queue is empty\n");
       return 0;
    }
    //printf("Queue is not empty\n");
    return 1;
}

int FindPIDinMessageQueue(struct MessageQueue *q, long Process_ID){
	struct  MessageNode * current = q->head;
	int index = 0;
	while (current != NULL){
		if (current->message_node->Target_ProcessID == Process_ID){
			return index;
		}
		index+=1;
		current = current->next;
	}
	index = -1;	//cannot find the pcbnode with the given processID, will return -1
	return index;
}

struct Message_Send * DeleteMessageNodeinQueue(struct MessageQueue *q, long index) {
    struct MessageNode * current = q->head;
    struct MessageNode * tempnode = NULL;
    struct Message_Send * tempdata = NULL;
    int i;

    if(index < 0){
    	return NULL;
    }

    if (index == 0){
    	tempdata = MessageQueue_deQueue(q);
    	return tempdata;

    }

    for (i = 0; i < index-1; i = i + 1) {
    	if (current->next == NULL) {
    		return NULL;
        }
        current = current->next;
    }

    tempnode = current->next;
    tempdata = tempnode->message_node;
    current->next = tempnode->next;				//remove tempnode from the queue

    free(tempnode);
    tempnode = NULL;
    return tempdata;
}

void MessageQueue_printQueue(struct MessageQueue *q) {
    struct  MessageNode * current = q->head;
    printf("The target process ID of  msg node in the message queue is:");
    while (current != NULL) {
        printf("%ld->", current->message_node->Target_ProcessID);
        current = current->next;
    }
    printf("\n");
}
