/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Portability attempted.
 1.3 July     1992: More Portability enhancements.
 Add call to SampleCode.
 1.4 December 1992: Limit (temporarily) printout in
 interrupt handler.  More portability.
 2.0 January  2000: A number of small changes.
 2.1 May      2001: Bug fixes and clear STAT_VECTOR
 2.2 July     2002: Make code appropriate for undergrads.
 Default program start is in test0.
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 4.0  July    2013: Major portions rewritten to support multiple threads
 4.20 Jan     2015: Thread safe code - prepare for multiprocessors
 ************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
#include 			 <math.h>

// Allows the OS and the hardware to agree on where faults occur
extern void *TO_VECTOR[];

char *call_names[]={ "mem_read ", "mem_write", "read_mod ", "get_time ",
		"sleep    ", "get_pid  ", "create   ", "term_proc", "suspend  ",
		"resume   ", "ch_prior ", "send     ", "receive  ", "PhyDskRd ",
		"PhyDskWrt", "def_sh_ar", "Format   ", "CheckDisk", "Open_Dir ",
        "OpenFile ", "Crea_Dir ", "Crea_File", "ReadFile ", "WriteFile",
		"CloseFile", "DirContnt", "Del_Dir  ", "Del_File "};




#define  MAX_NUM_PROCESS   16

#define	TimerQueueLock					MEMORY_INTERLOCK_BASE+0
#define DiskQueueLock 					MEMORY_INTERLOCK_BASE+1
#define ReadyQueueLock					MEMORY_INTERLOCK_BASE+2
#define MultiProcess_Controller_Lock	MEMORY_INTERLOCK_BASE+3
#define count_Process_Lock				MEMORY_INTERLOCK_BASE+4
#define Inode_Id_Lock					MEMORY_INTERLOCK_BASE+5
#define current_victim_Lock				MEMORY_INTERLOCK_BASE+6
#define pcbnode_Lock					MEMORY_INTERLOCK_BASE+7
#define Disk_is_Formatted_Lock			MEMORY_INTERLOCK_BASE+8
#define Format_is_complete_Lock			MEMORY_INTERLOCK_BASE+9
#define FrameTable_Lock					MEMORY_INTERLOCK_BASE+10
#define frame_manager_Lock				MEMORY_INTERLOCK_BASE+11
#define SwapBitmap_Lock					MEMORY_INTERLOCK_BASE+12
#define	diskmanager_Lock				MEMORY_INTERLOCK_BASE+13

#define                  DO_LOCK                     1
#define                  DO_UNLOCK                   0
#define                  SUSPEND_UNTIL_LOCKED        TRUE
#define                  DO_NOT_SUSPEND              FALSE

#define PTBL_RES_BIT	0x1000
#define Max_Message_Box 7
/*********************************************************************
*	 my global variables
*********************************************************************/
int is_MultiProcess = 0;
int MultiProcess_Controller = 0;
int count_Process = 0;			//count the total process number <  MAX_NUM_PROCESS
int Inode_Id = 1;
int current_victim = 0;
INT32 OurSharedID = 0;
struct Data * pcbnode[MAX_NUM_PROCESS];
//struct Data * current_execute_PCB;

struct timerQueue * timerQueue;
struct readyQueue * ReadyQueue;
struct diskQueue * diskQueue;
struct MessageQueue * MessageQueue;
int MailBox[Max_Message_Box];
int Disk_is_Formatted[MAX_NUMBER_OF_DISKS];
int Format_is_complete[MAX_NUMBER_OF_DISKS];

frame FrameTable[NUMBER_PHYSICAL_PAGES];
frameManager frame_manager;

int SwapBitmap[MAX_NUMBER_OF_DISKS][SwapSize*4];
DiskManager diskmanager;			//to be used during memory management: swap in/out data

// Prototypes For Functions In This File
struct Data * FindExecutingPCBNode(long contextID);
long CheckSamePCBName(char *Process_Name);
void PutNodeinTimerQueue(long sleeptime);
void StartTimer(SYSTEM_CALL_DATA *SystemCallData);
void Dispatcher();
void wasteTime();
void Make_Ready_to_Run(struct Data * datanode);
void InterruptHandler(void);
void Clear_Interruppt_Device(INT32 DeviceID);
void FaultHandler(void);
void Terminate_Whole_Simulation();
long* Terminate_with_ProcessID(long ProcessID);
void TerminateProsess(SYSTEM_CALL_DATA *SystemCallData);
void CreatProcess(SYSTEM_CALL_DATA *SystemCallData);
void GetProcessID(SYSTEM_CALL_DATA * SystemCallData);
void DiskWrite(SYSTEM_CALL_DATA *SystemCallData);
void DiskRead(SYSTEM_CALL_DATA *SystemCallData);
void svc(SYSTEM_CALL_DATA *SystemCallData);
long Get_Current_ContextID();
void StartContext(struct Data *nextStartNode);
void osInit(int argc, char *argv[]);
void Dolock(long lockType);
void DoTrylock(long lockType);
void DoUnlock(long lockType);
void DiskCheck(SYSTEM_CALL_DATA *SystemCallData);
void DiskFormat(SYSTEM_CALL_DATA *SystemCallData);
char* DiskSector(long DiskID, long Sector);
long DiskCheckBitmap(long DiskID);
void DiskOpenDIR(SYSTEM_CALL_DATA *SystemCallData);
void DiskUpdateBitmap(long DiskID, long updateSector);
void DiskRemoveUpdateBitmap(long DiskID, long updateSector);
void scheduler_printer(long targetPid,char* targetName);
struct Data * MyInitial(char * process_name, long contextID, short * PageTable);
void Define_Shared_Area(SYSTEM_CALL_DATA * SystemCallData);
void Send_MESSAGE(SYSTEM_CALL_DATA * SystemCallData);
int FindFreeFrame();
/*********************************************************************
*	 Project2 Code
*
*********************************************************************/
void Define_Shared_Area(SYSTEM_CALL_DATA * SystemCallData){
	int i;
	short Page_No;
	int frame_index;
	//get current pcbnode
	long current_contextID = Get_Current_ContextID();
	Dolock(pcbnode_Lock);
	struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
	DoUnlock(pcbnode_Lock);
	//get current PageTable
	short * CURR_PAGE_TBL = current_pcb->PageTable;
	//update shared_manager
	struct Shared_Manager CURR_Shared_Manager = current_pcb->Shared_Manager;
	CURR_Shared_Manager.StartingAddressOfSharedArea = (int*) SystemCallData->Argument[0]; // Local Virtual Address
	CURR_Shared_Manager.PagesInSharedArea = (int*) SystemCallData->Argument[1];               // Size of shared area
	strcpy(CURR_Shared_Manager.AreaTag,(char*)SystemCallData->Argument[2]); // How OS knows to put all in same shared area

	//update message_send
	current_pcb->send_message.SendisMade = 0;

	//set shared frame for master
	if(frame_manager.is_shared == 0){
		for (i = 0; i< CURR_Shared_Manager.PagesInSharedArea; i++){
			Page_No = (CURR_Shared_Manager.StartingAddressOfSharedArea / PGSIZE) + i;

			frame_index = frame_manager.current_available_frame;
			//update shared_frame array to store shared frame index
			frame_manager.shared_frame[frame_manager.num_of_shared_frame] = frame_index;
			frame_manager.num_of_shared_frame += 1;
			//update the FrameTable
			Dolock(FrameTable_Lock);
			FrameTable[frame_index].is_Occupied = 1;
			FrameTable[frame_index].is_shared_frame = 1;
			FrameTable[frame_index].Process_ID = current_pcb->Process_ID;
			FrameTable[frame_index].Page_No = Page_No;
			DoUnlock(FrameTable_Lock);
			//update pagetable
			CURR_PAGE_TBL[Page_No] = (UINT16)PTBL_VALID_BIT | (frame_index & PTBL_PHYS_PG_NO);

			//update frame counter
			//Dolock(frame_manager_Lock);
			frame_manager.counter_occupied_frame +=1;
			//DoUnlock(frame_manager_Lock);

			//Dolock(frame_manager_Lock);
			if(frame_manager.counter_occupied_frame < 64){
				frame_manager.current_available_frame = FindFreeFrame();
			}
			else {//frame_manager.counter_occupied_frame == 64
				frame_manager.isFull = 1;
				frame_manager.current_available_frame = -1;
			}
		}
		frame_manager.is_shared = 1;
	}
	else{//set shared frames to slaves
		for (i = 0; i< CURR_Shared_Manager.PagesInSharedArea; i++){
			Page_No = (CURR_Shared_Manager.StartingAddressOfSharedArea / PGSIZE) + i;
			frame_index = frame_manager.shared_frame[i];

			//update the FrameTable
			Dolock(FrameTable_Lock);
			FrameTable[frame_index].is_Occupied = 1;
			FrameTable[frame_index].is_shared_frame = 1;
			FrameTable[frame_index].Process_ID = current_pcb->Process_ID;
			FrameTable[frame_index].Page_No = Page_No;
			DoUnlock(FrameTable_Lock);
			//update pagetable
			CURR_PAGE_TBL[Page_No] = (UINT16)PTBL_VALID_BIT | (frame_index & PTBL_PHYS_PG_NO);
		}
	}
	// Unique number supplied by OS. First must be 0 and the
	// return values must be monotonically increasing for each
	// process that's doing the Shared Memory request.
	*SystemCallData->Argument[3]  = (long)OurSharedID;
	//update OurSharedID
	OurSharedID = OurSharedID + 1;
	*SystemCallData->Argument[4] = ERR_SUCCESS;
}

void Send_MESSAGE(SYSTEM_CALL_DATA * SystemCallData){
	int i;
	//get current pcbnode
	long current_contextID = Get_Current_ContextID();
	Dolock(pcbnode_Lock);
	struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
	DoUnlock(pcbnode_Lock);

	//update Message_Send
	current_pcb->send_message.Target_ProcessID = (long*) SystemCallData->Argument[0];
	current_pcb->send_message.MessageBuffer = (char*)SystemCallData->Argument[1];
	current_pcb->send_message.MessageSendLength = (int*) SystemCallData->Argument[2];


	int has_Target_Process = 0;
	for (i=0; i< MAX_NUM_PROCESS; i++) {
		if(pcbnode[i]!= NULL){
			if((pcbnode[i]->Process_ID == current_pcb->send_message.Target_ProcessID) | (current_pcb->send_message.Target_ProcessID == (-1))){
				if (current_pcb->send_message.MessageSendLength > 0){
					has_Target_Process = 1;
					break;
				}
			}
		}
	}

	if(has_Target_Process == 1){
		current_pcb->send_message.SendisMade = 1;
		//put the message into MessageQueue
		struct Message_Send temp_msg_node = {current_pcb->send_message.Target_ProcessID,current_pcb->Process_ID, current_pcb->send_message.MessageBuffer,
				current_pcb->send_message.MessageSendLength,current_pcb->send_message.SendisMade};
		struct Message_Send * temp = createMessageNode(&temp_msg_node);
		//put the new message node to MessageQueue
		MessageQueue_enQueue(MessageQueue, temp);
		//MessageQueue_printQueue(MessageQueue);
		*SystemCallData->Argument[3] = ERR_SUCCESS;
	}
	else{
		*SystemCallData->Argument[3] = ERR_BAD_PARAM;
	}
}

void Receive_MESSAGE(SYSTEM_CALL_DATA * SystemCallData){

	//get current pcbnode
	long current_contextID = Get_Current_ContextID();
	Dolock(pcbnode_Lock);
	struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
	DoUnlock(pcbnode_Lock);

	//update Message_Send
	//int is_Target_Process = 0;
	int is_Received = 0;
	current_pcb->receive_message.Source_ProcessID = (int*) SystemCallData->Argument[0];
	current_pcb->receive_message.MessageReceiveLength = (int*) SystemCallData->Argument[2];
	//If SourcePID = -1, then receive from any sender who has specifically
	//targeted you or who has done a broadcast.
	if (current_pcb->receive_message.Source_ProcessID == (-1)){
		//check MessageQueue
		long msg_index = FindPIDinMessageQueue(MessageQueue, current_pcb->Process_ID);
		if (msg_index > (-1)){
			//is_Target_Process = 1;
			struct Message_Send * receive_msg_data = DeleteMessageNodeinQueue(MessageQueue, msg_index);
			if (receive_msg_data->MessageSendLength <= current_pcb->receive_message.MessageReceiveLength){
				current_pcb->receive_message.Source_ProcessID = receive_msg_data->Source_ProcessID;
				current_pcb->receive_message.MessageReceiveLength = receive_msg_data->MessageSendLength;
				current_pcb->receive_message.ReceiveBuffer = receive_msg_data->MessageBuffer;
				release_MessageNode(receive_msg_data);
				is_Received = 1;
			}
		}
	}


	if (is_Received == 1){
		*SystemCallData->Argument[3] = current_pcb->receive_message.MessageReceiveLength;
		*SystemCallData->Argument[4] = current_pcb->receive_message.Source_ProcessID;
		//strcpy((char*)SystemCallData->Argument[1], current_pcb->receive_message.ReceiveBuffer);
		*SystemCallData->Argument[5] = ERR_SUCCESS;
	}
	else{
		*SystemCallData->Argument[4] = ERR_BAD_PARAM;
		//mark this ProcessID( which is waiting receiving msg) in MailBox
		MailBox[current_pcb->Process_ID] = 1;
		Dispatcher();
	}

}
/*********************************************************************
	 scheduler printer.
	 The main job here is to fill in the structure that is passed
	 to the printer.
*********************************************************************/
void scheduler_printer(long targetPid,char* targetName){

	SP_INPUT_DATA SPData;
	memset(&SPData, 0, sizeof(SP_INPUT_DATA));
	strcpy(SPData.TargetAction, targetName);

	long current_contextID = Get_Current_ContextID();
	//Dolock(pcbnode_Lock);
	struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
	//DoUnlock(pcbnode_Lock);
	SPData.CurrentlyRunningPID = current_pcb->Process_ID;
	//SPData.CurrentlyRunningPID =(INT16)current_execute_PCB->Process_ID;
	//count_Process = 0;	//when start a new test, initial to count the number of processes created by this new test.

	SPData.TargetPID = (INT16)targetPid;
	// The NumberOfRunningProcesses  used here is for a future implementation
	// when we are running multiple processors.  For right now, set this to 0
	// so it won't be printed out.
	SPData.NumberOfRunningProcesses = 0;


	SPData.NumberOfReadyProcesses = readyQueue_countQueue(ReadyQueue);   // Processes ready to run
	int i=0;
	struct readyQueuePCBNode * readyQueue_current = ReadyQueue->head;
	while (readyQueue_current != NULL) {
		SPData.ReadyProcessPIDs[i] = readyQueue_current->node->Process_ID;
		readyQueue_current = readyQueue_current->next;
		i++;
	}
	//SPData.NumberOfReadyProcesses = i+1;

	//Dolock(TimerQueueLock);
	SPData.NumberOfTimerSuspendedProcesses = timerQueue_countQueue(timerQueue) ;
	//DoUnlock(TimerQueueLock);
	int j = 0;
	//Dolock(TimerQueueLock);
	struct timerQueuePCBNode * timerQueue_current = timerQueue->tq_head;
	//DoUnlock(TimerQueueLock);
	//Dolock(TimerQueueLock);
	while (timerQueue_current != NULL) {
		SPData.TimerSuspendedProcessPIDs[j]= timerQueue_current->node->Process_ID;
		timerQueue_current = timerQueue_current->next;
		j++;
	}
	//SPData.NumberOfTimerSuspendedProcesses = j+1;
	//DoUnlock(TimerQueueLock);

	//Dolock(DiskQueueLock);
	SPData.NumberOfDiskSuspendedProcesses = diskQueue_countQueue(diskQueue);
	//DoUnlock(DiskQueueLock);
	int k = 0;
	//Dolock(DiskQueueLock);
	struct diskQueuePCBNode * diskQueue_current = diskQueue->head;
	//DoUnlock(DiskQueueLock);
	//Dolock(DiskQueueLock);
	while (diskQueue_current != NULL) {
		SPData.DiskSuspendedProcessPIDs[k] = diskQueue_current->node->Process_ID;
		diskQueue_current = diskQueue_current->next;
		k++;
	}
	//SPData.NumberOfDiskSuspendedProcesses = k+1;
	//DoUnlock(DiskQueueLock);

	CALL(SPPrintLine(&SPData));
}
/************************************************************************
 * Lock for queues; comes in: lockType
 ************************************************************************/
void Dolock(long lockType) {
	INT32 LockResult;
	READ_MODIFY(lockType, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}

void DoTrylock(long lockType) {
	INT32 LockResult;
	READ_MODIFY(lockType, DO_LOCK, DO_NOT_SUSPEND, &LockResult);
}

void DoUnlock(long lockType) {
	INT32 LockResult;
	READ_MODIFY(lockType, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}
/************************************************************************
 * Find the free available frame;
 * out: current free frame;
 ************************************************************************/
int FindFreeFrame(){
	int i;
	for (i=0; i < NUMBER_PHYSICAL_PAGES; i++){

		if (FrameTable[i].is_Occupied == 0){
			return i;
		}
	}
	return -1;  //if no free frame return -1
}
/************************************************************************
 * Find the free available swap Sector;
 * out: current free Sector in given disk;
 ************************************************************************/
int CheckSwapBitmap(long DiskID){
	int i;
	int j;
	int num_used_disk = 0;

	for (j=0;j < SwapSize*4;j++){

		int temp_swapbitmap = SwapBitmap[DiskID][j];

		if (temp_swapbitmap == 0){
			if (j == (SwapSize*4-1)){
				Dolock(diskmanager_Lock);
				diskmanager.disk_in_use[DiskID] = 1;
				DoUnlock(diskmanager_Lock);
			}
			return j;
		}

	}
	Dolock(diskmanager_Lock);
	diskmanager.disk_in_use[DiskID] = 1;
	DoUnlock(diskmanager_Lock);

	for (i=0; i < MAX_NUMBER_OF_DISKS; i++){
		Dolock(diskmanager_Lock);
		if (diskmanager.disk_in_use[i] == 1){
			num_used_disk++;
		}
		DoUnlock(diskmanager_Lock);
	}

	if (num_used_disk == MAX_NUMBER_OF_DISKS){
		Dolock(diskmanager_Lock);
		diskmanager.is_All_used = 1;
		DoUnlock(diskmanager_Lock);
	}
	return -1;	//no free swap area any more
}


/************************************************************************
 * Find victim to swap out follow LRU algorithm;
 * out: current free frame index;
 ************************************************************************/
int Find_Victim(){
	int count_modified_bit = 0;
	int count_check = 0;
	int reference_bit, modified_bit;
	struct Data * temp_pcbnode;
	long Pid;
	long Page_No;
	int temp_victim;
	while(1){
		//Dolock(current_victim_Lock);
		temp_victim = current_victim;
		//DoUnlock(current_victim_Lock);

		Pid = FrameTable[temp_victim].Process_ID;
		Dolock(pcbnode_Lock);
		temp_pcbnode = pcbnode[Pid];
		DoUnlock(pcbnode_Lock);
		//check PageTable: reference bit and modified bit

		Page_No = FrameTable[temp_victim].Page_No;
		reference_bit = (temp_pcbnode->PageTable[Page_No]) & PTBL_REFERENCED_BIT;
		modified_bit = (temp_pcbnode->PageTable[Page_No]) & PTBL_MODIFIED_BIT;
		count_check++;
		if (modified_bit > 0){
			count_modified_bit++;
			if(reference_bit > 0){
				//set refrence bit = 0
				temp_pcbnode->PageTable[FrameTable[temp_victim].Page_No] = temp_pcbnode->PageTable[FrameTable[temp_victim].Page_No] ^ PTBL_REFERENCED_BIT;
			}
			else if (count_modified_bit == count_check && count_check == 64){
				return temp_victim;
			}
		}
		else{
			if(reference_bit > 0){
				//set refrence bit = 0
				temp_pcbnode->PageTable[FrameTable[temp_victim].Page_No] = temp_pcbnode->PageTable[FrameTable[temp_victim].Page_No] ^ PTBL_REFERENCED_BIT;
			}
			else{
				return temp_victim;
			}
		}
		//Dolock(current_victim_Lock);
		current_victim++;
		//DoUnlock(current_victim_Lock);

		//Dolock(current_victim_Lock);
		if (current_victim > 63){
			current_victim = 0;
		}
		//DoUnlock(current_victim_Lock);

		if(count_check > 63){
			count_check = 0;
			count_modified_bit = 0;
		}
	}
	return -1;
}


void Find_Current_Free_Swap_Disk(){
	int i;

	for (i=0; i < MAX_NUMBER_OF_DISKS; i++){
		Dolock(Disk_is_Formatted_Lock);
		if(Disk_is_Formatted[i] == 1){
			//Dolock(diskmanager_Lock);
			if (diskmanager.disk_in_use[i] == 0){
				diskmanager.current_available_Disk = i;
				//DoUnlock(diskmanager_Lock);
				DoUnlock(Disk_is_Formatted_Lock);
				return;
			}
			//DoUnlock(diskmanager_Lock);
		}
		DoUnlock(Disk_is_Formatted_Lock);
	}

	for (i=0; i < MAX_NUMBER_OF_DISKS; i++){
		//Dolock(diskmanager_Lock);
		if (diskmanager.disk_in_use[i] == 0){
			diskmanager.current_available_Disk = i;
			//DoUnlock(diskmanager_Lock);
			return;
		}
		//DoUnlock(diskmanager_Lock);
	}

}


/************************************************************************
 * Find the process id for current executing pcbnode;
 * in: current executing context ID;
 * out: current executing PCB node;
 ************************************************************************/

struct Data * FindExecutingPCBNode(long contextID){
	int i;
	for (i=0; i < MAX_NUM_PROCESS; i++){
		if (pcbnode[i]!=NULL){
			if(pcbnode[i]->Process_Pointer ==(long*) contextID)
				return pcbnode[i];
		}
	}
	return NULL;
}

/************************************************************************
 * Check if there have created processes with same process name;
 * in: process name;
 * out: process id if there have same name;otherwise: 0;
 ************************************************************************/
long CheckSamePCBName(char *Process_Name) {
    int i;
    for (i=0; i<MAX_NUM_PROCESS; i++) {
    	if(pcbnode[i]!= NULL){
    		if(strcmp(pcbnode[i]->Process_Name, Process_Name) == 0)
    			return pcbnode[i]->Process_ID;   //there are same name in pcbQueue
    	}
    }
    return 0;		  //there donot have same name in pcbQueue
}

/************************************************************************
 * Put the (call SLEEP) pcbnode into timerQueue
 * in: sleeptime
 * out: start the timer if the insert PCB node is the first one in timer queue
 ************************************************************************/
void PutNodeinTimerQueue(long sleeptime){
	MEMORY_MAPPED_IO mmio;        // Structure used for hardware interface

	//check the Zclock to see current time
	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Clock, &mmio);
	long currentTime = mmio.Field1;

	//first need to find out the current executing pcbnode and put T into this pcbnode
	mmio.Mode = Z502GetCurrentContext;
	mmio.Field1 = 0;
	mmio.Field2 = 0;
	mmio.Field3 = 0;
	MEM_READ(Z502Context, &mmio);
	long contextID = mmio.Field1;
	//int Start_Status = 0;
	//Dolock(pcbnode_Lock);
	struct Data * CurrentExecutingPCBnode = FindExecutingPCBNode(contextID);
	//DoUnlock(pcbnode_Lock);
	struct Data * FirstPCBnode;
	//put T = current time + sleeptime (delay time)
	CurrentExecutingPCBnode->Argument = currentTime + sleeptime;

	Dolock(TimerQueueLock);
	timerQueue_insertNode(timerQueue, CurrentExecutingPCBnode);
	DoUnlock(TimerQueueLock);

	Dolock(TimerQueueLock);
	FirstPCBnode = timerQueue_Get_Fisrt_PCBnode(timerQueue);
	DoUnlock(TimerQueueLock);

	//use scheduler printer
	//char * targetname = "sleep";
	//scheduler_printer(CurrentExecutingPCBnode->Process_ID, targetname);

	//if (FirstPCBnode->Process_ID == CurrentExecutingPCBnode->Process_ID || CurrentExecutingPCBnode->Argument <= FirstPCBnode->Argument){
	if (FirstPCBnode->Process_ID == CurrentExecutingPCBnode->Process_ID){
		mmio.Mode = Z502Start;
		mmio.Field1 = sleeptime;   // You pick the time units
		mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Timer, &mmio);
	}

}

/************************************************************************
 * Response to the system call "SLEEP"
 * in: SystemCallData
 ************************************************************************/
void StartTimer(SYSTEM_CALL_DATA *SystemCallData){
	long sleeptime = (long) SystemCallData->Argument[0];
	if (sleeptime > 0)
		PutNodeinTimerQueue(sleeptime);
	else
		return;
	Dispatcher();
}

/************************************************************************
 * Take PCB node from Ready queue and start it;
 * Realize like call idle function when ready queue is empty;
 ************************************************************************/
void Dispatcher(){
	struct Data * nextStartNode;

	//while(!readyQueue_checkEmpty(ReadyQueue)){
	//	CALL(wasteTime());
	//}

	//check mailbox and MessageQueue first
	long i, msg_index;
	for (i=0; i<Max_Message_Box; i++) {
		if (MailBox[i] == 1){
			msg_index = FindPIDinMessageQueue(MessageQueue, i);
			if(msg_index >(-1)){
				Make_Ready_to_Run(pcbnode[i]);
				MailBox[i] = 0;
			}
		}
	}

	while (1){
		Dolock(ReadyQueueLock);
		int readyQueueisNotEmpty = readyQueue_checkEmpty(ReadyQueue);
		DoUnlock(ReadyQueueLock);
		if(readyQueueisNotEmpty)
			break;
		else
			CALL(wasteTime());
	}

	Dolock(ReadyQueueLock);
	nextStartNode = readyQueue_deQueue(ReadyQueue);
	DoUnlock(ReadyQueueLock);

	//use scheduler printer
	//char * targetname = "Dispatch";
	//scheduler_printer(nextStartNode->Process_ID, targetname);

	//update current_execute_PCB
	//current_execute_PCB = nextStartNode;

	StartContext(nextStartNode);
}
/************************************************************************
 * To realize similar function like call idle;
 ************************************************************************/
void wasteTime(){

}
/************************************************************************
 * Put PCB node into ready queue;
 ************************************************************************/
void Make_Ready_to_Run(struct Data * datanode){
	Dolock(ReadyQueueLock);
	readyQueue_enQueue(ReadyQueue, datanode);
	DoUnlock(ReadyQueueLock);
}
/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the OS.
 Call Make_Ready_to_Run when need;
 ************************************************************************/
void InterruptHandler(void) {
	INT32 DeviceID;
	INT32 Status;
	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	Status = mmio.Field2;

	Clear_Interruppt_Device(DeviceID);

	//RemoveFromTimerQueue() take the PCB off the timer queue.
	//and change the sleep time T=0
	if (DeviceID == 4){
		Dolock(TimerQueueLock);
		struct Data * tempnode = timerQueue_deQueue(timerQueue);
		DoUnlock(TimerQueueLock);

		tempnode->Argument = 0;

		Make_Ready_to_Run(tempnode);

		Dolock(TimerQueueLock);
		int timerQueueisNotEmpty = timerQueue_checkEmpty(timerQueue);
		DoUnlock(TimerQueueLock);

		while (timerQueueisNotEmpty){
		//if (timerQueue_checkEmpty(timerQueue)){
			// get current time
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
			MEM_READ(Z502Clock, &mmio);

			Dolock(TimerQueueLock);
			long interrupt_time =  timerQueue_Get_Fisrt_PCBnode(timerQueue)->Argument;
		    DoUnlock(TimerQueueLock);

		    if((interrupt_time - mmio.Field1) > 0){
		    	// Start Timer
		    	mmio.Mode = Z502Start;
		    	mmio.Field1 =interrupt_time - mmio.Field1;
		    	mmio.Field2 = mmio.Field3 = 0;
		    	MEM_WRITE(Z502Timer, &mmio);
		    	break;
		    }
		    else{
		    	Dolock(TimerQueueLock);
		    	tempnode = timerQueue_deQueue(timerQueue);
		    	DoUnlock(TimerQueueLock);

		    	tempnode->Argument = 0;

		    	Make_Ready_to_Run(tempnode);

		    	Dolock(TimerQueueLock);
		    	timerQueueisNotEmpty = timerQueue_checkEmpty(timerQueue);
		    	DoUnlock(TimerQueueLock);
		    }
		}
	}

	//RemoveFromdiskQueue() take the PCB off the disk queue.
	else if (DeviceID >= 5 && DeviceID <= 12){
		Dolock(DiskQueueLock);
		struct Data * tempnode = diskQueue_deQueue(diskQueue);
		DoUnlock(DiskQueueLock);
		Dolock(Format_is_complete_Lock);
		if(Format_is_complete[tempnode->Argument] ==1){
			//tempnode->Argument = 0;
			Make_Ready_to_Run(tempnode);
		}
		DoUnlock(Format_is_complete_Lock);
	}
	else{
		printf("Invalid DeviceID!!\n");
		return;
	}
}           // End of InterruptHandler
/************************************************************************
 Only used for INTERRUPT_HANDLER to clear interrupt device;
 ************************************************************************/
void Clear_Interruppt_Device(INT32 DeviceID){
	MEMORY_MAPPED_IO mmio;
	// Clear out this device - we're done with it
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	mmio.Field2 = mmio.Field3 = 0;
	MEM_WRITE(Z502InterruptDevice, &mmio);
}
/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.
 ************************************************************************/

void FaultHandler(void) {
	INT32 DeviceID;
	INT32 Status;

	int frame_index;
	int victim;
	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware
	char swap_in_buff[PGSIZE];
	char swap_out_buff[PGSIZE];
	SYSTEM_CALL_DATA * swap_in_data = (SYSTEM_CALL_DATA *)calloc(1, sizeof (SYSTEM_CALL_DATA));

	// Get cause of interrupt
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	mmio.Mode = Z502GetInterruptInfo;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	Status = mmio.Field2;			//page no = Status

	if (DeviceID == INVALID_MEMORY){
		short * PAGE_TBL_ADDR;
		short Page_No = Status;

		//check the page no, if bigger than the max logical address,terminate the whole process
		if (Status >= NUMBER_VIRTUAL_PAGES){
			Terminate_Whole_Simulation();
		}

		//get current executing pcb's page table
		mmio.Mode = Z502GetPageTable;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
		MEM_READ(Z502Context, &mmio);
		PAGE_TBL_ADDR = (short *) mmio.Field1;   // Gives us the page table

		int CURR_RES_Bit = PAGE_TBL_ADDR[Page_No] & PTBL_RES_BIT;	//reserved bit

		//get current pcbnode
		long current_contextID = Get_Current_ContextID();
		//Dolock(pcbnode_Lock);
		struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
		//DoUnlock(pcbnode_Lock);
		//get current PageTable
		short * CURR_PAGE_TBL = current_pcb->PageTable;
		//get current executing pcb's Shadow PageTable
		struct Shadow_PageTable * CURR_SPT_TBL = current_pcb->Shadow_PageTable;

		//int CRES_Bit = CURR_PAGE_TBL[Page_No] & PTBL_RES_BIT;	//reserved bit
		//int Modified_Bit = CURR_PAGE_TBL[Page_No] &  PTBL_MODIFIED_BIT;
		//int REF_Bit = CURR_PAGE_TBL[Page_No] & PTBL_REFERENCED_BIT;
		//int PG_VALID_Bit = CURR_PAGE_TBL[Page_No] &  PTBL_VALID_BIT;

		Dolock(frame_manager_Lock);
		int frame_isFull = frame_manager.isFull;
		//DoUnlock(frame_manager_Lock);
		if (!frame_isFull){
			//Dolock(frame_manager_Lock);
			frame_index = frame_manager.current_available_frame;
			//DoUnlock(frame_manager_Lock);
			//update the FrameTable
			Dolock(FrameTable_Lock);
			FrameTable[frame_index].is_Occupied = 1;
			FrameTable[frame_index].Process_ID = current_pcb->Process_ID;
			FrameTable[frame_index].Page_No = Page_No;
			DoUnlock(FrameTable_Lock);
			//update the PageTable
			PAGE_TBL_ADDR[Page_No] = (UINT16)PTBL_VALID_BIT | (frame_index & PTBL_PHYS_PG_NO);

			//update frame counter
			//Dolock(frame_manager_Lock);
			frame_manager.counter_occupied_frame +=1;
			//DoUnlock(frame_manager_Lock);

			//Dolock(frame_manager_Lock);
			if(frame_manager.counter_occupied_frame < 64){
				frame_manager.current_available_frame = FindFreeFrame();
			}
			else {//frame_manager.counter_occupied_frame == 64
				frame_manager.isFull = 1;
				frame_manager.current_available_frame = -1;
			}
			DoUnlock(frame_manager_Lock);
		}
		else{	//frame is full
			DoUnlock(frame_manager_Lock);

			Dolock(FrameTable_Lock);//***
			//<<<1. find victim frame
			Dolock(current_victim_Lock);
			victim = Find_Victim();
			DoUnlock(current_victim_Lock);
			//victim = 0;
			//read pid and pageNo from FrameTable
			long Victim_Pid = FrameTable[victim].Process_ID;
			long Victim_Page_No = FrameTable[victim].Page_No;

			Dolock(pcbnode_Lock);
			//Get victim's PageTable
			short * Victim_PAGE_TBL = pcbnode[Victim_Pid]->PageTable;
			//Get victim's ShadowPageTable
			struct Shadow_PageTable * Victim_SPT_TBL = pcbnode[Victim_Pid]->Shadow_PageTable;
			DoUnlock(pcbnode_Lock);
			//Get Bits
			//int Victim_RES_Bit = Victim_PAGE_TBL[Victim_Page_No] & PTBL_RES_BIT;	//reserved bit
			int Victim_Modified_Bit = Victim_PAGE_TBL[Victim_Page_No] &  PTBL_MODIFIED_BIT ;
			//int Victim_REF_Bit = Victim_PAGE_TBL[Victim_Page_No] & PTBL_REFERENCED_BIT ;
			//int Victim_PG_VALID_Bit = Victim_PAGE_TBL[Victim_Page_No] &  PTBL_VALID_BIT ;

			//<<<2.Find DiskID for swapOut
			Dolock(diskmanager_Lock);
			if(!diskmanager.is_All_used){
				Find_Current_Free_Swap_Disk();
			}
			else{
				printf("Swap disk is used up!!\n");
				Terminate_Whole_Simulation();
			}
			DoUnlock(diskmanager_Lock);

			//<<<3.format disk
			SYSTEM_CALL_DATA * disk_format =  (SYSTEM_CALL_DATA *)calloc(1, sizeof (SYSTEM_CALL_DATA));
			long ErrorReturned;
			Dolock(diskmanager_Lock);
			long SwapOut_DiskID = diskmanager.current_available_Disk;
			DoUnlock(diskmanager_Lock);

			Dolock(Disk_is_Formatted_Lock);
			if(!Disk_is_Formatted[SwapOut_DiskID]){
				disk_format->Argument[0]= (long *)SwapOut_DiskID;
				disk_format->Argument[1] = (long *)&ErrorReturned;
				DiskFormat(disk_format);
				free(disk_format);
			}
			DoUnlock(Disk_is_Formatted_Lock);
			//<<<4.SwapOut data	-> swap_out_buff
			Z502ReadPhysicalMemory(victim, (char *) swap_out_buff);

			//<<<4.SwapIn data if available -> swap_in_buff
			//if((Victim_SPT_TBL[Victim_Page_No].inDisk) ==1){//swapIn data was in swap area
			if(CURR_RES_Bit > 0){
				//SwapIn data
				long SwapIn_DiskID = CURR_SPT_TBL[Page_No].Diskid;
				long SwapIn_Sector = CURR_SPT_TBL[Page_No].Sector;
				//read the data in swap_in_buff from disk
				swap_in_data->Argument[0] = (long*)SwapIn_DiskID;
				swap_in_data->Argument[1] = (long*)(SwapLocation + SwapIn_Sector);
				swap_in_data->Argument[2] =(long*)swap_in_buff;
				DiskRead(swap_in_data);
			}

			//<<<5.SwapOut:Write to disk
			SYSTEM_CALL_DATA * swap_out_data = (SYSTEM_CALL_DATA *)calloc(1, sizeof (SYSTEM_CALL_DATA));
			long SwapOut_Sector;

			//if(Victim_RES_Bit > 0){//had ever been swapped out before
			if((Victim_SPT_TBL[Victim_Page_No].inDisk) ==1){
				SwapOut_DiskID = Victim_SPT_TBL[Victim_Page_No].Diskid;
				SwapOut_Sector = Victim_SPT_TBL[Victim_Page_No].Sector;
				if (Victim_Modified_Bit > 0){//the data is modified after last time swap in
					//update the data in its swap disk
					swap_out_data->Argument[0] = (long*)(SwapOut_DiskID);
					swap_out_data->Argument[1] = (long*)(SwapLocation + SwapOut_Sector);
					swap_out_data->Argument[2] =(long*)swap_out_buff;
					DiskWrite(swap_out_data);
				}
			}
			else{
				Dolock(SwapBitmap_Lock);
				SwapOut_Sector = CheckSwapBitmap(SwapOut_DiskID);
				DoUnlock(SwapBitmap_Lock);
				swap_out_data->Argument[0] = (long*)(SwapOut_DiskID);
				swap_out_data->Argument[1] = (long*)(SwapLocation + SwapOut_Sector);
				swap_out_data->Argument[2] =(long*)swap_out_buff;
				DiskWrite(swap_out_data);

				//update SwapBitmap
				Dolock(SwapBitmap_Lock);
				SwapBitmap[SwapOut_DiskID][SwapOut_Sector] = 1;
				DoUnlock(SwapBitmap_Lock);
			}


			//<<<5.SwapIn: Write to frame
			if(CURR_RES_Bit > 0){
				Z502WritePhysicalMemory(victim, (char *) swap_in_buff);
			}

			//<<<6. update FrameTable
			FrameTable[victim].is_Occupied = 1;
			FrameTable[victim].Process_ID = current_pcb->Process_ID;
			FrameTable[victim].Page_No = Page_No;

			DoUnlock(FrameTable_Lock);//***
			//<<<6.update shadowPageTable
			Victim_SPT_TBL[Victim_Page_No].inDisk = 1;
			Victim_SPT_TBL[Victim_Page_No].Diskid = SwapOut_DiskID;
			Victim_SPT_TBL[Victim_Page_No].Sector = SwapOut_Sector;

			//<<<7.Update PageTable
			Victim_PAGE_TBL[Victim_Page_No] = Victim_PAGE_TBL[Victim_Page_No] | PTBL_RES_BIT; //set reserved bit = 1 means data is swapped out to disk,imply to use shadow_pagetalbe later
			Victim_PAGE_TBL[Victim_Page_No] = Victim_PAGE_TBL[Victim_Page_No] ^ PTBL_VALID_BIT;//set page_valid_bit = 0 means data not stored in frame any more

			PAGE_TBL_ADDR[Page_No] = (UINT16)PTBL_VALID_BIT | (victim & PTBL_PHYS_PG_NO);
		}
	}

	printf("Fault_handler: Found vector type %d with value %d\n", DeviceID, Status);

	// Clear out this device - we're done with it//Zero all modified bit = 0
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	MEM_WRITE(Z502InterruptDevice, &mmio);
} // End of FaultHandler


/************************************************************************
 Terminate whole simulation
 ************************************************************************/
void Terminate_Whole_Simulation(){
	MEMORY_MAPPED_IO mmio;
	mmio.Mode = Z502Action;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_WRITE(Z502Halt, &mmio);
}
/************************************************************************
 Terminate process with given processID
 in: processID
 out: ERR_SUCCESS if success; -1 if wrong processID received;
 ************************************************************************/
long* Terminate_with_ProcessID(long ProcessID){
	struct Data* needTerminateNode;
	int index_ReadyQueue, index_timerQueue, index_diskQueue;
	//Dolock(pcbnode_Lock);
	needTerminateNode = pcbnode[ProcessID];
	//DoUnlock(pcbnode_Lock);
	//to check if the pcbnode in ReadyQueue or not, if it is in, remove it from ReadyQueue
	Dolock(ReadyQueueLock);
	index_ReadyQueue = readyQueue_findPCBNodeinQueue(ReadyQueue, ProcessID);
	DoUnlock(ReadyQueueLock);
	if (index_ReadyQueue > -1){
		Dolock(ReadyQueueLock);
		readyQueue_DeletePCBNodeinQueue(ReadyQueue, needTerminateNode, index_ReadyQueue);
		DoUnlock(ReadyQueueLock);
	}

	//to check if the pcbnode in timerQueue or notessid is 1:, if it is in, remove it from timerQueue
	Dolock(TimerQueueLock);
	index_timerQueue = timerQueue_findPCBNodeinQueue(timerQueue, ProcessID);
	DoUnlock(TimerQueueLock);

	if (index_timerQueue > -1){
		Dolock(TimerQueueLock);
		timerQueue_DeletePCBNodeinQueue(timerQueue, needTerminateNode, index_timerQueue);
		DoUnlock(TimerQueueLock);
	}
	//to check if the pcbnode in diskQueue or not, if it is in, remove it from diskQueue
	Dolock(DiskQueueLock);
	index_diskQueue = diskQueue_findPCBNodeinQueue(diskQueue, ProcessID);
	DoUnlock(DiskQueueLock);
	if (index_diskQueue > -1){
		Dolock(DiskQueueLock);
		diskQueue_DeletePCBNodeinQueue(diskQueue, needTerminateNode, index_diskQueue);
		DoUnlock(DiskQueueLock);
	}
	//update FrameTable
	int i;
	Dolock(FrameTable_Lock);
	for (i=0; i < NUMBER_PHYSICAL_PAGES; i++){
		if ((FrameTable[i].Process_ID) == ProcessID){
			FrameTable[i].is_Occupied = 0;
			Dolock(frame_manager_Lock);
			frame_manager.isFull = 0;
			frame_manager.counter_occupied_frame -=1;
			frame_manager.current_available_frame = i;
			DoUnlock(frame_manager_Lock);
		}
	}
	DoUnlock(FrameTable_Lock);
	//when all Queue removed the pcbnode, it can be released
	if (ProcessID == needTerminateNode->Process_ID){
		//Dolock(pcbnode_Lock);
		pcbnode[ProcessID] = NULL;
		//DoUnlock(pcbnode_Lock);

		release_DataNode(needTerminateNode);

		Dolock(count_Process_Lock);
		count_Process = count_Process - 1;
		DoUnlock(count_Process_Lock);
		return ERR_SUCCESS;
	}
	else
		return (long*)-1;
}
/************************************************************************
 Terminate process response SVC system call
 in: SystemCallData
 call Dispatcher when needed;
 ************************************************************************/
void TerminateProsess(SYSTEM_CALL_DATA *SystemCallData){

	long tempProcessID = (long)SystemCallData->Argument[0];

	if (tempProcessID == (long)-2){				//-2: terminate the whole simulation
		Terminate_Whole_Simulation();
		}
	else if (tempProcessID == (long) -1){		//-1: check readyQueue first
		Dolock(TimerQueueLock);
		int timerQueueisNotEmpty = timerQueue_checkEmpty(timerQueue);
		DoUnlock(TimerQueueLock);

		Dolock(ReadyQueueLock);
		int readyQueueisNotEmpty = readyQueue_checkEmpty(ReadyQueue);
		DoUnlock(ReadyQueueLock);

		Dolock(DiskQueueLock);
		int diskQueueisNotEmpty = diskQueue_checkEmpty(diskQueue);
		DoUnlock(DiskQueueLock);

		if ( timerQueueisNotEmpty || readyQueueisNotEmpty || diskQueueisNotEmpty){
		//if (timerQueue_checkEmpty(timerQueue) || readyQueue_checkEmpty(ReadyQueue)){
			//terminate itself first
			long contextID = Get_Current_ContextID();
			//Dolock(pcbnode_Lock);
			struct Data * tempnode = FindExecutingPCBNode(contextID);
			Dolock(pcbnode_Lock);
			SystemCallData->Argument[1] = (long*)Terminate_with_ProcessID(tempnode->Process_ID);
			DoUnlock(pcbnode_Lock);
			//Then load other existing pcbnode
			Dispatcher();
		}
		else {
			Terminate_Whole_Simulation();
		}
	}
	else if (tempProcessID >= (long)0){
		SystemCallData->Argument[1] = (long*)Terminate_with_ProcessID(tempProcessID);
	}
	else{		//the other processid is not possible in my system
		SystemCallData->Argument[1] = (long*)-1;
		printf("The process you want to terminate is not exist!!\n");
	}
}


/************************************************************************
 Creat_processes-this part used for the process to be
 created inside a test;
 in: SystemCallData
 Put new create process pcb node into ready queue;
 ************************************************************************/
void CreatProcess(SYSTEM_CALL_DATA *SystemCallData){
	MEMORY_MAPPED_IO mmio;

	long InitialPriority = (long) SystemCallData->Argument[2];
	long Process_ID;
	char *Process_Name = (char *) SystemCallData->Argument[0];
	char *Process_Status = "waiting";
	long Argument = 0; 	//initial sleep time T=0
	int i;
	//check illegal priority.

	if (InitialPriority == -3){
		*SystemCallData->Argument[4] = ERR_SUCCESS;
		return;
	}

	if(CheckSamePCBName(Process_Name)){
		*SystemCallData->Argument[4] = ERR_SUCCESS;
		return;
	}
	int tempcount;
	//Dolock(count_Process_Lock);
	count_Process += 1;
	tempcount = count_Process;
	//DoUnlock(count_Process_Lock);

	if (tempcount <=  MAX_NUM_PROCESS){
		printf("Currenr count_Process = %d\n", tempcount);
		//Initialize a context
		void *PageTable = (void *)calloc(2, NUMBER_VIRTUAL_PAGES);
		struct Shadow_PageTable *Shadow_PageTable = (struct Shadow_PageTable *)malloc(NUMBER_VIRTUAL_PAGES * sizeof(struct Shadow_PageTable));
		mmio.Mode = Z502InitializeContext;
		mmio.Field1 = 0;
		mmio.Field2 = (long) SystemCallData->Argument[1];
		mmio.Field3 = (long) PageTable;

		MEM_WRITE(Z502Context, &mmio);    // Start this new Context Sequence

		//create new PCB Node
		long *Process_Pointer = (long*) mmio.Field1;

		for (i=0; i < MAX_NUM_PROCESS; i++){
			if(!pcbnode[i]){
				Process_ID = i;
				break;
			}
		}

		if (tempcount == (MAX_NUM_PROCESS-1))
			*SystemCallData->Argument[4] = -1;

		struct Data temp_pcb = {InitialPriority, Process_ID, Process_Name,Process_Status,Process_Pointer, Argument, PageTable, Shadow_PageTable};

		for (i=0; i < MAX_NUM_PROCESS; i++){

			if(!pcbnode[i]){
				*(long*)SystemCallData->Argument[3] = Process_ID;
				pcbnode[i] = createDataNode(&temp_pcb);
				//put the new pcb node to ReadyQueue
				Dolock(ReadyQueueLock);
				readyQueue_enQueue(ReadyQueue, pcbnode[i]);
				DoUnlock(ReadyQueueLock);

				//use scheduler printer
				//char * targetname = "create";
				//scheduler_printer(pcbnode[i]->Process_ID, targetname);
				return;
			}
		}
	}
	//else if (count_Process > MAX_NUM_PROCESS){
	else{
		*SystemCallData->Argument[4] = -1;
		printf("There is the maximum %d processes can be created!\n", MAX_NUM_PROCESS);
		return;
		}

}

/************************************************************************
 * Get_Process_ID
 * in: SystemCallData
 ************************************************************************/
void GetProcessID(SYSTEM_CALL_DATA * SystemCallData){
	//MEMORY_MAPPED_IO mmio;
	long contextID;
	struct Data * temp;

	char *Process_Name = (char *) SystemCallData->Argument[0];

	if (strcmp((char*)SystemCallData->Argument[0], "") == 0){		//get current executing process id

		contextID = Get_Current_ContextID();
		//Dolock(pcbnode_Lock);
	 	temp = FindExecutingPCBNode(contextID);
	 	//DoUnlock(pcbnode_Lock);
	 	//return the process id
		*SystemCallData->Argument[1]= temp->Process_ID;
		*SystemCallData->Argument[2]= ERR_SUCCESS;
		return;
	}
	else if (CheckSamePCBName(Process_Name)){
		*SystemCallData->Argument[1]= CheckSamePCBName(Process_Name);
		*SystemCallData->Argument[2] = ERR_SUCCESS;
		return;
	}
	else{
		*SystemCallData->Argument[2] = -1;
		printf("There is no process with this name!!\n");
		return;
	}
}

/************************************************************************
 *File operation System
 ************************************************************************/
/************************************************************************
 *Check Bitmap for available sector
 *in:DiskID
 *out: available sector
 ************************************************************************/
long DiskCheckBitmap(long DiskID){
	SYSTEM_CALL_DATA * bitmap =  (SYSTEM_CALL_DATA *)calloc(1, sizeof (SYSTEM_CALL_DATA));
	char char_data[PGSIZE];
	long emptySector;
	int i,j,k;
	bitmap->Argument[0] = (long*)DiskID;
	for (i = 0; i < BitmapSize*4; i++ ){
		memset(char_data, 0, PGSIZE);
		bitmap->Argument[1] =(long*) (BitmapLocation + (long)i);
		bitmap->Argument[2] =(long*)(char*) char_data;
		//bitmap->Argument[2] =(long*) char_data;
		DiskRead(bitmap);

		for (j=0;j<PGSIZE;j++){
			for (k=7; k>=0;k--){
				if((char_data[j] & (00000001<<k)) == 0){
					emptySector = i*PGSIZE*8 + j*8 + (8-k);
					//emptySector = i*PGSIZE*8 + j*8 + (7-k)%8;
					free(bitmap);
					return emptySector;
				}
			}
		}
	}
	return 0;		//there is no available sector
}
/************************************************************************
 *Update Bitmap after write disk operation
 *in:DiskID, updateSector
 ************************************************************************/
void DiskUpdateBitmap(long DiskID, long updateSector){
	SYSTEM_CALL_DATA * bitmapupdate = (SYSTEM_CALL_DATA *)calloc(1, sizeof (SYSTEM_CALL_DATA));
	char char_data[PGSIZE];
	memset(char_data, 0, PGSIZE);

	int i = (int) ((updateSector - 1) / (8 * 16));
	int a = (int) (((updateSector - 1) % (8 * 16)) / 8);
	int b = (int) (((updateSector - 1) % (8 * 16)) % 8);
	b = b + 1;

	bitmapupdate->Argument[0] = (long*)DiskID;
	bitmapupdate->Argument[1] =(long*) (BitmapLocation + (long)i);
	bitmapupdate->Argument[2] =(long*)(char*) char_data;
	DiskRead(bitmapupdate);

	char_data[a] = char_data[a] | (00000001 << (8-b));

	DiskWrite(bitmapupdate);
	free(bitmapupdate);
}

/************************************************************************
 *Update Bitmap after write disk operation
 *in:DiskID, updateSector
 ************************************************************************/
void DiskRemoveUpdateBitmap(long DiskID, long updateSector){
	SYSTEM_CALL_DATA * bitmapupdate = (SYSTEM_CALL_DATA *)calloc(1, sizeof (SYSTEM_CALL_DATA));
	char char_data[PGSIZE];
	memset(char_data, 0, PGSIZE);
	int a = (int) updateSector / 8;
	int b = (int) updateSector % 8;
	int i;

	for (i = 0;i < 4 * BitmapSize;i++){
		if (a >= i * 16 && a < (i+1)*16){
			bitmapupdate->Argument[1] =(long*) (BitmapLocation + (long)i);
			a = a - 16 * i;
		}
	}
	bitmapupdate->Argument[0] = (long*)DiskID;
	bitmapupdate->Argument[2] =(long*)(char*) char_data;
	DiskRead(bitmapupdate);

	char_data[a] = char_data[a] ^ (00000001 << (8-b));

	DiskWrite(bitmapupdate);
	free(bitmapupdate);
}
/************************************************************************
 *Response for system call CREATE_FILE;
 *in:SystemCallData
 ************************************************************************/
void DiskCreateFile(SYSTEM_CALL_DATA *SystemCallData){
	MEMORY_MAPPED_IO mmio;
	SYSTEM_CALL_DATA * temp_SystemCallData = (SYSTEM_CALL_DATA *)calloc(1, sizeof (SYSTEM_CALL_DATA));
	char* file_name = (char *) SystemCallData->Argument[0];
	long current_contextID = Get_Current_ContextID();
	//Dolock(pcbnode_Lock);
	struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
	//DoUnlock(pcbnode_Lock);
	//char* current_dir_name = current_pcb->current_Dir.DirName;
	long current_dir_diskID = current_pcb->current_Dir.DiskID;
	long current_dir_Sector = current_pcb->current_Dir.Sector;
	if (strlen(file_name)>7){
		*SystemCallData->Argument[1]= ERR_BAD_PARAM;
		printf("Invalid name!");
		free(temp_SystemCallData);
		return;
	}

	//check other file name inside current_directory
	char char_data[PGSIZE];
	memset(char_data, 0, PGSIZE);
	temp_SystemCallData->Argument[0] = (long*)current_dir_diskID;
	temp_SystemCallData->Argument[1] = (long*)current_dir_Sector;
	temp_SystemCallData->Argument[2] = (long*)char_data;
	DiskRead(temp_SystemCallData);

	long index_sector=0;
	memcpy(&index_sector, char_data+12, 2);
	//read the index sector
	memset(char_data, 0, PGSIZE);
	temp_SystemCallData->Argument[1] = (long*)index_sector;
	temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
	DiskRead(temp_SystemCallData);
	int i;
	long file_Sector;
	char file_char_data[PGSIZE];
	int file_description=0;
	int notfile;
	char Subfile_name[8];		//7
	for (i=0;i<16;i=i+2){
		file_Sector = char_data[i]%256 + char_data[i+1]*256;
		if( file_Sector != 0){		//read file header
			memset(file_char_data, 0, PGSIZE);
			temp_SystemCallData->Argument[1] = (long*)file_Sector;
			temp_SystemCallData->Argument[2] = (long*)(char*)file_char_data;
			DiskRead(temp_SystemCallData);
			memcpy(&file_description, file_char_data+8, 1);
			notfile = file_description & 0b00000001;
			if (!notfile){
				memcpy(Subfile_name, file_char_data+1, 7);
				if (strcmp(Subfile_name, file_name) == 0){
					*SystemCallData->Argument[1]= ERR_BAD_PARAM;
					free(temp_SystemCallData);
					return;
				}
			}
		}
	}

	//check bitmap
	long emptySector1 = DiskCheckBitmap(current_dir_diskID);
	if (emptySector1){
		//update bitmap
		DiskUpdateBitmap(current_dir_diskID, emptySector1);
	}
	else{
		*SystemCallData->Argument[1] = ERR_BAD_PARAM;
		printf("There is no enough space!");
		free(temp_SystemCallData);
		return;
	}
	//check bitmap again since to create a dir/file need two sector (file head + index)
	long emptySector2 = DiskCheckBitmap(current_dir_diskID);
	if(emptySector2){
		//update bitmap
		DiskUpdateBitmap(current_dir_diskID, emptySector2);
	}
	else{//if there do not have the 2nd free sector, that means there donot have enough space in the disk for a new dir.
		DiskRemoveUpdateBitmap(current_dir_diskID, emptySector1);
		*SystemCallData->Argument[1] = ERR_BAD_PARAM;
		printf("There is no enough space!");
		free(temp_SystemCallData);
		return;
		}

	//set file/dir head
	if (emptySector1 && emptySector2){  //if there is still have available space
		//let index point to the new file/dir head's location
		memset(char_data, 0, PGSIZE);
		temp_SystemCallData->Argument[1] = (long*)index_sector;
		temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
		DiskRead(temp_SystemCallData);
		for (i=0;i<16;i=i+2){
			file_Sector = char_data[i]%256 + char_data[i+1]*256;
			if( file_Sector == 0){
				char_data[i] = emptySector1%256;
				char_data[i+1] = emptySector1/256;

				temp_SystemCallData->Argument[1] = (long*)index_sector;
				temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
				DiskWrite(temp_SystemCallData);
				break;
			}
		}

		memset(char_data, 0, PGSIZE);
		// get current time
		mmio.Mode = Z502ReturnValue;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
		MEM_READ(Z502Clock, &mmio);

		//memcpy(char_data+1,file_name,7);		//copy dir name
		memcpy(char_data+1,file_name,strlen(file_name));

		Dolock(Inode_Id_Lock);
		char_data[0] = Inode_Id; //Inode Id
		DoUnlock(Inode_Id_Lock);

		Dolock(Inode_Id_Lock);
		Inode_Id = Inode_Id + 1;	//update Inode_Id
		DoUnlock(Inode_Id_Lock);

		char_data[8] = 250;			//file description: 11111010 = 250 = FA
		memcpy(char_data+9, &mmio.Field1,3);	//creation time MSB
			//char_data[10] = 0;			//creation time
			//char_data[11] = 0;			//creation time  LSB
		long index_location = emptySector2;
		char_data[12] = index_location%256;			//index location
		char_data[13] = index_location/256;			//index location

		char_data[14] = 0;			//file size: initial = 0 byte
		char_data[15] = 0;
		//write the file/dir head into disk
		temp_SystemCallData->Argument[1] = (long*)emptySector1;
		temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
		DiskWrite(temp_SystemCallData);

		//set index sector
		memset(char_data, 0, PGSIZE);
		//write the initial index = 0 into emptySecotr2
		temp_SystemCallData->Argument[1] = (long*)emptySector2;
		temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
		DiskWrite(temp_SystemCallData);

		//return create success
		*SystemCallData->Argument[1] = ERR_SUCCESS;
		free(temp_SystemCallData);
	}
	else{
		*SystemCallData->Argument[1] = ERR_BAD_PARAM;
		free(temp_SystemCallData);
		printf("There is no enough space!");
	}
}
/************************************************************************
 *Response for system call CREATE_DIR;
 *in:SystemCallData
 ************************************************************************/

void DiskCreateDIR(SYSTEM_CALL_DATA *SystemCallData){
	MEMORY_MAPPED_IO mmio;
	SYSTEM_CALL_DATA * temp_SystemCallData = (SYSTEM_CALL_DATA *)calloc(1, sizeof (SYSTEM_CALL_DATA));
	char* Dir_name = (char *) SystemCallData->Argument[0];
	long current_contextID = Get_Current_ContextID();
	//Dolock(pcbnode_Lock);
	struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
	//DoUnlock(pcbnode_Lock);
	char* current_dir_name = current_pcb->current_Dir.DirName;
	long current_dir_diskID = current_pcb->current_Dir.DiskID;
	long current_dir_Sector = current_pcb->current_Dir.Sector;
	if (strlen(Dir_name)>7){
		*SystemCallData->Argument[1]= ERR_BAD_PARAM;
		printf("Invalid name!");
		free(temp_SystemCallData);
		return;
	}
	if (Dir_name == current_dir_name){
		*SystemCallData->Argument[1]= ERR_BAD_PARAM;
		free(temp_SystemCallData);
		return;
	}
	//check other directory name inside current_directory
	char char_data[PGSIZE];
	memset(char_data, 0, PGSIZE);
	temp_SystemCallData->Argument[0] = (long*)current_dir_diskID;
	temp_SystemCallData->Argument[1] = (long*)current_dir_Sector;
	temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
	DiskRead(temp_SystemCallData);

	long index_sector=0;
	memcpy(&index_sector, char_data+12, 2);
	//read the index sector
	memset(char_data, 0, PGSIZE);
	temp_SystemCallData->Argument[1] = (long*)index_sector;
	temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
	DiskRead(temp_SystemCallData);
	int i;
	long DIR_Sector;
	char DIR_char_data[PGSIZE];
	int file_description=0;
	int isdir;
	char SubDIR_name[8];		//7
	for (i=0;i<16;i=i+2){
		DIR_Sector = char_data[i]%256 + char_data[i+1]*256;
		if( DIR_Sector != 0){		//read file header
			memset(DIR_char_data, 0, PGSIZE);
			temp_SystemCallData->Argument[1] = (long*)DIR_Sector;
			temp_SystemCallData->Argument[2] = (long*)(char*)DIR_char_data;
			DiskRead(temp_SystemCallData);
			memcpy(&file_description, char_data+8, 1);
			isdir = file_description & 0b00000001;
			if (isdir){
				memcpy(SubDIR_name, char_data+1, 7);
				if (strcmp(SubDIR_name, Dir_name) == 0){
					*SystemCallData->Argument[1]= ERR_BAD_PARAM;
					free(temp_SystemCallData);
					return;
				}
			}
		}
	}
	//after checking all existing dir_name, and there is no same name dir existing
	//check bitmap to find available empty sector to create new dir.
	long emptySector1 = DiskCheckBitmap(current_dir_diskID);
	if (emptySector1){
		//update bitmap
		DiskUpdateBitmap(current_dir_diskID, emptySector1);
	}
	else{
		*SystemCallData->Argument[1] = ERR_BAD_PARAM;
		printf("There is no enough space!");
		free(temp_SystemCallData);
		return;
	}
	//check bitmap again since to create a dir/file need two sector (file head + index)
	long emptySector2 = DiskCheckBitmap(current_dir_diskID);
	if(emptySector2){
		//update bitmap
		DiskUpdateBitmap(current_dir_diskID, emptySector2);
	}
	else{//if there do not have the 2nd free sector, that means there donot have enough space in the disk for a new dir.
		DiskRemoveUpdateBitmap(current_dir_diskID, emptySector1);
		*SystemCallData->Argument[1] = ERR_BAD_PARAM;
		printf("There is no enough space!");
		free(temp_SystemCallData);
		return;
	}


	if (emptySector1 && emptySector2){  //if there is still have available space
		//let index point to the new file/dir head's location
		memset(char_data, 0, PGSIZE);
		temp_SystemCallData->Argument[1] = (long*)index_sector;
		temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
		DiskRead(temp_SystemCallData);
		for (i=0;i<16;i=i+2){
			DIR_Sector = char_data[i]%256 + char_data[i+1]*256;
			if( DIR_Sector == 0){
				char_data[i] = emptySector1%256;
				char_data[i+1] = emptySector1/256;

				temp_SystemCallData->Argument[1] = (long*)index_sector;
				temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
				DiskWrite(temp_SystemCallData);
				break;
			}
		}

		//set file/dir head
		memset(char_data, 0, PGSIZE);
		// get current time
		mmio.Mode = Z502ReturnValue;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
		MEM_READ(Z502Clock, &mmio);

		memcpy(char_data+1,Dir_name,7);		//copy dir name
		Dolock(Inode_Id_Lock);
		char_data[0] = Inode_Id; //Inode Id
		DoUnlock(Inode_Id_Lock);
		Dolock(Inode_Id_Lock);
		Inode_Id = Inode_Id + 1;	//update Inode_Id
		DoUnlock(Inode_Id_Lock);
		char_data[8] = 251;			//file description: 11111011 = 251 = FB
		memcpy(char_data+9, &mmio.Field1,3);	//creation time MSB

		long index_location = emptySector2;
		char_data[12] = index_location%256;			//index location
		char_data[13] = index_location/256;			//index location
		char_data[14] = 0;			//file size: initial = 0 byte
		char_data[15] = 0;
		//write the file/dir head into disk
		temp_SystemCallData->Argument[1] = (long*)emptySector1;
		temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
		DiskWrite(temp_SystemCallData);

		//set index sector
		memset(char_data, 0, PGSIZE);
		//write the initial index = 0 into emptySecotr2
		temp_SystemCallData->Argument[1] = (long*)emptySector2;
		temp_SystemCallData->Argument[2] = (long*)(char*)char_data;
		DiskWrite(temp_SystemCallData);

		//return create success
		*SystemCallData->Argument[1] = ERR_SUCCESS;
		free(temp_SystemCallData);
	}
	else{
		*SystemCallData->Argument[1] = ERR_BAD_PARAM;
		free(temp_SystemCallData);
		printf("There is no enough space!");
	}
}

/************************************************************************
 *Response for system call OPEN_DIR;
 *in:SystemCallData
 ************************************************************************/
void DiskOpenDIR(SYSTEM_CALL_DATA * SystemCallData){

	long DiskID = (long) SystemCallData->Argument[0];
	char * Dir_name = (char*) SystemCallData->Argument[1];
	long Sector;
	SYSTEM_CALL_DATA * OpenDir_SystemCallData = (SYSTEM_CALL_DATA *)calloc(1, sizeof (SYSTEM_CALL_DATA));
	char char_data[PGSIZE];

	if ((DiskID < 0 && DiskID!=-1) || DiskID >= MAX_NUMBER_OF_DISKS){			//Invalid diskid
		*SystemCallData->Argument[2] = ERR_BAD_PARAM;
		free(OpenDir_SystemCallData);
		return ;
	}

	long current_contextID = Get_Current_ContextID();
	//Dolock(pcbnode_Lock);
	struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
	//DoUnlock(pcbnode_Lock);

	//if diskid==-1 open in current disk currentdir->diskid
	if (DiskID == -1){

		if (current_pcb->current_Dir.DiskID >= 0){
			DiskID = current_pcb->current_Dir.DiskID;
		}
		else{
			*SystemCallData->Argument[2] = ERR_BAD_PARAM;
			free(OpenDir_SystemCallData);
			return ;
		}
	}

	if (strcmp(Dir_name,"..") == 0){
		if (strcmp(current_pcb->current_Dir.DirName,"root") == 0){
			*SystemCallData->Argument[2] = ERR_BAD_PARAM;
			free(OpenDir_SystemCallData);
			return;
		}
		current_pcb->current_Dir.DirName = current_pcb->parent_Dir.DirName;
		current_pcb->current_Dir.DiskID = current_pcb->parent_Dir.DiskID;
		current_pcb->current_Dir.Sector = current_pcb->parent_Dir.Sector;
		*SystemCallData->Argument[2] = ERR_SUCCESS;
		free(OpenDir_SystemCallData);
		return;
	}

	Sector = 0;			//read data from sector 0
	memset(char_data, 0, PGSIZE);		//initialize before reading
	OpenDir_SystemCallData->Argument[0] = (long*) DiskID;
	OpenDir_SystemCallData->Argument[1] = (long*) Sector;
	OpenDir_SystemCallData->Argument[2] = (long*)(char*) char_data;
	DiskRead(OpenDir_SystemCallData);

	//read file head in RootDir sector
	int RootDIR_Sector = char_data[8]%256 + char_data[9]*256;
	//initialize before reading
	memset(char_data, 0, PGSIZE);
	Sector = RootDIR_Sector;
	OpenDir_SystemCallData->Argument[1] = (long*) Sector;
	OpenDir_SystemCallData->Argument[2] = (long*)(char*) char_data;
	DiskRead(OpenDir_SystemCallData);

	char RootDIR_name[8];
	//read out the RootDir's name
	memcpy(RootDIR_name, char_data+1, 7);

	if (strcmp(RootDIR_name, Dir_name) == 0){
		//open rootdir
		current_pcb->current_Dir.DirName = Dir_name;
		current_pcb->current_Dir.DiskID = DiskID;
		current_pcb->current_Dir.Sector = Sector;
		*SystemCallData->Argument[2] = ERR_SUCCESS;
		free(OpenDir_SystemCallData);
		return;
	}

	//find the index to check the other directory
	int index_sector = 0;
	memcpy(&index_sector, char_data+12, 2);
	Sector = index_sector;
	//long index_Sector = char_data[12]%256 + char_data[13]*256;
	//read the index sector
	memset(char_data, 0, PGSIZE);
	OpenDir_SystemCallData->Argument[1] = (long*)Sector;
	OpenDir_SystemCallData->Argument[2] = (long*)(char*)char_data;
	DiskRead(OpenDir_SystemCallData);

	int i;
	long DIR_Sector;
	char DIR_char_data[PGSIZE];
	int file_description=0;
	int isdir;
	char SubDIR_name[8];		//7
	for (i=0;i<16;i=i+2){
		DIR_Sector = char_data[i]%256 + char_data[i+1]*256;
		if( DIR_Sector != 0){		//read file header
			memset(DIR_char_data, 0, PGSIZE);
			OpenDir_SystemCallData->Argument[1] = (long*)DIR_Sector;
			OpenDir_SystemCallData->Argument[2] = (long*)(char*)DIR_char_data;
			DiskRead(OpenDir_SystemCallData);
			memcpy(&file_description, DIR_char_data+8, 1);
			isdir = file_description & 00000001;
			if (isdir){
				memcpy(SubDIR_name, DIR_char_data+1, 7);
				if (strcmp(SubDIR_name, Dir_name) == 0){
					//put the current dir to parent dir
					current_pcb->parent_Dir.DirName = current_pcb->current_Dir.DirName;
					current_pcb->parent_Dir.DiskID = current_pcb->current_Dir.DiskID;
					current_pcb->parent_Dir.Sector = current_pcb->current_Dir.Sector;
					//update the current dir
					current_pcb->current_Dir.DirName = Dir_name;
					current_pcb->current_Dir.DiskID = DiskID;
					current_pcb->current_Dir.Sector = DIR_Sector;
					*SystemCallData->Argument[2] = ERR_SUCCESS;
					free(OpenDir_SystemCallData);
					return;
				}
			}
		}
	}
	DiskCreateDIR(SystemCallData);
	printf("Do not exit file directory!\n");
	free(OpenDir_SystemCallData);
}
/*
void DiskOpenDIR(SYSTEM_CALL_DATA *SystemCallData){
	long DiskID = (long)SystemCallData->Argument[0];
	char * Dir_name = (char*)SystemCallData->Argument[1];
	long Sector;

	if ((DiskID<0 && DiskID!=-1) || DiskID>= MAX_NUMBER_OF_DISKS){			//Invalid diskid
		*SystemCallData->Argument[1] = ERR_BAD_PARAM;
		return ;
	}
	//if diskid==-1 open in current disk currentdir->diskid
	if (DiskID == -1){
		long current_contextID = Get_Current_ContextID();
		struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
		if (current_pcb->current_Dir.DiskID >= 0){
			DiskID =current_pcb->current_Dir.DiskID;
		}
		else{
			*SystemCallData->Argument[1] = ERR_BAD_PARAM;
			return ;
		}
	}
	long current_contextID = Get_Current_ContextID();
	struct Data * current_pcb = FindExecutingPCBNode(current_contextID);

	char char_data[PGSIZE];
	memset(char_data, 0, PGSIZE);
	//check the dir name if == ".."; to set current dir = parent dir;
	if (strcmp(Dir_name,"..") == 0){
		//long current_contextID = Get_Current_ContextID();
		//struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
		if (strcmp(current_pcb->current_Dir.DirName,"root")){
			*SystemCallData->Argument[1] = ERR_BAD_PARAM;
			return;
		}
		current_pcb->current_Dir.DirName = current_pcb->parent_Dir.DirName;
		current_pcb->current_Dir.DiskID = current_pcb->parent_Dir.DiskID;
		current_pcb->current_Dir.Sector = current_pcb->parent_Dir.Sector;
		*SystemCallData->Argument[2] = ERR_SUCCESS;
		return;
	}

	Sector = 0;			//read data from sector 0
	SystemCallData->Argument[0] = (long*) DiskID;
	SystemCallData->Argument[1] = (long*)Sector;
	SystemCallData->Argument[2] = (long*)(char*)char_data;
	DiskRead(SystemCallData);

	int i;
	//for(i=0; i<16; i++)
	//	printf("%02X", char_data[i]);

	int RootDIR_Sector = char_data[8]%256 + char_data[9]*256;

	//int RootDIR_Sector=0;
	//memcpy(&RootDIR_Sector, char_data+8, 2);

	//printf("integer value of the string is %d\n", RootDIR_Sector);

	memset(char_data, 0, PGSIZE);
	Sector = RootDIR_Sector;
	SystemCallData->Argument[1] = (long*)Sector;
	SystemCallData->Argument[2] = (long*)(char*)char_data;
	DiskRead(SystemCallData);

	char RootDIR_name[8];
	memcpy(RootDIR_name, char_data+1, 7);

	//printf("dir name: %s\n", RootDIR_name);

	//char * CurrentDIR_name;

	if (strcmp(RootDIR_name, Dir_name) == 0){
		//strcpy(CurrentDIR_name, Dir_name);
		long current_contextID = Get_Current_ContextID();
		struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
		//struct Directory current_dir = current_pcb->current_Dir;

		current_pcb->current_Dir.DirName = Dir_name;
		current_pcb->current_Dir.DiskID = DiskID;
		current_pcb->current_Dir.Sector = Sector;
		*SystemCallData->Argument[2] = ERR_SUCCESS;
		return;
	}
	//find the index to check the other directory
	int index_sector=0;
	memcpy(&index_sector, char_data+12, 2);
	//read the index sector
	Sector = index_sector;
	memset(char_data, 0, PGSIZE);
	SystemCallData->Argument[1] = (long*)Sector;
	SystemCallData->Argument[2] = (long*)(char*)char_data;
	DiskRead(SystemCallData);
	//int i;
	long DIR_Sector;
	char DIR_char_data[PGSIZE];
	int file_description=0;
	int isdir;
	char SubDIR_name[8];		//7
	for (i=0;i<16;i=i+2){
		DIR_Sector = char_data[i]%256 + char_data[i+1]*256;
		if( DIR_Sector != 0){		//read file header
			memset(DIR_char_data, 0, PGSIZE);
			SystemCallData->Argument[1] = (long*)DIR_Sector;
			SystemCallData->Argument[2] = (long*)(char*)DIR_char_data;
			DiskRead(SystemCallData);
			memcpy(&file_description, DIR_char_data+8, 1);
			isdir = file_description & 00000001;
			if (isdir){
				memcpy(SubDIR_name, DIR_char_data+1, 7);
				if (strcmp(SubDIR_name, Dir_name) == 0){
						//strcpy(CurrentDIR_name, Dir_name);
						long current_contextID = Get_Current_ContextID();
						struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
						//put the current dir to parent dir
						current_pcb->parent_Dir.DirName = current_pcb->current_Dir.DirName;
						current_pcb->parent_Dir.DiskID = current_pcb->current_Dir.DiskID;
						current_pcb->parent_Dir.Sector = current_pcb->current_Dir.Sector;
						//update the current dir
						current_pcb->current_Dir.DirName = Dir_name;
						current_pcb->current_Dir.DiskID = DiskID;
						current_pcb->current_Dir.Sector = DIR_Sector;
						*SystemCallData->Argument[2] = ERR_SUCCESS;
						return;
					}
			}
		}
	}
	DiskCreateDIR(SystemCallData);
	printf("Do not exit file directory!\n");
	return;
}
*/

/************************************************************************
 * Disk Check : Response to system call CHECK_DISK
 * in: SystemCallData
 * generate a checkfile
 ************************************************************************/
void DiskCheck(SYSTEM_CALL_DATA *SystemCallData) {
    MEMORY_MAPPED_IO mmio;
    long DiskID = (long)SystemCallData->Argument[0];
    if (DiskID<0 || DiskID>= MAX_NUMBER_OF_DISKS){			//Invalid diskid
    	*SystemCallData->Argument[1] = ERR_BAD_PARAM;
    	return ;
    }
    mmio.Mode = Z502CheckDisk;
    mmio.Field1 = (long)SystemCallData->Argument[0];
    mmio.Field2 = mmio.Field3 = 0;
    MEM_READ(Z502Disk, &mmio);
    *SystemCallData->Argument[1] = ERR_SUCCESS;
}
/************************************************************************
 * Response for system call  FORMAT;
 * in : SystemCallData
 ************************************************************************/
void DiskFormat(SYSTEM_CALL_DATA *SystemCallData){	//block = sector = 16 bytes

	MEMORY_MAPPED_IO mmio;
	long DiskID = (long)SystemCallData->Argument[0];

	Dolock(Format_is_complete_Lock);
	Format_is_complete[DiskID] = 0;
	DoUnlock(Format_is_complete_Lock);

	long SectorID;
	//char char_data[PGSIZE];
	struct Data * InUseDisk;
	long contextID;
	char * char_data;
	if (DiskID<0 || DiskID>= MAX_NUMBER_OF_DISKS){			//Invalid diskid
		*SystemCallData->Argument[1] = ERR_BAD_PARAM;
		return ;
	}

	//check this disk is formatted or not
	//Dolock(Disk_is_Formatted_Lock);
	if(Disk_is_Formatted[DiskID]){
		*SystemCallData->Argument[1] = ERR_SUCCESS;
		printf("This Disk has been formatted!");
		return;
	}
	//DoUnlock(Disk_is_Formatted_Lock);

	for (SectorID = 0; SectorID <= RootDirLocation+1;SectorID++){
		//if disk is in use, wait here
			while (1){
				Dolock(DiskQueueLock);
				int DiskisInUse = diskQueue_checkDiskInUse(diskQueue, DiskID);
				DoUnlock(DiskQueueLock);
				if (!DiskisInUse)
					break;
				else
					CALL(wasteTime());
			}

			contextID = Get_Current_ContextID();
			//Dolock(pcbnode_Lock);
			InUseDisk = FindExecutingPCBNode(contextID);
			//DoUnlock(pcbnode_Lock);
			InUseDisk->Argument = DiskID;
			//put the pcbnode into diskQueue
			Dolock(DiskQueueLock);
			diskQueue_enQueue(diskQueue, InUseDisk);
			DoUnlock(DiskQueueLock);

			char_data = DiskSector(DiskID, SectorID);
			// Now see that the disk IS NOT running
			mmio.Mode = Z502Status;
			mmio.Field1 =DiskID;
			mmio.Field2 = mmio.Field3 = 0;
			MEM_READ(Z502Disk, &mmio);

			if ( SectorID == (RootDirLocation+1)){
				Dolock(Format_is_complete_Lock);
				Format_is_complete[DiskID] = 1;
				DoUnlock(Format_is_complete_Lock);
				//update related parameters
				Dolock(Inode_Id_Lock);
				Inode_Id = Inode_Id + 1;
				DoUnlock(Inode_Id_Lock);
				//record this disk has been formatted
				//Dolock(Disk_is_Formatted_Lock);
				Disk_is_Formatted[DiskID] = 1;
				//DoUnlock(Disk_is_Formatted_Lock);
				//update current dir's parameters, there is no open dir so that current dir is not available
				long current_contextID = Get_Current_ContextID();
				//Dolock(pcbnode_Lock);
				struct Data * current_pcb = FindExecutingPCBNode(current_contextID);
				//DoUnlock(pcbnode_Lock);
				current_pcb->current_Dir.DiskID = -1;
			}

			if(mmio.Field2 == DEVICE_FREE && char_data!= NULL){
				// Start the disk by writing a block of data
				mmio.Mode = Z502DiskWrite;
				mmio.Field1 = DiskID;
				mmio.Field2 = SectorID;
				mmio.Field3 = (long)char_data;

				// Do the hardware call to put data on disk
				MEM_WRITE(Z502Disk, &mmio);
			}
			if ( SectorID == (RootDirLocation+1)){
				Dispatcher();
			}
	}

	//&ERRORreturned
	*(SystemCallData->Argument[1]) = ERR_SUCCESS;
	free(char_data);

}

/************************************************************************
 * Called by DiskFormat();
 * contain all initial datas using during format;
 * in : DiskID, Sector;
 * out: initial data used during format;
 ************************************************************************/
char* DiskSector(long DiskID, long Sector){
	//static char char_data[PGSIZE ];
	char * char_data = malloc(16 * sizeof (char));
	MEMORY_MAPPED_IO mmio;
	switch (Sector){
		case 0:
			char_data[0] = DiskID;
			char_data[1] = BitmapSize; //Bitmap size = 4*4=16 blocks, which is 4*4*16*8 = 512*4=2048 bits. the disk size can be 2048 blocks.
			char_data[2] = RootDirSize; //RootDir size = 2 blocks.
			char_data[3] = SwapSize;	//swap size = SwapSize*4 blocks
			char_data[4] = 0;	//LSB Disk length= 2048  (Hexadecimal)
			char_data[5] = 8;	//MSB Disk length =2048,transfer to binary = 00001000 00000000 = 8 0
			char_data[6] = BitmapLocation;	//bitmap location = sector1
			char_data[7] = 0;
			char_data[8] = RootDirLocation;	//Rootdir location LSB=sector17
			char_data[9] = 0;	//MSB
			char_data[10] =SwapLocation;	//swap location = sector 19
			char_data[11] =0;
			char_data[12] = '\0';
			char_data[13] = '\0';
			char_data[14] = '\0';
			char_data[15] = '\0';
			break;
		case BitmapLocation :
		case BitmapLocation+1:
		case BitmapLocation+2:
		case BitmapLocation+3:
		case BitmapLocation+4:
		case BitmapLocation+5:
		case BitmapLocation+6:
		case BitmapLocation+7://bitmap
			char_data[0] = 255;	// 11111111=255 = FF
			char_data[1] = 255;
			char_data[2] = 255;
			char_data[3] = 255;
			char_data[4] = 255;
			char_data[5] = 255;
			char_data[6] = 255;
			char_data[7] = 255;
			char_data[8] = 255;   //11000000 =192 = C0
			char_data[9] = 255;
			char_data[10] = 255;
			char_data[11] = 255;
			char_data[12] = 255;
			char_data[13] = 255;
			char_data[14] = 255;
			char_data[15] = 255;
			break;
		case BitmapLocation + 8:
			char_data[0] = 255;	// 11111111=255 = FF
			char_data[1] = 252;	//11111100 =252 = FC
			char_data[2] = 0;
			char_data[3] = 0;
			char_data[4] = 0;
			char_data[5] = 0;
			char_data[6] = 0;
			char_data[7] = 0;
			char_data[8] = 0;
			char_data[9] = 0;
			char_data[10] = 0;
			char_data[11] = 0;
			char_data[12] = 0;
			char_data[13] = 0;
			char_data[14] = 0;
			char_data[15] = 0;
			break;
		case BitmapLocation + 9:
		case BitmapLocation + 10:
		case BitmapLocation + 11:
		case BitmapLocation + 12:
		case BitmapLocation + 13:
		case BitmapLocation + 14:
		case BitmapLocation + 15:
			char_data[0] = 0;
			char_data[1] = 0;
			char_data[2] = 0;
			char_data[3] = 0;
			char_data[4] = 0;
			char_data[5] = 0;
			char_data[6] = 0;
			char_data[7] = 0;
			char_data[8] = 0;
			char_data[9] = 0;
			char_data[10] =0;
			char_data[11] = 0;
			char_data[12] = 0;
			char_data[13] = 0;
			char_data[14] = 0;
			char_data[15] = 0;
			break;
		case RootDirLocation :					//rootdir sector
			// get current time
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
			MEM_READ(Z502Clock, &mmio);
			printf("creation time is : %ld\n", mmio.Field1);
			Dolock(Inode_Id_Lock);
			char_data[0] = Inode_Id; //Inode Id
			DoUnlock(Inode_Id_Lock);
			char_data[1] = 'r';	//name
			char_data[2] = 'o';
			char_data[3] = 'o';
			char_data[4] = 't';
			char_data[5] = '\0';
			char_data[6] = '\0';
			char_data[7] = '\0';
			char_data[8] = 251;			//file description: 11111011 = 251 = FB
			memcpy(char_data+9, &mmio.Field1,3);	//creation time MSB
			//char_data[10] = 0;			//creation time
			//char_data[11] = 0;			//creation time  LSB
			char_data[12] = RootDirLocation+ 1;			//index location: sector 18
			char_data[13] = 0;			//index location
			char_data[14] = 0;			//file size: initial = 0 byte
			char_data[15] = 0;
			break;
		case RootDirLocation + 1:					//indexblock: initialize
			char_data[0] = 0;
			char_data[1] = 0;
			char_data[2] = 0;
			char_data[3] = 0;
			char_data[4] = 0;
			char_data[5] = 0;
			char_data[6] = 0;
			char_data[7] = 0;
			char_data[8] = 0;
			char_data[9] = 0;
			char_data[10] =0;
			char_data[11] = 0;
			char_data[12] = 0;
			char_data[13] = 0;
			char_data[14] = 0;
			char_data[15] = 0;
			break;
		default:
			printf ("There is no action on Sector %ld\n", Sector);
			return NULL;
	}
	return char_data;
}

/************************************************************************
 * Response to system call PHYSICAL_DISK_WRITE;
 * in : SystemCallData;
 ************************************************************************/

void DiskWrite(SYSTEM_CALL_DATA *SystemCallData){
	MEMORY_MAPPED_IO mmio;
	long DiskID = (long)SystemCallData->Argument[0];
	if (DiskID<0 || DiskID>= MAX_NUMBER_OF_DISKS){			//Invalid diskid
			*SystemCallData->Argument[1] = ERR_BAD_PARAM;
			return ;
		}
	while (1){
		Dolock(DiskQueueLock);
		int DiskisInUse = diskQueue_checkDiskInUse(diskQueue, DiskID);
		DoUnlock(DiskQueueLock);
		if (!DiskisInUse)
			break;
		else
			CALL(wasteTime());
	}


	long Sector = (long)SystemCallData->Argument[1];
	long disk_buffer_write = (long) SystemCallData->Argument[2];


	struct Data * InUseDisk;
	long contextID = Get_Current_ContextID();
	//Dolock(pcbnode_Lock);
	InUseDisk = FindExecutingPCBNode(contextID);
	//DoUnlock(pcbnode_Lock);
	InUseDisk->Argument = DiskID;
	//put the pcbnode into diskQueue
	Dolock(DiskQueueLock);
	diskQueue_enQueue(diskQueue, InUseDisk);
	DoUnlock(DiskQueueLock);
	// Now see that the disk IS NOT running
	mmio.Mode = Z502Status;
	mmio.Field1 =DiskID;
	mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Disk, &mmio);

	if(mmio.Field2 == DEVICE_FREE){
		// Start the disk by writing a block of data
		mmio.Mode = Z502DiskWrite;
		mmio.Field1 = DiskID;
		mmio.Field2 = Sector;
		mmio.Field3 = disk_buffer_write;

		// Do the hardware call to put data on disk
		MEM_WRITE(Z502Disk, &mmio);
	}

	// Now see that the disk IS running
	mmio.Mode = Z502Status;
	mmio.Field1 = DiskID;
	mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Disk, &mmio);
	if (mmio.Field2 == DEVICE_IN_USE)        // Disk should report being used
		printf("Disk Test 2: Got expected result for Disk Status\n");
	else
		printf("Disk Test 2: Got erroneous result for Disk Status\n");

	Dispatcher();
}

/************************************************************************
 * Response to system call PHYSICAL_DISK_READ;
 * in : SystemCallData;
 ************************************************************************/
void DiskRead(SYSTEM_CALL_DATA *SystemCallData){
	MEMORY_MAPPED_IO mmio;
	long DiskID = (long)SystemCallData->Argument[0];
	if (DiskID<0 || DiskID>= MAX_NUMBER_OF_DISKS){			//Invalid diskid
			*SystemCallData->Argument[1] = ERR_BAD_PARAM;
			return ;
		}

	while (1){
			Dolock(DiskQueueLock);
			int DiskisInUse = diskQueue_checkDiskInUse(diskQueue, (long)SystemCallData->Argument[0]);
			DoUnlock(DiskQueueLock);
			if (!DiskisInUse)
				break;
			else
				CALL(wasteTime());
	}


	long Sector =(long)SystemCallData->Argument[1];
	long disk_buffer_read = (long) SystemCallData->Argument[2];

	struct Data * InUseDisk;
	long contextID = Get_Current_ContextID();
	//Dolock(pcbnode_Lock);
	InUseDisk = FindExecutingPCBNode(contextID);
	//DoUnlock(pcbnode_Lock);
	InUseDisk->Argument = DiskID;
	//put the pcbnode into diskQueue
	Dolock(DiskQueueLock);
	diskQueue_enQueue(diskQueue, InUseDisk);
	DoUnlock(DiskQueueLock);
	// The disk should now be done.  Make sure it's idle
	mmio.Mode = Z502Status;
	mmio.Field1 =DiskID;
	mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Disk, &mmio);
	if (mmio.Field2 == DEVICE_FREE){        // Disk should be free
		//Now we read the data back from the disk.  If we're lucky,
		//we'll read the same thing we wrote!
		// Start the disk by reading a block of data

		mmio.Mode = Z502DiskRead;
		mmio.Field1 = DiskID; // Pick same disk location
		mmio.Field2 = Sector;
		mmio.Field3 = disk_buffer_read;

		// Do the hardware call to read data from disk
		MEM_WRITE(Z502Disk, &mmio);
	}

	Dispatcher();

}
/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 The variable do_print is designed to print out the data for the
 incoming calls, but does so only for the first ten calls.  This
 allows the user to see what's happening, but doesn't overwhelm
 with the amount of data.
 ************************************************************************/

void svc(SYSTEM_CALL_DATA *SystemCallData) {
	short call_type;
	static short do_print = 10;
	short i;
	MEMORY_MAPPED_IO mmio;

	call_type = (short) SystemCallData->SystemCallNumber;

	if (do_print > 0) {
		printf("SVC handler: %s\n", call_names[call_type]);
		for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++) {
			//Value = (long)*SystemCallData->Argument[i];
			printf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
					(unsigned long) SystemCallData->Argument[i],
					(unsigned long) SystemCallData->Argument[i]);
		}
		do_print--;
	}

	switch (call_type){
		//Get time service call
		case SYSNUM_GET_TIME_OF_DAY:		//This value is found in syscalls.h
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 =0;
			MEM_READ(Z502Clock,&mmio);
			*(long *)SystemCallData -> Argument[0] = mmio.Field1;
			break;
		//terminate system call
		case SYSNUM_TERMINATE_PROCESS:
			TerminateProsess(SystemCallData);
			break;
		case SYSNUM_SLEEP:
			StartTimer(SystemCallData);
			break;
		case SYSNUM_GET_PROCESS_ID:
			Dolock(pcbnode_Lock);
			GetProcessID(SystemCallData);
			DoUnlock(pcbnode_Lock);
			break;
		case SYSNUM_CREATE_PROCESS:
			Dolock(count_Process_Lock);
			Dolock(pcbnode_Lock);
			CreatProcess(SystemCallData);
			DoUnlock(pcbnode_Lock);
			DoUnlock(count_Process_Lock);
			break;
		case SYSNUM_PHYSICAL_DISK_WRITE:
			DiskWrite(SystemCallData);
			break;
		case SYSNUM_PHYSICAL_DISK_READ:
			DiskRead(SystemCallData);
			break;
		case SYSNUM_FORMAT:
			Dolock(Disk_is_Formatted_Lock);
			DiskFormat(SystemCallData);
			DoUnlock(Disk_is_Formatted_Lock);
			break;
		case SYSNUM_CHECK_DISK:
			DiskCheck(SystemCallData);
			break;
		case SYSNUM_OPEN_DIR:
			DiskOpenDIR(SystemCallData);
			break;
		case SYSNUM_CREATE_DIR:
			DiskCreateDIR(SystemCallData);
			break;
		case  SYSNUM_CREATE_FILE:
			DiskCreateFile(SystemCallData);
			break;
		case SYSNUM_DEFINE_SHARED_AREA:
			Define_Shared_Area(SystemCallData);
			break;
		case SYSNUM_SEND_MESSAGE:
			Send_MESSAGE(SystemCallData);
			break;
		case SYSNUM_RECEIVE_MESSAGE:
			Receive_MESSAGE(SystemCallData);
			break;
		default:
			printf ("EEROR! call_type not recognized!\n");
			printf ("Call_type is - %i\n", call_type);
		}											//End of switch
}                                               // End of svc

/************************************************************************
 * Get_currentContext;
 * out: current executing context ID;
 ************************************************************************/
long Get_Current_ContextID(){
	MEMORY_MAPPED_IO mmio;
	long contextID;
	mmio.Mode = Z502GetCurrentContext;
	mmio.Field1 = 0;
	mmio.Field2 = 0;
	mmio.Field3 = 0;
	MEM_READ(Z502Context, &mmio);
	contextID = (long)mmio.Field1;
	return contextID;
}
/************************************************************************
 StartContext
 in: PCB node;
 ************************************************************************/
void StartContext(struct Data *nextStartNode){
	MEMORY_MAPPED_IO mmio;

	mmio.Mode = Z502StartContext;
	mmio.Field1 = (long) nextStartNode->Process_Pointer;
		// Field1 contains the value of the context returned in the last call
		// Suspends this current thread
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	//mmio.Field2 = START_NEW_CONTEXT_ONLY;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
}
/************************************************************************
MyInitial
The initial function to create PCB and queues to record the information
of a new process.
in: process name, context ID;
out: PCB node;
************************************************************************/

struct Data * MyInitial(char * process_name, long contextID, short * PageTable ){
	int i,j;
	//create new Data Node
	struct Shadow_PageTable *Shadow_PageTable = (struct Shadow_PageTable *)malloc(NUMBER_VIRTUAL_PAGES * sizeof(struct Shadow_PageTable));
	struct Data start_pcb = {.InitialPriority = 0, .Process_ID = 0, .Process_Name = process_name,
			 .Process_Status = "running", .Process_Pointer = (long*)contextID, .Argument = 0, .PageTable = PageTable,
			 .Shadow_PageTable = Shadow_PageTable};
	struct Data * tempnode;


	if(MultiProcess_Controller == 0){

		//initialize the pcb datanode array
		for (i=0; i<MAX_NUM_PROCESS; i++) {
			pcbnode[i] = NULL;
		}

		//Initialize array to record the disk is formatted or not
		for (i=0; i<MAX_NUMBER_OF_DISKS; i++){
			Disk_is_Formatted[i] = 0;
		}

		//Initialize array to record the disk is formatted completely or not
		for (i=0; i<MAX_NUMBER_OF_DISKS; i++){
			Dolock(Format_is_complete_Lock);
			Format_is_complete[i] = 1;
			DoUnlock(Format_is_complete_Lock);
		}

		//Initialize SwapBitmay array
		for (i=0; i<MAX_NUMBER_OF_DISKS; i++){
			for (j=0;j< SwapSize*4;j++){
				SwapBitmap[i][j] = 0;
			}
		}

		//Initialize FrameTable
		for (i=0; i< NUMBER_PHYSICAL_PAGES; i++) {
			FrameTable[i].is_Occupied = 0;
		}
		//initialize frame manager
		frame_manager.isFull = 0;			//is not full
		frame_manager.current_available_frame = 0;
		frame_manager.counter_occupied_frame = 0;

		//initialize disk manager, which is used during memory management
		diskmanager.is_All_used = 0;
		diskmanager.current_available_Disk = 0;

		for (i=0; i<MAX_NUMBER_OF_DISKS; i++){
			diskmanager.disk_in_use[i] = 0;
		}

		for (i=0; i<Max_Message_Box; i++) {
			MailBox[i] = 0;
		}
		//create all Queues
		timerQueue = create_TimerQueue();
		diskQueue = create_diskQueue();
		ReadyQueue = create_readyQueue();
		MessageQueue = create_MessageQueue();
	}


	pcbnode[MultiProcess_Controller] = createDataNode(&start_pcb);
	tempnode = pcbnode[MultiProcess_Controller];

	MultiProcess_Controller += 1;
	//return pcbnode[MultiProcess_Controller];
	return tempnode;
}
/************************************************************************
 OScreatprocesses
 This part create process for the initial input test;
 in: testID, process name;
 ************************************************************************/
void OSCreatProcess(long *testID, char *process_name){
	MEMORY_MAPPED_IO mmio;
	void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);

	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long) testID;
	mmio.Field3 = (long) PageTable;

	MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

	long contextID = mmio.Field1;

	Dolock(MultiProcess_Controller_Lock);
	Dolock(pcbnode_Lock);
	struct Data * startnode = MyInitial(process_name, contextID, PageTable);
	DoUnlock(pcbnode_Lock);
	DoUnlock(MultiProcess_Controller_Lock);

	//current_execute_PCB = startnode;
	StartContext(startnode);

	//Dolock(MultiProcess_Controller_Lock);
	//MultiProcess_Controller += 1;
	//DoUnlock(MultiProcess_Controller_Lock);
}
/************************************************************************
 osInit
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/
void osInit(int argc, char *argv[]) {
	void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
	INT32 i;
	MEMORY_MAPPED_IO mmio;
	long *testID;

	// Demonstrates how calling arguments are passed thru to here
	printf("Program called with %d arguments:", argc);
	for (i = 0; i < argc; i++)
		printf(" %s", argv[i]);
	printf("\n");
	printf("Calling with argument 'sample' executes the sample program.\n");

	// Here we check if a second argument is present on the command line.
	// If so, run in multiprocessor mode
	if (argc > 2) {
		if ( strcmp( argv[2], "M") || strcmp( argv[2], "m")) {
		printf("Simulation is running as a MultProcessor\n\n");
		mmio.Mode = Z502SetProcessorNumber;
		mmio.Field1 = MAX_NUMBER_OF_PROCESSORS;
		mmio.Field2 = (long) 0;
		mmio.Field3 = (long) 0;
		mmio.Field4 = (long) 0;
		MEM_WRITE(Z502Processor, &mmio);   // Set the number of processors
		}
	} else {
		printf("Simulation is running as a UniProcessor\n");
		printf(
				"Add an 'M' to the command line to invoke multiprocessor operation.\n\n");
	}

	//          Setup so handlers will come to code in base.c

	TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR ] = (void *) InterruptHandler;
	TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR ] = (void *) FaultHandler;
	TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR ] = (void *) svc;

	//  Determine if the switch was set, and if so go to demo routine.

	//PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
	if ((argc > 1) && (strcmp(argv[1], "sample") == 0)) {
		mmio.Mode = Z502InitializeContext;
		mmio.Field1 = 0;
		mmio.Field2 = (long) SampleCode;
		mmio.Field3 = (long) PageTable;

		MEM_WRITE(Z502Context, &mmio);   // Start of Make Context Sequence
		mmio.Mode = Z502StartContext;
		// Field1 contains the value of the context returned in the last call
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context

	} // End of handler for sample code - This routine should never return here

	char *name;

	if(strcmp(argv[1],"test0") == 0){
		testID = (long*)test0;
		name = "test0";
	}
	else if(strcmp(argv[1],"test1") == 0){
		testID = (long*)test1;
		name = "test1";
	}
	else if(strcmp(argv[1],"test2") == 0){
		testID = (long*)test2;
		name = "test2";
	}
	else if(strcmp(argv[1],"test3") == 0){
		testID = (long*)test3;
		name = "test3";
	}
	else if(strcmp(argv[1],"test4") == 0){
		testID = (long*)test4;
		name = "test4";
	}
	else if(strcmp(argv[1],"test5") == 0){
		testID = (long*)test5;
		name = "test5";
	}
	else if(strcmp(argv[1],"test6") == 0){
		testID = (long*)test6;
		name = "test6";
	}
	else if(strcmp(argv[1],"test7") == 0){
		testID = (long*)test7;
		name = "test7";
	}
	else if(strcmp(argv[1],"test8") == 0){
		testID = (long*)test8;
		name = "test8";
	}
	else if(strcmp(argv[1],"test9") == 0){
		testID = (long*)test9;
		name = "test9";
	}
	else if(strcmp(argv[1],"test10") == 0){
		testID = (long*)test10;
		name = "test10";
	}
	else if(strcmp(argv[1],"test11") == 0){
		testID = (long*)test11;
		name = "test11";
	}
	else if(strcmp(argv[1],"test12") == 0){
		testID = (long*)test12;
		name = "test12";
	}
	else if(strcmp(argv[1],"test13") == 0){
		testID = (long*)test13;
		name = "test13";
	}
	else if(strcmp(argv[1],"test14") == 0){
		testID = (long*)test14;
		name = "test14";
	}
	else if(strcmp(argv[1],"test15") == 0){
		testID = (long*)test15;
		name = "test15";
	}
	else if(strcmp(argv[1],"test16") == 0){
		testID = (long*)test16;
		name = "test16";
	}
	//else if(strcmp(argv[1],"testD") == 0)
		//testID = (long*)testD;
	//else if(strcmp(argv[1],"testX") == 0)
		//testID = (long*)testX;
	//else if(strcmp(argv[1],"testZ") == 0)
		//testID = (long*)testZ;
	//else if(strcmp(argv[1],"testM") == 0)
		//testID = (long*)testM;
	else if(strcmp(argv[1],"test21") == 0){
		testID = (long*)test21;
		name = "test21";
	}
	else if(strcmp(argv[1],"test22") == 0){
		testID = (long*)test22;
		name = "test22";
	}
	else if(strcmp(argv[1],"test23") == 0){
		testID = (long*)test23;
		name = "test23";
	}
	else if(strcmp(argv[1],"test24") == 0){
		testID = (long*)test24;
		name = "test24";
	}
	else if(strcmp(argv[1],"test25") == 0){
		testID = (long*)test25;
		name = "test25";
	}
	else if(strcmp(argv[1],"test26") == 0){
		testID = (long*)test26;
		name = "test26";
	}
	else if(strcmp(argv[1],"test27") == 0){
		testID = (long*)test27;
		name = "test27";
	}
	else if(strcmp(argv[1],"test28") == 0){
		testID = (long*)test28;
		name = "test28";
	}
	//else if(strcmp(argv[1],"testS") == 0)
		//testID = (long*)testS;
	else
		printf("There donot have this test!!\n");


	//  Creation and Switching of contexts is done in the OSCreateProcess() routine.
	OSCreatProcess(testID, name);

}                                               // End of osInit
