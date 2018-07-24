/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	sendback.c,v $
 * Revision 1.10  91/11/06  11:12:34  configtw
 * Fix merge error.
 * 
 * Revision 1.9  91/11/01  13:39:36  reiher
 * Added routine to free message pool entries in an emergency, and a debugging
 * routine to verify the integrity of the free message pool (PLR)
 * 
 * Revision 1.8  91/11/01  10:14:00  pls
 * 1.  Change ifdef's and version id.
 * 2.  Determine memory out condition locally.
 * 
 * Revision 1.7  91/07/17  15:12:14  judy
 * New copyright notice.
 * 
 * Revision 1.6  91/07/09  14:40:41  steve
 * Added MicroTime support
 * 
 * Revision 1.5  91/06/03  12:26:37  configtw
 * Tab conversion.
 * 
 * Revision 1.4  91/05/31  15:21:02  pls
 * Make memory out determination local to node.
 * 
 * Revision 1.3  91/03/25  16:35:59  csupport
 * Put in some commented debug code.
 * 
 * Revision 1.2  90/11/27  10:04:05  csupport
 * save memory_out_time in m_create()
 *
 * Revision 1.1  90/08/07  15:40:55  configtw
 * Initial revision
 * 
*/
char sendback_id [] = "@(#)sendback.c   $Revision: 1.10 $\t$Date: 91/11/06 11:12:34 $\tTIMEWARP";


/*

Purpose:

		The functions in this module are responsible for part of
		memory management.  Whenever an attempt to get memory fails,
		these routines try to find queued messages that can be returned
		to their sender, thereby freeing memory for the request that
		failed.  The basic method is to look through the various
		queues for items that are far in the future, and get rid of
		those.  The direction bit on chosen messages is complemented,
		and deliver() is called to send them from whence they came.

Functions:

		m_create(size,time,critical)-  try to create a list element of a 
				specific size
				Parameters - int size, VTime time, int critical
				Return - a pointer to the list element, or a NULL pointer

		make_memory(size,time) - free memory space from queues
				Parameters - int size, VTime time
				Return - 0 if it can't, 1 if it can

		send_back(msg) - return a message to its sender
				Parameters - Msgh *msg
				Return - Always returns 0

		get_message_to_send_back(time) - find a message that can be
				returned to its sender
				Parameters - VTime time
				Return - a pointer to a message, if one was found;
								a NULL pointer, otherwise

Implementation:

		m_create() loops until it succeeds, or is certain that the
		requested memory is unavailable.  At each iteration, it tries
		to allocate memory for the requested item.  If it succeeds, it
		goes no further, simply returning a pointer to the allocation.
		If it fails, it calls make_memory(), to free up some space.
		make_memory() will free up space only from a single object.
		Thus, if the space it frees is insufficient for the requested
		item, it must be called again, until all objects on the
		node have freed the space they can.  This is the purpose of
		the loop in this routine.  If the call to make_memory()
		ever fails, that means that no object could free any space.
		Should this happen, determine if the request was critical
		or not, by consulting the parameter supplied to m_create().
		If the request wasn't critical, return NULL, indicating failure.
		If it was, print an error message and trap to tester.

		make_memory() calls get_message_to_send_back() to find
		one message in some object's queue that can be returned.
		Should one be found, that message is returned to the sender.
		Since it might be a multi-packet message, make_memory() runs
		through a list containing one entry per packet, and each packet
		is sent back.  Return 0 if nothing got freed, 1 otherwise.

		send_back() simply calls a macro to reverse the direction
		bit on the message to indicate that it is going backwards.
		Then call deliver() to ship it off.

		get_message_to_send_back() first decides what time to
		consider as the minimum time to search for.  This time
		is the minimum of the time involved with the item that
		caused the problem in the first place, and min_vt,
		which usually contains gvt.  best_m is set to NULL, 
		indicating that we have not yet found any message to
		send back.  Now loop through all the objects kept on
		this node.  Starting at the end of the input queue,
		look at each message in the queue until one is found
		whose rcvtime is less than or equal to the time of the
		item that caused this attempt to free space, at which
		point go on to the next message, which is guaranteed to
		be at a later time.  Skip over antimessages, and all but 
		the first packet of multipacket messages.  For each candidate, 
		compare its sendtime to the time chosen at the head of this 
		routine.  If this message's sendtime is greater than the 
		best one yet found, set the best time, message, and object 
		to this messages's time, message, and object.  Now make a 
		list containing all packets of the message being sent back.
		Remove all such packets from the input queue.  If the
		removed message was the one pointed to by its object's
		ci pointer, null that pointer.  After the sendback list

*/

#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"

extern VTime    oldgvt1;
extern VTime    oldgvt2;

#if 0
/* temporary debugging code */
VTime outTime = { NEGINF, 0, 0 };
int outSize = -1;
#endif

extern int no_message_sendback;
int memory_out_flag;
VTime memory_out_time;
extern int messages_to_send;
extern int stdout_messages_to_send;

Byte *
FUNCTION m_create ( size, time, critical )

	int size;
	VTime time;
	int critical;
{
	register Byte * pointer;

  Debug

	for ( ;; )
	{
		pointer = (Byte *) l_create ( size );

		if ( pointer != NULL )
		{
#if 0
		/* the following test prevents gvt cycling when most of an
		   object can get memory, but part can't */

			if ((memory_out_flag) && neVTime(time,memory_out_time))
				memory_out_flag = 0;
#endif

			return ( pointer );
		}

/* only get here if there was not enough memory */

		if ( no_message_sendback )
		{
			if (
				eqVTime(gvt,time) &&
				eqVTime(gvt,oldgvt1) &&
				eqVTime(oldgvt1,oldgvt2))
				{
				memory_out_flag = 1;
				memory_out_time = time;
				}

			return ( NULL );
		}

		if ( make_memory ( size, time ) )
		{  /* we freed memory via message sendback */
			continue;   /* continue freeing til we have enough memory */
		}
		else
		{       /* uh oh--no more memory to free */
			if ( critical ) 
			{
				_pprintf("m_create OUT @ critical %f trying for size %d\n",
					time.simtime, size);
				tester();
			}

			/*  Check to see if we've run out of memory.  If 
				messages_to_send is non-zero, we may get some memory back
				once the other nodes clear out our buffers, so we're not
				out of memory if there are messages to send.  (Note that
				standard out messages don't count, for this purpose, since
				they will only be released if GVT increases; therefore, we
				don't check stdout_messages_to_send.) */

			if ( messages_to_send == 0 )
			{
/* Temporary debug code */
#if 0
				if ( neVTime ( outTime, time ) || size != outSize )
				{
				_pprintf("m_create OUT (non-critical) %f trying for size %d\n",
								time.simtime, size );
				outTime = time;
				outSize = size;
				if ( xqting_ocb != NULL )
				{
					_pprintf("executing ocb is %s\n",xqting_ocb->name);
				}
				}
#endif

/* End temporary debug code */
		
		/* we are out of memory if gvt has been stuck 3 times,
		   and this request is at time gvt */
				if (
					eqVTime(gvt,time) &&
					eqVTime(gvt,oldgvt2) )
					{
					  twerror ( "Out of memory, time %f, requesting %d bytes\n",
						time.simtime, size );
					  if ( xqting_ocb != NULL )
					  {
						_pprintf("executing ocb is %s\n",xqting_ocb->name);
					  }

					  tester ();
				    }
				
			}

			return ( NULL );
		}
	}  /* for ( ;; ) */
}


FUNCTION make_memory ( size, time )

	int size;
	VTime time;
{
	Msgh * message, * next_message, * get_message_to_send_back(); 

  Debug  

	/* states are not touched for now */

	message = get_message_to_send_back ( time );

	if ( message == NULL )
	{    
		/*  Emergency - try to free anything in the message pool. */

		if ( free_msg_pool_entries () )
		{
			return ( 1 );
		}

		return ( 0 );
	}    

	send_back ( message );      /* do the sendback */

	return ( 1 );
}


FUNCTION send_back ( message )

	Msgh * message;
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
#if BBN
		butterflytime ();
#endif
#endif
		message->cputime = node_cputime;
	}

	deliver ( message );        /* send out the message */
}

Msgh *  
FUNCTION get_message_to_send_back ( time )

	VTime time;
{
	register Ocb * o, * best_o;
	register Msgh * m, * best_m;
	VTime best_time;
	extern int aggressive;

  Debug

	best_time = time;

	if ( ltVTime ( best_time, min_vt ) )
		best_time = min_vt;     /* create minimum compare time */

	best_m = NULL;      /* init best match */

	for ( o = fstocb_macro; o; o = nxtocb_macro (o) )
	{   /* loop through objects on this node (prqhd queue )*/
		for ( m = lstimsg_macro (o); m; m = prvimsg_macro (m) )
		{
			/* skip this queue if time too early */
			if ( leVTime ( m->rcvtim, best_time ) ) break;

			/* skip anti messages */
			if ( isanti_macro (m) ) continue;

			if ( gtVTime ( m->sndtim, best_time ) )
			{   /* we found a better candidate */
				best_time = m->sndtim;  /* save the time */
				best_m = m;             /* save the message ptr */
				best_o = o;             /* save the object ptr */
			}
		}
	}

	if ( best_m != NULL )
	{   /* we found a candidate for sendback */
		l_remove ( best_m );    /* remove it from q */

		set_reverse_macro ( best_m );   /* mark it for sendback */
		smsg_stat ( best_o, best_m );   /* record some stats */

		if ( best_o->ci == best_m )
			best_o->ci = NULL;

		rollback ( best_o, best_m->rcvtim );    /* rollback if necessary */
	}

	return ( best_m );
}

/*  free_msg_pool_entries() searches through the pool of free messages
	looking for messages to return to the main memory heap.  A message in
	the pool can be returned if it is not part of the original allocation
	of message buffers given to this pool, and if there are sufficient free
	messages to prevent the system from churning memory into and out of the
	pool.  All messages meeting these criteria will be freed.  The function
	returns the number of messages it freed, 0 if no messages were freed. */

extern num_msg_buffs_released_from_pool;
extern int msg_free_pool_size;
extern int msg_buffers_alloc;
extern List_hdr *first_msg_buffer, *last_msg_buffer, *msg_free_pool;
#define HEADER(u)       u-1     /* header given user pointer */

FUNCTION free_msg_pool_entries()
{
	int freed;
	Msgh * msg, *next;
	List_hdr       *head;

	freed = 0;

	for ( msg = l_next_macro ( msg_free_pool ); msg != msg_free_pool;
			msg = next )
	{
		next = l_next_macro ( msg );

		if ( msg < last_msg_buffer || msg > first_msg_buffer )
		{
			l_remove (msg);
			m_release ( ((List_hdr *) msg) -1 );
			msg_free_pool_size--;
			msg_buffers_alloc--;
			freed++;
		}
	}

	num_msg_buffs_released_from_pool += freed;
	return ( freed );
}
#undef HEADER

verify_free_msg_pool()
{

	Msgh * msg, *next;

	msg = msg_free_pool;
	if ( l_prev_macro ( l_next_macro ( msg ) ) != msg  ||
		 l_next_macro ( l_prev_macro ( msg ) ) != msg )
	{
		_pprintf (" msg pool corrupted near header %x\n",msg_free_pool);
		tester();
	}
	for ( msg = l_next_macro ( msg_free_pool ) ; msg != msg_free_pool;
				msg = l_next_macro ( msg ) )
	{
		if ( l_prev_macro ( l_next_macro ( msg ) ) != msg  ||
		 	l_next_macro ( l_prev_macro ( msg ) ) != msg )
		{
			_pprintf("msg pool corrupted near %x\n",msg );
			tester();
		}
		
	}
}
