/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	objend.c,v $
 * Revision 1.6  91/12/27  08:46:59  reiher
 * Added one line to support event time throttling
 * 
 * Revision 1.5  91/07/17  15:11:13  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/06/03  12:25:48  configtw
 * Tab conversion.
 * 
 * Revision 1.3  91/04/01  15:44:11  reiher
 * Code to support Tapas Som's work.
 * 
 * Revision 1.2  91/03/26  09:30:20  pls
 * Add Steve's RBC code.
 * 
 * Revision 1.1  90/08/07  15:40:34  configtw
 * Initial revision
 * 
*/
char objend_id [] = "@(#)objend.c       1.42\t9/26/89\t16:44:32\tTIMEWARP";


/*
Purpose:
		
		objend.c contains the code to handle an object's completion
		of whatever it was doing.
		Generally speaking, this action is rather minimal.  It may
		be necessary to search for further work to do at this
		virtual time for this object, or it may be necessary to 
		advance virtual time and, possibly, save a state of this
		object. It will also be necessary to choose a new object
		for execution.

		In addition, objend.c contains a routine that cancels messages
		in the output queue, for purposes of rollback.  It implements
		a lazy cancellation policy.

Functions:

		objend() - handle whatever needs to be done when an object's
						current execution ends.
				Parameters - none
				Return - always returns 0

		cancel_omsgs(o,t1,t2) - lazily cancel any of this object's
						output messages between times t1 and t2
				Parameters - Ocb *o, VTime t1, VTime t2
				Return - always returns 0

Implementation:

		objend() calls savout() to keep track of the amount of time this
		object has spent so far.  Then, the object's type field is checked
		to see what it was doing.  If it was doing an event, the counter for
		number of completed events is incremented.
		dispatch() is called to choose an object to execute next.

		cancel_omsgs() looks for the last output message for the object.
		It then finds the first message in that bundle.  It starts looking
		at the times of each bundle, going backwards, until it finds a 
		bundle that is before the time at which it is to start cancellation.
		The function then runs forward from this bundle, until it runs out
		of messages or finds one later than the last time to be cancelled.
		If the message isn't marked, dqomsg() is called to dequeue it.
		The dequeued message is sent to its destination (if the destination
		object can be found in the world map; if not, destroy the message).  
		This dequeued message is an anti-message,, destined to cause 
		annihilation at the destination site.  If the message being looked
		at was marked, unmark it.  Go on to the next message.  

		cancel_omsgs() implements lazy cancellation.  When a message is
		put in the output queue, a check is made to see if it is already
		there.  If it is, the message is marked.  The meaning of this is
		that, if we have rolled back, and reproduce a message, mark its
		antimessage.  (Marked messages don't actually get sent.)  Messages
		that were rolled back over but not resent don't get marked.  
		At the end of an event, any messages in the output queue that are
		between the time of the event's start and the time of the next
		event's start are checked with cancel_omsgs().  The ones that
		weren't resent weren't marked, so they get sent out as antimessages.
		Marked messages were resent, so they are simply unmarked.  (We may
		roll back over this event again, in which case these messages may
		yet need to be cancelled, so we can't leave them marked.)
*/
		

#include "twcommon.h"
#include "twsys.h"


FUNCTION objend ()
{
	register Ocb * o;
	extern int aggressive;

  Debug

	o = xqting_ocb;

	destroy_message_vector (o);

	if ( o->centry == DCRT || o->centry == DDES )
	{
		unmark_macro ( o->ci );
	}
	else
	{
		register Msgh * n;

		for ( n = fstigb ( o->ci ); n; n = nxtigb ( n ) )
		{
			if ( n->mtype != DYNDSMSG )
				unmark_macro ( n );
		}
	}

	if ( o->centry == INIT || o->centry == DCRT || o->centry == DDES )
	{
#ifdef SOM

	/* Update the phase's highest-seen ept field to the Ept of the event
	  just completed.  Also update the amount of total unrolled back work
	  done by the phase.  */

		o->Ept = o->sb->Ept;

#endif SOM
		save_state (o);
	}

	if ( o->centry == EVENT )
	{
		o->stats.numecomp++;

		o->eventTimePermitted -= o->sb->effectWork;
#ifdef SOM
	/* Update the phase's highest-seen ept field to the Ept of the event
	  just completed.  Also update the amount of total unrolled back work
	  done by the phase.  */

		o->Ept = o->sb->Ept;

#endif SOM

		save_state ( o );
	}
	else
	if ( o->centry == TERM )
	{
#ifdef RBC
		if ( o->uses_rbc )
			l_destroy ( o->sb );
		else
		/* destroy_state and rollback chip don't mix */
#endif
		destroy_state ( o->sb );

		o->sb = NULL;
		l_destroy ( o->stk );
		o->stk = NULL;
#ifdef RBC
		if ( o->uses_rbc && rollback_op ( o, 1, posinfPlus1 ) )
		{
			printf ( "weird error term objend for %s\n", o->name );
			tester();
		}
#endif

		o->ci = NULL;
		o->co = NULL;
		o->control = EDGE;
		o->runstat = BLKINF;
		if ( ! aggressive )
			cancel_omsgs ( o, o->svt, o->phase_end );
		l_remove ( o );
		o->svt = posinfPlus1;
		l_insert ( l_prev_macro ( _prqhd ), o );
 
		dispatch ();
		return;
	}

	go_forward ( o ) ;

	dispatch ();
}


/* Cancel (lazily) output messages from t1 (inclusive) to t2 (exclusive) */

extern int cancellation_penalty;

FUNCTION cancel_omsgs ( o, t1, t2 )

	register Ocb *o;
	VTime t1, t2;
{
	register Msgh  *n, *m, *p;

  Debug

	m = lstomsg_macro ( o );

	if ( m != NULL )
		m = fstomb ( m );

	n = NULL;

	while ( m && geVTime ( m->sndtim, t1 ) )
	{
		n = m;
		m = prvobq ( m );
	}

	while ( n && ltVTime ( n->sndtim, t2 ) )
	{
		m = n;
		n = nxtobq ( n );

		while ( m != NULL )
		{
			p = nxtomb ( m );

			if ( ! ismarked_macro ( m ) )
			{
				dqomsg ( m );

				mark_macro ( m );

				smsg_stat ( o, m );

				deliver ( m );

				o->cancellations += cancellation_penalty;
			}

			m = p;
		}
	}
}
