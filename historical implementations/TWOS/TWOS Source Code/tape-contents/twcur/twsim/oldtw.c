/* "Copyright (C) 1989, California Institute of Technology. 
     U. S. Government Sponsorship under NASA Contract 
   NAS7-918 is acknowledged." */

#include "twcommon.h"
typedef int Vtime;
#ifndef FUNCTION
#define FUNCTION 
#endif

/************************************************************************
*
*			Time Warp Entry Point
*
*	obj_now - return the current virtual time
*
*	called by - application routine with MACRO "now"
*
*	- return the receive time of the current event message
*
*************************************************************************/

FUNCTION  Vtime obj_nowI()
{
    Vtime ret;
    VTime xnow;
    xnow = now;

    return( (Vtime)xnow.simtime);
}


/************************************************************************
*
*			Time Warp Entry Point
*
*	sendtime - return the send virtual time of a message
*
*	called by - application routines
*
*	- handle debug
*	- return the send time of the specified message
*	- handle debug
*
*************************************************************************/

FUNCTION  msgSendTimeI(msgnum)
int             msgnum;
{
    VTime   val;
    Vtime   valI;

    val = msgSendTime(msgnum);
    valI = val.simtime;
    return(valI);
}
/************************************************************************
*
*			Time Warp Entry Point
*
*	tell - queue event message
*
*	called by - application routines
*
*	- handle debug
*	- perform routine to format and init the event message
*	- perform routine to link the message into the queue
*
*************************************************************************/

FUNCTION tellI( rcvr, rcvtime, msg_selector, len, textptr )
Name_object     rcvr;
Vtime           rcvtime;
Long		msg_selector;
int	        len;
Message        *textptr;
{
	VTime  thetime;
	double asimtime;

	asimtime = rcvtime;
	thetime = newVTime( asimtime, 0L, 0L);
	tell( rcvr, thetime, msg_selector, len, textptr);

}


FUNCTION newObjI(rcver,rcvtim,objtype)
Name_object     rcver;
Vtime rcvtim;
Type_object objtype;
{
	VTime  thetime;
	thetime = newVTime( (double)rcvtim, 0L, 0L);
	newObj(rcver,thetime,objtype);
}

FUNCTION delObjI(rcver,rcvtim)
Name_object     rcver;
Vtime rcvtim;
{
	VTime  thetime;

	thetime = newVTime( (double)rcvtim, 0L, 0L);
	delObj(rcver,thetime);

}
