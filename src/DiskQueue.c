/*
 * DiskQueue.c
 *
 *  Created on: Sep 23, 2016
 *      Author: yaqiong
 */
#include "DiskQueue.h"
#include "Data.h"
#include "global.h"
#include <string.h>

// A utility function to create a new linked list node.
struct diskQueuePCBNode* diskQueue_Create_New_PCBNode(struct Data * datanode)
{
    struct diskQueuePCBNode *temp = (struct diskQueuePCBNode*)malloc(sizeof(struct diskQueuePCBNode));
    temp->node = datanode;
    temp->next = NULL;
    return temp;
}


//create an empty queue
struct diskQueue *create_diskQueue()
{
    struct diskQueue *q = (struct diskQueue*)malloc(sizeof(struct diskQueue));
    q->head = q->tail = NULL;
    return q;
}

// The function to add a PCBNODE to Queue
void diskQueue_enQueue(struct diskQueue *q, struct Data* datanode)
{
    // Create a new LL node
    struct diskQueuePCBNode *temp = diskQueue_Create_New_PCBNode(datanode);

    // If queue is empty, then new node is head and tail both
    if (q->tail == NULL)
    {
       q->head = q->tail = temp;
       return;
    }
    // If queue is not empty, add the new node at the end of queue and change tail
    q->tail->next = temp;
    q->tail = temp;
}

// Function to take out the first pcbnode from given queue q
struct Data *diskQueue_deQueue(struct diskQueue *q)
{
    // If queue is empty, return NULL.
    if (q->head == NULL)
       return NULL;

    // Store previous head and move head one node ahead
    struct diskQueuePCBNode *temp = q->head;
 //   q->head = q->head->next;
    q->head = temp->next;

    // If head becomes NULL, then change tail also as NULL
    if (q->head == NULL)
       q->tail = NULL;

    struct Data* datanode = temp->node;
    free(temp); //free the first node in the queue
    temp = NULL;
    return datanode;  //return the first pcb datanode
}

//Get the first pcbnode from the queue,donot remove the first element from the queue
struct Data *diskQueue_Get_Fisrt_PCBnode(struct diskQueue *q)
{
    // If queue is empty, return NULL.
    if (q->head == NULL)
       return NULL;

    // get first element
    struct diskQueuePCBNode *temp = q->head;
    return temp->node; //return the first node in the queue
}
//Check the queue is empty or not
int diskQueue_checkEmpty(struct diskQueue *q)
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

void diskQueue_printQueue(struct diskQueue *q) {
    struct diskQueuePCBNode * current = q->head;
    printf("The process ID of  pcb node in the disk queue is:");
    while (current != NULL) {
        printf("%ld->", current->node->Process_ID);
        current = current->next;
    }
    printf("\n");
}
//count total pcb node number inside the queue
int diskQueue_countQueue(struct diskQueue *q) {
    struct diskQueuePCBNode * current = q->head;
    int count=0;
    while (current != NULL) {
        count+=1;
        current = current->next;
    }
    return count;
}
//this function is not used
long diskQueue_haveSamePCBName(struct diskQueue *q, char *Process_Name) {
    struct diskQueuePCBNode * current = q->head;

    while (current != NULL) {
       if(strcmp(current->node->Process_Name, Process_Name) == 0)
    	   return current->node->Process_ID;   //there are same name in pcbQueue
       else
    	   current = current->next;
    }
    return 0;		  //there donot have same name in pcbQueue
}
//compare the given diskID with which in disk queue
//if there have same diskID, return 1, otherwise return 0;
int diskQueue_checkDiskInUse(struct diskQueue *q, long diskID){
	struct diskQueuePCBNode * current = q->head;

	while (current != NULL){
		if (current->node->Argument == diskID){
			return 1;
		}
		current = current->next;
	}
	//cannot find the pcbnode with the given diskID
	return 0;
}
//find pcb node with given process id.
//this function is used during terminate process with its id;
int diskQueue_findPCBNodeinQueue(struct diskQueue *q, long Process_ID){
	struct diskQueuePCBNode * current = q->head;
	int index = 0;
	while (current != NULL){
		if (current->node->Process_ID == Process_ID){
			return index;
		}
		index+=1;
		current = current->next;
	}
	index = -1;	//cannot find the pcbnode with the given processID, will return -1
	return index;
}

struct Data * diskQueue_GetPCBNodeinQueue(struct diskQueue *q, long contextID){
	struct diskQueuePCBNode * current = q->head;

	while (current != NULL){
		if (current->node->Process_Pointer == (long*) contextID){
			return current->node;
		}
		current = current->next;
	}
	//cannot find the pcbnode with the given processID, will return null
	return NULL;
}

//here only remove pcbnode from queue, donot release datanode yet
//this function is used during terminate process with its id;
long diskQueue_DeletePCBNodeinQueue(struct diskQueue *q, struct Data* datanode, long index) {
    struct diskQueuePCBNode * current = q->head;
    struct diskQueuePCBNode * tempnode = NULL;
    struct Data * tempdata = NULL;
    int i;

    if(index < 0){
    	return -1;
    }

    if (index == 0){
    	if (current->node->Process_ID == datanode->Process_ID){
    		tempdata = diskQueue_deQueue(q);
    		return tempdata->Process_ID;
    	}
    }

    for (i = 0; i < index-1; i = i + 1) {
    	if (current->next == NULL) {
                return -1;
        }
        current = current->next;
    }

    tempnode = current->next;
    tempdata = tempnode->node;
    if (tempdata->Process_ID == datanode->Process_ID){
    	current->next = tempnode->next;				//remove tempnode from the queue
    }
    free(tempnode);
    tempnode = NULL;
    return tempdata->Process_ID;
}
