/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */


#include "twcommon.h"
#include "twusrlib.h"
#include "TWULarray.h"
#include "twpackages.h"


int msg_listSoe(), msg_listEoe();

typedef struct
   {
   TWULarrayType msgList;
   } msgListState;

packageType userMsgListPackage = 
   {
   NULL, msg_listSoe, 1, msg_listEoe, 99, NULL, 0, NULL, sizeof(msgListState)
   };

typedef struct
   {
   long selector;
   Name sender;
   VTime sendTime;
   char * text;
   Pointer newText;
   int length;
   } userMsg;

#define MLSIZE_MULT  2
#define MLSIZE_INC  4



/**********************************************************************
 First we define the library interface points.  Only two points
 are needed:  start of event and end of event.  The start of event
 will copy the current TWOS message list into the user message list.
 Note that only a pointer to the message text is copied, not the
 whole text.  The end of event function will deallocate the
 dynamic array used for the user message list.
********************************************************************/
msg_listSoe()
{
   int i;
   msgListState * ps;
   userMsg tmp;

   ps = myPState(&userMsgListPackage);

   /* For the start of event function the user message
    * list must copy all current messages for this event
    * into the list.
    *
    * Note that these messages have already been ordered
    * by TWOS.
    *
    * First we allocate the memory for the list using a
    * dynamic array.  Then we copy the messages in.
    */
   TWULInitArray(&ps->msgList, MLSIZE_MULT * numMsgs, 
															sizeof(userMsg), MLSIZE_INC);
   
   for (i=0; i<numMsgs; i++)
      {
      tmp.selector = msgSelector(i);
      strcpy(tmp.sender, msgSender(i));
      tmp.sendTime = msgSendTime(i);
      tmp.text = msgText(i);
      tmp.newText = 0;
      tmp.length = msgLength(i);

      TWULAddElem(&ps->msgList, &tmp);      
      }
}      


msg_listEoe()
{
   msgListState * ps;
	int i, size;
	userMsg * tmp;

   ps = myPState(&userMsgListPackage);


	/* First go through the message list and destroy
    * all of the non-NULL newText pointers so that  
    * the memory is returned to the system.
    */
	size = TWULSizeArray(&ps->msgList);
	for (i=0; i<size; i++)
		{
		tmp = (userMsg *)TWULGetElem(&ps->msgList, i);
		if (tmp->newText != 0)
			disposeBlockPtr(tmp->newText);
		}

   /* Since this is the end of the event we can
    * trash the current messagae list.
    */
   TWULDispose(&ps->msgList);
}




/*************************************************************************
 These are the functions offered by the user message list.
************************************************************************/

void LIBinsert_message(selector, sender, sendTime, text, newText, length)
   long selector;
   char * sender;
   VTime sendTime;   
   char * text;
   Pointer newText;
   int length;
{
   msgListState * ps;
   userMsg * tmp, msg;
   int msg_rank();
	int rank, size, i;

   ps = myPState(&userMsgListPackage);

   /* First create a user message structure. 
    */
   msg.selector = selector;
   strcpy(msg.sender, sender);
   msg.sendTime = sendTime;
   msg.text = text;
   msg.newText = newText;
   msg.length = length;


   /* Now we have to determine the rank of the 
    * message.
    */
   rank = msg_rank(&msg, ps);
	size = TWULSizeArray(&ps->msgList);

	/* Move current elements over for new element.
    */
	for (i=size; i>rank; i--)
		{
		tmp = TWULGetElem(&ps->msgList, i-1);
		TWULInsertElem(&ps->msgList, tmp, i);
		}

   /* Now put the new message in the list.
    */   
	TWULInsertElem(&ps->msgList, &msg, i);	
}



void LIBdelete_message(msgNum)
   int msgNum;
{
   msgListState * ps;
   userMsg * tmp;

   ps = myPState(&userMsgListPackage);

	
	tmp = TWULGetElem(&ps->msgList, msgNum);
	if (tmp->newText != 0)
		disposeBlockPtr(tmp->newText);

   /* Simply remove the message from the dynamic array.
    */
   TWULRemoveElem(&ps->msgList, msgNum);
}



int LIBnumber_messages()
{
   msgListState * ps;

   ps = myPState(&userMsgListPackage);

   return ( TWULSizeArray(&ps->msgList) );
}



void * LIBmsgText(msgNum)
   int msgNum;
{
   userMsg * msg;
   msgListState * ps;

   ps = myPState(&userMsgListPackage);

	msg = TWULGetElem(&ps->msgList, msgNum);

   return ( (void *)msg->text );
}



char * LIBmsgSender(msgNum)
   int msgNum;
{
   userMsg * msg;
   msgListState * ps;

   ps = myPState(&userMsgListPackage);

	msg = TWULGetElem(&ps->msgList, msgNum);

   return msg->sender;
}



VTime LIBmsgSendTime(msgNum)
   int msgNum;
{
   userMsg * msg;
   msgListState * ps;

   ps = myPState(&userMsgListPackage);

	msg = TWULGetElem(&ps->msgList, msgNum);

   return msg->sendTime;
}



int LIBmsgLength(msgNum)
   int msgNum;
{
   userMsg * msg;
   msgListState * ps;

   ps = myPState(&userMsgListPackage);

	msg = TWULGetElem(&ps->msgList, msgNum);

   return msg->length;
}



long LIBmsgSelector(msgNum)
   int msgNum;
{
   userMsg * msg;
   msgListState * ps;

   ps = myPState(&userMsgListPackage);

	msg = TWULGetElem(&ps->msgList, msgNum);

   return msg->selector;
}


void LIBprintMsg(msgNum)
   int msgNum;
{
   int i;
   userMsg * msg;
   char * text;
   msgListState * ps;

   ps = myPState(&userMsgListPackage);

	msg = TWULGetElem(&ps->msgList, msgNum);

   printf("*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*\n");
   printf("Message selector:\t %ld\n", msg->selector);
   printf("Message sender:\t\t %s\n", msg->sender);
   printf("Message send time:\t %lf, %ld, %ld\n", GetSimTime(msg->sendTime),
               GetSequence1(msg->sendTime), GetSequence2(msg->sendTime) );
   printf("Message length:\t\t %d\n", msg->length);
   printf("Message text:\t\t ");
   
   for (i=0, text = msg->text; i<10; i++)
      printf("%c", text[i]);
   printf("\n");
   printf("*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*\n");
}
 


void LIBprintMsgList()
{
   int i;

   for (i=0; i< LIBnumMsgs; i++)
      LIBprintMsg(i);
}


/*******************************************************************
 These are subordinate functions.
*******************************************************************/

int msg_rank(msgPtr, ps)
   userMsg * msgPtr;
   msgListState * ps;
{
   int msg_lessThan();
   int i, size, high;
   userMsg * tmp;

   size = TWULSizeArray(&ps->msgList);

   for (i=0; i<size; i++)
      {
      tmp = TWULGetElem(&ps->msgList, i);
      
      if (msg_lessThan(msgPtr, tmp) )
			return i;	
      }

   return i;
}


   
int msg_lessThan(m1, m2)
   userMsg * m1, * m2;
{
   int i, minLength;
   char * c1, * c2;

   if (m1->selector < m2->selector)
      return 1;
   if (m1->selector > m2->selector)
      return 0;

   minLength = ((m1->length < m2->length) ? m1->length : m2->length);

   c1 = m1->text;
   c2 = m2->text;

	switch ( bytcmp(c1, c2, minLength) )
		{
		case 0:  return ((m1->length < m2->length) ? 1 : 0);
		case 1:  return 0;
		case -1:  return 1;
		default:  userError("TWUSRLIB ERROR: Undefined msg comp.");
		}
}
