/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */


#include "twcommon.h"
#include "twusrlib.h"
#include "outList.h"
#include "twpackages.h"
#include "TWULarray.h"

#define INTERNAL_EVT_SEL -2
#define INIT_SIZE_EVT_LIST 10
#define INC_EVT_LIST 5

int evt_listInit(), evt_listSoe(), evt_listEoe(), evt_listTell();

typedef struct
	{
	VTime recTime;
	VTime sendTime;
	long selector;
	int length;
	Pointer newText;
	} EvtListType;


typedef struct
   {
	TWULarrayType evtList;
	TWULarrayType recTimeList;
	int internalEvtFlag;
   }  evtListState;


packageType evtListPackage =
   {
   evt_listInit, evt_listSoe, 5, evt_listEoe, 50, evt_listTell, 1, 
											NULL, sizeof(evtListState)
   };



evt_listInit()
{
	evtListState * ps;
	
	ps = myPState(&evtListPackage);


	/* Initialize the two dynamic arrays.
    */
	TWULInitArray(&ps->evtList, INIT_SIZE_EVT_LIST, 
													sizeof(EvtListType), INC_EVT_LIST);
	TWULInitArray(&ps->recTimeList, INIT_SIZE_EVT_LIST, 
													sizeof(VTime), INC_EVT_LIST);
	
	ps->internalEvtFlag = 0;
}

	

/* The next internal event to be processed (lowest recTime)
 * should have an entry in the recTimeList.  If the entry
 * doesn't exist either we've sent a new message with the
 * lowest receive time, or we just processed an internal
 * event and the next internal event still needs an internal
 * event signal sent.  The result is the same in either case.
 */
evt_listEoe()
{
	EvtListType * evtPtr;
	VTime nextRecTime, * recTimePtr;
	evtListState * ps;
	int size, result, i;
	Name me;

	ps = myPState(&evtListPackage);
	myName(me);

	/* Check to see if an internal event has occurred.
    */
	if (ps->internalEvtFlag == 0)	
		return;

	ps->internalEvtFlag = 0;


	/* Find the internal event with the lowest VTime.
    */
	size = TWULSizeArray(&ps->evtList);
	if (size == 0) return;
	
	evtPtr = TWULGetElem(&ps->evtList, 0);
	nextRecTime = evtPtr->recTime;
	for (i=1; i<size; i++)
		{
		evtPtr = TWULGetElem(&ps->evtList, i);
		result = ltVTime(evtPtr->recTime, nextRecTime);
		if (result)
			nextRecTime = evtPtr->recTime;
		}


	/* Now look thru the recTimeList to see if there is 
    * a signal for that time.  If not, then send one.
    */
	size = TWULSizeArray(&ps->recTimeList);
	for (i=0; i<size; i++)
		{
		recTimePtr = TWULGetElem(&ps->recTimeList, i);
		result = eqVTime((*recTimePtr), nextRecTime);
		if (result) return;
		}

	tell(me, nextRecTime, INTERNAL_EVT_SEL, sizeof(Name), me);
	TWULAddElem(&ps->recTimeList, &nextRecTime);
}


evt_listSoe()
{
	evtListState * ps;
	int i;
	void xfer_intEvtsToMsgList();
	void remove_recTimeFromList();
	
	ps = myPState(&evtListPackage);
	
	/* Look through the user message list to see if there are any
    * internal event messages.
    */
	for (i=0; i<LIBnumMsgs; i++)
		{
		if (LIBmsgSelector(i) != INTERNAL_EVT_SEL)
			continue;
		
		/* We've found an internal event message.  Remove it
       * from the user message list and and then transfer
       * the internal events to the user message list.
       * Note that this will collapse the list so we don't
       * need to inc i.  Set the internal event flag for the 
       * end of event function.
       */
		LIBdelete_message(i);

		xfer_intEvtsToMsgList(ps);

		ps->internalEvtFlag = 1;

		/* Now remove the time from the recTimeList.
       */
		remove_recTimeFromList(ps);

		/* There is at most one internal event message per
       * event because the sending end checks to see if
       * it's already sent one for a particular recTime.
       * Thus, we can now just return.
		 */
		return;
		}
}


void remove_recTimeFromList(ps)
	evtListState * ps;
{
	Name me;
	VTime * timePtr;
	VTime temp;
	int listIndex, result;
	int timesToCheck;

	timesToCheck = TWULSizeArray(&ps->recTimeList);
	
	for (listIndex=0; listIndex < timesToCheck; listIndex++)
		{
		timePtr = TWULGetElem(&ps->recTimeList, listIndex);
		temp = now;
		result = eqVTime((*timePtr) , temp);
		if (result)
			{
			TWULRemoveElem(&ps->recTimeList, listIndex); 
			return;
			}
		}

	userError("Can't find recTime in recTime list in remove_recTimeFromList.");
}	
		


void xfer_intEvtsToMsgList(ps)
	evtListState * ps;
{
	Name me;
	EvtListType	 * evtPtr;
	int listIndex, result;
	int evtsToCheck;
	VTime	temp;

	/* First transfer the events to be messages in the user message
    * list.
    */
	evtsToCheck = TWULSizeArray(&ps->evtList);

	for (listIndex=0; evtsToCheck > 0; evtsToCheck--)
		{
		evtPtr = TWULGetElem(&ps->evtList, listIndex);
			
		temp = now;
		result = eqVTime(evtPtr->recTime, temp);
		if (result)	
			{
			LIBinsert_message(evtPtr->selector, myName(me), evtPtr->sendTime, 
					pointerPtr(evtPtr->newText), evtPtr->newText, evtPtr->length);
			TWULRemoveElem(&ps->evtList, listIndex);
			} /* end if result */
		else
			listIndex++;

		} /* end for listIndex */
}	


/* This function removes any messages from the outList that
 * are sent to self.  A removed message is placed in the 
 * event list.  If it has the lowest virtual receive time of
 * all messages in the event list, then an internal event
 * message is placed in the output list.
 */
evt_listTell(outListPtr)
	OutListType * outListPtr;
{
	evtListState * ps;
	int numMsgsToCheck, listIndex;
	OutMsgType * msgPtr;
	Name me;
	void queue_internalMsg();

	ps = myPState(&evtListPackage);


	/* Determine the number of messages that need to
    * be checked.
    */
	numMsgsToCheck = out_listSize(outListPtr);


	/* Loop thru the messages.
    */
	for (listIndex=0; numMsgsToCheck > 0; numMsgsToCheck--)
		{
		/* The msgPtr points to the message we are considering.
		 * If the message is to self to pull it from the out list
       * and place it in the internal evt queue.
       */
      msgPtr = get_outListMsg(outListPtr, listIndex);

		if ( strcmp(myName(me), msgPtr->receiver) == 0 )
			{
			queue_internalMsg(msgPtr, ps, outListPtr, me);
         remove_outListMsg(outListPtr, listIndex);
         }

      /* We only increment listIndex if we haven't removed a msg from
       * the outlist.  If we remove a msg then the list is collapsed,
       * which means that we need not increase the index.
       */
      else listIndex++;

      } /* end for numMsgsToCheck */
}


void queue_internalMsg(msgPtr, ps, outListPtr, me)
	OutMsgType * msgPtr;
	evtListState * ps;
	OutListType * outListPtr;
	char * me;
{
	VTime * recTimePtr;
	EvtListType newEvt, * evtPtr;
	char * text;
	int i, size, result;


	/* Allocate space from the message text and copy it over.
    */
	newEvt.newText = newBlockPtr(msgPtr->length);
	text = (char *)pointerPtr(newEvt.newText);
	TWULfastcpy(text, msgPtr->text, msgPtr->length);
	

	/* Copy over the pertinent information and put the message in the
    * list.  If an internal event signal needs to be sent for this
    * message, then it will be done in the end of event function.
    */
	newEvt.recTime = msgPtr->recTime;
	newEvt.sendTime = now;
	newEvt.selector = msgPtr->selector;
	newEvt.length = msgPtr->length;
	
	TWULAddElem(&ps->evtList, &newEvt);

	
	/* Set the internal event flag in the package state.
    */
	ps->internalEvtFlag = 1;
}
