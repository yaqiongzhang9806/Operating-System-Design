/*
 * TimerQueue.c
 *
 *  Created on: Sep 23, 2016
 *      Author: yaqiong
 */
#include "TimerQueue.h"
#include "global.h"
#include "Data.h"
#include <string.h>

/*
void Dolock(long lockType) {
	INT32 LockResult;
	READ_MODIFY(lockType, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}*/


// A utility function to create a new linked list node.
struct timerQueuePCBNode* timerQueue_Create_New_PCBNode(struct Data * datanode)
{
    struct timerQueuePCBNode *temp = (struct timerQueuePCBNode*)malloc(sizeof(struct timerQueuePCBNode));
    temp->node = datanode;
    temp->next = NULL;
    return temp;
}


//create an empty queue
struct timerQueue *create_TimerQueue()
{
    struct timerQueue *q = (struct timerQueue*)malloc(sizeof(struct timerQueue));
    q->tq_head = q->tq_tail = NULL;
    return q;
}

// The function to add a PCBNODE to Queue
void timerQueue_enQueue(struct timerQueue *q, struct Data* datanode)
{
    // Create a new LL node
    struct timerQueuePCBNode *temp = timerQueue_Create_New_PCBNode(datanode);

    // If queue is empty, then new node is tq_head and tq_tail both
    if (q->tq_tail == NULL)
    {
       q->tq_head = q->tq_tail = temp;
       return;
    }
    // If queue is not empty, add the new node at the end of queue and change tq_tail
    q->tq_tail->next = temp;
    q->tq_tail = temp;
}

// Function to take out the first pcbnode from given queue q
struct Data *timerQueue_deQueue(struct timerQueue *q)
{
    // If queue is empty, return NULL.
    if (q->tq_head == NULL)
       return NULL;

    // Store previous tq_head and move tq_head one node atq_head
    struct timerQueuePCBNode *temp = q->tq_head;
 //   q->tq_head = q->tq_head->next;
    q->tq_head = temp->next;

    // If tq_head becomes NULL, then change tq_tail also as NULL
    if (q->tq_head == NULL)
       q->tq_tail = NULL;

    struct Data* datanode = temp->node;
    free(temp); //free the first node in the queue
    temp = NULL;
    return datanode;  //return the first pcb datanode
}

//Get the first pcbnode from the queue,donot remove the first element from the queue

struct Data *timerQueue_Get_Fisrt_PCBnode(struct timerQueue *q)
{
    // If queue is empty, return NULL.
    if (q->tq_head == NULL)
       return NULL;

    // get first element
    struct timerQueuePCBNode *temp = q->tq_head;
    return temp->node; //return the first node in the queue
}
//Check the queue is empty or not
int timerQueue_checkEmpty(struct timerQueue *q)
{
    // If queue is empty
    if ((q->tq_head == NULL) && (q->tq_tail == NULL))
    {
       //printf("Queue is empty\n");
       return 0;
    }
    //printf("Queue is not empty\n");
    return 1;
}
//this insertNode function is for timerQueue only
//insert pcb node in timer queue with smallest wakeup time in the front;
void timerQueue_insertNode(struct timerQueue *q, struct Data* datanode){
	struct timerQueuePCBNode * current = q->tq_head;
	struct timerQueuePCBNode * current_front;
	struct timerQueuePCBNode * pcbNode = timerQueue_Create_New_PCBNode(datanode);
	// If queue is empty
	if (q->tq_tail == NULL){
		timerQueue_enQueue(q,datanode);
	}
	else{
		while (current != NULL) {
			if (current->node-> Argument < datanode-> Argument){	//compare the T value
				current_front = current;
				current = current->next;
			}
			else if (current == q->tq_head){
				q->tq_head = pcbNode;
				pcbNode->next = current;
				break;
			}
			else{
				current_front->next = pcbNode;
				pcbNode->next = current;
				break;
			}
		}
		if (pcbNode->next == NULL && q->tq_tail != pcbNode->next)
			timerQueue_enQueue(q, datanode);
	}
}

void timerQueue_printQueue(struct timerQueue *q) {
    struct timerQueuePCBNode * current = q->tq_head;
    printf("The process ID of  pcb node in the timer queue is:");
    while (current != NULL) {
        printf("%ld->", current->node->Process_ID);
        current = current->next;
    }
    printf("\n");
}
//count total pcb node number inside the queue
int timerQueue_countQueue(struct timerQueue *q) {
    struct timerQueuePCBNode * current = q->tq_head;
    int count=0;
    while (current != NULL) {
        count+=1;
        current = current->next;
    }
    return count;
}
//not used
long timerQueue_haveSamePCBName(struct timerQueue *q, char *Process_Name) {
    struct timerQueuePCBNode * current = q->tq_head;

    while (current != NULL) {
       if(strcmp(current->node->Process_Name, Process_Name) == 0)
    	   return current->node->Process_ID;   //there are same name in pcbQueue
       else
    	   current = current->next;
    }
    return 0;		  //there donot have same name in pcbQueue
}
//not used

int timerQueue_haveSamePCBID(struct timerQueue *q, long Process_ID) {
    struct timerQueuePCBNode * current = q->tq_head;

    while (current != NULL) {
       if(current->node->Process_ID == Process_ID)
    	   return 1;   //there are same id in pcbQueue
       else
    	   current = current->next;
    }
    return 0;		  //there donot have same id in pcbQueue
}
//find pcb node with given process id.
//this function is used during terminate process with its id;
int timerQueue_findPCBNodeinQueue(struct timerQueue *q, long Process_ID){
	struct timerQueuePCBNode * current = q->tq_head;
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

struct Data * timerQueue_GetPCBNodeinQueue(struct timerQueue *q, long contextID){
	struct timerQueuePCBNode * current = q->tq_head;

	while (current != NULL){
		if (current->node->Process_Pointer == (long*) contextID){
			return current->node;
		}

		current = current->next;
	}
	//cannot find the pcbnode with the given processID, will return null
	return NULL;
}

/*struct PCBNode * GetPCBNodeinQueue(struct Queue *q, long Process_ID){
	struct PCBNode * current = q->tq_head;

	while (current != NULL){
		if (current->Process_ID == Process_ID){
			return current;
		}
		current = current->next;
	}
	return NULL;
}
*/
//here only remove pcbnode from queue, donot release datanode yet;
//this function is used during terminate process with its id;
long timerQueue_DeletePCBNodeinQueue(struct timerQueue *q, struct Data* datanode, long index) {
    struct timerQueuePCBNode * current = q->tq_head;
    struct timerQueuePCBNode * tempnode = NULL;
    struct Data * tempdata = NULL;
    int i;

    if(index < 0){
    	return -1;
    }

    if (index == 0){
    	if (current->node->Process_ID == datanode->Process_ID){
    		tempdata = timerQueue_deQueue(q);
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
