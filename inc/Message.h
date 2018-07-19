/*
 * Message.h
 *
 *  Created on: Dec 4, 2016
 *      Author: yaqiong
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_


typedef struct Message{
	long Target_ProcessID;
	char* MessageBuffer;
	int MessageSendLength;
	int SendisMade;
}Message;


struct Message_Send
{
	long Target_ProcessID;
	long Source_ProcessID;
	char * MessageBuffer;
	int MessageSendLength;
	int SendisMade;
};

struct Message_Receive
{
	long Source_ProcessID;
	char* ReceiveBuffer;
	int MessageReceiveLength;
};

struct Message_Send * createMessageNode(struct Message_Send * datanode);
void release_MessageNode(struct Message_Send* datanode);
#endif /* MESSAGE_H_ */
