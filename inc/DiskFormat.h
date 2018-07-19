/*
 * DiskFormat.h
 *
 *  Created on: Oct 13, 2016
 *      Author: yaqiong
 */

#ifndef DISKFORMAT_H_
#define DISKFORMAT_H_


#define BitmapSize 	4			//4*4=16 block
#define BitmapLocation 1		//locate in sector 1
#define  RootDirSize 2 			//2 blocks
#define RootDirLocation		BitmapLocation + BitmapSize*4
#define SwapLocation 	RootDirLocation + RootDirSize			//locate in sector 19
#define SwapSize 255				//size is 255*4 = 1020 blocks

#endif /* DISKFORMAT_H_ */
