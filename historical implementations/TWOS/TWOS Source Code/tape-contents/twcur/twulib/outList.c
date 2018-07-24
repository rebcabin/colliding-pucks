/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

#include "twcommon.h"
#include "twusrlib.h"
#include "outList.h"
#include "TWULarray.h"


#define MAX_OUT_ELEMS 10
#define INC_OUT_ELEMS 5


void init_outList(outListPtr)
	OutListType * outListPtr;
{
	TWULInitArray((TWULarrayType *)outListPtr, MAX_OUT_ELEMS, 
											sizeof(OutMsgType), INC_OUT_ELEMS);
}
	

void add_outListMsg(outListPtr, rec, recTime, sel, len, txt, newText)
   OutListType * outListPtr; 
   char * rec;
   VTime recTime;
   long sel;
   int len;
   char * txt;
	Pointer newText;
{
	OutMsgType msg;

	clear(&msg, sizeof(OutMsgType));

	strcpy(msg.receiver, rec);
   msg.recTime = recTime;
   msg.selector = sel;
   msg.length = len;
   msg.text = txt;
   msg.newText = newText;

   TWULAddElem((TWULarrayType *)outListPtr, &msg);
}


int out_listSize(outListPtr)
   OutListType * outListPtr; 
{
	return TWULSizeArray( (TWULarrayType *)outListPtr );	
}


OutMsgType * get_outListMsg(outListPtr, i)
	OutListType * outListPtr;
	int i;
{
	return (OutMsgType *)TWULGetElem( (TWULarrayType *)outListPtr, i );
}			


void send_outList(outListPtr)
   OutListType * outListPtr; 
{
	OutMsgType * msgPtr;
	int size = out_listSize(outListPtr);
	int i;

	for (i=0; i<size; i++)
		{
		msgPtr = get_outListMsg(outListPtr, i);

      tell(msgPtr->receiver, msgPtr->recTime, msgPtr->selector, 
														msgPtr->length, msgPtr->text);
      } /* end for i */
}


void dispose_outList(outListPtr)
	OutListType * outListPtr;
{
	OutMsgType * msgPtr;
	int size = out_listSize(outListPtr);
	int i;

	for (i=0; i<size; i++)
		{
		msgPtr = get_outListMsg(outListPtr, i);

		if (msgPtr->newText != NULL)
			disposeBlockPtr(msgPtr->newText);
		}
	
	TWULDispose((TWULarrayType *)outListPtr);
}	
	


void remove_outListMsg(outListPtr, i)
	OutListType * outListPtr;
	int i;
{
	OutMsgType * msgPtr;
	
	msgPtr = get_outListMsg(outListPtr, i);
	if (msgPtr->newText != NULL)
      disposeBlockPtr(msgPtr->newText);

	TWULRemoveElem((TWULarrayType *)outListPtr, i);
}

