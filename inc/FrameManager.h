/*
 * FrameManager.h
 *
 *  Created on: Nov 8, 2016
 *      Author: yaqiong
 */

#ifndef FRAMEMANAGER_H_
#include "global.h"

typedef struct frame{
	long Process_ID;
	long Page_No;
	int is_Occupied;
	int is_shared_frame;
}frame;

typedef struct frameManager{
	int isFull;
	int current_available_frame;
	int counter_occupied_frame;
	int shared_frame[64];
	int num_of_shared_frame;
	int is_shared;
}frameManager;


#endif /* FRAMEMANAGER_H_ */
