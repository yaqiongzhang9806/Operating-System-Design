/*
 * ShadowPageTable.h
 *
 *  Created on: Nov 8, 2016
 *      Author: yaqiong
 */

#ifndef SHADOWPAGETABLE_H_
#define SHADOWPAGETABLE_H_

struct Shadow_PageTable{
	long Diskid;
	long Sector;
	int inDisk;
};

#endif /* SHADOWPAGETABLE_H_ */
