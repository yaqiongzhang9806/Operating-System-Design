/*
 * shared_manager.h
 *
 *  Created on: Dec 2, 2016
 *      Author: yaqiong
 */

#ifndef SHARED_MANAGER_H_
#define SHARED_MANAGER_H_
#include <stdio.h>
#include <stdlib.h>
#include "global.h"

struct Shared_Manager{
	int StartingAddressOfSharedArea;
	int PagesInSharedArea;
	char AreaTag[32];
};


#endif /* SHARED_MANAGER_H_ */
