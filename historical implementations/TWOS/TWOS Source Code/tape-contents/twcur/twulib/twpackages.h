/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/************************************************************
 * twpackages.h -- header file for package interfaces       *
 ************************************************************/


#define MSGLIST 1         /* only msglist */
#define MULTIPACKET 3     /* multipacket + msglist */
#define INT_EVENT_LIST 5  /* internal evt list + msglist */

#define ENHANCED_MSG_PACKAGES 7

void LIBinsert_message();
void LIBdelete_message();
int LIBnumber_messages();
#define LIBnumMsgs LIBnumber_messages()
void * LIBmsgText();
char * LIBmsgSender();
VTime LIBmsgSendTime();
int LIBmsgLength();
long LIBmsgSelector();
void LIBprintMsg();
void LIBprintMsgList();
#define LIBtell twulib_tell_handler
