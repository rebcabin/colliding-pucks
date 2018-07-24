/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	msgcntl.c,v $
 * Revision 1.12  91/11/04  10:16:19  pls
 * 1.  Remove IH messages from those which are ACK'd.
 * 2.  Allow Sun to print out RMQ.
 * 3.  Error if sending emergbuf on node.
 * 
 * Revision 1.11  91/11/01  09:47:19  reiher
 * debugging code, plus a bug fix (PLR)
 * 
 * Revision 1.10  91/07/22  13:17:13  configtw
 * Fix new bug in Mark3 code.
 * 
 * Revision 1.9  91/07/17  15:10:48  judy
 * New copyright notice.
 * 
 * Revision 1.8  91/07/11  08:54:57  steve
 * Allow sun version to send zero length messages.
 * 
 * Revision 1.7  91/07/09  14:31:18  steve
 * 1. Modified Sun version to support signal driven sockets.
 * 2. Removed mistuff, change d 128 to IH_NODE, other numbers to constants
 * 3. MicroTime support added.
 * 4. Modified Ack_msg a bit.
 * 
 * Revision 1.6  91/06/04  13:45:21  configtw
 * Call tester() if emergbuf goes to rmq.
 * 
 * Revision 1.5  91/06/03  12:25:36  configtw
 * Tab conversion.
 * 
 * Revision 1.4  91/05/31  14:45:30  pls
 * 1.  Add some PARANOID checks.
 * 2.  Add send_e_from_q() for emergbuf.
 * 
 * Revision 1.3  91/04/01  15:42:42  reiher
 * A change to make sure IH messages don't contribute to gvt calculation,
 * and a bug fix to look at the correct flag in a message.
 * 
 * Revision 1.2  90/12/10  10:42:13  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.1  90/08/07  15:40:25  configtw
 * Initial revision
 * 
*/
char msgcntl_id [] = "@(#)msgcntl.c     1.48\t10/2/89\t15:02:31\tTIMEWARP";


/*

Purpose:

		msgcntl.c contains the low-level message sending routines that get
		messages from the tester output queue to the underlying message
		delivery system.  Acks are also largely handled by routines in this
		module.

Functions:

		rcvmsg(msg,node) - check an incoming message's validity
				Parameters - Msgh * msg, Int node
				Return - Always returns zero

		print_acks() - print the contents of the acks table
				Parameters - none
				Return - Always returns zero

		rcvack(msg,node) - remove an ack from the acks table
				Parameters - Ack_msg  *msg, Int node
				Return - Always returns zero

		queack(msg,node) - list an ack as outstanding
				Parameters - Msgh * msg, Int node
				Return - Always returns zero

		sndack(msg,node) - send an ack
				Parameters - Msgh * msg, Int node
				Return - Always returns zero

		enq_msg(msg) - put an outgoing message in the queue for its
						destination node
				Parameters - Msgh * msg
				Return - Always returns zero

		print_queues() - print the queues of messages going out to other nodes
				Parameters - none
				Return - Always returns zero

		send_from_q() - send out any acks, broadcast messages, or normal 
						messages going to other nodes
				Parameters - none
				Return - Always returns zero

		log_ack_pending(msg,node) - make an entry for a message's outstanding
						ack in the acks table
				Parameters - Msgh * msg, Int node
				Return - zero, or crash

		destroy_msg(msg) - get rid of a message
				Parameters - Msgh *msg
				Return - Always returns zero

		checksum(tw_msg) - compute a checksum for a message
				Parameters - Msgh * tw_msg
				Return - the checksum

Implementation:

		rcvmsg() is  a checking function called by read_the_mail() and
		other routines when they are in PARANOID mode.  It checks the
		validity of certain message fields.

		print_acks() prints the table of outstanding acknowledgements.

		rcvack() removes an ack's entry from the table of outstanding
		acknowledgements.

		queack() puts an entry in the acknowledgement queue.

		sndack() calls send_message() to ship off an acknowledgement.
		The entry in the acknowledgement queue is made by log_ack_pending().

		enq_msg() puts an entry for an outgoing message into a node's 
		list in the pmq[] table.  System messages are put before the first
		non-system message, and regular messages are put in in send time 
		order, after all system messages.  

		print_queues() prints the queue kept for each node in the local
		pmq[] table.  

		send_from_q(), if we're not on a Mark3, first tries to use
		send_message(0 to send out any queued acks.  Whatever machine
		we're on, if there is a message to be broadcast, and this node
		is not the only node participating in the simulation, call 
		send_message() to send it out.  If BRDCST_ABLE is set, indicating
		a low level broadcast ability, just send it out once.  If there is
		no low level broadcast ability, loop, calling send_message() for
		each node.  After broadcast messages are taken care of, call
		send_stdout_msg(), if necessary.  Then run through all entries in
		the pmq[] array.  For each, look at each message in the queue of
		messages to be sent to the entry's node.  If there aren't already
		too many acks outstanding, call send_message() to send it, and
		go on to the next message.  Remove and destroy any message that
		got sent.

		log_ack_pending() finds an unused entry in the ack[] array, and
		fills it with an entry for the message in question.

		destroy_msg() gets rid of a message's storage by calling l_destroy().

		checksum() computes a checksum for a message.

*/


#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

#if PARANOID
VTime MinLvt;
#endif

#ifdef MARK3
extern Msgh * inuse_messages;
#endif

extern char brdcst_flag[];

#ifndef BBN
extern int max_acks;
extern int max_neg_acks;
#endif

FUNCTION rcvmsg ( msg, node )

	Msgh * msg;
	Int node;
{
  Debug

	if ( msg->low.from_node != node
	|| ( msg->low.to_node != tw_node_num && msg->low.to_node != IH_NODE && msg->low.to_node != 0 )
	||   msg->low.type == ACK_MSG )

/*	Note:  this may produce error messages on the Mark3.  The
	low.type field is overwritten by the Mercury send_msg routine which
	stores IN_USE and DONE information there.  See the call to
	send_message_fast() in send_from_q().
*/
	{
		_pprintf ( "Bad Msg from Node %d ", node );
		_pprintf ( "%.8x %.8x\n", *((int *)msg), *(((int*)msg)+1) );
		showmsg(msg);
		tester();
#ifdef SUN
		tester ();
		dump_socket ( node );
		tester ();
#endif
	}
}

print_acks ()
{
#ifdef BBN
	_pprintf ( "messages_to_send = %d\n", messages_to_send );
#else
	int i;

	_pprintf ( "acks_pending = %d, messages_to_send = %d\n",
				acks_pending, messages_to_send );

	for ( i = 0; i < MAX_ACKS; i++ )
		if ( ack[i].busy )
			_pprintf ( "node %d num %d time %f\n",
				ack[i].node, ack[i].num, ack[i].time.simtime );
#endif
}

#ifndef BBN
FUNCTION rcvack ( msg, node )

	Ack_msg * msg;
	Int node;
{
	register int i;

  Debug

	rcvack_cnt++;

	if ( msg->low.to_node == IH_NODE )
		msg->low.to_node = 0;

#ifdef PARANOID
	if ( msg->low.from_node != tw_node_num
	||   msg->low.to_node != node )
	{
		_pprintf ( "Bad Ack from Node %d ", node );
		_pprintf ( "%.8x %.8x\n", *((int *)msg), *(((int*)msg)+1) );

#ifdef SUN
		tester ();
		dump_socket ( node );
		tester ();
#endif
	}
#endif

	for ( i = 0; i < MAX_ACKS; i++ )
	{
		if ( ack[i].busy
		&&   ack[i].node == node
		&&   ack[i].num  == msg->num )
			break;
	}

	if ( i < MAX_ACKS )
	{
		ack[i].busy = FALSE;
		acks_pending--;
	}
	else
	{
		_pprintf ( "ack not found %d %d\n", node, msg->num );
		print_acks ();
		tester ();
	}
}


FUNCTION queack ( msg )

	Msgh * msg;
{
  Debug

	if ( acks_queued < MAX_QACKS )
	{
		acks_queued++;

		qack[ack_q_tail++] = * (Ack_msg *) msg;

		if ( ack_q_tail >= MAX_QACKS )
			ack_q_tail = 0;
	}
	else
	{
		_pprintf ( "Too many qacks\n" );
		tester ();
	}
}

FUNCTION sndack ( msg, node )

	Msgh * msg;
	Int node;
{
	int cc;
	Ack_msg m;

  Debug

	m.low.type = ACK_MSG;
	m.num = msg->gid.num;
	m.low.from_node = tw_node_num;
	m.low.to_node = node;
	m.low.length = sizeof(Ack_msg);

#ifdef MARK3
	cc = ack_q_tail;
	queack ( &m );
	send_message_fast ( &qack[cc], sizeof(Ack_msg), node, ACK_MSG, &qackst[cc]);
#else
	cc = send_message ( (char *) &m, sizeof(Ack_msg), node, ACK_MSG );
	if ( cc == -1 )
		queack ( &m );
	else if ( cc != sizeof(Ack_msg) )
		printf ( "sndack: cc = %d\n", cc );
#endif
}

#endif /* not BBN */

enq_msg ( msg )

	register Msgh * msg;
{
	register Msgh * next, * prev;

	if ( issys_macro ( msg ) )
	{  /* here if system message */
		for ( next = pmq, prev = (Msgh *) l_next_macro ( next );
								! l_ishead_macro ( prev );
			  next = prev, prev = (Msgh *) l_next_macro ( next ) )
		{  /* position to first non system message */
			if ( ! issys_macro ( prev ) )
				break;
		}
	}
	else
	if ( isanti_macro ( msg ) )
	{  /* it's a negative message */
		for ( next = pmq, prev = (Msgh *) l_next_macro ( next );
								! l_ishead_macro ( prev );
			  next = prev, prev = (Msgh *) l_next_macro ( next ) )
		{
			if ( issys_macro ( prev ) )
				continue;       /* bypass system messages */

			if ( isposi_macro ( prev ) )
				break;          /* stop at first regular message */

			if ( ltVTime ( msg->sndtim, prev->sndtim ) )
				break;          /* or when past the q's send time */
		}
	}
	else
	{  /* regular message--go through queue backwards */
		for ( next = (Msgh *) l_prev_macro ( pmq );
				   ! l_ishead_macro ( next );
			  next = (Msgh *) l_prev_macro ( next ) )
		{
			if ( issys_macro ( next ) )
				break;  /* stop at first system message */

			if ( isanti_macro ( next ) )
				break;  /* or first negative message */

			if ( geVTime ( msg->sndtim, next->sndtim ) )
				break;  /* or when past the q's send time */
		}
	}

	l_insert ( next, msg );     /* put this message in the queue */

	messages_to_send++;         /* and increment count */
}

#ifdef DLM
#if BBN || SUN
extern Msgh * rmq;
#endif
#endif DLM

print_queues ()
{
	Msgh * msg;

	dprintf ( "PMQ\n" );

	showmsg_head ();

    if ( ! l_ishead_macro ( pmq ) )
		_pprintf("pmq header %x has been corrupted\n", pmq );
	else
	{
		for ( msg = (Msgh *) l_next_macro ( pmq );
			  ! l_ishead_macro ( msg );
		  		msg = (Msgh *) l_next_macro ( msg ) )
		{
			showmsg ( msg );
		}
	}

#ifdef DLM
#if BBN || SUN
	dprintf ( "RMQ\n" );

	showmsg_head ();

	if ( ! l_ishead_macro ( rmq ) )
		_pprintf("rmq header %x has been corrupted\n", rmq );
	else
	{
		for ( msg = (Msgh *) l_next_macro ( rmq );
			  ! l_ishead_macro ( msg );
		  		msg = (Msgh *) l_next_macro ( msg ) )
		{
			showmsg ( msg );
		}
	}
#endif
#endif DLM

#ifdef MARK3_OR_BBN
	dprintf ( "\n" );
	dprintf ( "STDOUT Q\n" );

	showmsg_head ();

	if ( ! l_ishead_macro ( stdout_q ) )
		_pprintf ( "stdout_q header %x has been corrupted\n", stdout_q );
	else
	{
		for ( msg = (Msgh *) l_next_macro ( stdout_q );
			  ! l_ishead_macro ( msg );
		  		msg = (Msgh *) l_next_macro ( msg ) )
		{
			showmsg ( msg );
		}
	}
#endif
}

FUNCTION send_e_from_q ()
/* send emergbuf out if it's in the pmq */
{
	Msgh        *msg;
	int         len,node,res;

#ifndef MARK3
#ifdef PARANOID
	if  ( ! l_ishead_macro ( pmq ) )
	{	
		twerror ( "pmq header %x is corrupted\n", pmq );
		tester();
	}
#endif PARANOID
	for (msg = (Msgh *) l_next_macro(pmq);
		!l_ishead_macro(msg); msg = (Msgh *) l_next_macro(msg))
		{       /* loop through pmq looking for emergbuf */
		if (msg == emergbuf)
			{   /* found emergbuf here */
#ifdef BBN
			if ( msg->low.to_node == CP )
				node = 0;    /* CP is node 0 */
			else
				node = msg->low.to_node & 127;
#else
			node = msg->low.to_node & 127;
#endif
			len = msg->txtlen + sizeof ( Msgh );
			if (node != tw_node_num)
				{       /* only handle this case for now */
				/* send out the message */
				res = send_message ( msg, len, node, NORMAL_MSG );

				if (res == -1)
					break;      /* send buffs may be full */

				if ( res != len )
					{   /* only partial send */
					_pprintf ( "sysbuf: res %d != len %d\n", res, len );
					tester ();
					}
				l_remove(msg);
				destroy_msg(msg);
				messages_to_send--;
				}       /* if (node != tw_node_num) */
			else
				{	/* sending emergbuf on node */
				_pprintf("send_e_from_q: on node send %x\n",msg);
				tester();
				}
			break;      /* stop looking in pmq */
			}   /* if (msg == emergbuf) */
		}       /* end of pmq--hope something happened */
#endif
}  /* send_e_from_q */

FUNCTION send_from_q ()
{
	register Msgh * msg, * next_msg;
	register int len, node, res;
	int not_done_yet;
	extern int mlog;

  Debug

#ifdef MARK3
	while ( acks_queued )
	{
		if ( qackst[ack_q_head] != DONE )
			break;

		acks_queued--;

		ack_q_head++;
		if ( ack_q_head >= MAX_QACKS )
			ack_q_head = 0;
	}
#else SUN_OR_TRANSPUTER
#ifndef BBN
	while ( acks_queued )
	{
		acks_queued--;
/* it is already reversed now
		node = qack[ack_q_head].from_node;
*/
		node = qack[ack_q_head].low.to_node;
		res = send_message ( (char *) &qack[ack_q_head], sizeof(Ack_msg),
								node, ACK_MSG );
		if ( res == -1 )
		{
			acks_queued++;
			return;
		}
		else
		{
			ack_q_head++;
			if ( ack_q_head >= MAX_QACKS )
				ack_q_head = 0;

			if ( res != (sizeof(Ack_msg)) )
			{
				_pprintf ( "send_from_q: %d = res != (sizeof(Ack_msg))\n",
					res );
				tester ();
			}
		}
	}
#endif
#endif

	if ( brdcst_msg != NULL )
	{  /* handle pending broadcast first */
#ifdef BRDCST_ABLE
		if ( tw_num_nodes == 1 )
		{  /* only one node--don't bother */
			rm_msg = brdcst_msg;
			brdcst_msg = NULL;
			messages_to_send--;
		}
		else
		{
			len = brdcst_msg->txtlen + sizeof ( Msgh );

#ifdef MICROTIME
#ifndef BBN
			if ( mlog )
			{
				MicroTime ();
				msg->msgtimef = node_cputime;
			}
#endif
#else
#ifdef MARK3
			if ( mlog )
			{
				mark3time ();
				brdcst_msg->msgtimef = node_cputime;
			}
#endif
#endif
#ifdef MARK3
			if ( ! islocked_macro ( brdcst_msg ) )
			{
				send_message_fast ( brdcst_msg, len, ALL, NORMAL_MSG,
					 &brdcst_msg->low.type);
				l_insert ( l_prev_macro ( inuse_messages ), brdcst_msg );
				entcpy ( brdcst_buf, brdcst_msg, len );
				lock_macro ( brdcst_buf );
				rm_msg = brdcst_buf;
			}
			else
			{
				send_message ( brdcst_msg, len, ALL, NORMAL_MSG );
				rm_msg = brdcst_msg;
			}
			brdcst_msg = NULL;
			messages_to_send--;
#else
			res = send_message ( brdcst_msg, len, ALL, NORMAL_MSG );     /* send it */

			if ( res != -1 )
			{   /* success */
				rm_msg = brdcst_msg;
				brdcst_msg = NULL;
				messages_to_send--;
			}
#endif
		}
#else   /* not BRDCST_ABLE */
		not_done_yet = FALSE;

		len = brdcst_msg->txtlen + sizeof ( Msgh );

		for ( node = 0; node < tw_num_nodes; node++ )
		{
			if ( brdcst_flag[node] != 0 )
			{
				brdcst_msg->low.to_node = node;

				res = send_message ( brdcst_msg, len, node, NORMAL_MSG );

				if ( res == -1 )
					not_done_yet = TRUE;
				else
				{
					brdcst_flag[node] = 0;

					if ( res != len )
					{
						_pprintf ( "send_from_q: res %d != len %d\n", res, len );
						tester ();
					}
				}
			}
		}
		if ( not_done_yet == FALSE
		&&   rm_msg == NULL )
		{
			rm_msg = brdcst_msg;
			brdcst_msg = NULL;
			messages_to_send--;
		}
#endif
	}  /* if brdcst_msg != NULL */

#ifdef MARK3_OR_BBN
	if ( ! no_stdout )
		send_stdout_msg ();     /* handle stdout msg queue */
#endif

	if ( brdcst_msg != NULL )
		return;

	/* here if no more pending broadcasts */
	for ( msg = (Msgh *) l_next_macro ( pmq );
			  ! l_ishead_macro ( msg );
		  msg = next_msg )
	{
		next_msg = (Msgh *) l_next_macro ( msg );

		len = msg->txtlen + sizeof ( Msgh );
#ifdef BBN
		if ( msg->low.to_node == CP )
		   node = 0;    /* CP is node 0 */
		else
		   node = msg->low.to_node & 127;
#else
		node = msg->low.to_node & 127;
#endif

		if ( node == tw_node_num )
		{  /* message to this node */
			if ( rm_msg != NULL )
				continue;  /* skip this msg if rm_msg full */

			if (msg == emergbuf)
				{  /* uh-oh, emergbuf will end up in rmq */
				_pprintf("Emergbuf going to rmq %x\n",emergbuf);
				tester();
				}
			else
				rm_msg = msg;       /* else short circuit the send process */

#if 0
			if ( ! ( msg->flags & ( SYSMSG | MOVING ) ) &&
				gtVTime ( min_msg_time, msg->sndtim ))
				min_msg_time = msg->sndtim;     /* update min_msg_time */
#endif

			l_remove ((List_hdr *)  msg );
			messages_to_send--;
		}
		else
		{  /* send to remote node */
#ifndef BBN
			if ( isanti_macro ( msg ) && acks_pending < max_neg_acks )
				;
			else
			if ( ! issys_macro ( msg )
			&&   acks_pending >= max_acks )
				break;
#endif
#ifdef MICROTIME
			if ( mlog )
			{
				MicroTime ();
				msg->msgtimef = node_cputime;
			}
#else
#ifdef MARK3
			if ( mlog )
			{
				mark3time ();
				msg->msgtimef = node_cputime;
			}
#endif
#endif
#ifdef MARK3
			if ( ! islocked_macro ( msg ) )
				send_message_fast ( msg, len, node, NORMAL_MSG, &msg->low.type);
			else
				send_message ( msg, len, node, NORMAL_MSG );
#else
			/* send out the message */
			res = send_message ( msg, len, node, NORMAL_MSG );

			if ( res == -1 )
			{
				break;  /* some kind of error */
			}

			if ( res != len )
			{   /* only partial send */
				_pprintf ( "send_from_q: res %d != len %d\n", res, len );
				tester ();
			}
#endif
			if ( !(msg->flags & (SYSMSG|MOVING) )
				&& (strcmp ( msg->rcver, "$IH" ) != 0) )
				log_ack_pending ( msg, node );  /* record an ack pending */

			l_remove ( msg );   /*remove msg from queue */

#ifdef MARK3
			if ( ! islocked_macro ( msg ) )
				l_insert ( l_prev_macro ( inuse_messages ), msg );
			else
#endif
			destroy_msg ( msg );        /* deallocate msg */
			messages_to_send--;
		}
	}   /* go through the whole queue */
}  /* send_from_q */

#ifdef MARK3
FUNCTION send_message_fast ( buf, len, dest, type, status )

	char * buf;
	int len, dest, type, *status;
{
	int iret;

#ifdef TIMING
	start_timing ( SYSTEM_TIMING_MODE );
#endif

	if ( dest != CP && dest != ALL )
		dest += node_offset;

	LEDS ( 2 );
	send.buf  = (int *)buf;
	send.mlen = len;
	send.dest = dest;
	send.type = type;
	iret = send_msg ( &send, status );
	if ( iret != OK )
	{
		_pprintf ( "ERROR: send_msg returned %d\n", iret );
		tester ();
	}

#ifdef TIMING
	stop_timing ();
#endif
	return ( iret );
}
#endif /* MARK3 */

/*The following comment makes lint shut up.  Please do not delete it. */
/*ARGSUSED*/
FUNCTION send_message ( buf, len, dest, type )

	char * buf;
	int len, dest, type;
{
	int iret;
#ifdef BBN
	MSG_STRUCT  message_struct;
#endif
#ifdef SUN
	MSG_STRUCT  message_struct;
#endif

#ifdef TIMING
if ( ltSTime ( gvt.simtime, posinfPlus1.simtime ) )
	start_timing ( SYSTEM_TIMING_MODE );
#endif

#ifdef TRANSPUTER

	xsend ( buf, len, dest, type );

	/*  
	send_from_q's ! BRDCST_ABLE part is patterned on the behavior of the
	Sun version of send_message.  To make the Transputer code compatible with
	that, iret will be set to the value that indicates success when run on the
	SUN
	*/

	iret = len; 

#endif  /* TRANSPUTER */

#ifdef MARK3

	if ( dest != CP && dest != ALL )
		dest += node_offset;

	LEDS ( 2 );
	send.buf  = (int *)buf;
	send.mlen = len;
	send.dest = dest;
	send.type = type;
	iret = send_msg_w ( &send );
	if ( iret != OK )
	{
		_pprintf ( "ERROR: send_msg_w returned %d\n", iret );
		tester ();
	}
#endif

#if BBN || SUN
	message_struct.buf = buf;
	message_struct.mlen = len;
	message_struct.dest = dest;
	message_struct.source = tw_node_num;
	message_struct.type = type;

	iret = send_msg ( &message_struct );        /* send it on out */
#endif

#ifdef TIMING
if ( ltSTime ( gvt.simtime, posinfPlus1.simtime ) )
	stop_timing ();
#endif

	return ( iret );
}

log_ack_pending ( msg, node )

	Msgh * msg;
	Int node;
{
#ifndef BBN
	register int i;

	for ( i = 0; i < MAX_ACKS; i++ )
		if ( ! ack[i].busy )
			break;

	if ( i == MAX_ACKS )
	{
		_pprintf ( "log_ack_pending: you blew it dummy!\n" );
		crash ();
	}

	/* save in ack pending array */
	ack[i].busy = TRUE;
	ack[i].node = node;
	ack[i].num  = msg->gid.num;
	ack[i].time = msg->sndtim;

	acks_pending++;
#endif

#ifdef PARANOID
	if (ltVTime(min_msg_time,gvt))
	{
		twerror("log_ack_pending: min_msg_time %f < gvt %f at start of routine\n",      
						min_msg_time.simtime,gvt.simtime);
		tester();
	}
#endif

   if ( gtVTime ( min_msg_time, msg->sndtim ) &&
	   strcmp ( msg->rcver, "$IH") != 0  )
		min_msg_time = msg->sndtim;     /* update min_msg_time */

#if PARANOID
	if (ltVTime(min_msg_time,MinLvt) &&
		ltVTime(MinLvt,posinfPlus1) &&
		gvt.simtime > 0.0 &&
		strcmp(msg->rcver,"$IH") != 0)
	{
		twerror("log_ack_pending: setting min_msg_time %f < MinLvt %f\n",
						min_msg_time.simtime,MinLvt.simtime);
		showmsg(msg);
		tester();
	}
#endif

	if (ltVTime(min_msg_time,gvt))
	{
		twerror("log_ack_pending: setting min_msg_time %f < gvt %f\n",
						min_msg_time.simtime,gvt.simtime);
		showmsg(msg);
		tester();
	}
}

destroy_msg ( msg )

	Msgh * msg;
{
#if PARANOID
	List_hdr    *l;
#endif
	if ( islocked_macro ( msg ) )
		{
#if PARANOID
		l = (List_hdr*)msg - 1;
		if ((l->next != l) || (l->prev !=l))
			{
			_pprintf("destroy_msg: locked but not released %x\n",msg);
			tester();
			}
#endif
		unlock_macro ( msg );
		}
	else
		l_destroy ( msg );
}

#ifdef CHECKSUM
int checksum ( tw_msg )

	Msgh * tw_msg;
{
	register Byte * pointer;
	register int counter;
	register int sum;

	pointer = ( (char *)&tw_msg->checksum ) + sizeof ( tw_msg->checksum );
	counter = sizeof ( Msgh ) - ( pointer - (char *)tw_msg ) + tw_msg->txtlen;
	sum = 0;

	while ( counter-- )
	{
		sum += *pointer++;
	}

	return ( sum );
}
#endif
