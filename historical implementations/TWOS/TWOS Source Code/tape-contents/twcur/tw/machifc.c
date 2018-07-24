/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	machifc.c,v $
 * Revision 1.8  91/11/01  09:41:12  pls
 * 1.  Change ifdef's, version id.
 * 2.  denymsg() fix to not lose memory on Sun's.
 * 3.  Eliminate error in sndmsg() for node 0 to node 0 message.
 * 
 * Revision 1.7  91/07/17  15:09:41  judy
 * New copyright notice.
 * 
 * Revision 1.6  91/07/09  14:22:19  steve
 * Added support for Sun receive queue in acceptmsg and r_min. Removed 
 * mistuff. Replaced 128 with IH_NODE.
 * 
 * Revision 1.5  91/06/03  12:24:48  configtw
 * Tab conversion.
 *
 * Revision 1.4  91/05/31  13:29:51  pls
 * 1.  Check for rm_msg NULL in peekmsg() (bug 7).
 * 2.  Add PARANOID code to validate state in setctx().
 *
 * Revision 1.3  91/04/01  15:39:40  reiher
 * A bug fix to make sure IH messages aren't considered in GVT calculation,
 * plus a number of tests to catch gvt problems.
 * 
 * Revision 1.2  90/11/27  09:41:28  csupport
 * comment out GVT repetition warning messages
 * 
 * Revision 1.1  90/08/07  15:40:06  configtw
 * Initial revision
 * 
*/
char machifc_id [] = "@(#)machifc.c     $Revision: 1.8 $\t$Date: 91/11/01 09:41:12 $\tTIMEWARP";


/* 

Purpose:

		machifc.c contains code that is the machine interface for Time
		Warp.  Most of the code deals with low level message sending
		activities.  Some of it concerns context switching.

Functions:

		timrint(gvttimrproc) - arrange for a procedure to be called when the
						timer expires
				Parameters -  int (*gvttimrproc) ()
				Return - Always returns SUCCESS

		peekmsg() - look at the message in rm_msg
				Parameters - none
				Return - a pointer to the message

		acceptmsg(tw_msg) - put tw_msg in the rm_buf buffer
				Parameters - Msgh * tw_msg
				Return - Always returns zero

		denymsg(tw_msg) - return a message to its sender
				Parameters - Msgh * tw_msg
				Return - returns zero, or exits

		sndmsg(tw_msg,length,node) - put a message into an output queue
				Parameters -  Msgh * tw_msg, Int length, Int node
				Return - SUCCESS or crashes

		brdcst(tw_msg,length) - send a broadcast message, using the broadcast
						capability, if present; send iteratively to all
						nodes, if not
				Parameters - Msgh * tw_msg, Int length
				Return - SUCCESS or crashes

		setctx(state,entry_point,stack) - prepare for a context switch
				Parameters - State * state, Byte * entry_point, Byte * stack
				Return - Always returns zero

		setnull() - clear an object context
				Parameters - none
				Return - Always returns zero

Implementation:

		timrint() sets a function pointer variable to some value so 
		that some particular function can be called when the main()
		loop has determined that the timer counter has reached a certain
		value. 

		peekmsg() locks rm_msg and returns a pointer to it.

		acceptmsg() puts its message into rm_buf, and unlocks rm_msg.

		denymsg() reverses the sender and receiver of rm_msg, and calls
		send_message() to return it to its sender.  

		logmsg() finds the message in the ack[] array with the minimum
		send time.  If that time is less than min_msg_time, min_msg_time
		is set to this lower value.  This routine is used in conjunction
		with gvt calculation.

		minmsg() looks through the pmq[] table's message queues, searching
		for the message with the min rcvtim.  This routine, also, is used
		in conjunction with gvt calculation.  minmsg() looks at the times
		of messages waiting to be sent, logmsg() looks at the times of 
		messages that have been sent, but not yet acked.

		sndmsg() fills in some fields on header of the message to be sent.
		It calculates the checksum(), if that's being done, and stores it
		in the header.  If we're sending an EMSG to node IH_NODE of a Mark3,
		call stdout_msg() for it.  Whether or not we're on a Mark3, if node 
		is IH_NODE and tw_node_num is 0, call ih_msgproc() and destroy_msg().
		Otherwise, call enq_msg() and send_from_q().  

		brdcst() first fills in some message fields.  If we don't have
		a low-level broadcasting ability, set the brdcst_flag[] entry
		for each node to 1.  Call send_from_q().  

		setctx() prepares for a switch of context by setting certain
		pointers, such as object_data.

		setnull() nulls the object_context pointer.
*/


#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

extern Byte * object_context;
extern Byte * object_data;
extern char brdcst_flag[];

FUNCTION timrint ( gvttimrproc )

	int ( * gvttimrproc ) ();
{
  Debug

	timrproc = gvttimrproc;     /* routine to initiate */

	timeon ();                  /* set the interrupt timer */

	return ( SUCCESS );
}

#if DLM

FUNCTION timlint ( loadtimrproc )

	int ( * loadtimrproc ) ();
{
  Debug


	timlproc = loadtimrproc;

	timelon ();

	return ( SUCCESS );
}
#endif DLM


Msgh *
FUNCTION peekmsg ()
{
  Debug

	if (rm_msg != NULLMSGH)
		lock_macro ( rm_msg );

	return ( rm_msg );
}

FUNCTION acceptmsg ( tw_msg )

	Msgh * tw_msg;
{
  Debug

	if ( tw_msg != NULL )
	{  /* copy to tw_msg */
		entcpy ( tw_msg, rm_msg, rm_msg->txtlen + sizeof (Msgh) );

		unlock_macro ( tw_msg );        /* unlock it */
	}

#if MARK3
	if ( rm_msg == (Msgh *) recv.buf )
	{
		give_buf ( &recv );
	}
#endif

#if BBN
	if ( rm_msg != (Msgh *) rm_buf  && rm_msg !=NULL )
		l_destroy ( rm_msg );   /* get rid of message storage */
#endif

#if SUN
	if ( rm_msg != (Msgh *) rm_buf  && rm_msg !=NULL )
		l_destroy ( rm_msg );   /* get rid of message storage */
#endif

	rm_msg = NULL;
}

/*The following comment makes lint shut up.  Please do not delete it. */
/*ARGSUSED*/
FUNCTION denymsg ( tw_msg )

	Msgh * tw_msg;
{
	int node, len, res, i;
	Msgh * msg;

  Debug

	if ( no_message_sendback )
	{
		_pprintf ( "Denymsg Called\n" );
		tw_exit ( 0 );
	}

#ifndef BBN
	if ( acks_pending == MAX_ACKS )
	{
		_pprintf ( "Denymsg & MAX_ACKS: %x\n", rm_msg );
		tester ();
	}
#endif

	msg = rm_msg;

	node = msg->low.from_node;     /* from node */

	msg->low.from_node = tw_node_num;      /* from node */
	msg->low.to_node = node;     /* to node */
/*
	msg->low.type = NORMAL_MSG;        * Ack flag *
*/

	len = msg->txtlen + sizeof ( Msgh );

	unlock_macro ( msg );

#if MARK3
	send_message ( msg, len, node, NORMAL_MSG );
#else
	for ( ;; )
	{
		for ( i = 0; i < 1000; i++ )
		{
			res = send_message ( msg, len, node, NORMAL_MSG );

			if ( res != -1 )
				break;
		}
		if ( i < 1000 )
			break;

		_pprintf ( "Denymsg stuck writing to Node %d\n", node );
	}
#endif

	if ( ! ( msg->flags & MOVING ) )
		log_ack_pending ( msg, node );

#if MARK3
	if ( rm_msg == (Msgh *) recv.buf )
	{
		give_buf ( &recv );
	}
#endif

#if BBN || SUN
	if ( rm_msg != (Msgh *) rm_buf )
		l_destroy ( rm_msg );
#endif

	rm_msg = NULL;
}

FUNCTION logmsg ()
{
#if BBN
  Debug

	/* find minimum of messages sent but not yet ack'd */
	min_msg_time = butterfly_min ();
if ( ltVTime ( min_msg_time, gvt ) )
{
_pprintf("min_msg_time %f below gvt %f in logmsg\n",min_msg_time.simtime,
	  gvt.simtime);
tester();
}
#else
	register int i, n;

  Debug

	min_msg_time = posinfPlus1;

	for ( i = 0, n = acks_pending; n > 0; i++ )
	{
		if ( ack[i].busy )
		{
			n--;

			if ( ltVTime ( ack[i].time, min_msg_time ) )
			{
				min_msg_time = ack[i].time;
			}
		}
	}
if ( ltVTime ( min_msg_time, gvt ) )
{
_pprintf("this code should be ifdefed out!; min_msg_time %f set below gvt %f\n",
			  min_msg_time.simtime, gvt.simtime );
tester();
}
#endif
}

#if DLM
extern VTime oldgvt1 ;
extern VTime oldgvt2 ;
#endif DLM

/* calculate minimum time based on migrating objects, min_msg_time,
   pmq and rmq */


FUNCTION minmsg ( t )

	VTime * t;
{
	register Msgh * msg;
	VTime min;
#if BBN
	VTime r_min();
#endif
#if SUN
	VTime r_min();
#endif
#if DLM
	int gvtMatch;
#endif DLM

	min = migr_min ();  /* calculate minimum time for migrating objects */

if ( ltVTime ( min, gvt ) )
{
_pprintf("in minmsg, min %f less than gvt %f after migr_min\n",
	  min.simtime, gvt.simtime);
tester();
}
#if DLM
	gvtMatch = 0;
	if ( oldgvt2.simtime != NEGINF  && 
		eqVTime ( min, oldgvt1 ) && 
		eqVTime ( oldgvt1, oldgvt2 ) )
	{
		gvtMatch = 1;
/*
		_pprintf ( "GVT %f repeats three times, after migr_min() \n",
						min.simtime);
*/
	}
#endif DLM

	if ( gtVTime ( min, min_msg_time ) )
		min = min_msg_time;     /* minimize on min_msg_time */

if ( ltVTime ( min, gvt ) )
{
_pprintf("in minmsg, min %f less than gvt %f after checking min_msg_time\n",
	  min.simtime, gvt.simtime);
tester();
}
#if DLM
	if ( !gvtMatch && oldgvt2.simtime != NEGINF  && 
		eqVTime ( min, oldgvt1 ) &&
		eqVTime ( oldgvt1, oldgvt2 ) )
	{
		gvtMatch = 1;
/*
		_pprintf ( "GVT %f repeats three times, after min_msg_time check \n",
						min.simtime);
*/
	}
#endif DLM

	for ( msg = (Msgh *) l_next_macro ( pmq );
			  ! l_ishead_macro ( msg );
		  msg = (Msgh *) l_next_macro ( msg ) )
	{  /* cycle through node's message output queue */
		if ( issys_macro ( msg ) )
			continue;   /* skip system messages */
		if ( strcmp ( msg->rcver, "$IH") == 0 )
			continue;  /* skip msgs to the IH */
		if ( ! ( msg->flags & MOVING ) )
		{  /* object is not in process of moving */
			if ( gtVTime ( min, msg->sndtim ) )
				min = msg->sndtim;      /* minimize on sndtim */

			if ( isposi_macro ( msg ) )
				break;  /* break on positive message */
		}
	}

#if DLM
if ( ltVTime ( min, gvt ) )
{
_pprintf("in minmsg, min %f less than gvt %f after checking output queue\n",
	  min.simtime, gvt.simtime);
tester();
}
	if ( !gvtMatch && oldgvt2.simtime != NEGINF  && 
		eqVTime ( min, oldgvt1 ) && 
		eqVTime ( oldgvt1, oldgvt2 ) )
	{
		gvtMatch = 1;
/*
		_pprintf ( "GVT %f repeats three times, after pmq check \n",
						min.simtime);
*/
	}
#endif DLM
#if BBN
	min = r_min ( min );        /* minimize over node's input msg queue */
   if ( ltVTime ( min, gvt ) )
{
_pprintf("in minmsg, min %f less than gvt %f after r_min\n",
	  min.simtime, gvt.simtime);
tester();
}

#endif
#if SUN
	min = r_min ( min );        /* minimize over node's input msg queue */
   if ( ltVTime ( min, gvt ) )
{
_pprintf("in minmsg, min %f less than gvt %f after r_min\n",
	  min.simtime, gvt.simtime);
tester();
}

#endif

#if DLM
	if ( !gvtMatch && oldgvt2.simtime != NEGINF  && 
		eqVTime ( min, oldgvt1 ) && 
		eqVTime ( oldgvt1, oldgvt2 ) )
	{
/*
		_pprintf ( "GVT %f repeats three times, after r_min check \n",
						min.simtime);
*/
	}
#endif DLM
	*t = min;   /* pass back the minimum time */

  Debug
}


FUNCTION sndmsg ( tw_msg, length, node )

	Msgh * tw_msg;
	Int length;
	Int node;
{
	register LowLevelMsgH * ll_msg = (LowLevelMsgH *) tw_msg;
  Debug

#if TIMING
	start_timing ( TESTER_TIMING_MODE );
#endif
	ll_msg->from_node = tw_node_num;   /* from node */
	ll_msg->to_node = node;          /* to node */
	ll_msg->type = NORMAL_MSG;             /* ack flag */
	ll_msg->length = length;

	tw_msg->txtlen = length - sizeof ( Msgh );
/*
	tw_msg->low.from_node = tw_node_num;  
	tw_msg->low.to_node = node;        
	tw_msg->low.type = NORMAL_MSG;          

	tw_msg->txtlen = length - sizeof ( Msgh );
*/
#if CHECKSUM
	tw_msg->checksum = checksum ( tw_msg );
#endif

	if ( node == IH_NODE )
	{
#if MARK3 || BBN
		if ( tw_msg->mtype == EMSG )
		{  /* event message */
			stdout_msg ( tw_msg );      /* send to stdout */

			goto sndmsg_end;            /* and finish up */
		}
		else                            /* not event message */
		if ( tw_msg->mtype == SIM_END_MSG
		&& ( ! no_stdout ) )
		{
			save_sim_end_msg = tw_msg;  /* the simulation is done */
			messages_to_send++;
			goto sndmsg_end;
		}
#endif
		if ( tw_node_num == 0 )
		{  /* if we're on node 0 */
			if ( rm_msg != NULL )
			{
				r_enq(tw_msg);
				_pprintf ( "rm_msg not Null!\n" );
				goto sndmsg_end;
			}

			rm_msg = tw_msg;

			ih_msgproc ( tw_msg );      /* handle the message */

			goto sndmsg_end;            /* finish up */
		}
		else
		{  /* we're not on node 0 */
			node = 0;           /* send IH messages to node 0 */
			/* ???must change it below too */
			/* it is a mistake to change this in the message
			since the main loop uses this to check for an ih_msgproc
			bound message */
		}
	}

	enq_msg ( tw_msg );         /* put msg in output queue */

	send_from_q ();             /* send out messages in queue */

sndmsg_end:

#if TIMING
	stop_timing ();
#endif

	return ( SUCCESS );
}

FUNCTION brdcst ( tw_msg, length )

	register Msgh * tw_msg;
	register Int length;
{
	register int node;

  Debug

#if TIMING
	start_timing ( TESTER_TIMING_MODE );
#endif

	tw_msg->low.from_node = tw_node_num;
	tw_msg->low.to_node = ALL;
	tw_msg->low.type = NORMAL_MSG;
	tw_msg->low.length = length;

	tw_msg->txtlen = length - sizeof ( Msgh );

#if CHECKSUM
	tw_msg->checksum = checksum ( tw_msg );
#endif

	if ( brdcst_msg != NULL )
	{
		_pprintf ( "brdcst_msg overwrite\n" );
		crash ();
	}

	brdcst_msg = tw_msg;        /* point to the message */

#ifndef BRDCST_ABLE
	for ( node = 0; node < tw_num_nodes; node++ )
	{
		if ( node != tw_node_num )
			brdcst_flag[node] = 1;
	}
#endif

	messages_to_send++;

	send_from_q ();     /* this does the send */

#if TIMING
	stop_timing ();
#endif

	return ( SUCCESS );
}

FUNCTION setctx ( state, entry_point, stack )

	State * state;
	Byte * entry_point;
	Byte * stack;
{
  Debug

#if PARANOID
	validState(state - 1);
#endif
	object_data    = (Byte *)state;
	object_context = stack + objstksize;

	if ( entry_point != NULL )
		* ( (Byte **) (object_context-4) ) = entry_point;
}

FUNCTION setnull ()
{
  Debug

	object_context = NULL;
}
