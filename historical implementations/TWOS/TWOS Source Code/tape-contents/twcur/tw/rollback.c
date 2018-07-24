/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	rollback.c,v $
 * Revision 1.10  91/12/27  11:18:07  pls
 * Fix TIMING code.
 * 
 * Revision 1.9  91/12/27  08:48:06  reiher
 * Added code to support event count and event time throttling
 * 
 * Revision 1.8  91/07/17  15:11:56  judy
 * New copyright notice.
 * 
 * Revision 1.7  91/06/03  12:26:29  configtw
 * Tab conversion.
 * 
 * Revision 1.6  91/05/31  15:16:12  pls
 * Fix migration bugs.
 * 
 * Revision 1.5  91/04/01  15:45:48  reiher
 * Error tests to catch gvt problems.
 * 
 * Revision 1.4  91/03/26  09:37:42  pls
 * 1.  Add Steve's RBC code.
 * 2.  Remove zip forward optimization.
 * 
 * Revision 1.3  90/12/10  10:52:25  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.2  90/08/09  16:23:11  steve
 * Added limited jump forwarded code for phase migration
 * 
 * Revision 1.1  90/08/07  15:40:49  configtw
 * Initial revision
 * 
*/
char rollback_id [] = "@(#)rollback.c   1.58\t10/6/89\t14:23:00\tTIMEWARP";


/*

Purpose:

		The code in this module is responsible for determining if it
		is necessary to roll back an object, and for performing the 
		rollback if it is necessary.  There are two major routines,
		rollback() and go_forward(), and several smaller routines.

		rollback() is called at time when rollbacks are not necessary,
		so it must check to see if a rollback is needed at each 
		invocation.  If a rollback is not needed, rollback() need do little
		else.  If a rollback is necessary, rollback() has to restore a
		state prior to the requested rollback time.  It also has to check all 
		of the messages in the object's output queue, to see if any
		of them must be sent out as anti-messages.  In actuality,
		rollback() does not, itself, send out anti-messages, because of
		the lazy cancellation strategy used in this version of Time
		Warp.  Rather, it calls unmark_all_output(), to remove the mark from
		all messages over which it rolled back.  As the object re-
		executes its code, it may send out some duplicates, which will
		result in the restoration of the marks on those messages.  After
		the object completes its execution, it will send anti-messages
		for any unmarked objects (again, using go_forward()).  

		go_forward() is responsible for arranging for an object to
		be ready to run at a specific virtual time.  It sets various
		object states to values indicating its readiness, sets up some 
		pointers for the object, puts the object in the correct place
		in the object queue, cancels any messages that need anti-messages
		sent, and sets up the object scheduler virtual time.

		The other routines contained in this module are supporting
		routines for these two routines.

Functions:

		rollback(o,t_to) - if a rollback needs to be performed, do it
				Parameters - Ocb *o, VTime t_to
				Return - Always returns 0

		go_forward(o,t) - take an object forward to a specific time
				Parameters - Ocb *o, VTime t
				Return - Always returns 0

		latest_earlier_state(o,t) - find the right state to roll back to
				Parameters - Ocb *o, VTime t
				Return - a pointer to the state, if found; NULL
						otherwise

		earliest_later_bundle(o,t) - find the first input bundle after time t
				Parameters - Ocb *o, VTime t
				Return - a pointer to the first message in the bundle,
						or NULL pointer if no bundle exists

		specified_output_bundle(o,t) - find an input bundle with time t
				Parameters - Ocb *o, VTime t
				Return - a pointer to the first message in the bundle,
						or NULL pointer if no bundle exists

Implementation:

		rollback() first checks to see if the object in question is the
		stdout object.  This object never rolls back, so further checks 
		are superfluous; the routine simply returns.  Next, a sanity 
		check to make sure that the object is not trying to go to a time
		before gvt is made.  If that passes, check the time of the call
		to the time in the object's scheduler virtual time field.  If the
		specified time is greater than the scheduler_vt, no rollback is needed, 
		so return.

		If a rollback is needed, set the object's svt to the rollback
		time specified in the call.  Call incr_rollbacks() to keep track
		of how many rollbacks were done.  Reset the state saving timer.
		If the object is in the middle of something (indicated by a
		non-EDGE control state), run through the output messages in the
		current bundle, unmarking them, so that the next call to 
		go_forward() will cancel them.  

		Find an state whose time is no later than the time we need to
		roll back to, using latest_earlier_state().  Delete all later
		states in the state queue.  Set the object's current state to
		the old state just recovered.  It may be that the state that
		should be restored is in the object's temporary state buffer,
		rather than in the state queue.  Check for this case here.
		If the temporary state shouldn't be restored, destroy it and
		its associated stack.  Call go_forward() to perform lazy
		cancellation of messages.

		go_forward() is used to take an object forward to its next
		execution time, from the time of its current state.  It can
		be called due to the completion of an object's execution, or
		because of a rollback.  In either case, the purpose is to 
		prepare an object for its next execution.  Towards this purpose,
		set its run status to READY and its control state to EDGE.
		If the state buffer has an entry, set the time of state to
		that entry's time.  Otherwise, set it to the send time of
		the current state pointer.  Call earliest_later_inputbundle()
		to get a pointer to the next set of input messages to be
		processed.  If no such pointer exists, set the object's
		scheduler time to POSINF, move it to the end of the object
		queue, call cancel_omsgs() to cancel any remaining later
		output messages, and set the object's run status to BLKINF.

		Assuming a next bundle was found, the object's scheduler
		time is set to the rcvtime of that bundle.  Then the object
		is inserted into the correct place in the object queue.
		cancel_omsgs() is used to get rid of any messages between
		its last virtual run time and the next virtual run time.

		latest_earlier_state() runs backwards through an object's
		state queue looking for a state earlier than the specified
		one.  Since the state queue is ordered by state time, the
		first one found is the latest earlier state.

		earliest_later_bundle() runs through an object's input queue,
		searching for the next bundle that should be processed.
		Run backwards through the input queue, examining the receive times
		of each bundle; we are looking for the first bundle that is 
		earlier than the specified time.  For each bundle moved over
		backwards, increment the running count of bundles rolled back
		over.  If we find a bundle with the same time as the specified
		time, run forward until one bundle is found whose time is greater
		than the specified one.  Each bundle traversed in this direction
		causes a decrementation of the count of bundles rolled over
		backwards. Return a pointer to the first message in the bundle.

		specified_output_bundle() runs backward through the output
		queue until it finds an output bundle with a time less than
		or equal to the specified time.  Once such a message is found,
		it runs forward through the queue till it finds a message with
		a send time equal to the specified time, or until it runs out
		of messages.

*/

#include "twcommon.h"
#include "twsys.h"

int aggressive = 0; /* true for aggressive cancellation */

enable_aggressive_cancellation ()
{
	aggressive = 1;
}

#ifdef RBC
int addOneMore;
#endif

FUNCTION rollback ( o, t_to )

	register Ocb *o;
	VTime t_to;
{
	if ( o->runstat == ITS_STDOUT )
	{
		return;         /* no rollbacks for stdout */
	}

Debug

	/* If we shipped a state to the next phase without saving it, and we're
		rolling back, don't count it as committed. */

	o->loststate = 0;

	if ( gtVTime ( t_to, o->svt) )
	{
		/* Rollback not necessary */

		return;
	}

	if ( ltVTime ( t_to, gvt ) && gtVTime ( t_to, neginfPlus1 ) )
	{
		_pprintf ( "rollback: object %s t_to %f less than gvt %f\n",
			o->name, t_to.simtime, gvt.simtime );
		tester ();
	}

	o->runstat = GOFWD;

	if ( neVTime ( o->svt, t_to ) )
	{   /* if it's not a rollback to current time */
		register List_hdr * next;

		o->svt = t_to;          /* set rollback time */

if ( ltVTime ( o->svt, gvt))
{
	twerror ("rollback: svt %f set before gvt %f for %s\n",o->svt.simtime,
			  gvt.simtime, o->name);
	tester();
}
		next = l_next_macro ( o );
		l_remove ( o );         /* remove o from object list */
		nqocb ( o, next );      /* reinsert o in new time order */
	}

	if ( o->sb != NULL )
	{
		adjustEffectWork(o, o->sb);     /* subtract off current work */
#ifdef RBC
		if ( o->uses_rbc )
			l_destroy ( o->sb );
		else
		/* destroy_state and rollback chip don't mix */
#endif
		destroy_state ( o->sb );        /* release all state memory */
		o->eventsPermitted++;

		/* can't be last_sent */
		o->sb = NULL;
		if ( o->stk != NULL )
		{
			l_destroy ( o->stk );       /* release temp stack memory */
			o->stk = NULL;
		}
		if ( o->msgv != NULL )
			destroy_message_vector ( o );       /* release this memory */


#ifdef RBC
		addOneMore = 1;
#endif
	}
#ifdef RBC
	else
	{
		addOneMore = 0;
	}
#endif

	cancel_states ( o, t_to );  /* wipe out states >= rollback time */

	if ( aggressive )
		cancel_all_output ( o, t_to );
	else                                /* we're lazy */
		unmark_all_output ( o, t_to );  /* unmark output >= t_to */
}

extern int cancellation_penalty;

FUNCTION cancel_all_output ( ocb, vt )

	Ocb         *ocb;
	VTime       vt;
{
	register Msgh *msg;

	for ( msg = lstomsg_macro ( ocb ); msg; msg = lstomsg_macro ( ocb ) )
	{
		if ( geVTime ( msg->sndtim, vt ) )
		{
			dqomsg ( msg );
			if ( ocb->co == msg )
			{
				ocb->co = NULL;
			}
			mark_macro ( msg );
			smsg_stat ( ocb, msg );
			deliver ( msg );
			ocb->cancellations += cancellation_penalty;
		}
		else
		{
			break;
		}
	}
}

FUNCTION unmark_all_output ( ocb, vt )

/* unmark all messages in the output list with times >= vt */

	Ocb         *ocb;
	VTime       vt;
{
	register Msgh *msg;

	for ( msg = lstomsg_macro ( ocb ); msg; msg = prvomsg_macro ( msg ) )
	{
		if ( geVTime ( msg->sndtim, vt ) )
		{                               /* this msg time >= vt */
			unmark_macro ( msg );       /* so clear the USED bit */
		}
		else
		{
			break;
		}
	}
}

FUNCTION cancel_states ( ocb, vt )

 /* get rid of any states belonging to ocb that are equal or past vt */

   Ocb          *ocb;
   VTime        vt;
{
/*PJH Are we getting into trouble with register here?
	register */
	State *st;
#ifdef RBC
	int count = addOneMore;
#endif

	for ( st = lststate_macro ( ocb ); st; st = lststate_macro ( ocb ) )
	{
		if ( geVTime ( st->sndtim, vt ) )
		{
#ifdef RBC
			count++;
#endif

			ocb->eventTimePermitted += st->effectWork;
			l_remove ( st );            /* remove state from the list */
			if ( ocb->cs == st )
			{                           /* zero the current state */
				ocb->cs = NULL;
			}
			adjustEffectWork(ocb, st);  /* subtract off work time */
			if ( st == ocb->last_sent )
			{
				ocb->out_of_sq = 1;
				/* for the new jump forward op */
			}
			else
#ifdef RBC
				if ( ocb->uses_rbc )
					l_destroy ( st );
				else
				/* destroy_state and rollback chip don't mix */
#endif
				destroy_state ( st );   /* release its memory */
				ocb->eventsPermitted++;
		}
		else
		{
#if 0
			/*  This code will print a message when the ocb's type is about to
				change due to rollback.  It is only temporary, to verify
				that the code is doing what it should do. */

			if ( ocb->typepointer != st->otype )
			{
/*
				_pprintf("rollback changing type of %s from %s to %s\n",
						ocb->name, ocb->typepointer->type, st->otype->type );
*/
			}
#endif

			/*  Restore this ocb's typepointer to the type specified by
				the last remaining state.  This is to handle the case where
				a dynamic create message was cancelled, to ensure that the
				ocb is restored to type NULL. */

			ocb->typepointer = st->otype;

			break;
		}
	}
#ifdef RBC
	if ( ocb->uses_rbc && count )
		rollback_op ( ocb, count, vt );
#endif
}

#ifdef TIMING
#define ROLLBACK_TIMING_MODE 5
#endif

/* We must not return prematurely in the following routine.  Instead,
exit via get_out, so that TIMING will work properly. */

FUNCTION go_forward ( o )

	Ocb * o ;
{
	List_hdr * next;
  Debug

#ifdef TIMING
	start_timing ( ROLLBACK_TIMING_MODE );
#endif

	o->cs = lststate_macro ( o );

	if ( o->cs == NULL )
	{
		o->runstat = BLKSTATE;
		goto get_out;
	}

look_again:

	o->runstat = READY ;
	o->control = EDGE;
	o->co      = NULL;

	o->ci = earliest_later_inputbundle ( o );

	if ( o->ci != NULL )
	{   
			/* Set the object to execute at that virtual time. */
			o->svt = o->ci->rcvtim;
if ( ltVTime ( o->svt, gvt))
{
	twerror ("go_forward: svt %f set before gvt %f for %s\n",o->svt.simtime,
			  gvt.simtime, o->name);
	tester();
}


			/* Cancel any output msgs that come after this msgs virtual time. */
			if ( ! aggressive )
				cancel_omsgs ( o, o->cs->sndtim, o->svt );

			if ( o->runstat != READY ) /* did cancel cause a rollback? */
			{
				o->ci = NULL;   /* was goto look_again */
				next = l_next_macro ( o );
				l_remove ( o );         /* remove o from object list */
				nqocb ( o, next );      /* reinsert o in new time order */
				goto get_out;
			}
			else
			{
				o->co = specified_outputbundle ( o );
			}
	}
	else
	{
		VTime infinity;

		if ( gtVTime ( gvt, posinf ) )
			infinity = posinfPlus1;
		else
			infinity = posinf;

/* If there is no bundle to execute, see why.  If the reason is that
		there really isn't anything to do and nothing is migrating, or that
		the next thing to do is after the virtual time currently in transit,
		don't change svt.  Otherwise, change svt to indicate that the next
		execution is at the later of the current gvt and the virtual time
		in transit.  PLRBUG */

		if ( eqVTime ( o->phase_limit, o->phase_end ) )
				o->svt = infinity;
		else
		if (ltVTime ( o->svt, o->phase_limit ) )
		{
			if ( geVTime ( o->phase_limit, gvt ) )
				o->svt = o->phase_limit;
			else
				o->svt = gvt;
		}

if ( ltVTime ( o->svt, gvt))
{
	twerror ("go_forward (2): svt %f set before gvt %f for %s\n",o->svt.simtime,
			  gvt.simtime, o->name);
	tester();
}

/*
		if ( gtVTime ( infinity, o->phase_limit ) )
			o->svt = o->phase_limit;
		else
			o->svt = infinity;
*/

		o->runstat = BLKINF;

/* Moved test for BLKINF into the !aggressive test, as runstat must be
		BLKINF unless cancel_omsgs () clears it.  PLRBUG */

		if ( ! aggressive )
		{
			cancel_omsgs ( o, o->cs->sndtim, o->svt );

			if ( o->runstat != BLKINF ) /* did cancel cause a rollback? */
				{
				o->ci = NULL;   /* was goto look_again */
				next = l_next_macro ( o );
				l_remove ( o );         /* remove o from object list */
				nqocb ( o, next );      /* reinsert o in new time order */
				goto get_out;
				}
		}
	}

	if ( o->runstat == BLKINF && 
		 ltSTime ( o->phase_end.simtime, posinf.simtime ) )
	{
		State * state;

		state = lststate_macro ( o );

		if ( state != NULL )
			send_state_copy ( state, o );
	}

	{
		register Ocb * next, * prev;

		next = (Ocb *) l_next_macro ( o );
		prev = (Ocb *) l_prev_macro ( o );

		if ( ( ! l_ishead_macro ( next ) && gtVTime ( o->svt, next->svt ) )
		||   ( ! l_ishead_macro ( prev ) && ltVTime ( o->svt, prev->svt ) ) )
		{
			l_remove ( o );
			nqocb ( o, next );
		}
	}

get_out:

	if ( ltVTime ( o->svt, gvt ) && gtVTime ( o->svt, neginfPlus1 ) )
	{
		_pprintf ( "go_forward: object %s svt %f less than gvt %f\n",
			o->name, o->svt.simtime, gvt.simtime );

		if ( o->loststate == 1 )
		{
			_pprintf("go_forward: lost state for this ocb\n");
		}

		tester ();
	}

#ifdef TIMING
	stop_timing ();
#endif
}

FUNCTION Msgh * earliest_later_inputbundle ( o )

	Ocb * o;
{
	VTime t;
	int type;
	Msgh *m, *n;
	int find_count;
	extern int qlog;

Debug

	t = o->cs->sndtim;
	type = o->cs->stype;

#   define MLATER(m)    geVTime ( (m)->rcvtim, t )

	m = o->ci ? o->ci : lstimsg_macro (o);

	find_count = 0;

	/* first, find an earlier or equal time */
	/* Three cases are possible m later than t, */
	/* m Equal t, m earlier than t */

  if ( m != NULL )
  {
	if (MLATER (m))
	{
		/* back up pointer until MNOTLATER */

		for (n = prvimsg_macro (m);; n = prvimsg_macro (m))
		{
			find_count++;

			if (n == NULL)
			{
				/* Ok, there is no earlier bundle, so m must be the */
				/* earliest later one */
				break;          /* bye! */
			}

			else
			if (MLATER (n))
			{
				/* The one before m is STILL later, so keep going */
				m = n;
				continue;
			}

			else                /* MNOTLATER (n) */
			{
				/* n is just now one step too far, so m must be IT */
				break;          /* bye! */
			}
		}
	}

	else
	{
		/* Go to the first later one or NULL */
		for (m = nxtimsg_macro (m);; m = nxtimsg_macro (m))
		{
			find_count++;

			if (m == NULL || MLATER (m))
			{
				/* Got it */
				break;
			}
		}

	}

	/* If we found a bundle, then make sure that we get to the right
		group within the bundle.  Check the type of the group against the
		type provided in the call, to make sure that the subtime of the
		selected message is later.  If there is no later subtime at this
		virtual time, choose the message at the next virtual time. */

	if ( m != NULL && eqVTime ( m->rcvtim, t) )
	{
		for (; m; m =  nxtimsg_macro(m))
		{
			find_count++;

			if ( gtVTime ( m->rcvtim, t ) )
				break;

			if ( m->mtype > type)
				break;
		}
	}
  }

	if ( m != NULL && geVTime ( m->rcvtim, o->phase_limit ) )
		m = NULL;

	return ( m );
}


FUNCTION Msgh * specified_outputbundle ( o )

	register Ocb * o;
{
	register Msgh * m, * p = NULL;

  Debug

	for ( m = lstomsg_macro ( o ); m; m = prvomsg_macro ( m ) )
	{
		if ( eqVTime ( m->sndtim, o->ci->rcvtim ) )
		{
			if ( m->stype == o->ci->mtype )
			{
				unmark_macro ( m );
				p = m;
			}
			else
			if ( m->stype > o->ci->mtype )
			{
				if ( p == NULL )
					p = m;
				break;
			}
		}
		else
		if ( ltVTime ( m->sndtim, o->ci->rcvtim ) )
		{
			if ( p == NULL )
				p = m;
			break;
		}
	}

	if ( p == NULL )
		p = o->oqh;

	return ( p );
}
