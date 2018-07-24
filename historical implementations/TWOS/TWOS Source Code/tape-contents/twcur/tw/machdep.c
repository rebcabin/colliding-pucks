/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	machdep.c,v $
 * Revision 1.7  91/12/27  09:07:28  pls
 * Fix up TIMING code.
 * 
 * Revision 1.6  91/11/06  11:11:08  configtw
 * Do casting.
 * 
 * Revision 1.5  91/07/17  15:09:28  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/07/09  14:04:24  steve
 * 1. Added signal driven socket support.
 * 2. combined the Sun and BBN read_the_mail routines.
 * 3. removed mistuff and changed 128 to IH_NODE
 * 
 * Revision 1.3  91/06/03  12:24:42  configtw
 * Tab conversion.
 *
 * Revision 1.2  91/04/01  15:39:15  reiher
 * Bug fix to not consider IH messages in gvt calculation.
 * 
 * Revision 1.1  90/08/07  15:40:01  configtw
 * Initial revision
 * 
*/
char machdep_id [] = "@(#)machdep.c     1.58\t10/2/89\t14:45:42\tTIMEWARP";


/*

Purpose:

		machdep.c is meant to contain as much of Time Warp's machine
		dependent code as possible.  Some machine dependent code must
		be kept elsewhere, because it is intimately related to higher
		level functions.  The smaller pieces of code that have little
		to do with anything else are kept here.

		machdep.c is a temporary arrangement. In the future, we expect to
		replace it with a separate file for each type of machine supported.
		For instance, there would be a sun.c, a mark3.c, and a butterfly.c.
		These modules would contain only the code needed for that particular
		machine.

Functions:

		check_mercury_queue() -  look for a message in the mercury queue
						more important than what we're doing
				Parameters - none
				Return - TRUE or FALSE

		ioint(ptr1,ptr2) - check whether we want to switch context on
						an interrupt
				Parameters - MSG_STR_PTR ptr1, int * ptr2
				Return - 0 if the message is to be processed, 1 if the
						message has been cleared
				[Sun version has no parameters]

		read_the_mail() - examine incoming messages
				Parameters - none
				Return - Always returns zero

		print_acks() - print the contents of the acks table
				Parameters - none
				Return - Always returns zero

		ioint_rcvack(msg,node) - remove an ack from the acks table because of
						an interrupt
				Parameters - Ack_msg  *msg, Int node
				Return - 0 if failure, 1 if successful

		_pprintf(form,arg1,arg2, . . . , arg15) - printf to stdout with node 
						number added to the front
				Parameters -  char * form, int * arg1, int * arg2, . . . ,
								int * arg15
				Return - Always returns zero

		check_for_keyin() - on a Sun, check if there's any keyboard input
				Parameters - none
				Return - Always returns zero

Implementation:

		check_mercury_queue() calls cnt_msgs() to determine how many
		messages are queued.  For each, call peek_msg().  If the message
		came from the CP, or was an ack, or it was a system message, or
		its receive time is not greater than the simulation time of the
		currently executing object go to the end of the routine.  Otherwise,
		look at the next message.  If the end of the routine is reached
		by normal loop termination, we didn't find an earlier message, 
		and FALSE is returned.  If one of the earlier mentioned tests
		succeeded, we found a message that needs immediate processing,
		so we return TRUE.  (Essentially, this routine checks to see
		if an incoming message is of sufficient importance to interrupt
		the executing object.)

		ioint() deals with interrupts.  If the type of the interrupt is
		unknown, or is from the CP, just increment a counter.  If the
		interrupting message is an ack, call ioint_rcvack().  If it's
		any other type of message, check to see if it should preempt
		the object, calling switch_preempt() if it should.  ioint() is
		called by the assembly language routine that performs switching
		functions.  switch_preempt() is also in that assembly language
		code.  If the object is preempted, the system will return to
		the main loop, where it will check for mercury messages and
		process the interrupting one.  If we're not on a Mark3, then
		all that ioint() does is call check_for_keyin().

		read_the_mail() looks at incoming messages.  It calls get_msg()
		to retrieve one.  If it came from the CP, essentially throw it
		away, possibly after storing some information it contained.
		If it came from elsewhere, copy the message to rm_buf.  If it's
		an ack, call rcv_ack() and get another message.  Otherwise, 
		store a pointer to the message in rm_msg.  If the message didn't
		come from this node, call send_ack() to acknowledge it.  When the
		top of the main Time Warp loop is reached, the message just put
		into rm_msg will be handled fully.

		The Sun version is a little different, mostly in the way it
		gets the messages.  It reads them off a socket, rather than
		calling get_msg().  Otherwise, it is similar to the Mark3 version.

		dump_socket() is a Sun routine to print the contents of a socket
		when something has gone wrong.  It is diagnostic in purpose.

		ioint_rcvack() removes an ack's entry from the table of outstanding
		acknowledgements when an interrupt mandates it.

		_pprintf() is like printf(), but first prints the number of the
		node executing it, providing some idea of where the diagnostic
		message being printed came from.

		check_for_keyin() reads a control channel attached to the keyboard.
		It is only used on the Mark3.

*/


#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

extern int host_input_waiting;
extern char standalone;
extern char buff[], *bp;

#ifdef MARK3
extern int peek_limit;

int check_mercury_queue ()
{
	static int nmsg;
	register int iret, i, n;
	register Msgh * pbuf;

	cnt_msgs ( &nmsg );

	if ( nmsg )
	{
		n = nmsg; if ( n > peek_limit ) n = peek_limit;

		for ( i = 0; i < n; i++ )
		{
peek_again:
			recv.source = ANY;
			iret = peek_msg ( &recv );
			if ( iret == OK )
			{
				if ( recv.source == CP )
				{
					get_mercury_msg ();
					goto peek_again;
				}
				if ( recv.mlen == sizeof ( Ack_msg ) )
				{
					get_mercury_msg ();
					goto peek_again;
				}

				pbuf = (Msgh *) recv.buf;

				if ( issys_macro ( pbuf ) )
				{
					get_mercury_msg ();
					break;
				}

				if ( leVTime ( pbuf->rcvtim, xqting_ocb->svt ) )
				{
/*
					if ( ( xqting_ocb->svt.simtime - pbuf->rcvtim.simtime )
								>= 100 )
						_pprintf
						( "Msg for %s at %f beats Exec of %s at %f\n",
							pbuf->rcver, pbuf->rcvtim.simtime,
							xqting_ocb->name, xqting_ocb->svt.simtime );
*/
					get_mercury_msg ();
					break;
				}
			}
			else
				break;
		}
	}
}
#endif

#ifdef MARK3
int interrupt_disable;
extern int object_running;
extern int mlog;

#undef COMMAND
#include <sys/mercsys.h>
#include <sigmsg.h>
 
int ioint_spurious;
int ioint_multi_cnt;
 
update_arrival ( new_q )
 
	register QMSG_PTR new_q;
{
	register QMSG_PTR arrival_q;
 
	if ( ( arrival_q = lock_queue() ) != 0 )
	{
		addQ ( arrival_q, new_q );
		ARRIVAL_QUEUE = arrival_q;
	}
	else
		ARRIVAL_QUEUE = new_q;
}

ioint ()
{
	MSG_STR_PTR ptr1;
	Msgh * msg;
 
	register QMSG_PTR new_q, qptr, nptr;
 
	ioint_cnt++;
 
	if ( ( new_q = get_arrival() ) == 0 )
	{
		ioint_spurious++;
		return;         /* paranoid test for spurious interrupt */
	}
 
	for ( qptr = new_q, nptr = NULL; nptr != new_q ; qptr = nptr )
	{
		if ( new_q == NULL )
			new_q = qptr;
 
		nptr = (QMSG_PTR) qptr->next;
 
		ptr1 = &(qptr->msg);
 
		if ( ptr1->type < 0 )
		{
			ioint_merc_cnt++;
			continue;
		}
 
		if ( ptr1->source == CP )
		{
			ioint_cp_cnt++;
			continue;
		}
 
		if ( tw_node_num >= tw_num_nodes
		||  ptr1->source < node_offset || ptr1->source > node_limit )
		{
			if ( new_q == qptr )
			{
				if ( nptr == new_q )
					nptr = NULL;
				new_q = NULL;
			}
			give_buf ( &(qptr->msg) );
			qptr = delete_msg ( qptr );
			if ( qptr != NULL )
			{
				ioint_multi_cnt++;
			}
			continue;
		}    
 
		msg = (Msgh *)(ptr1->buf);
 
		if ( mlog )
		{
			if ( ptr1->mlen >= sizeof(Msgh) )
			{
				mark3time ();
				msg->msgtimet = node_cputime;
			}
		}   
 
		if ( object_running == 0 || interrupt_disable )
		{   
			continue;
		}   
 
		if ( ptr1->mlen == sizeof ( Ack_msg ) )
		{   
			register int ok;
			ioint_ack_cnt++;
			ioint_rcvack ( ptr1->buf, ptr1->source );
			if ( new_q == qptr )
			{
				if ( nptr == new_q )
					nptr = NULL;
				new_q = NULL;
			}
			give_buf ( &(qptr->msg) );
			qptr = delete_msg ( qptr );
			if ( qptr != NULL )
			{
				ioint_multi_cnt++;
			}
			continue;
		}
 
		if ( issys_macro ( msg ) )
		{
			ioint_twsys_cnt++;
			goto preempt_object;
		}
 
		ioint_msg_cnt++;
 
		if ( isreverse_macro ( msg ) )
		{
			if ( leVTime ( msg->sndtim, xqting_ocb->svt ) )
			{
				ioint_rcvtim_cnt++;
				goto preempt_object;
			}
		}
		else
		{
			if ( leVTime ( msg->rcvtim, xqting_ocb->svt ) )
			{
				ioint_rcvtim_cnt++;
				goto preempt_object;
			}
		}
 
preempt_object:
		;               /* cubix is no longer reentrant! */
#if 0 
		xqting_ocb->sb->cputime += mark3time ();

		object_end_time = node_cputime;
		xqting_ocb->stats.cputime += object_end_time - object_start_time;
		xqting_ocb->stats.cycletime += object_end_time - object_start_time;
		xqting_ocb->sb->effectWork += object_end_time - object_start_time;
 
		switch_preempt ();
 
#ifdef TIMING
		if ( object_running == 0 )
			stop_timing ();
#endif
#endif
	}
	 
	if ( new_q )
		update_arrival ( new_q );
}
#endif


#ifdef SUN

ioint ()
{
	extern int maybe_socket_io;

	maybe_socket_io = 1;
}
#endif


#if BBN || SUN
#if BBN
extern VTime stdout_ok_time;
#endif

extern Msgh * rmq;
int messages_received;
#if SUN
int recv_q_limit = 5;
#endif
#if BBN
int recv_q_limit = 10;
#endif

FUNCTION read_the_mail ( check_only )

	int check_only;
{
	int node;

	MSG_STRUCT  input_msg_struct;
	Msgh * q_msg, * r_deq();

	extern int mlog;

  Debug

#ifdef TIMING
	start_timing ( SYSTEM_TIMING_MODE );
#endif

get_again:

	input_msg_struct.buf = (int *)rm_buf;
	input_msg_struct.source = ANY;
	input_msg_struct.type = ANY;

	while (
#if SUN
		maybe_socket_io &&
#endif
		(messages_received < recv_q_limit) && get_msg ( & input_msg_struct ) )
	{  
		if ( input_msg_struct.source  == CP )
		{
#if BBN
			if ( input_msg_struct.mlen == 0 )
				stdout_acks_pending--;
			else
			if ( input_msg_struct.mlen == sizeof(stdout_ok_time) )
				stdout_ok_time = * (VTime *)input_msg_struct.buf ;
			else
#endif
				_pprintf ( "Rec'd Len %d from CP\n", input_msg_struct.mlen );
			goto  get_again;
		}

		if ( rm_msg != NULL )
			_pprintf ( "rm_msg overwrite\n" );

		rm_msg = (Msgh *)input_msg_struct.buf;

		if ( mlog )
		{
#ifdef MICROTIME
			MicroTime();
#else
			butterflytime ();
#endif
			rm_msg->msgtimet = node_cputime;
		}

#ifdef PARANOID
#if BBN
		node = input_msg_struct.source - node_offset;
#endif
#if SUN
		node = input_msg_struct.source;
#endif

		rcvmsg ( rm_msg, node );

		if (rm_msg->low.from_node != node)
		{
			_pprintf("Bad Message in read_the_mail, input_msg_struct addr %x\n",
				&input_msg_struct);
			showmsg(rm_msg);
			tester();
		}
#endif

#ifdef CHECKSUM
		if ( checksum ( rm_msg ) != (Msgh *)rm_msg->checksum )
		{
			_pprintf ( "Checksum Error\n" );
			showmsg ( rm_msg );
			tester ();
		}
#endif 
		q_msg = (Msgh *)l_create ( msgdefsize );

		if ( q_msg != NULL )
		{
			r_enq ( rm_buf );
			rm_buf = (char *)q_msg;
			input_msg_struct.buf = (int *)rm_buf;
			rm_msg = NULL;
		}
		else
		{
			break;
		}
	}

	if ( rm_msg == NULL )
	{
		if ( check_only )
		{
			rm_msg = (Msgh *)l_next_macro ( rmq );

			if ( l_ishead_macro ( rm_msg ) )
				rm_msg = NULL;
			else
			if ( issys_macro ( rm_msg ) )
				rm_msg = r_deq ();
			else
			if ( leVTime ( rm_msg->rcvtim, xqting_ocb->svt ) )
				rm_msg = r_deq ();
			else
				rm_msg = NULL;
		}
		else
			rm_msg = r_deq ();
	}
#ifdef TIMING
	stop_timing ();
#endif
}

r_enq ( msg )

	Msgh * msg;
{
	register Msgh * next, * prev;

	if ( issys_macro ( msg ) )
	{
		for ( next = rmq, prev = (Msgh *) l_next_macro ( next );
								! l_ishead_macro ( prev );
			  next = prev, prev = (Msgh *) l_next_macro ( next ) )
		{
			if ( ! issys_macro ( prev ) )
				break;
		}
	}
	else
	if ( isanti_macro ( msg ) )
	{
		for ( next = rmq, prev = (Msgh *) l_next_macro ( next );
								! l_ishead_macro ( prev );
			  next = prev, prev = (Msgh *) l_next_macro ( next ) )
		{
			if ( issys_macro ( prev ) )
				continue;

			if ( isposi_macro ( prev ) )
				break;


			if ( ltVTime ( msg->sndtim, prev->sndtim ) )
				break;
/*
			if ( ltVTime ( msg->rcvtim, prev->rcvtim ) )
				break;
*/
		}
	}
	else
	{
		for ( next = (Msgh *) l_prev_macro ( rmq );
				   ! l_ishead_macro ( next );
			  next = (Msgh *) l_prev_macro ( next ) )
		{
			if ( issys_macro ( next ) )
				break;

			if ( geVTime ( msg->sndtim, next->sndtim ) )
				break;
/*
			if ( geVTime ( msg->rcvtim, next->rcvtim ) )
				break;
*/
		}
	}
 
	l_insert ( next, msg );

	messages_received++;
}

Msgh * r_deq ()
{
	Msgh * msg;

	msg = (Msgh *)l_next_macro ( rmq );

	if ( l_ishead_macro ( msg ) )
		msg = NULL;
	else
	{
		l_remove ( msg );
		messages_received--;
	}

	return ( msg );
}

VTime r_min ( min )

	VTime min;
{
	register Msgh * msg;

	for ( msg = (Msgh *) l_next_macro ( rmq );
			  ! l_ishead_macro ( msg );
		  msg = (Msgh *) l_next_macro ( msg ) )
	{
		if ( issys_macro ( msg ) )
			continue;
		  
		if ( ! ( msg->flags & MOVING ) )
		{
			if ( gtVTime ( min, msg->sndtim ) &&
			  strcmp ( msg->rcver, "$IH") != 0  )
				min = msg->sndtim;
		}
	}

	return ( min );
}
#endif
 

#ifdef TRANSPUTER

#include "kmsgh.h"

FUNCTION read_the_mail ()

{
	int good_enough_msg;
	int retval, len, rtn_src, rtn_type;
	int node;

	len = msgdefsize;   /* this is size of rm_buf */

	good_enough_msg = FALSE;

	while ( ! good_enough_msg )
	{
		retval = xrecv ( rm_buf, len, ANY_SRC, /* TYPE */ 0, & rtn_src, & rtn_type );

		if ( retval != -1 )
		{
			node = rtn_src;

			if ( ((LowLevelMsgH *)rm_buf)->type == ACK_MSG )      /* ACK */
			{
				rcvack ( rm_buf, node );
			}

			else        /* not an ACK, a reg. msg */
			{
				good_enough_msg = TRUE;

				if ( rm_msg != NULL )
				{
					_pprintf 
					( "read_the_mail:  R M _ M S G  O V E R W R I T E  O F :\n" );
					dumpmsg ( rm_msg );
					_pprintf ( "read_the_mail:  O V E R W R I T I N G   M S G :\n" );
					dumpmsg ( rm_msg );
				}

				rm_msg = rm_buf;

#ifdef PARANOID
				rcvmsg ( rm_msg, node );
#endif 

				if ( ( node != tw_node_num )  &&  ( ! issys_macro ( rm_msg ) ) )
				{
					sndack ( rm_msg, node );
				}
			}
		}

		else    /* since there is no msg to be had, that will have to be good enough. */
		{
			good_enough_msg = TRUE;
		}
	}
}

#endif  /* TRANSPUTER */


#ifdef MARK3
extern VTime stdout_ok_time;

FUNCTION read_the_mail ()
{
	int iret;
 
  Debug

get_again:

	cnt_msgs ( &iret );
	if ( iret == 0 )
		return;

#ifdef TIMING
	start_timing ( SYSTEM_TIMING_MODE );
#endif

	recv.source = ANY;
	iret = peek_msg ( &recv );

#ifdef TIMING
	stop_timing ();
#endif
 
	if ( iret == OK )
	{
		get_mercury_msg ();

		if ( rm_msg == NULL )
			goto get_again;
	}
}

get_mercury_msg ()
{
	int node;

		LEDS ( 1 );

		unlink_msg ( &recv );

		if ( recv.source == CP )
		{
			if ( recv.mlen == 0 )
				stdout_acks_pending--;
			else
			if ( recv.mlen == sizeof(stdout_ok_time) )
				stdout_ok_time = * (VTime *) recv.buf;
			else
				_pprintf ( "Received Type %d from CP\n", recv.mlen );

#ifdef TIMING
	start_timing ( SYSTEM_TIMING_MODE );
#endif
			give_buf ( &recv );

#ifdef TIMING
	stop_timing ();
#endif
			return;
		} 

		node = recv.source - node_offset;

		if ( ((LowLevelMsgH *)recv.buf)->type == ACK_MSG )       /* ACK */
		{
			rcvack ( recv.buf, node );
#ifdef TIMING
			start_timing ( SYSTEM_TIMING_MODE );
#endif
			give_buf ( &recv );
#ifdef TIMING
			stop_timing ();
#endif
			return;
		}

		if ( rm_msg != NULL )
			_pprintf ( "rm_msg overwrite\n" );

		rm_msg = (Msgh *) recv.buf;

#ifdef PARANOID
		rcvmsg ( rm_msg, node );
#endif 
#ifdef CHECKSUM
		if ( checksum ( rm_msg ) != (Msgh *)rm_msg->checksum )
		{
			_pprintf ( "Checksum Error\n" );
			showmsg ( rm_msg );
			tester ();
		}
#endif
		if ( node != tw_node_num
		&&  ! ( rm_msg->flags & ( SYSMSG | MOVING ) ) )
		{
			sndack ( rm_msg, node );
		}
}
#endif


#ifdef SUN
dump_socket ( node )

	int node;
{
	int i, j, k;
	unsigned char buff[2];

	if ( node > MAX_NODES )             /* if it's a pointer */
		node = * (int *)node;           /* make it the node number */

	printf ( "socket %d\n", node );
	for ( ;; )
	{
		printf ( "\n" );
		for ( i = 0; i < 8; i++ )
		{
			for ( j = 0; j < 4; j++ )
			{
				k = read ( msg_ichan[node], buff, 1 );
				if ( k == -1 )
				{
					printf ( "\n" );
					return;
				}
				printf ( "%.2x", buff[0] );
			}
			printf ( " " );
		}
	}
}

#endif

#ifdef MARK3
ioint_rcvack ( msg, node )
 
	Ack_msg * msg;
	Int node;
{
	register int i;
 
	if ( msg->low.to_node == IH_NODE )
		msg->low.to_node = 0;

	if ( msg->low.from_node != tw_node_num
	||   msg->low.to_node != node )
	{
		return 0;       /* error */
	}
 
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
		return 0;       /* error */
	}
 
	return 1;   /* ok */
}
#endif

#ifndef TRANSPUTER

/* The following comment is necessary to suppress lint warnings about 
		variable arguments in _pprintf() calls.  Please don't delete. */
/*VARARGS*/
_pprintf ( form, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
				arg11, arg12, arg13, arg14, arg15 )

	char * form;
	int *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8, *arg9, *arg10,
		*arg11, *arg12, *arg13, *arg14, *arg15;
{
#ifdef MARK3_OR_BBN
	static char buff[256];

	sprintf ( buff, "%d--", tw_node_num + node_offset );
	sprintf ( buff + strlen ( buff ),
				form, arg1, arg2, arg3, arg4, arg5,
				arg6, arg7, arg8, arg9, arg10,
				arg11, arg12, arg13, arg14, arg15 );

	printf ( "%s", buff );
#endif

#ifdef SUN
	int count ;
	static char buff[256];
	int len;

	if ( standalone )
	{
		printf ( form, arg1, arg2, arg3, arg4, arg5, 
						arg6, arg7, arg8, arg9, arg10,
						arg11, arg12, arg13, arg14, arg15 );
		fflush ( stdout );
	}
	else
	{
		sprintf ( buff, form, arg1, arg2, arg3, arg4, arg5, 
						arg6, arg7, arg8, arg9, arg10,
						arg11, arg12, arg13, arg14, arg15 );

		len = strlen ( buff );

/*
		if ( buff[len-1] != '\n' )
		{
			buff[len++] = '\\';
			buff[len++] = '\n';
			buff[len] = 0;
		}
*/
		string_to_cp_w ( buff );
/*
		count = 0;
		while ( write ( ctl_ochan, buff, len ) == -1 )
		{
			if (count ++ == 10000)
			{
				printf ("Trying to do _pprintf to host...\n");
				count = 0;
			}
		}
*/
	}
#endif
}

#endif  /* not TRANSPUTER */

#if 0
check_for_keyin ()
{
	int stat;
printf ( "check for keyin called\n" );
tester();

#ifdef TIMING
	start_timing ( SYSTEM_TIMING_MODE );
#endif
	stat = read ( ctl_ichan, buff, 80 );

	if ( stat > 0 )
	{
		buff[stat] = 0;

		bp = buff;

		host_input_waiting = 1;
	}
#ifdef TIMING
	stop_timing ();
#endif
}
#endif

#ifdef TRANSPUTER

entcpy ( dest, src, numbytes )

char    * dest;
char    * src;
int     numbytes;

{
	memcpy ( dest, src, numbytes );
}


clear ( ptr, numbytes )

char    * ptr;
int     numbytes;

{
	bzero ( ptr, numbytes );
}


/*---------------------------------------------------------------------*/

_pprintf ( form, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
				arg11, arg12, arg13, arg14, arg15 )

	char * form;
	int *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8, *arg9, *arg10,
		*arg11, *arg12, *arg13, *arg14, *arg15;
{

	kprintf 
	( 
		form, 
		arg1,  arg2,  arg3,  arg4,  arg5, 
		arg6,  arg7,  arg8,  arg9,  arg10, 
		arg11, arg12, arg13, arg14, arg15 
	);

}

#endif  /* TRANSPUTER */

/*---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/
