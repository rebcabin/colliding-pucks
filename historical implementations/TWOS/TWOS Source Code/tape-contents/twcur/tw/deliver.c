/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	deliver.c,v $
 * Revision 1.7  91/11/01  09:19:43  pls
 * 1.  Change ifdef's and version id.
 * 2.  Eliminate unnecessary message copying for
 * 	forwarded messages.
 * 
 * Revision 1.6  91/07/17  15:08:00  judy
 * New copyright notice.
 * 
 * Revision 1.5  91/07/09  13:36:49  steve
 * MicroTime support for Sun Version. Removed mistuff.
 * 
 * Revision 1.4  91/06/03  12:24:00  configtw
 * Tab conversion.
 * 
 * Revision 1.3  91/05/31  12:50:43  pls
 * Add MSG parameter to FindObject call.
 * 
 * Revision 1.2  90/08/27  10:40:11  configtw
 * Fix dynamic load management
 * 
 * Revision 1.1  90/08/07  15:38:12  configtw
 * Initial revision
 * 
*/
char deliver_id [] = "@(#)deliver.c     $Revision: 1.7 $\t$Date: 91/11/01 09:19:43 $\tTIMEWARP";


/*
Purpose:

		The main routine contained in this module is deliver(), and
		its purpose is to send a message on its way from the
		sending object to the receiver.  The low-level work of
		message delivery is done by Mercury and the hardware,
		so deliver() just takes care of high level Time Warp
		delivery functions.  Other than keeping statistics, this
		work mostly amounts to determining whether or not the
		message needs to be sent off-node or not.  If not, a
		local delivery routine is called.  If the message is
		to be sent to another node, the sndmsg() routine is
		called to pass it to the lower levels of the software.
		Suitable error checking is done throughout.

		The other routine in this module is sysmsg(), which 
		performs a function similar to deliver(), but for
		system messages.  System messages do not have specific
		objects as receivers, and are guaranteed to be going
		off-node.  The only complexity is whether the message
		is to be broadcast or not.

Functions:

		deliver(m) - figure out how to get a message to its
				destination object
				Parameters - Msgh *m
				Return - Always returns 0

		sysmsg(msgtype,on,len,node) - send a system message off-node
				Parameters - Int msgtype, Msgh *on, Int len, Int node
				Return - Always returns 0

Implementation:

		Determine if the message is being sent in forward or reverse
		direction.  If forward, look for its receiver in the world
		map; if reverse, look for its original sender.  In either
		case, use find_in_world_map() to find the destination.
		If a destination has been found, call smsg_stat() to
		keep some statistics, then decide whether the message 
		is being sent on- or off-node.  If it's on-node, this
		routine will take care of receiving it.  Call rmsg_stat()
		to keep some message receipt statistics.  Based on 
		whether the message was forward or reverse, put it into
		the receiving object's input or output queue, respectively,
		using the appropriate enqueueing routine.

		If the message is not destined for this node, call sndmsg()
		to get it off-node. 

		If the object that should be receiving the message cannot
		be found in the world map, try to return it to the sender.
		Look for the sender in the world map.  It is presumed to
		be local.  If it's not the currently executing ocb, 
		print an error message.  If it is, destroy the message
		and set an error condition in the executing object's
		ocb.

		sysmsg() is somewhat simpler.  It sets some bits in the
		message's header to indicate that it is a system message,
		fills in a couple of other fields in the header, then
		simply tries to send the system message out.  There are
		two possible cases.  If the message is to be broadcast,
		brdcst() is called.  If the message is point-to-point,
		sndmsg() is used.

*/
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"

extern	Msgh	*rm_msg;


#if TIMING
#define DELIVER_TIMING_MODE 8
#endif

FUNCTION        deliver (m)
	Msgh           *m;
{
	Objloc         *location;
	Ocb            *o;
	Int             reverse;
	Name *objname;
	VTime time;

  Debug

#if TIMING
	start_timing ( DELIVER_TIMING_MODE );
#endif

	reverse = isreverse_macro (m);

	if (reverse)
	{   /* going back to sender */
		objname = (Name *) m->snder;
		time = m->sndtim;
	}
	else
	{   /* get receiver info */
		objname = (Name *) m->rcver;
		time = m->rcvtim;
	}

/*  Find where the object is, if the local node knows.*/

	location = GetLocation(objname,time);


/* If there is no local information for this object, or if the object isn't
		on this node.  */

	if (location == (Objloc *) NULL || location->node != tw_node_num ||
		location->po == NULL)
	{
		if ( islocked_macro ( m ) )     /* from another node */
			{	/* m must be rm_msg */
			if ( rm_msg != (Msgh *) rm_buf)
				{	/* not pointing to our receive buffer, so don't m_create */
				unlock_macro(m);
				rm_msg = NULL;
				}
			else
				{	/* we can't hang onto rm_buf */
				m = (Msgh *) m_create ( msgdefsize, time, CRITICAL );

				acceptmsg ( m );    /* get & unlock message from rm_msg */
				}

			if (reverse)
			{   /* going back to sender */
				objname = (Name *) m->snder;
			}
			else
			{   /* get receiver info */
					objname = (Name *) m->rcver;
			}

			/* If this message was sent to this node from some other node,
				and the object that should get the message isn't on this node,
				then the sending node has an incorrect cache entry.  Before
				forwarding the message to the proper destination, send a
				cache invalidation request to the node that sent the message
				here.  Since this message could be either forward or reverse,
				make sure that the invalidation goes to the right place. */

#if PARANOID
			if (m->low.from_node == tw_node_num)
			{
				_pprintf("invalidating own cache, object %s, phase %f\n",
						objname,time);
				showmsg(m);
			}
#endif
			SendCacheInvalidate(objname,time,(int) m->low.from_node);
/*Here is where to increment the statistic for messages forwarded due to
		migration. */

			rfaults++;
		}

		if (location == (Objloc *) NULL || (location->node == tw_node_num &&
				location->po == NULL))
		{  /* node is unknown or it's this node & object is unknown */
			FindObject(objname,time,m,FinishDeliver,MSG);
		}
		else
			{  /* we know the node--send the message */
			sndmsg (m, m->txtlen + sizeof (Msgh), location->node);
			}
		}	/* if no local info on object */
	else  /* object is on this node */
	{
		o = location->po;
		rmsg_stat (o, m);

		/* for a normal message, put it in the object's input queue.
		if it's a reverse message, put it in the original sender's
		output queue. */

		reverse ?
				nq_output_message (o, m) :
				nq_input_message (o, m);
	}
#if TIMING
   stop_timing ();
#endif
}

/* call this routine when a pending action is concluded */
FUNCTION FinishDeliver(m,location)
	Msgh *m;
	Objloc *location;
{
	Debug

	if (location->node == tw_node_num)
	{  /* this node */
		rmsg_stat (location->po, m);
		if (isreverse_macro(m))
		{  /* reverse message:  put in object's output queue */
			nq_output_message(location->po, m) ;
		}
		else
		{  /* else input queue */
			nq_input_message (location->po, m);
		}
	}    
	else
	{   /* object is off-node-- send the message to it */
		sndmsg (m, m->txtlen + sizeof (Msgh), location->node);
	}
}

FUNCTION        sysmsg (msgtype, om, len, node)
	Int             msgtype;
	Msgh           *om;
	Int             len;
	Int             node;

/* send out a system message */

{
	extern int mlog, node_cputime;

  Debug

	if ( mlog )
	{
#if MICROTIME
		MicroTime ();
#else
#if MARK3
		mark3time ();
#endif
#if BBN  /*PJH Not sure about this one... */
		butterflytime ();
#endif
#endif
		om->cputime = node_cputime;
	}

	BITSET (om->flags, SYSMSG); /* mark as a system message */
	om->mtype = msgtype;
	om->txtlen = len;

	if (node == BCAST)
	{  /* it's a broadcast */
		brdcst (om, len + sizeof (Msgh));
	}
	else
	{  /* send it out */
		sndmsg (om, len + sizeof (Msgh), node);
	}
}
