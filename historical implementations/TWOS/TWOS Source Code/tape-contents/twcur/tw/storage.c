/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	storage.c,v $
 * Revision 1.14  92/02/25  10:36:16  reiher
 * changed VTime comparison to use VTime comparison macro
 * 
 * Revision 1.13  91/12/27  11:31:43  pls
 * 1.  Make EVTLOG output consistent with Simulator.
 * 2.  Add support for variable length address tables (SCR 214).
 * 
 * Revision 1.12  91/12/27  08:49:56  reiher
 * Added routines and code to support event and event time throttling
 * 
 * Revision 1.11  91/11/01  09:51:45  reiher
 * critical path stuff and some ifdef PLR debugging code (the latter shoudl
 * not be compiled in) (PLR)
 * 
 * Revision 1.10  91/07/17  15:12:57  judy
 * New copyright notice.
 * 
 * Revision 1.9  91/07/09  14:46:49  steve
 * MicroTime support and replaced miparm.me with tw_node_num
 * 
 * Revision 1.8  91/06/03  12:26:56  configtw
 * Tab conversion.
 * 
 * Revision 1.7  91/05/31  15:35:53  pls
 * Add debugging code
 * 
 * Revision 1.6  91/04/01  15:47:20  reiher
 * Code to support proper garbage collection of the dead ocb list.
 * 
 * Revision 1.5  91/03/28  09:54:30  configtw
 * Add RBC change for Steve.
 * 
 * Revision 1.4  91/03/26  09:48:40  pls
 * Add Steve's RBC code.
 *
 * Revision 1.3  90/12/10  10:53:11  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.2  90/11/27  10:07:45  csupport
 * expand state error message, and call tester()
 * 
 * Revision 1.1  90/08/07  15:41:12  configtw
 * Initial revision
 * 
*/
char storage_id [] = "@(#)storage.c     1.49\t10/2/89\t16:23:20\tTIMEWARP";


/*

Purpose:

		The routines in storage.c perform garbage collection for the
		system.  They recover the storage used by messages and states
		with times before the current gvt.  In addition, the module
		contains a routine for allocating critical data space.  Also,
		there are two routines that log input and output messages in
		files.  These two routines have been ifdef'ed out.

Functions:

		mcheck(alloc,size) - allocate critical data space
				Parameters - Byte *(*alloc) (), Uint size
				Return - a pointer to the allocation, if found, NULL otherwise.

		gcpast() - garbage collect the past of all objects
				Parameters - none
				Return - Always returns zero

		objpast(ocb) - garbage collect an object's past
				Parameters - Ocb *ocb
				Return - Always returns zero

		log_input_message(msg) - store a record of an input message
				Parameters - Msgh *msg
				Return - Always returns zero

		log_output_message(msg)-  store a record of an output message
				Parameters - Msgh *msg
				Return - Always returns zero

Implementation:

		gcpast() runs through the local object list, calling objpast()
		for every object in it.

		objpast() garbage collects the input, output, and state queues
		of a single object, getting rid of all items so far in the 
		past that they are certain never to be needed again.  It starts
		with the state queue, looking at the oldest state first.  Until
		it finds a state that is at or later than gvt, it deletes states.
		Once such a state is found, the state just before it is saved, so
		that the object can roll back before virtual time.  delstate()
		is used to free the memory of the old states.  A sanity check is
		made on each state examined, making sure that it is either not earlier
		than gvt, or has a NOERR serror.  If the sanity check fails, print
		an error message and halt Time Warp.

		Next, objpast() deals with the input queues.  If the gvt is POSINF,
		all messages in the input queue will be garbage collected.  Otherwise,
		those earlier than the earliest remaining state in the state queue
		will be dumped.  Each message to be gotten rid of is given to
		delimsg().  A sanity check on each message makes sure that the
		message being deleted is not an anti-message.  If it is, an error
		message is printed, and Time Warp traps to the Tester.  Output
		messages are handled similarly, but there is no sanity test on
		output messages.

		log_input_message() and log_output_message() are practically 
		identical.  They check to see if they have yet opened their 
		output files, and open them if they haven't.  Then they print
		a line describing the message being logged.  Both of these
		routines have been ifdef'ed out of the kernel.  
*/

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"

extern long highEpt ;
extern long highestEpt ;
extern long critNode ;
extern long nodesReporting;
extern Name critObject ;
extern long critEnabled ;
extern long firstEst ;
extern long statesPruned ;
extern long msgsPruned ;
long maxLocalEpt = -1;
Ocb * maxEptObj;
State *last;
extern double throttleMultFactor;
extern int throttleAddFactor;
#define MAXIQLEN 500
#define SHORTEN_LIST
int maxIQLen = MAXIQLEN;
long shortenQueues = TRUE;

FILE * HOST_fopen ();

FUNCTION        gcpast ()
{
	register Ocb *ocb;
	Ocb * nxtocb;
	extern int mlog, node_cputime;
	static Msgh gcpast_msg;
#ifdef SHORTEN_LIST
	int splitCount;
#endif SHORTEN_LIST

Debug

	if ( mlog )
	{
		char objname[10];
		static int first_time = 1;

		if ( first_time )
		{  /* print once for each node */
			first_time = 0;
			sprintf ( objname, "gcpast%d", tw_node_num );
			make_static_msg ( &gcpast_msg, EMSG, objname, gvt, objname, gvt,
				0, 0 );
		}
		gcpast_msg.sndtim = gcpast_msg.rcvtim = gvt;
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
		gcpast_msg.cputime = node_cputime;
	}

	/* Garbage collect the past of each object */

	for ( ocb = fstocb_macro; ocb; ocb = nxtocb )
	{  
	  int removed;

	  /* go through the object list for this node (prqhd) */

	  /* Keep track of the highest Ept we've ever seen, so we can print
			out the local node's contribution to the critical path length.
			Do it here so we catch all states before they get fossil 
			collected or truncated.  */

	  last = lststate_macro ( ocb );

	  if ( last != NULLSTATE && last->Ept > maxLocalEpt && 
				ocb->runstat != ITS_STDOUT)
	  {

		maxLocalEpt = last->Ept;
		maxEptObj = ocb;
	  }
	  nxtocb = nxtocb_macro ( ocb );

#ifdef SHORTEN_LIST
		/* Don't re-execute objpast() on an ocb that was just split off
			because its input queue was too long.  The split off part 
			has already had its garbage collection done.  Redoing it may
			do no harm, but some code associated with the rollback hardware or
			with the event logging mechanism might be screwed up. */

		if ( BITTST ( ocb->migrStatus, SPLITFORIQLEN ) )
		{
			BITCLR ( ocb->migrStatus, SPLITFORIQLEN );
			continue;
		}
#endif SHORTEN_LIST

	  removed = objpast ( ocb );
#ifdef SHORTEN_LIST
		if ( !removed && shortenQueues == TRUE )
		  splitCount = countQueues ( ocb );

		/* If the input queue is too long and the object isn't migrating, 
			split it. */

		if ( splitCount > maxIQLen  && shortenQueues == TRUE && 
			 eqVTime ( ocb->phase_end, ocb->phase_limit ) )
		{
			shortenIQLen ( ocb, splitCount );
		}
#else
	  if ( !removed )
		  countQueues ( ocb );
#endif SHORTEN_LIST
	}  

	if ( mlog )
		msglog_entry ( &gcpast_msg );
}

#define REMOVED 1
#define NOT_REMOVED 0

FUNCTION        objpast (ocb)
	Ocb            *ocb;        /* OCB of object whose past is to be garbage
								 * collected */
{
	register State *state,
				   *nstate;
	register Msgh  *msg,
				   *nmsg;
	VTime  garbtime;            /* future limit, inclusive (can delete any msg
								 * older than this time, or equal to it) */
	register int i;

	int garbage_count;
	int eventsCommitted = 0;
	int eventsNotCommitted = 0;
	int eventTimeCommitted = 0;
	int eventTimeNotCommitted = 0;

#ifdef RBC
	int state_count;
	int simulation_done;
#endif

#ifdef EVTLOG
	int cnt;
	static FILE * fp = 0;
	char		filename[30];

	if ( evtlog )
	{
		if ( fp == 0 )
            {   /* open the event log for this node */
            sprintf (filename, "evtlog%d", tw_node_num);
            fp = HOST_fopen ( filename, "w" );
            }
	}
#endif

Debug

	if ( ocb->runstat == ITS_STDOUT )
	{  /* commit this objects messages */
		commit ( ocb );
	}

	state = fststate_macro (ocb);       /* get the first saved state */

	if (state == NULL)
		return ( NOT_REMOVED );

	for ( ;; )
	{

		/*  An error state will be committed only if the state is in error,
				it is pre-gvt, it isn't the pre-interval state for a phase
				with a phase begin later than gvt, and the state error isn't
				NULLOBJST for neginf. */

		if ((state->serror != NOERR &&  
				ltVTime ( state->sndtim, gvt ) ) &&
				geVTime ( gvt, ocb->phase_begin ) &&
				(state->serror != NULLOBJST && 
				neSTime ( state->sndtim.simtime, neginf.simtime ))
		   )
		{
			/*
			 * A fatal condition has arisen here -- the user program has
			 * committed a genuine error, which won't be rolled back.
			 */

			twerror ( "objpast F committed state error %s in %s at time %f,%d,%d",
						state->serror, ocb->name, state->sndtim.simtime,
						state->sndtim.sequence1, state->sndtim.sequence2 );
			tester();
			tw_exit (0);
		}

		nstate = nxtstate_macro (state);

		if ( nstate == NULL || geVTime ( nstate->sndtim, gvt ) )
			break;      /* exit if last state or we're up to gvt */

		state = nstate;
	}   /* for ( ;; ) */

	/* get time of the latest state to be dumped */
	garbtime = state->sndtim;

	if ( geVTime ( gvt, ocb->phase_end ) )
	{  /* the state is beyond end of phase */
		garbtime = ocb->phase_end;

		/* If we shipped a state to a later phase without saving a copy,
				and we committed that work, count it, even though there's
				no local copy. */

		if ( ocb->loststate )
		{
			ocb->loststate = 0;
			ocb->stats.nscom++; /* increment committed states count */
		}
	}

	if ( eqSTime ( gvt.simtime, posinfPlus1.simtime ) )
	{
		garbtime = gvt; /* we're done */
#ifdef RBC
		simulation_done = TRUE;
	}
	else
	{
		simulation_done = FALSE;
#endif
	}

#ifdef PLR
	/*  This debugging code looks through all input queue entries about
		to be garbage collected before any of them are deallocated.  If
		there is a problem, tester() is called immediately.  In the 
		production version of the code, the testing is done at the same
		time as the garbage collection, on a message-by-message basis.
		Doing it that way is faster, but information may be lost once
		the error is detected, making diagnosis of the problem impossible. */

	msg = fstimsg_macro (ocb);  /* first input message */

	for ( ; msg && leVTime ( msg->rcvtim, garbtime ); msg = nmsg )
	{
		nmsg = nxtimsg_macro (msg);     /* get next input message */

		if ( ismarked_macro ( msg ) )
		{  /* maybe obselete??? */
			twerror ( "storage E marked message in input queue for %s",
						msg->rcver );
			tester ();
		}

		if ( isanti_macro ( msg ) )     /* we've got a problem */
		{  /* this msg never found its positive twin */
			twerror (
			"storage E antimessage in input queue during garbage collect");
			showmsg ( msg );
			tester ();
		}

	}
#endif PLR
#ifdef EVTLOG
	if ( evtlog )
	{
		msg = fstomsg_macro (ocb);

		cnt = 0;

		while (msg != NULL && leVTime ( msg->sndtim, garbtime ) )
		{
			if ( msg->mtype == EMSG || msg->mtype == DYNCRMSG || 
				msg->mtype == DYNDSMSG)
				cnt++;

			nmsg = nxtomsg_macro (msg);

			if ( nmsg == NULL || neVTime ( nmsg->sndtim, msg->sndtim ) )
			{
				if ( cnt > 0 )
				{
/*                 fprintf ( fp, "%s\t%f\t%d\t%d\t%d\n", msg->snder,
				   msg->sndtim.simtime, msg->sndtim.sequence1,
				   msg->sndtim.sequence2, cnt ); */

                   HOST_fprintf ( fp, "%s\t%.4f\t%d\t%d\t%d\n", msg->snder,
                   msg->sndtim.simtime, msg->sndtim.sequence1,
                   msg->sndtim.sequence2, cnt );

				   cnt = 0;
				}
			}

			msg = nmsg;
		}
	}
	else
	if ( chklog )
	if ( ocb->evtlog )
	{
#define ERROR .0001
		VTime	chkTime;
		int cnt = 0;

		msg = fstomsg_macro (ocb);

		while ( msg != NULL && leVTime ( msg->sndtim, garbtime ) )
		{
			if ( msg->mtype == EMSG || msg->mtype == DYNCRMSG ||
				msg->mtype == DYNDSMSG )
				cnt++;

			nmsg = nxtomsg_macro (msg);

			if ( nmsg == NULL || neVTime ( nmsg->sndtim, msg->sndtim ) )
			{
				if ( cnt > 0 )
					{
					chkTime = ocb->evtlog[ocb->ce].vtime;
					chkTime.simtime -= ERROR;  /* account for floating errors */
					if ( gtVTime (chkTime, msg->sndtim ) )
						{
		_pprintf ( "%s has %d coemsg's at time %f,%d,%d, should have 0\n",
		msg->snder, cnt, msg->sndtim.simtime, msg->sndtim.sequence1, msg->sndtim.sequence2 );
						tester ();
						}
					else
						{
						chkTime.simtime += (2.0 * ERROR);
						if ( ltVTime ( chkTime, msg->sndtim ) )
							{
		_pprintf ( "%s has 0 coemsg's at time %f,%d,%d, should have %d\n",
		msg->snder, ocb->evtlog[ocb->ce].vtime.simtime, ocb->evtlog[ocb->ce].vtime.sequence1, ocb->evtlog[ocb->ce].vtime.sequence2, ocb->evtlog[ocb->ce].cnt );
							tester ();
							}
						else
							if ( ocb->evtlog[ocb->ce].cnt != cnt )
							{
		_pprintf ( "%s has %d coemsg's at time %f,%d,%d, should have %d\n",
		msg->snder, cnt, msg->sndtim.simtime, msg->sndtim.sequence1, msg->sndtim.sequence2, ocb->evtlog[ocb->ce].cnt );
							tester ();
							}
						}
					ocb->ce++;
					cnt = 0;
					}	/* if cnt > 0 */
			}		/* if (nmsg == NULL...) */

			msg = nmsg;
		}
	}
#endif

	state = fststate_macro (ocb);

	/* Garbage collect past states */
#ifdef RBC
	state_count = 0;
#endif

	for ( ;; )
	{  /* loop through the states again */
		nstate = nxtstate_macro (state);

		if ( geVTime ( state->sndtim, garbtime ) )
			break;      /* break out of the loop */

		if ( geVTime ( state->sndtim, ocb->phase_begin ) )
			ocb->stats.nscom++; /* in the phase--commit it */

#ifdef RBC
		if ( ocb->uses_rbc )
		{
			state_count++;
			l_remove ( state );
			l_destroy ( state );
		}
		else
		{
#endif
		if ( nstate != NULL && nstate->address_table != NULL )
		{  /* handle deferred address table entries */
			for ( i = 0;i < l_size(nstate->address_table) / sizeof(Address);i++)
			{
				if ( nstate->address_table[i] == DEFERRED )
				{  /* move deferred stuff from this state to next */
#if 0
if (state->address_table[i] == NULL)
{
_pprintf("objpast: null segment--state: %x nstate: %x\n",state,nstate);
tester();
}
#endif
					l_remove ( state->address_table[i] );
					nstate->address_table[i] = state->address_table[i];
					state->address_table[i] = NULL;
				}
			}
		}

#ifdef SOM
	  /*  Add in the time spent creating this state to the committed work
		  for this phase.  Also set the phase's highest committed Ept
		  to the Ept for this state.  */

	  ocb->comWork += state->effectWork;
	  ocb->comEpt = state->Ept;
#endif SOM
	  eventsCommitted ++;
	  eventTimeCommitted += state->effectWork;

		/* If the critical path computation facility has been enabled,
			don't fossil collect this state, as it might be needed
			later for critical path purposes.  Instead, mark it as eligible
			for fossil collection, so that the critical path code can free
			the state if it isn't needed.  If the state has already been
			marked as a fossil, we've been through this process once.  Don't
			do it again, as it can muck up the critical path computation.  i

			Only the state header is needed, so the earlier removal of dynamic 
			memory segments is OK.  Also, if memory needs require it, the data 
			portion of the state could be released here, leaving only the 
			header.  That operation might be a bit complicated, however, so 
			it isn't done yet. */

		if ( critEnabled == TRUE )
		{
			truncateState ( state, ocb );
		}
		else
		{
			/* If critEnabled is false, don't save stuff for critical
				path computation. */

			l_remove ( state );             /* take state out of list */
			destroy_state ( state );        /* release all its memory */
		}
#ifdef RBC
		} /* destroy_state and RBC are incompatible */
#endif

		if ( nstate == NULL )
			break;      /* if we're at the end of the state list */

		state = nstate;
	}   /* for ( ;; ) */

	/* Loop through the truncated state queue, looking for truncated
		state with resultingEvents fields of zero.  Any state in this
		queue has already been committed, and is being saved only for
		critical path computation purposes.  If its resultingEvents field
		is zero, then the state isn't on the critical path, and can be
		deleted.  The code that set the resulting events field to zero
		has already taken care of informing predecessors that this state
		is not on the critical path, so we can simply garbage collect it. */

	if ( critEnabled == TRUE )
	{
		truncState *ts, * nextTs;

		for ( ts = l_next_macro ( ocb->tsqh ) ; !l_ishead_macro ( ts ); 
				ts = nextTs )
		{
			nextTs = l_next_macro ( ts );

			if ( ts->resultingEvents == 0 )
			{
				l_remove ( ts );

				l_destroy ( ts );

				statesPruned++;
			}
		}
	}

#ifdef RBC
	if ( ocb->uses_rbc && state_count )
		if ( simulation_done )
		{
				if ( --state_count )
						advance_op ( ocb, state_count, garbtime );
				term_op ( ocb );
		}
		else
		{
				advance_op ( ocb, state_count, garbtime );
		}
#endif

	/* Garbage collect the past input messages */

	msg = fstimsg_macro (ocb);  /* first input message */

	garbage_count = 0;

	for ( ; msg && leVTime ( msg->rcvtim, garbtime ); msg = nmsg )
	{
		garbage_count++;

		nmsg = nxtimsg_macro (msg);     /* get next input message */

#ifndef PLR
		if ( ismarked_macro ( msg ) )
		{  /* maybe obselete??? */
			twerror ( "storage E marked message in input queue for %s",
						msg->rcver );
			tester ();
		}

		if ( isanti_macro ( msg ) )     /* we've got a problem */
		{  /* this msg never found its positive twin */
			twerror (
			"storage E antimessage in input queue during garbage collect");
			showmsg ( msg );
			tester ();
		}
		else
#endif PLR
		{
			if ( ! BITTEST ( msg->flags, FOSSILMSG ) )
				stats_garbtime (ocb, msg, nmsg);    /* count committed events */
		}

#ifdef IMSG_LOG
		log_input_message ( msg );
#endif
		if ( msg->mtype == DYNCRMSG || msg->mtype == CMSG )
		{  /* if this is a create message */
			/* If the FOSSILMSG flag is on, this msg has already been
				counted in the crcount, so don't count it again. */

			if (ocb->crcount > 0 && ! BITTEST ( msg->flags, FOSSILMSG ) )
			{
		 /*  If this create message is attempting to create an object
			  of the same type as this object already is, don't flag
			  it as an error.  An object can be legally created multiple
			  times, provided it is always created as the same type. */

			  Crttext *ChangeText;
			  Typtbl *NewType;

			  ChangeText = ( Crttext * ) ( msg + 1 );

			  NewType = find_object_type ( ChangeText->tp );

			  if ( NewType != ocb->typepointer )
			  {
				  twerror ( "Too many committed creates for object %s\n",
					  ocb->name );
				  showmsg ( msg );
			  }
			  else
			  {
				  /* There is a state set up for this duplicate create
					  message, so we have to count it in the crcount to
					  properly balance the stats. */
				  /* I think this is wrong.  crcount is used only to keep
					track of the number of committed creates and destroys,
					not for statistics.  If we increment crcount here, we
					may not detect certain errors when dynamic creation
					and destruction interact. */

/*
				  ocb->crcount++;
*/
			  }

			}
			else
			{
				ocb->crcount++;
			}
		}
		else
		if ( msg->mtype == DYNDSMSG )
		{
			if ( ocb->crcount < 1 )
			{
				twerror ( "Too many committed destroys for object %s\n",
						ocb->name );
				showmsg ( msg );
			}
			else
			{
				ocb->crcount--;
			}
		}
		else
		if ( msg->mtype == EMSG )
		{
			if ( ocb->crcount < 1 )
			{  /* I see no object here */
				twerror ( "Committed message to NULL object %s\n",
						ocb->name );
				showmsg ( msg );
				tester();
			}
		}

		/* If the critical path computation facility has been enabled,
			don't fossil collect this msg, as it might be needed
			later for critical path purposes.  Instead, mark it as eligible
			for fossil collection, so that the critical path code can free
			the msg if it isn't needed.  Only the msg header is
			needed, so earlier removal of the data portion of the message
			OK.  That operation might be a bit complicated, however, so it 
			isn't done yet. */

		if ( critEnabled == TRUE )
		{
			BITSET ( msg->flags, FOSSILMSG );

			/* If the noncritical flag is already set, we can fully fossil
				collect this message.  Otherwise, we should merely truncate
				it. */

			if ( BITTEST ( msg->flags, NONCRITMSG ) )
			{
				delimsg ( msg );
				msgsPruned++;
			}
			else
			{
				truncateMessage ( msg );
			}
		}
		else
		{
			delimsg ( msg );
		}
	}

	if ( eqSTime ( garbtime.simtime, posinfPlus1.simtime ) && msg != NULL )
	{
		_pprintf ( "left over msg %x for %s\n", msg, ocb->name );
		tester ();
	}

	/* Garbage collect the past output messages */

	msg = fstomsg_macro ( ocb );        /* get first message in q */

	while ( msg != NULL && leVTime ( msg->sndtim, garbtime ) )
	{
		nmsg = nxtomsg_macro ( msg );   /* get the next message */

		if ( !ismarked_macro ( msg ) )
		{  /* unmarked anti-messages should have been sent out */
			twerror ( "storage E unmarked message in output queue for %s, garbtime is %.2f", msg->snder, garbtime.simtime );
			tester ();
		}

		if ( isposi_macro ( msg ) )
		{
			/* Positive committed msg in the output queue - possibly due
				to race conditions between sendback, migration, and
				cancellation.  If so, sending off the positive msg to its
				destination would fix it, but let's see it happen first 
				and make sure that this is, indeed, the situation, before
				proceeding further. */

			twerror ( "objpast: committed positive msg %x in %s's output queue\n",
						msg, ocb->name );
			tester ();
		}

		stats_garbouttime ( ocb, msg, nmsg );   /* update committed stats */

#ifdef OMSG_LOG
		log_output_message ( msg );
#endif
		if ( msg == ocb->co )
		{  /* new header */
			ocb->co = (Msgh *) l_prev_macro ( msg );
		}

		delomsg ( msg );        /* dump the message */

		msg = nmsg;
	}

#if 0
/*  This code is dead, but it includes RBC stuff.  That RBC stuff will have
to be put in the new version of the code, if it is to work. */

		/* Move Null objects without any active input messages or output
				messages to the Dead Ocb List.  */
/* The first test here isn't correct.  It should look to see if there's only
		one state, a destroy state.  */
		if (ocb->typepointer == NULL_TYPE)
		{
			msg = fstimsg_macro (ocb);

			if (msg == NULL)
			{
				msg = fstomsg_macro(ocb);

				if (msg == NULL)
				{
					State * final_state;
					Int HomeNode;


					/* This object probably has a state left over resulting
						from its destruction message.  If so, be sure to
						free that storage and count it as a committed state.*/

					final_state = fststate_macro(ocb);
					if (final_state != NULLSTATE)
					{
						ocb->stats.nscom++;
						l_remove(final_state);
#ifdef RBC
						if ( ocb->uses_rbc )
							l_destroy ( final_state );
						else
						/* destroy_state and rollback chip don't mix */
#endif
						destroy_state ( final_state );
					}

					l_remove (ocb);
					l_insert (DeadOcbList,ocb);
					/* Now we must remove the home list entry for this
						object, or stuff might get misdelivered. */

					HomeNode = name_hash_function(ocb->name,HOME_NODE);
					if (HomeNode == tw_node_num)
					{
						RemoveFromHomeList(ocb->name);
					}
					else
					{
						RemoteRemoveFromHomeList(ocb->name,HomeNode);
						/*
						_pprintf("trying to clear non-local home node\n");
						tester();
						*/
					}

					/* And, we must clear the storage site's cache entry, or
						that site might try to deliver to an object that
						doesn't exist. This site is the storage site, so
						we can be fairly sure that we'll be able to do it
						from here.  Of course, clearing the home list entry
						may have already cleared the cache entry, if this
						site is also the home site for the object.*/ 

				RemoveFromCache(ocb->name);
				}
			}

		}
#endif

	/* Now, if gvt hasn't progressed to posinf, garbage collect any phases
		that have completed all their work.  Any phase whose end is earlier
		than gvt will never run again, so we don't need a full Ocb for it.
		Instead, a stub structure will be kept in the deadOcbList.  That
		structure consists only of its name, phaseBegin, and phaseEnd, so
		the statistics for the phase we're about to garbage collect must
		be kept somewhere.  They will be forwarded to the next phase in
		a system message.  Phases that end at positive infinity will never
		be garbage collected, so every object will have somewhere to store
		all its statistics. */


#if 0
	if ( ltVTime ( ocb->phase_end, posinf ) && ltVTime (ocb->phase_end, gvt ) )
	{
		deadOcb * deadPhase;
		Msgh    * statsMsg;
		int     txtlen;
		Objloc  * location;
		Stattext sendStats;


		if ( fstimsg_macro ( ocb ) != NULL ||
			 fstomsg_macro ( ocb ) != NULL ||
			 fststate_macro ( ocb ) != NULL )
		{
			_pprintf ( "old phase %s %f not garbage collected because of leftovers\n", 
				ocb->name, ocb->phase_end.simtime );
			return ( NOT_REMOVED );
		}

/*
		_pprintf ( "about to garbage collect %s %f\n", ocb->name, 
				ocb->phase_end.simtime );
*/

		/* Now try to get the location of the phase that should be receiving
				this stats message.  Almost always, that phase will have an
				entry in the object location cache.  If not, and assuming
				this isn't the home node and the phase isn't local, for the
				moment we'll give up.  We could do fancy stuff involving
				asking the home node, but we're only trying to save a few
				hundred bytes, here, so it may not be worth the effort. */

		location = GetLocation ( ocb->name, ocb->phase_end );

		if ( location == NULL )
		{
			_pprintf ( "Can't find location of phase %s %f, giving up attempt to garbage collect dead phase\n",
						ocb->name, ocb->phase_end );
			return ( NOT_REMOVED );
		}
		else
		if ( location->node == tw_node_num )
		{
			/* Hey, it's on the same node.  Let's not bother sending a
				message, in this case.  */

			/* This option not implemented yet. */

			_pprintf("not garbage collecting local phase for %s\n",ocb->name);
			return ( NOT_REMOVED );
		}

		deadPhase = ( deadOcb * ) l_create ( sizeof ( deadOcb ));

		if ( deadPhase == NULL )
		{
			_pprintf("couldn't get memory to garbage collect phase %s %f\n",
						ocb->name, ocb->phase_end.simtime );
			return ( NOT_REMOVED );
		}

		strcpy ( deadPhase->name, ocb->name );
		deadPhase->phaseBegin = ocb->phase_begin;
		deadPhase->phaseEnd = ocb->phase_end;

		txtlen = sizeof ( Stattext );
		sendStats.phaseEnd = ocb->phase_end;
		sendStats.stats = ocb->stats;


		statsMsg = make_message ( ADDSTATS, ocb->name, ocb->phase_begin,
						ocb->name, ocb->phase_end, txtlen, &sendStats );

		statsMsg->flags |= SYSMSG;

		sndmsg ( statsMsg, sizeof ( Msgh ) + txtlen, location->node );


		if (location->node == tw_node_num)
		{
				_pprintf("improper send when phase is local\n");
				tester();
		}

#ifdef PARANOID

		/* This should never happen, as make_message() calls m_create() with
				the CRITICAL flag set, so m_create() will die rather than
				fail to make the message. */

		if ( statsMsg == NULL )
		{
			_pprintf("Couldn't allocate msg buffer to garbage collect phase %s %f\n",
				ocb->name, ocb->phase_end );
			l_destroy ( deadPhase );
			return ( NOT_REMOVED );
		}
#endif PARANOID


		l_remove ( ocb );

		nukocb ( ocb );

		addToDeadOcbQ ( deadPhase );

		return ( REMOVED );
	}
#endif


	/* Figure out how many events this object is permitted to maintain
		beyond GVT.  Subtract off any events that have been performed, but
		are not yet committed.  Put the result into the ocb's events
		permitted field.  Start the count at -1, since all objects save
		one committed state at all times. */

	eventsNotCommitted = -1;
	eventTimeNotCommitted = 0;

	for ( state = fststate_macro (ocb); state != NULLSTATE; 
			state = nxtstate_macro( state ))
	{
		eventsNotCommitted++;

		if ( geVTime ( state->sndtim, garbtime ) )
			eventTimeNotCommitted += state->effectWork;
	}      

	/* Currently, a single multiplicative and single additive factor are
		used for both eventTimePermitted and eventsPermitted, since the
		two are never used at the same time.  If necessary, separate
		parameters could be added to TWOS. */

	ocb->eventTimePermitted = ( throttleMultFactor * eventTimeCommitted + 
							throttleAddFactor) - eventTimeNotCommitted;

	ocb->eventsPermitted = ( throttleMultFactor * eventsCommitted +
							throttleAddFactor) - eventsNotCommitted;

	return ( NOT_REMOVED );
}


#ifdef IMSG_LOG

log_input_message ( msg )

	Msgh * msg;
{
	static FILE * fp = 0;

	if ( fp == 0 )
	{
		fp = HOST_fopen ( "IMSG_LOG", "w" );
	}

	HOST_fprintf ( fp, "%-20s %5d %-20s %5d %5d\n",
				msg->snder,
				msg->sndtim,
				msg->rcver,
				msg->rcvtim,
				msg->mtype
		);
}

#endif

#ifdef OMSG_LOG

log_output_message ( msg )

	Msgh * msg;
{
	static FILE * fp = 0;

	if ( fp == 0 )
	{
		fp = HOST_fopen ( "OMSG_LOG", "w" );
	}

	HOST_fprintf ( fp, "%-20s %5d %-20s %5d %5d\n",
				msg->snder,
				msg->sndtim,
				msg->rcver,
				msg->rcvtim,
				msg->mtype
		);
}

#endif

/* Count and average all entries in an object's state queue, input queue, and
		output queue.  Store the results in the object's statistics fields.
*/

FUNCTION countQueues(ocb)
	Ocb *ocb;
{
	State *s;
	Msgh *m;
	int count;
	int average;
#ifdef SHORTEN_LIST
	int iqCount;
#endif SHORTEN_LIST

	count = 0;
	for (s = fststate_macro (ocb); s; s = nxtstate_macro (s))
	{
		count++;
if (count > 100)
{
/*
		_pprintf("%d states in state queue for object %s\n",count,ocb->name);
*/
}
	}

	ocb->stats.sqlen += count;  /* count of states in the queue */
	if (count > ocb->stats.sqmax)
		ocb->stats.sqmax = count;       /* record the max */

#ifdef RBC
	rbc_check_frame_count ( ocb, count );
#endif

	count = 0;

	for (m = fstimsg_macro (ocb); m; m = nxtimsg_macro (m))
	{
		count++;
if (count > 100)
{
/*
		_pprintf("%d msgs in input queue for object %s\n",count,ocb->name);
*/
}
	}

	ocb->stats.iqlen += count;  /* count of input messages */
	if (count > ocb->stats.iqmax)
		ocb->stats.iqmax = count;       /* record the max */

#ifdef SHORTEN_LIST
	iqCount = count;
#endif SHORTEN_LIST
	count = 0;

	for (m = fstomsg_macro (ocb); m; m = nxtomsg_macro (m))
	{
		count++;
if (count > 350)
{
/*
		_pprintf("%d msgs in output queue for object %s\n",count,ocb->name);
*/
}
	}

	ocb->stats.oqlen += count;  /* count of output messages */
	if (count > ocb->stats.oqmax)
		ocb->stats.oqmax = count;       /* record the max */

#ifdef SHORTEN_LIST
	return ( iqCount );
#endif SHORTEN_LIST
}

FUNCTION deadOcb * findPhaseInDeadQ ( object, phaseEnd )
	Ocb * object;
	VTime phaseEnd;
{
	deadOcb * phase;

	Debug

	for ( phase = fstDocb_macro; phase; phase = nxtDocb_macro (phase) )
	{  
	  if ( namecmp ( object, phase->name ) == 0 &&
			  eqVTime ( phaseEnd, phase->phaseBegin ) )
		  break;
	}  

	return phase;
}

FUNCTION addToDeadOcbQ ( phase )
	deadOcb * phase;
{
	deadOcb * position;

	position = DeadOcbList;

	l_insert ( (List_hdr *) position, (List_hdr *) phase);

}

#ifdef SHORTEN_LIST

/* Find the halfway point in the message queue for this object.
	Get the receive time for the message at that point, and split
	at that time.  Set the migrStatus field of the new phase to 
	indicate that the split occurred, but only if the new phase will
	be put into the scheduler queue behind the earlier phase.  (Otherwise,
	garbage collection won't look at it again, anyway.)  As written, this
	routine does not dot all its i's and cross all its t's.  For instance,
	the queue length statistics of the newly split off phase is not
	adjusted.  If we decide to keep this function, we need to fix up these
	things */

FUNCTION shortenIQLen ( ocb, length )
	Ocb * ocb;
	Int length;
{
	VTime splitTime, svt;
	Msgh *m;
	int target, count;
	Ocb * newOcb;

	target = length/2;
	svt = ocb->svt;

	if ( length <= 0 || target <= 0)
	{
		twerror("shortenIQLen: invalid value for object %s, length %d or target %d\n",
				ocb->name, length, target);
		tester();
	}

	count = 0;

	for ( m = fstimsg_macro ( ocb) ; m != NULLMSGH; m = nxtimsg_macro ( m ) )
	{
		count++;

		if ( count >= target )
			break;

	}

	if ( m == NULLMSGH )
	{
		twerror("shortenIQLen: mistake in finding split point for %s, length %d\n", ocb->name, length);
		tester();
		return;
	}

	splitTime = m->rcvtim;

	/* If the split occurs at an earlier time than svt (unlikely, but
		possible), then Som's code used to support the critical path 
		computation may produce incorrect results.  If we keep this feature,
		we'll need to fix something up. */

	newOcb = split_object ( ocb, splitTime );

	if ( newOcb == NULL )
		return;
/*
	else
		_pprintf("Object %s split because its input queue was too long\n",
				ocb->name);
*/

	if ( geVTime ( splitTime, svt) )
	{
		BITSET ( newOcb->migrStatus, SPLITFORIQLEN );
	}

}

FUNCTION setMaxIQLen ( length )
	int * length;
{

    if ( *length <= 1 )
	{
		_pprintf ( "illegal value for max IQ length %d; default not changed\n", 
			*length );
	}
	else
	{
		maxIQLen = *length;

		if ( tw_node_num == 0 )
			_pprintf ( "max IQ length set to %d\n", maxIQLen ) ;
	}
}
#endif SHORTEN_LIST

