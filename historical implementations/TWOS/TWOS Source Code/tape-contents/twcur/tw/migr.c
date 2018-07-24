/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	migr.c,v $
 * Revision 1.17  92/02/25  10:34:52  reiher
 * Changed migration of dynamic memory table to precalculate size and
 * allocate correct size initially.  Also, bug fix in send_state_copy()
 * 
 * Revision 1.16  92/01/03  09:05:42  configtw
 * Take out reordering optimization (bug fix).
 * 
 * Revision 1.15  91/12/27  11:01:15  pls
 * 1.  Use l_size macro.
 * 2.  Add support for variable length address tables.
 * 3.  Carry libPointer over when migrating (bug 16).
 * 
 * Revision 1.14  91/12/27  08:45:23  reiher
 * Added code for event and event time throttling, and critical path computation
 * 
 * Revision 1.13  91/11/06  11:11:46  configtw
 * Fix DLM compile errors on Suns.
 * 
 * Revision 1.12  91/11/01  09:41:59  reiher
 * multiple migration bug fixes, made sure acks/naks match with their migrating
 * entity, more debugging routines (PLR)
 * 
 * Revision 1.11  91/07/17  15:10:09  judy
 * New copyright notice.
 * 
 * Revision 1.10  91/07/09  14:26:09  steve
 * Added MicroTime support. Removed mistuff.
 * 
 * Revision 1.9  91/06/04  11:29:21  configtw
 * Fix Sun3 compile error.
 * 
 * Revision 1.8  91/06/03  15:51:43  configtw
 * Take out preint state deletion in recv_state().
 * 
 * Revision 1.7  91/06/03  14:25:27  configtw
 * Fix code for DLM off.
 * 
 * Revision 1.6  91/06/03  12:24:59  configtw
 * Tab conversion.
 * 
 * Revision 1.5  91/05/31  14:24:00  pls
 * 1. Add parameter to FindObject() call.
 * 2. Fix dlm bugs.
 * 
 * Revision 1.4  91/04/16  09:36:17  pls
 * 1.  Take out diagnostic messages.
 * 2.  Don't allow migration of ocb's in BLKPT mode (because of bug 4 fix).
 * 
 * Revision 1.3  91/04/01  15:41:00  reiher
 * Wholesale changes in phase migration code, including pre-interval state
 * optimizations and a number of important bug fixes.
 * 
 * Revision 1.2  90/08/09  16:56:40  steve
 * Added msgloging to migrating states.
 * Set state->ocb fields
 * 1st draft of jump forward for phase migration
 * 
 * Revision 1.1  90/08/07  15:40:13  configtw
 * Initial revision
 * 
*/
char migr_id [56] = "@(#)migr.c 1.31\t10/6/89\t14:25:23\tTIMEWARP";


/*              migr.c - Object Migration Routines      */

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

/*
		There is a protocol involved in sending a state to another
		node which may take a while to complete. Therefore, a
		queue of states to send is used to hold onto any number
		of states. Only the first state in the sendStateQ can be
		in the process of sending to another node.
*/

int states_to_send;

/*
		There is a protocol involved in sending an object phase
		to another node which may take a while to complete.
		Therefore, a queue of ocb's to send is used to hold onto
		any number of object phases. Only the first ocb in the
		sendOcbQ can be in the process of sending to another node.
*/

int ocbs_to_send;
Ocb * tempOcb;
char tempName[20];

extern Pending_entry * PendingListHeader;
extern int loadCount;
extern int migrGraph;
extern int resendState;
extern int aggressive;
extern long critEnabled;
int phaseNaksSent = 0;
int phaseNaksRecv = 0;
int stateNaksSent = 0;
int stateNaksRecv = 0;


/*      send_state_copy is called by go_forward in rollback.c
		when an object is BLKINF and phase_end is < POSINF.

		loadstatebuffer in state.c makes a copy of the object's
		cs (current state) and leaves a pointer to the copy
		in the object's sb (state buffer). For this to work
		the cs must point to the state to copy and the sb
		must be available (NULL). The sb should always be
		NULL when the runstat is BLKINF.

		Once a copy of the state is made, this routine should
		release the object's stack, which was allocated by
		loadstatebuffer, and clear the object's sb and stk.

		The state copy is send to the next phase of the object
		by calling send_state unless the next phase is on the
		same node. In that case the state is put in the next
		phase's state queue.
*/

Ocb * findInSendQueue();
State * copystate ();

FUNCTION send_state_copy ( state, ocb )

	State * state;
	Ocb * ocb;
{
	Objloc * location;
	int finish_send_state_copy ();
	State * new_state;
	Ocb         *next;
	State * last_sent = ocb->last_sent;
	Ocb * inSendQueue;

  Debug

	/*  If the state in question is erroneous, let's not bother sending
		it.  The migration code doesn't currently send it correctly,
		anyway, and an erroneous state won't do any good on the destination
		end, either. */

	if ( state->serror != NOERR )
	{

#if 0
		_pprintf("send_state_copy: not sending state for %s because of state error \n", ocb->name );
#endif

		return;
	}

	fstateout++;

	ocb->last_sent = state;


/* The limited jump forward optimization is within this ifdef.   last_sent
		points to the last state sent.  If such a state exists, compare it
		to the state about to be sent.  If they are the same, then if 
		last_state is no longer saved in the state queue (because it was 
		committed), destroy last_state.  Then clear the out_of_sq flag.
		In either case, if the compare is the same, simply return.  If
		the compare is not the same, check whether out_of_sq is set, which
		means that a committed state must be destroyed.
*/

	if ( last_sent )
	{
		/* Coming soon */
		if (  state_compare ( last_sent, state, ocb->pvz_len ) ==  0 )
		{
/*
_pprintf("JF wins: Object %s, phase end %f\n",ocb->name, ocb->phase_end.simtime);
*/
			if ( ocb->out_of_sq )
			{
/*
_pprintf("JF wins: destroying state for jump forward\n");
*/
				l_remove ( last_sent );
				destroy_state ( last_sent );
			}
			ocb->out_of_sq = 0;
			dupStateSend++;
			return; /* jump forward for phase wins !!! */
		}
		else if ( ocb->out_of_sq )
		{
/*
_pprintf("JF loses: destroying state for jump forward\n");
*/
			l_remove ( last_sent );
			destroy_state ( last_sent );
		}
	}

	ocb->out_of_sq = 0;

	new_state = copystate ( state );

	if ( new_state == NULL )
	{
		l_remove ( state );

		new_state = state;

		/* If we later commit the bundle associated with this state, remember
				that the state should count as committed. */

		ocb->loststate = 1;

		ocb->last_sent = NULL;

#if 0
		_pprintf ( "send_state_copy: lost a state for %s\n", ocb->name );
#endif

		if ( ocb->runstat == READY )
			ocb->runstat = GOFWD;
	}
	else
	if ( new_state == (State *) -1 )
	{

		/* In this case, the node could not obtain enough memory to make
				a copy of the state for shipping, and could not even obtain
				enough memory to copy any deferred memory segments lying
				around in earlier versions of the state.  Set the object
				into STATESEND status, which will cause it to try again
				later.  Also, change the svt to the phase's phase_end which
				will reorder the scheduler queue, so that the retry will happen
				at the appropriate priority.  Since there isn't a copy of a 
				state to send, return immediately. */

		ocb->runstat = STATESEND;
		ocb->svt = ocb->phase_end;

		/* By this time, we know that the limited jump forward optimization
			won't work for this state.  In unusual cases, a rollback and
			new attempt to send a different state might have profited from 
			the limited jump forward optimization, but the chances are poor
			and there are possible complications, so just set ocb->last_sent
			to NULL, thereby bypassing the optimization on the next attempt
			by this OCB to send a state. */

		ocb->last_sent = NULL;
		next = l_next_macro ( ocb );
		l_remove ( ocb );               /* remove ocb from object list */
		nqocb ( ocb, next );    /* reinsert ocb in new time order */

		return;
	}

	if ( ocb->runstat == STATESEND )
	{

		/* If we got this far with this status, we have a copy of the state
			to send.  Change the run status of this phase to BLKINF.   Also,
			reorder the scheduler queue to put it in its proper place.  */

		ocb->runstat = BLKINF;

		ocb->svt = posinf;
		next = l_next_macro ( ocb );
		l_remove ( ocb );               /* remove ocb from object list */
		nqocb ( ocb, next );    /* reinsert ocb in new time order */
	}

	new_state->ocb = (Byte *) ocb;
	new_state->sflag |= STATEFORW;

	/*  First check to see if the phase to receive the state is in the
		queue of phases to be migrated.  If so, and if the phase hasn't
		started migrating yet, the state copy will be 
		put into the state queue of that phase locally, rather than being
		shipped to the eventual destination node.  The process to do so is
		moderately complex, as the phase getting the state must be rolled
		back when it is in the sending queue, rather than the schedule
		queue.  As a result, some of the normal rollback code cannot be used. */

#if 0
	inSendQueue = findInSendQueue ( ocb->name, ocb->phase_end );

	if ( inSendQueue != NULL && !(inSendQueue->migrWait & WAITFORACK) &&
				!(inSendQueue->migrWait & WAITFORDONE ) )
	{
		rollbackInMigrQ++;
		putStateInSendQueue ( inSendQueue, new_state );
		return;
	}
#endif 0

	location = GetLocation ( ocb->name, ocb->phase_end );

	if ( location && (location->node != tw_node_num || location->po != NULL) )
	{
		finish_send_state_copy ( new_state, location );
	}
	else
	{

		FindObject (
						ocb->name,
						ocb->phase_end,
						new_state,
						finish_send_state_copy,
						NOTMSG
				   );
	}
}

FUNCTION finish_send_state_copy ( state, location )

	State * state;
	Objloc * location;
{
	Ocb * ocb;
	int flags = 0;

  Debug

	ocb = (Ocb *) state->ocb;

/* Potential Problem if a split command is issued between
		send_state_copy() & finish_send_state_copy() */

	ocb->next_node = location->node;

	if ( ocb->next_node == tw_node_num )
	{
		put_state_in_sq ( location->po, state );
		state->ocb = location->po;
		/* This is a bug fix having nothing to do with my new code.  If
				two adjacent phases of an object are on the same node,
				TW2.4.1 wouldn't handle it properly.  When the earlier
				phase forwarded a state to the later phase, the state 
				would be queued, but the later phase wouldn't be rolled
				back, so it might never execute.  Also, neither the object
				statistics nor the node statistics took account of these
				state receives on the receiving end, so stats wouldn't
				balance.  */

		rollback_state ( location->po, state->sndtim );

		ocb->stats.stforw++;
		fstatein++;
	}
	else
	{
		Ocb * migrating;

		/*  Set the flags for send_state().  In this routine, the state
				copy is always pre-interval.  If the destination phase has
				not yet gotten to the destination node, also set the flags
				to show that it must wait for the migration. */

		flags |= PRE_INTERVAL;

		migrating = findInSendQueue (ocb->name, ocb->phase_end);

		if ( migrating != NULL )
		{
				flags |= WAIT_FOR_MIGRATE;
		}

		send_state ( state, ocb, flags );
	}
}


/*      All control information for sending a state to another node
		has been added to the state header (State and 
		State_Migr_Hdr in twsys.h).

		In version 2.5 all migration information will be in     
		State_Migr_Hdr. For now however, the segment and packet
		information will reside in State.

		States are divided into segments and segments are divided
		into packets. The global variable pktlen is used for the
		packetizing. Segment 0 is the state header and data area.
		Dynamically allocated blocks of memory are segments 1 thru
		MaxAddresses. These segment numbers are the same as the
		offsets into the address_table and are not necessarily
		contiguous due to allocates and frees by the objects.
*/

FUNCTION send_state ( state, ocb, flags )

	State * state;
	Ocb * ocb;
	int flags;
{
	Byte * segment;
	int seglen;
	register int i;
	State_Migr_Hdr  * migr_hdr;

  Debug

/*PJH#1  Create A State migration header      */

	/*  send_state() should never be handed a state in an error condition.
		If this state is erroneous, trap to tester().  */

	if ( state->serror != NOERR )
	{
		twerror ( "send_state: %s trying to send erroneous state %x\n",
						ocb->name, state );
		tester();
	}

/*  No split command in between send_statecopy() & send_state() */

	migr_hdr = ( State_Migr_Hdr *) l_create ( sizeof ( State_Migr_Hdr ) );

	if ( migr_hdr == NULL )
	{
		twerror("send_state: NULL migr_hdr for state %d, ocb %d\n",
			state, ocb );
		tester();
	}

	migr_hdr->time_to_deliver = ocb->phase_end; 
	migr_hdr->to_node = ocb->next_node;       
	migr_hdr->state = state;
	migr_hdr->waiting_for_ack = FALSE;
	migr_hdr->waiting_for_done = FALSE;
	migr_hdr->acksExpected = 0;
	migr_hdr->acksReceived = 0;
	strcpy ( migr_hdr->name, ocb->name );

	/* Set the migration header flags to indicate whether the state is
		pre-interval, and whether it is being migrated or forwarded. */

	migr_hdr->migr_flags = flags;


	state->ocb = (Byte *) ocb;
	state->segno = 0;
	state->no_segs = 1;
	segment = (Byte *) state;

	/*  The state points to the type table entry for this state's type, which 
		contains a field describing the size of the state.  This is a 
		preferable method to examining the size of the memory block currently 
		holding the state.   Once we're sure that this works, the paranoid 
		test can be removed.  */

	seglen =  state->otype->statesize + sizeof (State)+12 ;

		/* the 12 bytes holds "state limit" */
#ifdef PARANOID
	if ( seglen != (((List_hdr *)segment)-1)->size )
	{
		twerror("send_state: type table length for a state doesn't match memory block size, state ptr %x\n", state);
		_pprintf("      seglen = %d, list hdr length = %d\n",seglen,
						(((List_hdr *)segment)-1)->size);
		tester();
	}
#endif PARANOID
	state->tot_pkts = ( seglen + pktlen - 1 ) / pktlen;
	if ( state->address_table != NULL )
	{
		for ( i = 0; i < l_size(state->address_table) / sizeof(Address); i++ )
		{
			if ( state->address_table[i] != NULL )
			{
				state->no_segs++;
				segment = (Byte *) state->address_table[i];
				if ( segment == DEFERRED )
					state->tot_pkts++;
				else
				{
					seglen = l_size(segment);
					state->tot_pkts += 
						( seglen + pktlen - 1 ) / pktlen;
				}
			}
		}
	}

	/* Set the addrTableSize field to indicate how big to make the table
		at the destination end upon state reassembly. */

	if ( state->address_table != NULL )
		state->addrTableSize = l_size(state->address_table)/sizeof(Address);
	else
		state->addrTableSize = 0;

	state->pktno = 0;
	state->no_pkts = 0; /* depends on segno */

/* Pre-Interval State optimization.     

 	If we are about to insert a pre-interval state in the sendStateQ here we 
	should check for any other pre-interval states that are in the queue for 
	this object & phase and remove and delete them.  However, if the state
	we're trying to insert is a migrating pre-interval state, it should
	not remove any existing states, as they represent later versions of the
	pre-interval state.                                    */

	if ( sendStateQ && ( migr_hdr->migr_flags & PRE_INTERVAL) &&
			! ( migr_hdr->migr_flags & MIGRATING ) )
	 {   
		State_Migr_Hdr  * scan_hdr, * next_scan;
		Ocb * ocb1, * ocb2;

		scan_hdr = nxtmigrh_macro ( sendStateQ );
		while ( scan_hdr != NULL )
		 { 
		   next_scan = nxtmigrh_macro ( scan_hdr );

		   /* If the scan_hdr state is a pre-interval one, and it's not
				migrating, and it's not in the middle of moving, and it is
				going to the same node as the migr_hdr state, and the two 
				states have the same time_to_deliver, and the name of the 
				receiving object is the same, delete the scan_hdr state, as 
				the migr_hdr state will replace it.  If this code causes
				trouble, the first thing to try is to also check the
				send times on the states.  It shouldn't be necessary, but
				perhaps it is.*/

		   if (  (scan_hdr->migr_flags & PRE_INTERVAL) &&
				 !(scan_hdr->migr_flags & MIGRATING)   &&
				 !(scan_hdr->migr_flags & STATE_MOVING) &&
				 (scan_hdr->to_node == migr_hdr->to_node) &&
				 eqVTime ( scan_hdr->time_to_deliver,
						   migr_hdr->time_to_deliver)
			  )
			  {  
				 ocb1 = (Ocb *) migr_hdr->state->ocb;
				 ocb2 = (Ocb *) scan_hdr->state->ocb;

				 if ( strcmp (ocb1->name, ocb2->name ) == 0)
				  {
#if 0
		_pprintf ("about to delete pre-int state from migr queue\n");
		dump_state_migr_hdr ( scan_hdr );
		showstate ( scan_hdr->state );
		dump_state_migr_hdr ( migr_hdr );
		showstate ( migr_hdr->state );
		tester();
#endif

					 ocb2->num_states--;
					 states_to_send--;
					 didNotSendState++;      
					 destroy_state ( scan_hdr->state );
					 l_remove ( scan_hdr );
					 l_destroy ( scan_hdr );
				  }   
			   }
		   scan_hdr = next_scan;
		  }
	  }          


	l_insert ( l_prev_macro ( sendStateQ ),  migr_hdr );

	states_to_send++;

	send_state_from_q ();
}


/*      The protocol for sending a state to another node is as follows.
		First, send packet 0 of segment 0 of the state. One packet
		must be at least big enough for the state header because it
		contains control information like the total number of packets
		to be sent. Next, the sending waits for an acknowledgement
		from the receiving node. No other state messages (STATEMSG)
		can be sent while a state acknowledgement is pending. There
		are three state acknowledgement messages: STATEACK, STATENAK
		and STATEDONE. STATEACK means the receiving node has
		allocated memory for the first segment, has stored packet 0
		and is waiting for the rest of the state packets and segments.
		STATENAK is sent any time the receiving node fails to allocate
		memory for a state segment. In that case the entire protocol
		starts over. STATEDONE means the last packets has arrived
		and been stored. If there is only one packet altogether then
		STATEDONE is sent instead of STATEACK. After receiving STATEACK,
		the sending node is free to send all the rest of the packets.
		There is no sensitivity to message arrival order after the
		first packet. There is enough information in each packet to
		put it in its place without reference to any other packet
		except the first one. The receiver knows when it has the
		entire state by keeping track of a total packet count in
		the state header.
*/

/*  At some point, remove all use of the ocb ptr in the following function.
		The ocb in question might possibly have been migrated by the time
		we try to follow it, in which case we're in for a world of hurt.
		This wouldn't happen often, but might occasionally.  To fix it, 
		copy any info we need out of it into the migr_hdr at the time the
		state is put into the migrating state queue.  This will require
		putting more stuff into the migr_hdr. */

FUNCTION send_state_from_q ()
{
	Msgh * statemsg;
	State_Migr_Hdr * migr_hdr;
	Byte * segment, * packet;
	int seglen, lenpkt, offset;
	register int i, k;
	Ocb * ocb;
	Ocb * migrating;

  Debug


	migr_hdr = (State_Migr_Hdr *) l_next_macro ( sendStateQ );

	if ( migr_hdr == sendStateQ )
		return;

	if ( migr_hdr->waiting_for_ack )
		return;

	if ( migr_hdr->waiting_for_done )
		return;


	/*  Check to see if the destination phase for this state is still in
		the phase migration queue.  If it is, don't send this state.
		Instead, reorder the queue to bring up the next state not marked
		as waiting for migration, and send that one, instead.  If no such
		state is found, move this state to the end of the queue and return.
		More complex manipulations than the latter are possible, but it
		should do for now. */

	ocb = (Ocb *) migr_hdr->state->ocb;

	if ( migr_hdr->migr_flags & WAIT_FOR_MIGRATE )
	{
		Ocb * sendQueueHead;

		/*  If the destination phase is still in the migration queue, and
				it is not at the front (or, if at the front, has not yet
				set up an ocb on the destination node), delay this migration
				in favor of doing something else. */

		migrating = findInSendQueue ( ocb->name, migr_hdr->time_to_deliver );
		sendQueueHead = nxtocb_macro ( sendOcbQ );

		if (  migrating != NULL && ( migrating != sendQueueHead  || 
			 migrating->migrStatus == MIGRNOTSTARTED ||
			 migrating->migrStatus == MIGRSTART ) )
		{

			while ( migr_hdr = l_next_macro ( migr_hdr ) )
			{
				/*  If there are no suitable states, return. */

				if ( migr_hdr == sendStateQ )
				{
/*
					send_ocb_from_q ();
*/
					return;
				}

				ocb = (Ocb *) migr_hdr->state->ocb;

				/* If this migrating state is not waiting for a phase to
						complete its migration, send this one. */

				if ( !(migr_hdr->migr_flags & WAIT_FOR_MIGRATE) )
						break;

				/* If the state is waiting for a phase to move, check to
						see if the phase has moved or is moving.   If so, send 
						this state.  If not, keep looking for another state. */

				migrating = findInSendQueue ( ocb->name, 
								migr_hdr->time_to_deliver );

				if ( migrating != NULL &&
						migrating != sendQueueHead )
					continue;
				else
				{
					/* Indicate that this state can now be migrated. */

					migr_hdr->migr_flags &= ~WAIT_FOR_MIGRATE;
					break;
				}
			}

			/* If we get here, we have a candidate for migration.  Reorder the 
				state migration queue to put this state at its head.  Otherwise,
				confusion will reign if the state must be sent in multiple 
				pieces. */

			l_remove ( migr_hdr );

			l_insert (  sendStateQ ,  migr_hdr );

		}
		else 
		{

			/* Indicate that this state can now be migrated.  In this case,
				we needn't reorder the state migration queue, as this state
				is already at the head of it.  */

			migr_hdr->migr_flags &= ~WAIT_FOR_MIGRATE;
		}
	}


	statemsg = (Msgh *) l_create ( msgdefsize );

	if ( statemsg == NULL )
		return;

	i = 0;

	if ( migr_hdr->state->segno == 0 )
	{
		segment = (Byte *) migr_hdr->state;

		if ( migr_hdr->state->pktno == 0 )
{
			migr_hdr->waiting_for_ack = TRUE;
			migr_hdr->migr_flags |= STATE_MOVING;
}

	}
	else
	{
		for ( k = migr_hdr->state->segno; ; i++ )
		{
			if ( migr_hdr->state->address_table[i] != NULL )
				k--;

			if ( k == 0 )
				break;
		}
		segment = (Byte *) migr_hdr->state->address_table[i++];
	}

	if ( segment == DEFERRED )
	{
		seglen = (int)DEFERRED;

		migr_hdr->state->no_pkts = 1;

		packet = (Byte *) (&segment);

		lenpkt = sizeof ( segment );
	}
	else
	{
		seglen = l_size(segment);

		migr_hdr->state->no_pkts = ( seglen + pktlen - 1 ) / pktlen;

		offset = pktlen * migr_hdr->state->pktno;

		packet = segment + offset;

		lenpkt = seglen - offset;

		if ( lenpkt > pktlen )
			lenpkt = pktlen;
	}


	make_static_msg ( 
						statemsg, 
						STATEMSG, 
						ocb->name, 
						migr_hdr->state->sndtim, 
						ocb->name, 
						migr_hdr->time_to_deliver, 
						lenpkt, 
						packet 
					);

	statemsg->pktno  = migr_hdr->state->pktno;
	statemsg->no_pkts = migr_hdr->state->no_pkts;
	statemsg->seglen = seglen;
	statemsg->segno = i;
	statemsg->flags |= SYSMSG;


	/* Use the migration header's flags to determine if the state is migrating
		or is a pre-interval state.  Who knows what may have happened to the
		ocb runstat in the meantime? */

	ocb->migrStatus |= (Byte) SENDSTATE;
	if ( migr_hdr->migr_flags & MIGRATING )
	{
		statemsg->flags |= MOVING;
		statemsg->rcvtim = ocb->phase_begin;
	}

	{
		extern int mlog, node_cputime;

		if ( mlog )
		{
#ifdef MICROTIME
			MicroTime ();
#else
#ifdef MARK3
			mark3time ();
#endif
#ifdef BBN
			butterflytime ();
#endif
#endif
			statemsg->cputime = node_cputime;
		}
	}

	migr_hdr->acksExpected++;

	sndmsg ( statemsg, sizeof(Msgh) + lenpkt, migr_hdr->to_node );

	migr_hdr->state->pktno++;

	if ( migr_hdr->state->pktno == migr_hdr->state->no_pkts )
	{
		migr_hdr->state->pktno = 0;

		migr_hdr->state->segno++;

		if ( migr_hdr->state->segno == migr_hdr->state->no_segs )
		if ( migr_hdr->state->no_segs > 1 || migr_hdr->state->no_pkts > 1 )
		{
			if ( migr_hdr->state->tot_pkts != migr_hdr->acksExpected )
			{
				_pprintf("send_state_from_q: tot_pkts %d != acksExpected %d for state %x\n",migr_hdr->state->tot_pkts, migr_hdr->acksExpected, migr_hdr->state );
				tester();
			}

			migr_hdr->waiting_for_done = TRUE;
		}
	}
}  /* send_state_from_q */

typedef struct
{
	Name name;
	VTime svt;
	int AckSndNode;
	int AckRcvNode;

}  MigrAckInfo;

FUNCTION recv_state_done ( msg )
	Msgh * msg;
{
	State_Migr_Hdr * migr_hdr;
	Ocb * o;
	MigrAckInfo *Ackmsg;

  Debug

	migr_hdr = (State_Migr_Hdr *) l_next_macro ( sendStateQ );

	Ackmsg = ( MigrAckInfo * ) ( msg + 1 );

	if ( l_ishead_macro ( migr_hdr ) )
	{
		twerror ( "recv_state_done: no states in migration queue\n");
		printf("object %s, Vtime %f, acking node %d\n",Ackmsg->name,
				Ackmsg->svt.simtime, Ackmsg->AckSndNode );
		tester();
	}

	/*  It's OK to get a state done message only if the state is 
		waiting_for_done or it consists of only one segment of only one
		packet. */

	if ( migr_hdr->waiting_for_done != TRUE  &&
		 ( migr_hdr->state->no_segs >1 || migr_hdr->state->no_pkts >1) )
	{
		twerror("recv_state_done: premature done signal\n");
		printf("object %s, Vtime %f, acking node %d\n",Ackmsg->name,
				Ackmsg->svt.simtime, Ackmsg->AckSndNode );
		tester ();
	}

	o = (Ocb *) migr_hdr->state->ocb;

	/* The moving state can make a gvt contribution if it is a pre-interval
		non-migrating state, since it will cause a rollback at the 
		destination end.  It will roll back the destination phase to its
		delivery time, which is the same as the sending phase's phase end,
		so that time is the gvt contribution that should be made. */

	if ( ( migr_hdr->migr_flags & PRE_INTERVAL ) &&
		 !( migr_hdr->migr_flags & MIGRATING ) &&
		 ltVTime ( migr_hdr->time_to_deliver, min_msg_time ) )
	{
		min_msg_time = migr_hdr->time_to_deliver;
	}

	if ( ltVTime ( min_msg_time, gvt ) )
	{
		twerror("recv_state_done: min_msg time %f set before gvt %f\n",
					min_msg_time.simtime, gvt.simtime);
		printf("object %s, Vtime %f, acking node %d\n",Ackmsg->name,
				Ackmsg->svt.simtime, Ackmsg->AckSndNode );
		tester();
	}

	o->migrStatus &= (Byte) ~SENDSTATE;
	l_remove ( migr_hdr );

	migr_hdr->acksReceived++;

	/* If we've gotten all the acks by the time the done message comes
		in, destroy the migration header.  Otherwise, move it into a
		queue of migration headers for completed moves with outstanding
		acks. */

	if ( migr_hdr->acksReceived < migr_hdr->acksExpected )
	{
		char buff[MINPKTL];

_pprintf("putting migr header %x into statesMovedQ\n",migr_hdr);

		sprintf ( buff, "	State to MovedQ, node %d object %s time %f, %d, %d  Ack Exp %d Ack Rcv %d\n",
			tw_node_num, o->name, migr_hdr->state->sndtim.simtime,
			migr_hdr->state->sndtim.sequence1,migr_hdr->state->sndtim.sequence2,
			migr_hdr->acksExpected, migr_hdr->acksReceived );

		send_to_IH ( buff, strlen ( buff ) + 1, MIGR_LOG );

		migr_hdr->time_to_deliver = migr_hdr->state->sndtim;
		destroy_state ( migr_hdr->state );
		l_insert ( statesMovedQ, migr_hdr );
	}
	else
	{
		destroy_state ( migr_hdr->state );
		l_destroy ( migr_hdr );
	}


	states_to_send--;   /* done sending state */

	send_ocb_from_q ();
}

FUNCTION recv_state_ack ( msg )
	Msgh * msg;
{
	State_Migr_Hdr * migr_hdr;
	MigrAckInfo * Ackmsg;

  Debug

	Ackmsg = ( MigrAckInfo * ) ( msg + 1 );

	migr_hdr = (State_Migr_Hdr *) l_next_macro ( sendStateQ );

	/* If there isn't a migration header, or the migration header at the
		front of the queue isn't for this phase, look in the statesMovedQ
		for this header. */

	if ( l_ishead_macro ( migr_hdr ) || 
		 strcmp ( Ackmsg->name, migr_hdr->name) != 0 || 
		 neVTime (migr_hdr->state->sndtim, Ackmsg->svt)  ) 
	{

		/* If findInMovedQ() is zero, then the state in question isn't
			in the statesMovedQ, either. */

_pprintf("calling findInMovedQ, name %s, svt %f\n", Ackmsg->name, Ackmsg->svt.simtime);
		if ( findInMovedQ ( msg ) == 0 )
		{
			twerror("recv_state_ack: no states in migration queue\n");
			printf("	object %s, Vtime %f, acking node %d\n",Ackmsg->name,
					Ackmsg->svt.simtime, Ackmsg->AckSndNode );
			printf("	msg %x\n", msg );
			tester();
		}
		else
			return;
	}

	migr_hdr->waiting_for_ack = FALSE;
	migr_hdr->acksReceived++;

	send_state_from_q ();       /* send the rest */
}

FUNCTION recv_state_nak ( msg )
	Msgh * msg;
{
	State_Migr_Hdr * migr_hdr;
	MigrAckInfo *Ackmsg;

  Debug

	Ackmsg = ( MigrAckInfo * ) ( msg + 1 );

	stateNaksRecv++;

	migr_hdr = (State_Migr_Hdr *) l_next_macro ( sendStateQ );

	if ( l_ishead_macro ( migr_hdr ) )
	{

		twerror("recv_state_nak: no states in migration queue\n");
		printf("object %s, Vtime %f, acking node %d\n",Ackmsg->name,
				Ackmsg->svt.simtime, Ackmsg->AckSndNode );
		tester();
	}

	migr_hdr->waiting_for_ack = FALSE;
	migr_hdr->waiting_for_done = FALSE;

	migr_hdr->state->segno = 0;
	migr_hdr->state->pktno = 0;
	migr_hdr->acksExpected = 0;
	migr_hdr->acksReceived = 0;

#ifdef DLM
	resendState = FALSE;        /* don't resend til next GVT click */
#endif

#if 0
	_pprintf("Backing off state send for %s\n",migr_hdr->state->ocb->name);
#endif

#if 0
	/*  Now check to see if we want to reorder the state queue.  This queue
		is not ordered when states are put into it, because the state at the
		front might be in the middle of being moved.  States already being
		moved must remain at the front of the queue until they are completely
		moved.  At this point, however, the receival of a nak indicates that
		the state at the head of the queue is not in the midddle of a move,
		so it can be shuffled.  The ordering is done by characteristic virtual
		time. */

	/*	If'd out because this routine may improperly misorder pre-interval
		states in the send queue. The problem only occurs when a state
		movement is nak'd, there are 2 pre-interval states going to the
		same phase, and the later generated one would be ordered before
		the earlier generated one.*/

	reorderStateMigrQ ();
#endif

#if 0
	/*  This causes a node to restart a state send immediately upon receipt
		of a nak.  It is probably better to let the node go off and do
		something else before trying again, as the most likely reason for
		the nak was that insufficient memory was available to receive the
		state.  The main loop of TWOS will eventually call send_state_from_q
		again.  */

	send_state_from_q ();       /* try again */
#endif
}

FUNCTION recv_state ( msg )

	Msgh * msg;
{
	Objloc * location;
	Ocb * ocb;
	State * state;
	Byte * segment;
	Byte * addr;
	int offset, i, j;

  Debug

	location = GetLocation ( msg->rcver, msg->rcvtim );

	if ( location == NULL )
	{
/*  The state has beaten the initial migration message.  The code is set up
		to avoid this condition, in most cases, but odd timing can still
		cause it to happen, at least in theory.  Sending a nak and having
		a retransmission later should fix the problem. */

		twerror ( "recv_state F can't find %s at %f", msg->rcver, msg->rcvtim.simtime );
		send_state_nak ( msg, NULLOCB );
		return;
	}

	if ( location->node != tw_node_num )
	{

/* Bug fix to deal with pending list having brought in a stale cache entry. */

		RemoveFromCache(msg->rcver,msg->rcvtim);

		location = GetLocation ( msg->rcver, msg->rcvtim );


		/*  A NULL location means it hasn't shown up yet, so nak and return. */

		if ( location == NULL )
		{
_pprintf("Sending a state nak because location is null\n");
			send_state_nak ( msg, NULLOCB );
			return;
		}
		else

		/*  But if the local object location info thinks the phase is on
				another node, there's a serious problem. */

		if ( location->node != tw_node_num )
		{

			twerror ( "recv_state F bad location for %s at %f\n", msg->rcver, 
						msg->rcvtim.simtime );
			tester ();
			return;
		}
	}

	ocb = location->po;

	state = ocb->rstate;

	if ( msg->segno == 0 )
	{
		segment = (Byte *) state;
	}
	else
	{
		i = msg->segno - 1;

		/*  If rstate is NULL at this point, we have naked a state while a
				packet is still in transit.  The state will be completely
				resent.  This packet should merely be ignored. */

		if ( state == NULL )
		{
#if 0
			_pprintf("recv_state: %s NULL state for segment other than 0\n", 
						ocb->name );
#endif
			return;
		}

#ifdef PARANOID
		if ( state->address_table == NULL )
		{
				twerror("recv_state: Address table not set up in time for %s, state %x\n",
						ocb->name, state);
				tester();
		}

		if (ocb->rstate == NULL)
			{
			_pprintf("recv_state: rstate is NULL, object %s, i = %d, msg = %x\n",ocb->name,i,msg);
			tester();
			}
#endif PARANOID

		segment = (Byte *) state->address_table[i];
	}

	if ( segment == NULL )
	{
		if ( msg->seglen == (int)DEFERRED )
		{
			segment = DEFERRED;
		}
		else
		{
			segment = m_create ( msg->seglen, msg->rcvtim, NONCRITICAL );

#if 0
			if ( segment == NULL )
			{
				State * preInt;

				/*  Bummer.  We don't have, and can't make, enough memory
						to handle the incoming segment.  In the special case
						that the segment is a non-migrating pre-interval
						state, perhaps something can be done.  We can try
						freeing our existing pre-interval state immediately,
						in the hopes that it will release a large enough
						chunk of memory. */

				preInt = fststate_macro ( ocb );

				/* If we're moving a pre-interval state, and we have enough
						of it here to examine its time, and we already have
						some other state in the ocb's state queue, and that
						state is a pre-interval state . . . */

				if ( state != NULLSTATE &&
					 ltVTime ( state->sndtim, ocb->phase_begin ) &&
					 preInt != NULL && 
					 ltVTime ( preInt->sndtim, ocb->phase_begin ) ) 
				{

#if 0
_pprintf("recv_state: destroying %s's preinterval state to make room\n",
				ocb->name );
#endif

					l_remove ( preInt );        /* take state out of list */

					if ( preInt == ocb->cs )
						ocb->cs = NULL;

					destroy_state ( preInt );   /* release all its memory */

					/*  Now try again. */

					segment = m_create ( msg->seglen, msg->rcvtim, NONCRITICAL);
				}
			}
#endif

			/* If emergency measures haven't helped, undo any state migration
				already done and send a nak.  The state will be resent later. 
				*/

			if ( segment == NULL )
			{
				if ( ocb->rstate != NULL )
				{
					destroy_state ( ocb->rstate );
					ocb->rstate = NULL;
#if 0
  _pprintf("recv_state: out of memory-- %s seg %d pkt %d\n",ocb->name,msg->segno,msg->pktno);
#endif
				}

				send_state_nak ( msg, ocb );

				return;
			}
		}

		if ( msg->segno == 0 )
		{
			state = (State *) (msg + 1);

			if ( state->address_table != NULL )
			{

				state->address_table = (Address *)
					m_create ( state->addrTableSize * sizeof(Address),
					msg->rcvtim, NONCRITICAL );

				if ( state->address_table == NULL )
				{
					l_destroy ( segment );
					send_state_nak ( msg, ocb );
					return;
				}

				clear ( state->address_table,
					l_size(state->address_table) );
			}

#if 0
		_pprintf("address_table %s %x\n",ocb->name,state->address_table);
#endif

			state = (State *) segment;

			ocb->rstate = state;        /* state under construction */
			ocb->migrStatus |= (Byte) RECVSTATE;
		}
		else
		{
			state->address_table[i] = (Address) segment;
		}
	}

	if ( segment != DEFERRED )
	{
		offset = pktlen * msg->pktno;

		entcpy ( segment+offset, msg+1, msg->txtlen );
	}

	if ( state->tot_pkts > 1 )
	{
		send_state_ack ( msg, ocb );
	}

	state->tot_pkts--;

/*  The state has been totally received. */

	if ( state->tot_pkts == 0 )
	{
		ocb->rstate = NULL;

		ocb->migrStatus &= (Byte) ~RECVSTATE;
		send_state_done ( msg, ocb );

#if PARANOID
		validState(state);
#endif

		if ( msg->flags & MOVING )
		{
			State * s = fststate_macro ( ocb );

/*  If the send time of this incoming state is at or after svt, then the
		phase is going to discard it, anyway, when it re-executes at svt.
		Get rid of it now.  */

			if ( geVTime ( state->sndtim, ocb->svt ) )
			{
/*
_pprintf("migrating state at or after svt for %s\n", ocb->name);
_pprintf("migrating state is %x, sndtim = %f, svt = %f\n",state, state->sndtim.simtime, ocb->svt.simtime );
*/
				destroy_state ( state );
			}
			else
			if ( ltVTime ( state->sndtim, ocb->phase_begin )
			&& ( s != NULL && ltVTime ( s->sndtim, ocb->phase_begin ) )  )
			{
#if 0
_pprintf("pre-interval state beats migrating pre-interval state for %s\n",
				ocb->name);
_pprintf("migrating state is %x, sndtim = %f\n",state, state->sndtim );
				tester();
#endif
			}
			else
			{
				put_state_in_sq ( ocb, state );
				state->ocb = ocb;
			}
									            /* Don't replace the state
									            /* before phase_begin with
									            /* a moving state because
									            /* it is the oldest one. */
			ocb->num_states--;

			if ( ( ocb->num_states + ocb->num_imsgs + ocb->num_omsgs ) == 0 )
			{
				rollback_phase ( ocb, msg );
			}
		}
		else
		{
			ocb->stats.stforw++;
			fstatein++;

/*
			if (ltVTime(state->sndtim,gvt))
			{
				_pprintf("forwarded state to %s at time %2f, less than gvt %2f\n",
						ocb->name, state->sndtim.simtime, gvt.simtime);
			}
*/

			put_state_in_sq ( ocb, state );
			state->ocb = ocb;
			rollback_state ( ocb, state->sndtim );
		}
	}
}  /* recv_state */

FUNCTION rollback_phase ( ocb, msg )

	Ocb * ocb;
	Msgh * msg;
{
  Debug

	if ( ocb->runstat == BLKINF || ocb->runstat == BLKSTATE )
	{   /* if blocked, restart at current time */
		rollback ( ocb, ocb->svt );
	}

/*  Should this happen before the call to rollback()? */

	ocb->phase_limit = ocb->next_limit;

	if ( ltVTime ( ocb->phase_limit, ocb->phase_end ) )
	{
		ocb->migrStatus &= (Byte) ~RECVVTIME;
		send_vtime_done ( msg, ocb );
	}
	else
	{
		ocb->migrStatus = (Byte) MIGRDONE;
		send_phase_done ( msg, ocb );
	}
}

FUNCTION rollback_state ( ocb, statetime )

	Ocb * ocb;
	VTime statetime;
{
	register Msgh * msg;

  Debug

	msg = fstimsg_macro ( ocb );

	if ( msg )
	{
		rollback ( ocb, msg->rcvtim );
	}
	else
		if ( neVTime ( ocb->phase_end, ocb->phase_limit ) )
		{
			VTime rollbackTime;

			/* The next vtime is being migrated at this moment, and any of
				its input messages haven't arrived yet.  We must set the
				phase's svt so that when the vtime has been fully migrated
				the phase will re-execute it. */

			if ( ltVTime ( ocb->phase_limit, ocb->phase_begin ) )
				rollbackTime = ocb->phase_begin;
			else
				rollbackTime = ocb->phase_limit;

			rollback ( ocb, rollbackTime );
		}
		else
			if ( neVTime ( ocb->phase_end, posinfPlus1) )
			{
				State * s;

				/*  This is a phase with no work to do and another later phase it
						must feed.  Having just received a pre-interval state,
						this phase must now send it to the next phase, since it
						has no local events to process. */

				s = fststate_macro ( ocb );

				if ( s != lststate_macro ( ocb ) )
				{
					twerror ( "rollback_state: empty input queue, but last state not equal first state for %s\n",ocb->name);
					tester ();
				}

				if ( s == NULLSTATE )
				{
					twerror ( "rollback_state: trying to forward null state for %s\n",
								ocb->name );
					tester();
				}

				send_state_copy ( s, ocb );
			}

}

FUNCTION put_state_in_sq ( ocb, state )

	Ocb * ocb;
	State * state;
{
	State * s, * n;

  Debug

/* Pre-Interval State...        */

	if ( ltVTime ( state->sndtim, ocb->phase_begin ) )
	{
		s = fststate_macro ( ocb );

		if ( s != NULL && ltVTime ( s->sndtim, ocb->phase_begin ) )
		{
			l_remove ( s );
			destroy_state ( s );
			if ( ocb->cs == s )
				ocb->cs = NULL;
		}

		/*  This phase may be part of a dynamically created object, and
				the earlier phase may have rolled back the creation or
				processed a dynamic destroy.  In such cases, this incoming
				preinterval state carries in its type field the information
				that the object should be of type NULL at this phase's
				phase_begin.  Therefore, when putting a pre-interval state
				in the state queue, check to see if its type matches the
				existing type field in the phase's ocb, and change the ocb
				type field if necessary.  The printf code is temporary,
				to ensure that the code is working.  The rest of the code
				is permanent. */

		if ( state->otype != ocb->typepointer )
		{

#if 0
			_pprintf("phase %s changing type from %s to %s because of arriving pre-interval state\n",

				ocb->name, ocb->typepointer->type, state->otype->type );
#endif
		}

		ocb->typepointer = state->otype;
	}

	s = ocb->sqh;

	/*  Find the correct position in the state queue for the incoming
		state.  Starting at the beginning, cycle through the states
		until you either find a state with a greater time, you find a 
		state with the same time but a greater state type (e.g., EVENT
		is greater than CREATE, and less than DESTROY), or reach the end
		of the queue. */

	for ( n = nxtstate_macro ( s ); n; n = nxtstate_macro ( s ) )
	{
		if ( gtVTime ( n->sndtim, state->sndtim ) ||
			 ( eqVTime ( n->sndtim, state->sndtim) && 
				n->stype > state->stype ) )
			break;

		if ( eqVTime ( n->sndtim, state->sndtim )  &&
			n->stype == state->stype )
		{
			_pprintf("Trying to insert duplicate state (%x) into %s at %f\n",
								state,ocb->name,state->sndtim.simtime);
			tester();
		}

		s = n;
	}

	l_insert ( s, state );
}


FUNCTION send_state_ack ( msg, ocb )

	Msgh * msg;
	Ocb * ocb;
{
	Msgh * p;
	MigrAckInfo	* Ackmsg;

  Debug

	p = sysbuf ();

	Ackmsg = (MigrAckInfo *) ( p + 1 );

	strcpy( Ackmsg->name, ocb->name );
	Ackmsg->svt = ocb->rstate->sndtim;
    Ackmsg->AckSndNode = tw_node_num;

	Ackmsg->AckRcvNode = ocb->prev_node;

	sysmsg ( STATEACK, p, sizeof(MigrAckInfo), ocb->prev_node );
}

FUNCTION send_state_nak ( msg, ocb )

	Msgh * msg;
	Ocb * ocb;
{
	Msgh * p;
	MigrAckInfo	* Ackmsg;
	int prev_node;

  Debug
	stateNaksSent++;

	p = sysbuf ();

	Ackmsg = (MigrAckInfo *) ( p + 1 );

	strcpy( Ackmsg->name, msg->rcver );
	Ackmsg->svt = msg->rcvtim;
    Ackmsg->AckSndNode = tw_node_num;

	if ( ocb == NULLOCB)
		prev_node = msg->low.from_node;
	else
		prev_node = ocb->prev_node;

	Ackmsg->AckRcvNode = prev_node; 

	sysmsg ( STATENAK, p, sizeof(MigrAckInfo), prev_node );
}

FUNCTION send_state_done ( msg, ocb )

	Msgh * msg;
	Ocb * ocb;
{
	Msgh * p;
	MigrAckInfo	* Ackmsg;

  Debug

	p = sysbuf ();

	Ackmsg = (MigrAckInfo *) ( p + 1 );

	strcpy( Ackmsg->name, ocb->name );
	Ackmsg->svt = ocb->phase_limit;
    Ackmsg->AckSndNode = tw_node_num;

	Ackmsg->AckRcvNode = ocb->prev_node; 

	sysmsg ( STATEDONE, p, sizeof(MigrAckInfo), ocb->prev_node );
}

FUNCTION split_object_cmd ( name, stime )

	char * name;
	STime * stime;
{
	Objloc * location;
	Ocb * ocb;
	VTime vtime;

  Debug

	vtime = newVTime ( *stime, 0, 0 );

	location = GetLocation ( name, vtime );

	if ( location == NULL )
	{
		twerror ( "split_object_cmd E can't locate object %s with time %f",
				name, *stime );
		return;
	}

	if ( location->node != tw_node_num )
	{
		twerror ( "split_object_cmd E object %s with time %f is on node %d",
				name, *stime, location->node );
		return;
	}

	ocb = location->po;

	split_object ( ocb, vtime );
}

FUNCTION Ocb * split_object ( ocb, vtime )

	Ocb * ocb;
	VTime vtime;
{
	Ocb * ocb2;
	State * s, * preInt, * stateCopy;
	Msgh * m;
	Int HomeNode;
	char        oldStartVts[20];
	char        oldEndVts[20];
	char        newStartVts[20];
	char        newEndVts[20];

  Debug

	if ( eqVTime ( ocb->phase_begin, vtime )
	||   eqVTime ( ocb->phase_end, vtime ) )
	{
		twerror ( "split_object E invalid to split object %s from time %f to time %f at time %f", ocb->name, ocb->phase_begin.simtime, ocb->phase_end.simtime, vtime.simtime );
		tester();
		return ( NULL );
	}


	/* This test ensures that we do not split an object that is still 
		migrating into the node hosting it.  */

	if ( neVTime ( ocb->phase_end, ocb->phase_limit ) )
	{

		twerror ( "split_object E trying to split object %s at time %f before phase limit %f reaches phase end %f\n", ocb->name, vtime.simtime, ocb->phase_limit.simtime, ocb->phase_end.simtime);
		twerror ( "     phase limit -   %f      %d      %d\n",
						ocb->phase_limit.simtime, ocb->phase_limit.sequence1,
						ocb->phase_limit.sequence2);
		twerror ( "     phase end -     %f      %d      %d\n",
						ocb->phase_end.simtime, ocb->phase_end.sequence1,
						ocb->phase_end.sequence2);
		tester();
		return ( NULL );
	}

	/*  This test ensures that we do not try to split a null object. */

	if ( ocb->typepointer == &type_table[1] )
	{
/*
		twerror ( "split_object: trying to split null object %s at time %f\n",
				ocb->name, vtime.simtime );
*/
		return ( NULL );
	}

	if ( ocb->runstat == BLKPKT )
	{
		/* Don't try to split an object trying to send a message.  The code
				probably works, but might not, especially in the case of
				dynamic creation and destruction.  So just back off, instead.
		*/

		return ( NULL );
	}


	/*  Don't try to split ocbs that currently have an erroneous state.
		There is a string associated with that erroneous state and the
		state migration code is not prepared to handle moving that string.  
		Moreover, unless the error state gets rolled back, the ocb won't
		do any work on its new node, anyway. */

	for ( s = fststate_macro ( ocb ); s; s = nxtstate_macro ( s ) )
	{
		if ( s->serror != NOERR )
		{

#if 0
			_pprintf("split_phase: Aborting split of %s because of error state in state queue \n", ocb->name );
#endif

			return ( NULL );
		}
	}

	/* Now make a copy of the state to be used as a pre-interval state
		for the new phase.  Do it here because this attempt can fail.
		If it does, we need to abort the migration, and it will be a lot
		easier to do that before we've done a lot of other stuff. */

	for ( s = fststate_macro ( ocb ); s; s = nxtstate_macro ( s ) )
	{
		if ( geVTime ( s->sndtim, vtime ) )
			break;
	}

	/*  If s is NULLSTATE, there are no states to be split off, except, 
		perhaps, a pre-interval state.  So choose the last state of the
		ocb's state queue as the pre-interval state, in this case.  On the
		other hand, if s points to a state, that state is the first one 
		in the queue to go to the later phase, so the previous state (if
		any) is a pre-interval state that should be copied. */

	if ( s == NULLSTATE )
		preInt = lststate_macro ( ocb );
	else
		preInt = prvstate_macro ( s );

	if ( preInt != NULLSTATE )
	{
		stateCopy = copystate ( preInt );

		if ( stateCopy == ( State * ) -1 )
		{

			/* In this case, not only could we not make a state copy, but
				we couldn't even make the original copy of the pre-interval
				state a whole copy, with physical copies of all segments.
				Nothing can be done to complete this split, so just abort
				it. */

#if 0
			_pprintf( "split_object: split of %s aborted because of too little memory\n", 
				ocb->name );
#endif

			return ( NULL );
		}
	}

#ifdef DLM
	if ( migrGraph )
	{
		ttoc1 (oldStartVts, ocb->phase_begin );
		ttoc1 (oldEndVts, ocb->phase_end );
		ttoc1 (newStartVts, vtime );
		ttoc1 (newEndVts, ocb->phase_end );
	}
#endif

	ocb2 = mkocb ();

	/* mkocb() will return NULL if it couldn't lay hands on enough memory
		to make an ocb data structure.  In that case, just back off this
		split.  If we've already allocated a state, deallocate it. */

	if ( ocb2 == ( Ocb * ) NULL )
	{
		if ( stateCopy != NULL )
		{
			destroy_state ( stateCopy );
		}

		return ( NULL );
	}

	strcpy ( ocb2->name, ocb->name );
	ocb2->typepointer = ocb->typepointer;
	ocb2->libPointer = ocb->libPointer;
	ocb2->oid = ocb->oid;
	ocb2->phase_begin = vtime;
	ocb2->phase_end = ocb->phase_end;
	ocb2->crcount = 1;
	ocb2->generation = 0;
#ifdef SOM
	/* This code will not be correct if we do not use the NEAR_FUTURE
		form of splitting. */

	/* The new phase inherits the causally connected Ept from the earlier
		phase.  But the new phase has not yet done any work (under NEAR_FUTURE
		splitting), so all its work fields get set to 0. */

	ocb2->Ept = ocb->Ept;
	ocb2->comEpt = ocb->comEpt;
	ocb2->lastComEpt = ocb->lastComEpt;
	ocb2->work = 0;
	ocb2->comWork = 0;
	ocb2->lastComWork = 0;
#endif SOM

	/* Bug fix necessary to let two adjacent phases of same object operate
		on one node.  Until the later phase starts migrating, it should
		not be held up by phase_limit, a field intended to make sure
		it doesn't execute past the time currently being migrated. */

	ocb2->phase_limit = ocb2->phase_end;

	if ( ltVTime ( ocb->svt, vtime ) )
	{
		ocb2->svt = posinf;
		ocb2->control = EDGE;
		ocb2->runstat = BLKINF;

		l_insert ( l_prev_macro ( _prqhd ), ocb2 );
	}
	else
	{
		ocb2->svt = ocb->svt;
if ( ltVTime ( ocb2->svt, gvt ))
{
	twerror ( "split_object: ocb %s svt %f set to earlier than gvt %f\n",
				ocb2->name, ocb2->svt.simtime, gvt.simtime);
	tester();
}
		ocb2->control = ocb->control;
		ocb2->runstat = ocb->runstat;
		ocb2->cs = ocb->cs;
		ocb2->ci = ocb->ci;
		ocb2->co = ocb->co;
		ocb2->sb = ocb->sb;
		ocb2->stk = ocb->stk;
		ocb2->msgv = ocb->msgv;
		ocb2->centry = ocb->centry;
		ocb2->ecount = ocb->ecount;
		ocb2->pvz_len = ocb->pvz_len;
		ocb2->argblock = ocb->argblock;
		ocb2->eventTimePermitted = ocb->eventTimePermitted;
		ocb2->eventsPermitted = ocb->eventsPermitted;

		l_insert ( ocb, ocb2 );

		ocb->svt = posinf;
		ocb->control = EDGE;
		ocb->runstat = BLKINF;
		ocb->cs = NULL;
		ocb->ci = ocb->co = NULL;
		ocb->sb = NULL;
		ocb->stk = NULL;
		ocb->msgv = NULL;

		l_remove ( ocb );
		l_insert ( l_prev_macro ( _prqhd ), ocb );

		if ( xqting_ocb == ocb )
			xqting_ocb = ocb2;
	}

	/* Remove the object's entry from the local cache.  Do not change the
		home list.  Despite the object's being split, the old home list entry
		will still correctly direct messages, since both parts are still on
		the same node.  The entry will be changed if either phase is migrated.
	*/

	RemoveFromCache(ocb->name,vtime);

	ocb->phase_end = vtime;
	ocb->next_node = tw_node_num;

	/* s was set earlier in this routine, but the copystate() routine
		could have invoked message sendback, which could free
		the state s points to.  s does not change, but what it points to
		could be on the free list, or reallocated somewhere else.  So we
		have to set s again.  This duplication of effort could have been
		avoided had we not made the state copy until right after the split,
		but then we would need to be able to back out of the split if the
		state copy failed.  That's lots of code, though it's not very
		complicated, so we'll live with the duplication, for the moment. */

	for ( s = fststate_macro ( ocb ); s; s = nxtstate_macro ( s ) )
	{
		if ( geVTime ( s->sndtim, vtime ) )
			break;
	}

	if ( s != NULL )
	{
		split_list ( ocb->sqh, ocb2->sqh, s );
	}
	else
	{
/*
		_pprintf("no state queue to split for %s, time %f\n", ocb->name, vtime.simtime);
*/
	}

	/* The critical path algorithm can put truncated states into a special
		queue.  If that algorithm is being used, that queue must be split
		between the two phases. */

	if ( critEnabled )
	{
		truncState *t;

		for ( t = l_next_macro ( ocb->tsqh ); ! l_ishead_macro ( t );
				t = l_next_macro ( t ) )
		{
			if ( geVTime ( t->sndtim, vtime ) )
				break;
		}

		if ( t != NULL )
		{
			split_list ( ocb->tsqh, ocb2->tsqh, t );

		}
	}

	/*  If there is a pre-interval state for the later phase, we made a
		copy of it earlier in this routine.  Now put the copy in the 
		state queue of the new phase. */

	if ( preInt != ( State * ) NULL )
	{
		/*  The type of the earlier phase should be the same as the type
				of the last state of that phase.  This might be important
				if the split point moves dynamic create or destroy messages
				into the later phase. */

		ocb->typepointer = preInt->otype;

		if ( stateCopy == NULL )
		{
			l_remove ( preInt );

			stateCopy = preInt;

			ocb->loststate = 1;

#if 0
			_pprintf ( "split_object: lost a state for %s\n", ocb->name );
#endif

			if ( ocb->runstat == READY )
				ocb->runstat = GOFWD;
		}
		else
		{
			ocb->last_sent = preInt;
			ocb->out_of_sq = 0; /* should be OK -- JUMP FORWARD stuff */
		}


		stateCopy->ocb = (Byte *) ocb2;

		l_insert ( ocb2->sqh, stateCopy );

	}

	for ( m = fstimsg_macro ( ocb ); m; m = nxtimsg_macro ( m ) )
	{
		if ( geVTime ( m->rcvtim, vtime ) )
			break;
	}

	if ( m != NULL )
	{
		split_list ( ocb->iqh, ocb2->iqh, m );
	}

	for ( m = fstomsg_macro ( ocb ); m; m = nxtomsg_macro ( m ) )
	{
		if ( geVTime ( m->sndtim, vtime ) )
			break;
	}

	if ( m != NULL )
	{
		split_list ( ocb->oqh, ocb2->oqh, m );
	}

#ifdef DLM
	if ( migrGraph )
	{
		int icount, ocount, scount;
		char buff[MINPKTL];

		icount = ocount = scount = 0;

		for (s = fststate_macro (ocb2); s; s = nxtstate_macro (s))
		{
			scount++;
		}

		for (m = fstimsg_macro (ocb2); m; m = nxtimsg_macro (m))
		{
			icount++;
		}

		for (m = fstomsg_macro (ocb2); m; m = nxtomsg_macro (m))
		{
			ocount++;
		}

		sprintf ( buff, "Split %s %s %s %s %s %d %d %d %d\n", ocb->name, 
				oldStartVts, oldEndVts, newStartVts, newEndVts, tw_node_num, 
				icount, ocount, scount );

		send_to_IH ( buff, strlen ( buff ) + 1, MIGR_LOG );
	}
#endif

	return ( ocb2 );
}

split_list ( q1, q2, elem )

	List_hdr * q1;
	List_hdr * q2;
	List_hdr * elem;
{
	q1--;       /* point to list headers */
	q2--;
	elem--;

	q2->prev = q1->prev;
	q2->next = elem;

	q1->prev = elem->prev;
	q1->prev->next = q1;

	elem->prev = q2;
	q2->prev->next = q2;
}


/*      The protocol for moving a phase to another node is as follows.
		First take the ocb for the phase to be moved out of the main
		ocb list and add it to the sendOcbQ. Next update the next_node
		field of the immediately preceding phase (assuming for now
		that we're only moving the second phase of an object that has
		just been split). We will have to change the way states are
		forwarded from the end of one phase to the beginning of the
		next phase to use object location instead of next_node in the
		ocb. Next notify the home node (for the first time) about the
		existence of the new object phase. This will also have to
		change when object location can handle object migration. Then
		send a MOVEPHASE message to the destination node and wait for
		a PHASEACK message in reply. There is no PHASENAK yet but it
		will probably be necessary. When the PHASEACK message arrives,
		the object's states, input messages and output messages can
		all be sent. There is no sensitivity to the order of arrival
		of these messages because the receiving node has control
		information in the ocb: num_states, num_imsgs and num_omsgs.
		When all of the messages have arrived the receiving node
		sends a PHASEDONE message to the sender. On receipt of the
		PHASEDONE message, the sender can remove the ocb from the
		sendOcbQ and destroy it.
*/

FUNCTION move_phase_cmd ( name, stime, node )

	char * name;
	STime * stime;
	int * node;
{
	Objloc * location;
	VTime vtime;
	Ocb * ocb;

  Debug

	*node = *node % tw_num_nodes;

	if ( *node == tw_node_num )
	{
		return;
	}

	vtime = newVTime ( *stime, 0, 0 );

	location = GetLocation ( name, vtime );

	if ( location == NULL )
	{
		twerror ( "move_phase_cmd E can't locate object %s with time %f",
				name, *stime );
		return;
	}

	if ( location->node != tw_node_num )
	{
		twerror ( "move_phase_cmd E object %s with time %f is on node %d",
				name, *stime, location->node );
		return;
	}

	ocb = location->po;

	move_phase ( ocb, *node );
}

FUNCTION move_phase ( ocb, node )

	Ocb * ocb;
	int node;
{
	State * s;
	Msgh * m;
	Int  HomeNode;

  Debug


	/* Don't permit migration of phases during initialization or termination
		times. */

	if ( node == tw_node_num )
	{
		_pprintf ( "move_phase: trying to move phase %s, %f to its current node\n",
					ocb->name, ocb->phase_begin.simtime );
		tester();
	}

	if ( leVTime ( gvt, neginfPlus1 ) )
	{
		_pprintf ( "move_phase: trying to migrate %s during initialization\n",
				ocb->name );
		tester();
	}

	if ( geVTime ( gvt, posinf ) )
	{
		_pprintf ( "move_phase: trying to migrate %s during termination\n",
				ocb->name );
		tester();
	}

	if ( ocb->runstat == BLKPHASE )
	{
		_pprintf ( "move_phase: ocb %x is BLKPHASE\n", ocb );
		tester ();
	}

   if ( ocb->runstat == BLKPKT )
   {
		/* Safest not to try this migration, as we're not sure the code
				would work correctly in all cases, particularly dynamic
				creation and destruction. */

		return ( NULL) ;
	}

	/*  Don't try to move ocb's that currently have an erroneous state.
		There is a string associated with that erroneous state and the
		state migration code is not prepared to handle moving that string.  
		Moreover, unless the error state gets rolled back, the ocb won't
		do any work on its new node, anyway. */

	for ( s = fststate_macro ( ocb ); s; s = nxtstate_macro ( s ) )
	{
		if ( s->serror != NOERR )
		{

#if 0
			_pprintf("move_phase: Aborting migration of %s because of error state in state queue \n", ocb->name );
#endif

			return ( NULL );
		}
	}

	l_remove ( ocb );

	if ( ocb == xqting_ocb )
	{
		xqting_ocb = NULL;
		setnull ();
	}

	ocb->runstat = BLKPHASE;

	if ( ocb->sb != NULL )
	{
		adjustEffectWork ( ocb, ocb->sb );
		destroy_state ( ocb->sb );
		ocb->eventsPermitted++;
		ocb->sb = NULL;
		l_destroy ( ocb->stk );
		ocb->stk = NULL;
		destroy_message_vector ( ocb );

		/*  If we are running in aggressive cancellation mode, make sure
				that any messages sent for the event we are running get
				cancelled.   The test for control == NONEDGE is probably
				superfluous, as a non-null state buffer implies nonedge
				control, but it's cheap and extra safety.  Moreover,
				the nonedge status is what really should trigger the 
				cancellation, not the presence of a state in the state buffer.  
		*/

		if ( aggressive  && ocb->control == NONEDGE )
			cancel_all_output ( ocb, ocb->svt );
	}

	l_insert ( l_prev_macro ( sendOcbQ ), ocb );

	ocb->migrStatus = MIGRNOTSTARTED;
	ocbs_to_send++;

	migrout++;

	ocb->next_node = node;

	for ( s = fststate_macro ( ocb ); s; s = nxtstate_macro ( s ) )
	{
		ocb->num_states++;
	}

	for ( m = fstimsg_macro ( ocb ); m; m = nxtimsg_macro ( m ) )
	{
		ocb->num_imsgs++;
	}

	for ( m = fstomsg_macro ( ocb ); m; m = nxtomsg_macro ( m ) )
	{
		ocb->num_omsgs++;
	}

/*
	_pprintf (
		"Migrating %d states, %d imsgs, %d omsgs for object %s to node %d\n",
		ocb->num_states,ocb->num_imsgs,ocb->num_omsgs,ocb->name,node );
*/

	if ( migrGraph )
	{
		char startVts[20];
		char endVts [20];
		char buff[MINPKTL];

		ttoc1 (startVts, ocb->phase_begin );
		ttoc1 (endVts, ocb->phase_end );

		sprintf( buff, "Move %s %s %s %d %d %d %d\n", ocb->name, 
				startVts, endVts, node, ocb->num_imsgs, ocb->num_omsgs,
				ocb->num_states );

		send_to_IH ( buff, strlen ( buff ) + 1, MIGR_LOG );
	}

	/* Notify the home node that the phase has moved, and remove the 
		entry for the phase from the local cache. */

	HomeNode = name_hash_function ( ocb->name, HOME_NODE );

	if ( HomeNode == tw_node_num )
	{
		ChangeHLEntry ( ocb->name, ocb->phase_begin, node, ++(ocb->generation));
	}
	else
	{
		RemoteChangeHListEntry ( ocb->name, ocb->phase_begin, HomeNode, node,
				++(ocb->generation) );
		RemoveFromCache ( ocb->name, ocb->phase_begin );
	}

	SendCacheInvalidate ( ocb->name,ocb->phase_begin, node );

	if ( ocbs_to_send == 1 )
		send_ocb_from_q ();

#define PHASE_MOVED 1

	return ( PHASE_MOVED );
}


typedef struct
{
	int oid;
	int num_states;
	int num_imsgs;
	int num_omsgs;
	VTime svt;
	Pointer libPointer;
	int generation;
	int crcount;
	char type[TOBJLEN];
#ifdef SOM
	long    Ept;
	long    comEpt;
	long    lastComEpt;
	long    work;
	long    comWork;
	long    lastComWork;
#endif SOM
}
	PhaseInfo;


FUNCTION send_phase ( ocb )

	Ocb * ocb;
{
	PhaseInfo * phase;
	Msgh * p;
	char buff[MINPKTL];

  Debug
#ifdef PARANOID
	/* Check to make sure that send_phase() has not already been run for this 
		phase. */

	if ( ocb->migrWait & WAITFORACK )
	{
		_pprintf ( "send_phase: send_phase called twice for %s (%f), addr %x\n",
				ocb->name, ocb->phase_begin.simtime, ocb );
		tester();
	}
#endif PARANOID

	p = sysbuf ();

	phase = (PhaseInfo *) ( p + 1 );

	phase->oid = ocb->oid;
	phase->num_states = ocb->num_states;
	phase->num_imsgs = ocb->num_imsgs;
	phase->num_omsgs = ocb->num_omsgs;

	phase->svt = ocb->svt;
if ( ltVTime (phase->svt,gvt))
{
	twerror("send_phase: svt %f set below gvt %f for %s\n",phase->svt.simtime,
		gvt.simtime, ocb->name);
	tester();
}

	phase->libPointer = ocb->libPointer;
	phase->generation = ocb->generation;
	phase->crcount = ocb->crcount;
#ifdef SOM
	phase->Ept = ocb->Ept;
	phase->comEpt = ocb->comEpt;
	phase->lastComEpt = ocb->lastComEpt;
	phase->work = ocb->work;
	phase->comWork = ocb->comWork;
	phase->lastComWork = ocb->lastComWork;
#endif SOM

	entcpy ( phase + 1, &ocb->stats, sizeof (ocb->stats) );

	strcpy ( p->snder, ocb->name );
	strncpy ( phase->type, ocb->typepointer->type, TOBJLEN );
	strcpy ( p->rcver, ocb->name );
	p->sndtim = ocb->phase_begin;
	p->rcvtim = ocb->phase_end;

	sysmsg (MOVEPHASE, p, sizeof(PhaseInfo)+sizeof(ocb->stats), ocb->next_node);

	ocb->migrWait |= (Byte) WAITFORACK;

#ifdef MICROTIME
	MicroTime ();
#else
#ifdef MARK3
	mark3time ();
#endif
#ifdef BBN
	butterflytime ();
#endif
#endif
#ifdef DLM
	sprintf ( buff, "%d %d %d %s %f %d %f %d %d %d %f\n",
		tw_node_num, node_cputime, loadCount, ocb->name, 
		ocb->phase_begin.simtime, ocb->next_node, ocb->svt.simtime,
		ocb->num_states, ocb->num_imsgs, ocb->num_omsgs, gvt.simtime );
#else
	sprintf ( buff, "%d %d %s %f %d %f %d %d %d\n",
		tw_node_num, node_cputime, ocb->name, ocb->phase_begin.simtime,
		ocb->next_node, ocb->svt.simtime,
		ocb->num_states, ocb->num_imsgs, ocb->num_omsgs );
#endif DLM

#ifdef DLM
	if ( !migrGraph )
	{
		send_to_IH ( buff, strlen ( buff ) + 1, MIGR_LOG );
	}
#endif
}

FUNCTION recv_phase_ack ( msg )
	Msgh * msg;
{
	Ocb * ocb;
	MigrAckInfo *Ackmsg;

  Debug

	Ackmsg = ( MigrAckInfo * ) ( msg + 1 );

	ocb = (Ocb *) l_next_macro ( sendOcbQ );

	if ( l_ishead_macro ( ocb ) )
	{

		twerror("recv_phase_ack: no phases in migration queue\n");
		printf("object %s, Vtime %f, acking node %d\n",Ackmsg->name,
				Ackmsg->svt.simtime, Ackmsg->AckSndNode );
		tester();
	}

	ocb->migrWait &= (Byte) ~WAITFORACK;
	ocb->migrStatus = OCBSETUP;

	send_vtime ( ocb );
}

FUNCTION recv_phase_nak ( msg )
	Msgh * msg;
{
	Ocb * ocb;
	MigrAckInfo *Ackmsg;

  Debug

	phaseNaksRecv++;

	Ackmsg = ( MigrAckInfo * ) ( msg + 1 );

	ocb = (Ocb *) l_next_macro ( sendOcbQ );

	if ( l_ishead_macro ( ocb ) )
	{

		twerror("recv_phase_nak: no phases in migration queue\n");
		printf("object %s, Vtime %f, acking node %d\n",Ackmsg->name,
				Ackmsg->svt.simtime, Ackmsg->AckSndNode );
		tester();
	}

	ocb->migrWait &= (Byte) ~WAITFORACK;

#if 0
	_pprintf("recv_phase_nak: send of %s nak'ed\n",ocb->name );
#endif

	send_ocb_from_q ();
}

#ifdef DLM
extern VTime oldgvt1 ;
extern VTime oldgvt2 ;
#endif DLM

FUNCTION VTime migr_min ()
{
	State_Migr_Hdr * migr_hdr;
	Msgh * msg;
	Ocb * ocb;
	VTime min;

  Debug

	min = posinfPlus1;

/*  Non-migrating pre-interval state make a gvt contribution,
		since they can cause rollbacks. */

	if ( sendStateQ )
	for ( migr_hdr = nxtstate_macro ( sendStateQ ); migr_hdr != NULL;
		  migr_hdr = nxtstate_macro ( migr_hdr ) )
	{  /* loop through states of migrating objects */
		ocb = (Ocb *) migr_hdr->state->ocb;
		if ( !( migr_hdr->migr_flags & MIGRATING)  && 
				gtVTime ( min, migr_hdr->time_to_deliver ) )
		{  /* find minimum time */
			min = migr_hdr->time_to_deliver;
		}
	}

/*  For objects in the migration queue, their gvt contribution is their
		svt. */

	if ( sendOcbQ )
	for ( ocb = nxtocb_macro ( sendOcbQ ); ocb != NULL;
		  ocb = nxtocb_macro ( ocb ) )
	{  /* loop through migrating objects */
		if ( gtVTime ( min, ocb->svt ) )
			min = ocb->svt;     /* calculate minimum time */
	}

	if ( ltVTime ( min, gvt ) )
	{
		twerror("migr_min: min %f less than gvt %f\n",min.simtime,gvt.simtime);
		tester();
	}

#ifdef DLM
	if ( oldgvt2.simtime != NEGINF  && 
		eqVTime ( min, oldgvt1 ) && 
		eqVTime ( oldgvt1, oldgvt2 ) )
	{
		_pprintf ( "GVT %f repeats three times, in migr_min() \n",
						min.simtime);
	}
#endif DLM

	return ( min );
}

typedef struct
{
	VTime vt;
	VTime limit;
	int num_states;
	int num_imsgs;
	int num_omsgs;
	char type[TOBJLEN];
}
	VTimeInfo;

FUNCTION send_ocb_from_q ()
{
	Ocb * ocb;

  Debug

	ocb = (Ocb *) l_next_macro ( sendOcbQ );

	if ( ocb == sendOcbQ )
		return;

	if ( ocb->migrWait & WAITFORACK )
		return;

	if ( ocb->migrWait & WAITFORDONE )
		return;

	ocb->migrStatus = (Byte) MIGRSTART;

	send_phase ( ocb );
}

FUNCTION send_vtime ( ocb )

	Ocb * ocb;
{
	State * s;
	Msgh * m, * n;
	VTimeInfo * vtime;
	Msgh * p;
	char buff[MINPKTL];

  Debug

	p = sysbuf ();

	vtime = (VTimeInfo *) ( p + 1 );

	vtime->vt = vtime->limit = ocb->phase_end;

	vtime->num_states = 0;
	vtime->num_imsgs = 0;
	vtime->num_omsgs = 0;

	s = fststate_macro ( ocb );
	m = fstimsg_macro ( ocb );
	n = fstomsg_macro ( ocb );

	if ( s != NULL )
		vtime->vt = s->sndtim;

	if ( m != NULL && ltVTime ( m->rcvtim, vtime->vt ) )
		vtime->vt = m->rcvtim;

	if ( n != NULL && ltVTime ( n->sndtim, vtime->vt ) )
		vtime->vt = n->sndtim;

	if ( s != NULL && eqVTime ( s->sndtim, vtime->vt ) )
		vtime->num_states++;

	if ( s != NULL )
	{
		s = nxtstate_macro ( s );

		if ( s != NULL )
			vtime->limit = s->sndtim;
	}

	while ( m != NULL && eqVTime ( m->rcvtim, vtime->vt ) )
	{
		vtime->num_imsgs++;
		m = nxtimsg_macro ( m );
	}

	if ( m != NULL && ltVTime ( m->rcvtim, vtime->limit ) )
		vtime->limit = m->rcvtim;

	while ( n != NULL && eqVTime ( n->sndtim, vtime->vt ) )
	{
		vtime->num_omsgs++;
		n = nxtomsg_macro ( n );
	}

	if ( n != NULL && ltVTime ( n->sndtim, vtime->limit ) )
		vtime->limit = n->sndtim;

	strncpy ( p->snder, ocb->name, NOBJLEN );
	strncpy ( vtime->type, ocb->typepointer->type, TOBJLEN );
	strncpy ( p->rcver, ocb->name, NOBJLEN );
	p->sndtim = vtime->vt;
	p->rcvtim = ocb->phase_begin;

	sysmsg ( MOVEVTIME, p, sizeof(VTimeInfo), ocb->next_node );

	ocb->migrStatus = (Byte) SENDVTIME;
	ocb->migrWait |= (Byte) WAITFORACK;

	ocb->phase_limit = vtime->vt;

	if ( gtVTime ( ocb->phase_limit, ocb->svt ) )
	{
		if ( ltVTime ( ocb->svt, min_msg_time ) )
		{
			min_msg_time = ocb->svt;
		}
		if ( ltVTime ( min_msg_time, gvt ) )
			{
			twerror("recv_vtime_done: min_msg time %f set before gvt %f\n",
				min_msg_time.simtime, gvt.simtime);
			tester();
			}
		ocb->svt = ocb->phase_limit;
	}
#ifdef PARANOID
if ( ltVTime ( ocb->svt, gvt ) )
{
	twerror ( "send_vtime: svt %f set before gvt %f\n", ocb->svt.simtime,
				gvt.simtime);
	tester ();
}
#endif PARANOID
}

FUNCTION recv_vtime_ack ( msg )

	Msgh * msg;
{
	Ocb * ocb;
	State * s;
	Msgh * m;
	VTimeInfo * vtime;
	int  flags = 0;

  Debug

	ocb = (Ocb *) l_next_macro ( sendOcbQ );

	ocb->migrWait &= (Byte) ~WAITFORACK;
	ocb->migrWait |= (Byte) WAITFORDONE;
	ocb->migrStatus = SENDINGVTIME;

	vtime = (VTimeInfo *) ( msg + 1 );


	m = fstomsg_macro ( ocb );

	while ( vtime->num_omsgs-- )
	{

/* Trivial bug fix - changed nxtimsg_macro to nxtomsg_macro.  Only cosmetic,
		since both macros are identical. PLRBUG */

		register Msgh * n = nxtomsg_macro ( m );

		l_remove ( m );

		set_reverse_macro ( m );

		if ( neVTime ( m->sndtim, vtime->vt ) )
		{
			_pprintf("trying to send output message with vtime %f, ne this vtime %f\n",
				m->sndtim.simtime, vtime->vt.simtime);

			_pprintf("msg pointer %x\n",m);
			tester();
		}

		m->flags |= MOVING;

		{
			extern int mlog, node_cputime;

			if ( mlog )
			{
#ifdef MICROTIME
				MicroTime ();
#else
#ifdef MARK3
				mark3time ();
#endif
#ifdef BBN
				butterflytime ();
#endif
#endif
				m->cputime = node_cputime;
			}
		}

		sndmsg ( m, m->txtlen + sizeof (Msgh), ocb->next_node );

		m = n;
	}

	for ( m = fstimsg_macro ( stdout_ocb ); m; )
	{
		register Msgh * n = nxtimsg_macro ( m );

		if ( geVTime ( m->rcvtim, ocb->phase_end ) ) break;

		if ( gtVTime ( m->sndtim, vtime->vt ) ) break;

		if ( geVTime ( m->sndtim, ocb->phase_begin )
		&&   namecmp ( m->snder, ocb->name ) == 0 )
		{
			l_remove ( m );

			m->flags |= MOVING;

		{
			extern int mlog, node_cputime;

			if ( mlog )
			{
#ifdef MICROTIME
				MicroTime ();
#else
#ifdef MARK3
				mark3time ();
#endif
#ifdef BBN
				butterflytime ();
#endif
#endif
				m->cputime = node_cputime;
			}
		}

			sndmsg ( m, m->txtlen + sizeof (Msgh), ocb->next_node );
		}

		m = n;
	}

	m = fstimsg_macro ( ocb );

	while ( vtime->num_imsgs-- )
	{
		register Msgh * n = nxtimsg_macro ( m );

		l_remove ( m );

		m->flags |= MOVING;

		{
			extern int mlog, node_cputime;

			if ( mlog )
			{
#ifdef MICROTIME
				MicroTime ();
#else
#ifdef MARK3
				mark3time ();
#endif
#ifdef BBN
				butterflytime ();
#endif
#endif
				m->cputime = node_cputime;
			}
		}

		sndmsg ( m, m->txtlen + sizeof (Msgh), ocb->next_node );

		m = n;
	}

/*PJH There is a problem here??? */

	if ( vtime->num_states )
	{
		s = fststate_macro ( ocb );

		l_remove ( s );

		/*  Set the flags to be given to send_state().  This state is
				definitely migrating, and might be pre-interval. */

		flags |= MIGRATING;

		if ( ltVTime ( s->sndtim, ocb->phase_begin ) )
			flags |= PRE_INTERVAL;

		send_state ( s, ocb, flags );

		return; /* wait for send_state_done */
	}
}

FUNCTION recv_vtime_nak ()
{
  Debug

	tester ();
}

FUNCTION recv_vtime_done ()
{
	Ocb * ocb;
	char buff[MINPKTL];

  Debug

	ocb = (Ocb *) l_next_macro ( sendOcbQ );

	if ( ocb == sendOcbQ)
	{
		twerror ("recv_vtime_done: no ocb in sendOcbQ\n");
		tester();
	}

	ocb->migrStatus = (Byte) SENDNEXTVTIME;
	ocb->migrWait &= (Byte) ~WAITFORDONE;

	if ( gtVTime ( ocb->phase_limit, ocb->svt ) )
	{
	_pprintf("recv_vtime_done: Illegal condition\n");
	tester();

#if 0
		if ( ltVTime ( ocb->svt, min_msg_time ) )
		{
			min_msg_time = ocb->svt;
		}
if ( ltVTime ( min_msg_time, gvt ) )
{
	twerror("recv_vtime_done: min_msg time %f set before gvt %f\n",
				min_msg_time.simtime, gvt.simtime);
	tester();
}
/*
		ocb->svt = ocb->phase_limit;
if ( ltVTime ( ocb->svt, gvt ) )
{
	twerror ( "recv_vtime_done: svt %f set before gvt %f\n", ocb->svt.simtime,
				gvt.simtime);
	tester ();
}
*/
#endif

	}

	send_vtime ( ocb );
}

FUNCTION recv_phase_done ( msg )
	Msgh * msg;
{
	Ocb * ocb;
	char buff[MINPKTL];
	MigrAckInfo *Ackmsg;

  Debug

	Ackmsg = ( MigrAckInfo * ) ( msg + 1 );

	ocb = (Ocb *) l_next_macro ( sendOcbQ );

	if ( ocb == sendOcbQ )
	{
		twerror("recv_phase_done: no phases in migration queue\n");
		printf("object %s, Vtime %f, acking node %d\n",Ackmsg->name,
				Ackmsg->svt.simtime, Ackmsg->AckSndNode );
		tester();
	}

	ocb->migrStatus = (Byte) MIGRDONE;

	if ( ltVTime ( ocb->svt, min_msg_time ) )
		min_msg_time = ocb->svt;
if ( ltVTime ( min_msg_time, gvt ) )
{
	twerror("recv_phase_done: min_msg time %f set before gvt %f\n",
				min_msg_time.simtime, gvt.simtime);
	tester();
}

	l_remove ( ocb );

#ifdef MICROTIME
	MicroTime ();
#else
#ifdef MARK3
	mark3time ();
#endif
#ifdef BBN
	butterflytime ();
#endif
#endif


	/*  Don't put a message in the migration log if we are creating it for
		graphical purposes. */

	if ( !migrGraph )
	{
		sprintf ( buff, "%d %d %s %f %d %f\n",
			tw_node_num, node_cputime, ocb->name, ocb->phase_begin.simtime,
			ocb->next_node, gvt.simtime );

		send_to_IH ( buff, strlen ( buff ) + 1, MIGR_LOG );
	}

	nukocb ( ocb );

	ocbs_to_send--;

	if ( ocbs_to_send )
		send_ocb_from_q ();
#ifdef PARANOID
	else
		if ( l_next_macro ( sendOcbQ) != sendOcbQ )
		{
				twerror("stopped migrating phases before queue was empty\n");
				tester();
		}
#endif PARANOID
}

FUNCTION recv_vtime ( msg )

	Msgh * msg;
{
	Ocb * ocb;
	VTimeInfo * vtime;
	Objloc * location;

  Debug

	location = GetLocation ( msg->rcver, msg->rcvtim );

	if ( location == NULL )
	{
		twerror ( "recv_vtime F can't find %s at %f", msg->rcver, msg->rcvtim.simtime );
		tester ();
		return;
	}

	if ( location->node != tw_node_num )
	{

/* Bug fix to deal with pending list having brought in a stale cache entry. */

		RemoveFromCache(msg->rcver,msg->rcvtim);

		location = GetLocation ( msg->rcver, msg->rcvtim );


		/*  A NULL location means it hasn't shown up yet, so nak and return. */

		if ( location == NULL )
		{
		_pprintf("Sending a state nak because location is null\n");
			send_state_nak ( msg, NULLOCB );
			return;
		}
		else

		/*  But if the local object location info thinks the phase is on
				another node, there's a serious problem. */

		if ( location->node != tw_node_num )
		{

			twerror ( "recv_vtime F bad location for %s at %f\n", msg->rcver, 
						msg->rcvtim.simtime );
			tester ();
			return;
		}
	}

	ocb = location->po;

	vtime = (VTimeInfo *) ( msg + 1 );

	ocb->num_imsgs = vtime->num_imsgs;
	ocb->num_omsgs = vtime->num_omsgs;
	ocb->num_states = vtime->num_states;

	ocb->stats.numimmigr += ocb->num_imsgs;
	ocb->stats.numommigr += ocb->num_omsgs;
	ocb->stats.numstmigr += ocb->num_states;

	ocb->phase_limit = vtime->vt;
	ocb->next_limit = vtime->limit;
	ocb->migrStatus = (Byte) RECVVTIME;

	send_vtime_ack ( msg, ocb );
}

FUNCTION send_vtime_ack ( msg, ocb )

	Msgh * msg;
	Ocb * ocb;
{
	Msgh * p;

  Debug

	p = sysbuf ();

	*((VTimeInfo *) ( p + 1 )) = *((VTimeInfo *) ( msg + 1 ));

	sysmsg ( VTIMEACK, p, sizeof(VTimeInfo), ocb->prev_node );
}

FUNCTION send_vtime_done ( msg, ocb )

	Msgh * msg;
	Ocb * ocb;
{
	Msgh * p;

  Debug

	p = sysbuf ();

	*((VTimeInfo *) ( p + 1 )) = *((VTimeInfo *) ( msg + 1 ));

	sysmsg ( VTIMEDONE, p, sizeof(VTimeInfo), ocb->prev_node );
}


FUNCTION recv_phase ( msg )

	Msgh * msg;
{
	Ocb * ocb;
	PhaseInfo * phase;
	Pending_entry * request, * next;
	Objloc * location;

  Debug

	ocb = mkocb ();
	if ( ocb == NULL )
	{
		/*  We might want to change this from an error, if it occurs.
			In theory, the protocol should be able to handle getting a nak.
			At the moment, though, attempting to migrate to a node that
			is actually out of memory and cannot allocate an ocb would
			cause an infinite loop of migrations and naks.
		*/

		_pprintf("recv_phase: no room for ocb of migrating phase\n");
		send_phase_nak( msg, NULLOCB );
		return;
	}

	migrin++;

	phase = (PhaseInfo *) ( msg + 1 );

	ocb->oid = phase->oid;
	ocb->num_states = phase->num_states;
	ocb->num_imsgs = phase->num_imsgs;
	ocb->num_omsgs = phase->num_omsgs;
#ifdef SOM
	ocb->Ept = phase->Ept;
	ocb->comEpt = phase->comEpt;
	ocb->lastComEpt = phase->lastComEpt;
	ocb->work = phase->work;
	ocb->comWork = phase->comWork;
	ocb->lastComWork = phase->lastComWork;
#endif SOM

	ocb->svt = phase->svt;
if (ltVTime (ocb->svt,gvt))
{
	twerror("recv_phase: svt %f set below gvt %f for %s\n",ocb->svt.simtime,
				gvt.simtime, ocb->name);
	tester();
}
	ocb->libPointer = phase->libPointer;
	ocb->generation = phase->generation;
	ocb->crcount = phase->crcount;

	entcpy ( &ocb->stats, phase + 1, sizeof(ocb->stats) );

	ocb->stats.nummigr++;

	strcpy ( ocb->name, msg->rcver );
	ocb->typepointer = find_object_type ( phase->type );
	ocb->control = EDGE;
	ocb->runstat = BLKINF;
	ocb->phase_begin = msg->sndtim;
	ocb->phase_end = msg->rcvtim;
	ocb->phase_limit = ocb->phase_begin;
	ocb->crcount = 1;
	ocb->migrStatus = (Byte) MIGRSTART;
	/* prev_node contains the node number of the node migrating the
		phase to this node. */
	ocb->prev_node = msg->low.from_node;

	nqocb ( ocb, _prqhd );

	if ( ltVTime ( ocb->svt, gvt ) )
	{
		twerror ( "recv_phase: object %s svt %f less than gvt %f\n",
			ocb->name, ocb->svt.simtime, gvt.simtime );
		tester ();
	}

	RemoveFromCache ( ocb->name, ocb->phase_begin );

	location = GetLocation ( ocb->name, ocb->phase_begin );

	if (location == (Objloc *) NULL || location->po == (Ocb *) NULL)
	{
		twerror("Can't find object %s, %2f in recv_phase\n",
				ocb->name, ocb->phase_begin.simtime);
		tester();
	}

	/* If the object is being migrated to its home node, that node may have
		gotten requests for it between the time the home list entry changed
		and the ocb was set up.  Any such requests are sitting in the pending
		list.  Look for them and finish their work. */

	if (location != NULL && location->node == tw_node_num )
	{
		request = FindInPendingList(ocb->name,ocb->phase_begin,ocb->phase_end,
				(Pending_entry *) l_next_macro(PendingListHeader));

		while ( request != NULL )
		{
/*
				_pprintf("Fixing up timing problem between home list change and migration start, object %s\n",ocb->name);
*/
				/*Remove this entry from the pending list. */
				next = (Pending_entry *) l_next_macro(request);
				RemoveFromPendingList(request,location);

				request = FindInPendingList(ocb->name,ocb->phase_begin,
								ocb->phase_end,next);
		}
	}

	if ( ocb->num_states + ocb->num_imsgs + ocb->num_omsgs )
	{
		send_phase_ack ( msg, ocb );
	}
	else
	{
		ocb->migrStatus = (Byte) MIGRDONE;
		send_phase_done ( msg, ocb );
	}
}

FUNCTION send_phase_ack ( msg, ocb )

	Msgh * msg;
	Ocb * ocb;
{
	Msgh * p;
	MigrAckInfo	* Ackmsg;

  Debug

	p = sysbuf ();

	Ackmsg = (MigrAckInfo *) ( p + 1 );

	strcpy( Ackmsg->name, msg->rcver );
	Ackmsg->svt = msg->rcvtim;
    Ackmsg->AckSndNode = tw_node_num;
	Ackmsg->AckRcvNode = ocb->prev_node; 

	sysmsg ( PHASEACK, p, sizeof(MigrAckInfo), ocb->prev_node );
}

FUNCTION send_phase_nak ( msg, ocb )

	Msgh * msg;
	Ocb * ocb;
{
	Msgh * p;
	MigrAckInfo	* Ackmsg;
	int	prev_node;

  Debug
	phaseNaksSent++;

	p = sysbuf ();

	Ackmsg = (MigrAckInfo *) ( p + 1 );

	strcpy( Ackmsg->name, msg->rcver );
	Ackmsg->svt = msg->rcvtim;
    Ackmsg->AckSndNode = tw_node_num;

	if ( ocb == NULLOCB )
		prev_node = msg->low.from_node;
	else
		prev_node = ocb->prev_node;

	Ackmsg->AckRcvNode = prev_node; 

	sysmsg ( PHASENAK, p, sizeof(MigrAckInfo), prev_node );
}

FUNCTION send_phase_done ( msg, ocb )

	Msgh * msg;
	Ocb * ocb;
{
	Msgh * p;
	MigrAckInfo	* Ackmsg;

  Debug

	p = sysbuf ();

	Ackmsg = (MigrAckInfo *) ( p + 1 );

	strcpy( Ackmsg->name, msg->rcver );
	Ackmsg->svt = msg->rcvtim;
    Ackmsg->AckSndNode = tw_node_num;
	Ackmsg->AckRcvNode = ocb->prev_node; 

	sysmsg ( PHASEDONE, p, sizeof(MigrAckInfo), ocb->prev_node );
}

PrintsendStateQ()
{
	State_Migr_Hdr * migr_hdr;

#ifdef DLM
	dprintf ( "%d", tw_node_num );
#endif DLM
	showstate_head ();

	if ( sendStateQ )
	for ( migr_hdr = nxtmigrh_macro ( sendStateQ ); migr_hdr != NULL;
		  migr_hdr = nxtmigrh_macro ( migr_hdr ) )

	{

		dump_state_migr_hdr ( migr_hdr );
		showstate ( migr_hdr->state );
		dprintf("       segno = %d, no_segs = %d, pktno = %d, no_pkts = %d, tot_pkts = %d\n", 
				migr_hdr->state->segno, migr_hdr->state->no_segs, 
				migr_hdr->state->pktno, migr_hdr->state->no_pkts,
				migr_hdr->state->tot_pkts);
	}
}

PrintsendOcbQ()
{
	Ocb *ocb;

	dprintf ( "%d", tw_node_num );

	showocb_head ();

	if ( sendOcbQ )
	for ( ocb = nxtocb_macro ( sendOcbQ ); ocb != NULL;
		  ocb = nxtocb_macro ( ocb ) )
	{
		/* Print an asterisk in front of any object currently migrating. */

		if ( ocb->migrWait & WAITFORACK || ocb->migrWait & WAITFORDONE )
			dprintf("* ");
		else
			dprintf("  ");
		showocb ( ocb );
	}
}

/* Count the number of phases currently being migrated by this node. */

numMigrating ()

{
	int count = 0;
	Ocb *ocb;

	if ( sendOcbQ )
	for ( ocb = nxtocb_macro ( sendOcbQ ); ocb != NULL;
		  ocb = nxtocb_macro ( ocb ) )
			count++;

	return ( count );

}


/*  Search the queue of phases to be sent for a particular phase.  Return
		NULL if it isn't found, a pointer to it otherwise. */

FUNCTION Ocb * findInSendQueue ( name, time )
	Name name;
	VTime time;
{
	Ocb * ocb;

	if ( sendOcbQ )
		for ( ocb = nxtocb_macro ( sendOcbQ ); ocb != NULL; 
				ocb = nxtocb_macro ( ocb ) )
		{
			if ( strcmp ( name, ocb->name ) == 0 && 
				geVTime ( time, ocb->phase_begin ) &&
				ltVTime ( time, ocb->phase_end ) )
			return ocb;
		}

	return ( NULL );
}

FUNCTION verifySendOcbQ ()
{
	Ocb * ocb;

	if ( sendOcbQ )
		for ( ocb = nxtocb_macro ( sendOcbQ ); ocb != NULL; 
				ocb = nxtocb_macro ( ocb ) )
				_pprintf("%s    ",ocb->name);
	_pprintf("sendOcbQ verified\n");

}

/* Insert a state into the state queue of a phase currently in the migration
		queue.  We must then check for rollback of the phase, which cannot
		use ordinary rollback code. */

/*  This function is probably incorrect because it doesn't adjust the
	number of states, input messages, and output messages to be sent.
	If those change without changing the count in the ocb, the entire 
	migration will hang expected more input, or some other equally terrible
	thing will happen.  Probably, resetting those ocb values after all
	rollback activity ceases will fix the problem. */

FUNCTION putStateInSendQueue ( ocb, state )

	Ocb * ocb;
	State * state;
{
	State *firstState;
	Msgh *firstIMsg, *firstOMsg;

	/* Find the first state of the migrating phase. */

	firstState = fststate_macro ( ocb );

	/* If this first state has a sndtime less than phase_begin, it
		must be discarded.  Otherwise, the state copy can be
		inserted at the front of the queue. */

	if ( firstState != NULL && 
		ltVTime ( firstState->sndtim, ocb->phase_begin ) )
	{

		l_remove ( firstState );
		destroy_state ( firstState );
		if ( ocb->cs == firstState )
			ocb->cs = NULL;
	}

	/*  This is a pre-interval state, so it should always be first in
		the state queue.  PARANOID code checks to see that it isn't
		misordered. */

	l_insert ( ocb->sqh, state );

#ifdef PARANOID
	{
		State * newState, * nextState ;

		newState = nxtstate_macro ( ocb->sqh );
		nextState = nxtstate_macro ( newState );

		if ( nextState != ocb->sqh &&
			geVTime ( newState->sndtim, nextState->sndtim ) )
		{
			_pprintf("state misorder in send queue, object %s, ocb ptr %x, first state %x, second state %x\n", 
			ocb->name, ocb, newState, nextState);
			tester();
		}
	}
#endif PARANOID

	/*  Now perform a half-assed rollback of the phase.  Its svt may
		need to be reset, states may need to be discarded, output
		messages may need to be cancelled.  It's a pity we can't use
		the code from rollback() for this purpose, but messing with
		it to handle an unusual dlm case is dangerous, and it would
		add code to the main line of TWOS to handle a very infrequent
		occurrence. */

	firstIMsg = fstimsg_macro ( ocb );

	/*  If there are no input messages being sent, then there is no need for
		further rollback activity.  Otherwise, if the first input message is 
		already the one to be run next, as it usually will be, no further work 
		is needed.  We're just rolling back to the same place we were already 
		at, simply with a different initial state. */

	if ( firstIMsg == NULL || leVTime ( ocb->svt, firstIMsg->rcvtim ) )
	{
		return;
	}

	/*  Must do a real rollback.  Adjust the svt, cancel any states at later
		times, and, depending on whether lazy or aggressive cancellation is
		being used, either unmark or cancel all output messages at later
		times.  */

	ocb->svt = firstIMsg->rcvtim;

if ( ltVTime (ocb->svt, gvt))
{
	twerror("putStateInSendQ: svt %f set before gvt %f for %s\n",
				ocb->svt.simtime, gvt.simtime,ocb->name);
	tester();
}
	cancel_states ( ocb, ocb->svt );

	if ( aggressive )
		cancel_all_output ( ocb, ocb->svt );
	else
		unmark_all_output ( ocb, ocb->svt );
}

/*  This function reorders the state migration queue by characteristic 
		virtual time.  The queue is probably short, so a simple (even
		stupid) method can be used - bubble sort.  Should this method
		cause problems, a better sort can be added.  */

FUNCTION reorderStateMigrQ ()
{
	State_Migr_Hdr *migrHdr, *nxtMigrHdr;

	for ( migrHdr = nxtstate_macro ( sendStateQ ); migrHdr != NULL; 
			migrHdr = nxtstate_macro ( migrHdr ) )
	{

		nxtMigrHdr = nxtstate_macro ( migrHdr );

		if ( nxtMigrHdr != NULL &&
			 ltVTime ( nxtMigrHdr->time_to_deliver, migrHdr->time_to_deliver) )
		{
			l_remove ( migrHdr );
			l_insert ( nxtMigrHdr, migrHdr );
			migrHdr = nxtstate_macro ( sendStateQ ) ;
			continue;
		}
	}
}

/* findInMovedQ() looks through a queue of states that have received
	their done message, but not all of their acks.  If found, it increments
	the acknowledgement received field.  If that field equals the 
	acknowledgements sent, remove the entry from the queue. */

FUNCTION findInMovedQ ( msg )
	Msgh * msg;
{
	State_Migr_Hdr *migrHdr;
	MigrAckInfo * Ackmsg;

	Ackmsg = ( MigrAckInfo * ) ( msg + 1 );

	for ( migrHdr = l_next_macro ( statesMovedQ ); migrHdr != statesMovedQ;
			migrHdr = l_next_macro ( migrHdr ) )
	{
		/* Skip over entries that don't match.  If a matching entry is
			found, increment its acksReceived field and check if all
			expected acks have come in. */

		if ( strcmp ( migrHdr->name, Ackmsg->name) == 0 &&
			eqVTime ( migrHdr->time_to_deliver, Ackmsg->svt ) )
		{
			migrHdr->acksReceived++;

			/* Check to see if all acks for this state have been received 
				yet. */

			if ( migrHdr->acksReceived == migrHdr->acksExpected )
			{
				l_remove ( migrHdr );
				l_destroy ( migrHdr );
			}

			return ( 1 );
		}
	}

	/* If we get here, we didn't find a match for this ackmsg. Return 0. */ 

	return ( 0 );

}

/* showStatesMovedQ() displays the queue of states for which a state
	done message is received before all state acks have been received. */

FUNCTION showStatesMovedQ	 ()
{
	State_Migr_Hdr *migrHdr;

	dprintf ( "%d: statesMovedQ\n", tw_node_num );
	dprintf("	Name		Vtime	Acks Expected	Acks Received\n");

	for ( migrHdr = l_next_macro ( statesMovedQ ); migrHdr != statesMovedQ;
			migrHdr = l_next_macro ( migrHdr ) )
	{
		dprintf("	%s		%f		%d		%d\n",migrHdr->name, 
					migrHdr->time_to_deliver.simtime, migrHdr->acksExpected, 
					migrHdr->acksReceived );
	}
}
