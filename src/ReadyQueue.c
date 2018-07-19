/*
 * ReadyQueue.c
 *
 *  Created on: Sep 23, 2016
 *      Author: yaqiong
 */
#include "ReadyQueue.h"
#include "Data.h"
#include "global.h"
#include <string.h>

// A utility function to create a new linked list node.
struct readyQueuePCBNode* readyQueue_Create_New_PCBNode(struct Data * datanode)
{
    struct readyQueuePCBNode *temp = (struct readyQueuePCBNode*)malloc(sizeof(struct readyQueuePCBNode));
    temp->node = datanode;
    temp->next = NULL;
    return temp;
}


//create an empty queue
struct readyQueue *create_readyQueue()
{
    struct readyQueue *q = (struct readyQueue*)malloc(sizeof(struct readyQueue));
    q->head = q->tail = NULL;
    return q;
}

// The function to add a PCBNODE to Queue
void readyQueue_enQueue(struct readyQueue *q, struct Data* datanode)
{
    // Create a new LL node
    struct readyQueuePCBNode *temp = readyQueue_Create_New_PCBNode(datanode);

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
struct Data *readyQueue_deQueue(struct readyQueue *q)
{
    // If queue is empty, return NULL.
    if (q->head == NULL)
       return NULL;

    // Store previous head and move head one node ahead
    struct readyQueuePCBNode *temp = q->head;
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

struct Data *readyQueue_Get_Fisrt_PCBnode(struct readyQueue *q)
{
    // If queue is empty, return NULL.
    if (q->head == NULL)
       return NULL;

    // get first element
    struct readyQueuePCBNode *temp = q->head;
    return temp->node; //return the first node in the queue
}
//Check the queue is empty or not
int readyQueue_checkEmpty(struct readyQueue *q)
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
//only used during debug
void readyQueue_printQueue(struct readyQueue *q) {
    struct readyQueuePCBNode * current = q->head;
    printf("The process ID of  pcb node in the ready queue is:");
    while (current != NULL) {
        printf("%ld->", current->node->Process_ID);
        current = current->next;
    }
    printf("\n");
}

//count total pcb node number inside the queue
int readyQueue_countQueue(struct readyQueue *q) {
    struct readyQueuePCBNode * current = q->head;
    int count=0;
    while (current != NULL) {
        count+=1;
        current = current->next;
    }
    return count;
}
//this function is not used
long readyQueue_haveSamePCBName(struct readyQueue *q, char *Process_Name) {
    struct readyQueuePCBNode * current = q->head;

    while (current != NULL) {
       if(strcmp(current->node->Process_Name, Process_Name) == 0)
    	   return current->node->Process_ID;   //there are same name in pcbQueue
       else
    	   current = current->next;
    }
    return 0;		  //there donot have same name in pcbQueue
}
//find pcb node with given process id.
//this function is used during terminate process with its id;
int readyQueue_findPCBNodeinQueue(struct readyQueue *q, long Process_ID){
	struct readyQueuePCBNode * current = q->head;
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

struct Data * readyQueue_GetPCBNodeinQueue(struct readyQueue *q, long contextID){
	struct readyQueuePCBNode * current = q->head;

	while (current != NULL){
		if (current->node->Process_Pointer == (long*) contextID){
			return current->node;
		}
		current = current->next;
	}
	//cannot find the pcbnode with the given processID, will return null
	return NULL;
}

//here only remove pcbnode from queue, donot release datanode yet;
//this function is used during terminate process with its id;
long readyQueue_DeletePCBNodeinQueue(struct readyQueue *q, struct Data* datanode, long index) {
    struct readyQueuePCBNode * current = q->head;
    struct readyQueuePCBNode * tempnode = NULL;
    struct Data * tempdata = NULL;
    int i;

    if(index < 0){
    	return -1;
    }

    if (index == 0){
    	if (current->node->Process_ID == datanode->Process_ID){
    		tempdata = readyQueue_deQueue(q);
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
