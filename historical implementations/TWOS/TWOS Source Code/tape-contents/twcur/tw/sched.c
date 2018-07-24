/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	sched.c,v $
 * Revision 1.11  92/02/25  10:35:53  reiher
 * added definition of objectBlocked
 * 
 * Revision 1.10  91/12/27  08:48:44  reiher
 * Added routines and code to support all styles of throttling
 * 
 * Revision 1.9  91/11/01  13:36:37  reiher
 * Added throttling code (PLR)
 * 
 * Revision 1.8  91/11/01  10:11:49  pls
 * 1.  Change ifdef's and version id.
 * 2.  Fix bug 14.
 * 
 * Revision 1.7  91/07/17  15:12:08  judy
 * New copyright notice.
 * 
 * Revision 1.6  91/06/03  12:26:34  configtw
 * Tab conversion.
 * 
 * Revision 1.5  91/05/31  15:18:01  pls
 * Handle STATESEND runstat.
 * 
 * Revision 1.4  91/03/26  09:38:52  pls
 * Add Steve's RBC code.
 * 
 * Revision 1.3  90/12/10  10:52:40  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.2  90/11/27  09:54:15  csupport
 * add time info to stack null error message
 * 
 * Revision 1.1  90/08/07  15:40:54  configtw
 * Initial revision
 * 
*/
char sched_id [] = "@(#)sched.c $Revision: 1.11 $\t$Date: 92/02/25 10:35:53 $\tTIMEWARP";


/* 
Purpose:

		This code performs the basic scheduler functions for a node.
		When, for any reason, any object stops execution, this code
		decides what object to run next.

		The heart of the scheduler is dispatch(), the last routine
		in this module.  dispatch() is the routine that actually
		runs through the local list of objects, examining each to
		determine if it can be run.  The remaining functions in this
		module offer support for dispatch().

Functions:

		load_obj(o) - prepare object o for loading 
				Parameters - Ocb *o
				Returns - SUCCESS or FAILURE

		print_dispatch_stats() - print a bunch of statistics related
				to the dispatcher
				Parameters - none
				Returns - always returns 0

		dispatch() - choose an object to run next
				Parameters - none
				Returns - always returns 0

Implementation:

		load_obj() first does a special check to see if this object
		is still in the initialization phase.  It checks to see
		if gvt is still less than 0, and, if so, whether the object's
		scheduler time is greater than 0.  If both conditions hold,
		then the object has completed initialization, but other
		objects may not have, so this object should not be loaded.
		If the object can be loaded, check to see if it's in an
		edge or a non-edge state.  If it's in an edge state, call
		objhead() to find out if this object is waiting to do something.
		objhead() will do the actual context switching, if necessary.
		If it's not in an edge state, restore it to whatever state
		it was in when it was last running.  The context switch for
		this is located in this function.

		print_dispatch_stats() is merely a collection of print
		statements.

		dispatch() is the actual scheduler.  Starting at the
		first ocb in the ocb list, run through each object
		in the list.  For each object, until one is found to
		run, if the object is blocked,
		try to unblock it.  If that routine is successful, or
		if the object wasn't blocked in the first place, use
		load_obj() to try to run that object.  If it succeeds, 
		return.  If load_obj() fails, try another object.
		There is also a lot of code in this routine for gathering
		statistics, but it does not affect the operation of the
		routine.
*/

#include "twcommon.h"
#include "twsys.h"


int throttle = FALSE;
int throttleType = 0;
int objectBlocked = 0;
extern int blockObjects;
#define MIN_PERCENT_FREE 30
int memThrottlePercent = MIN_PERCENT_FREE;
#define MAX_BEYOND_GVT 20
int maxBeyondGvt = MAX_BEYOND_GVT;

extern STime time_window;
double throttleMultFactor = 2.;
int throttleAddFactor = 200000;
extern VTime dynWindow;
int omsgsPermitted = 100;

FUNCTION load_obj ( o )

	Ocb            *o;
{
	register int retval;

	extern STime gvt_sync;

Debug

	if ( leSTime ( gvt.simtime, gvt_sync ) && 
				gtSTime ( o->svt.simtime, gvt_sync ) 
	   )
		return FAILURE;


	/*  Check for whether throttling is enabled, and, if it is, whether
		this event should be throttled.  If a GVT sync point has been set,
		no throttling of any kind will take place before GVT reaches that
		value.  Also, any event at GVT will never be throttled. */

	if ( throttle == TRUE )
	{
		int blockObject;

		blockObject = 0;

		switch ( throttleType )
		{
			case NOTHROT:
				break;
			case  OMSGTHROT :
			{
				/* Throttle optimism based on how many output messages a 
						phase has sent. */

				if ( countOutputMsgs ( o ) > omsgsPermitted )
				{
					blockObject = 1;
				}

				break;
			}
		
			case ETIMETHROT:
			{
				/* Throttle optimism based on how much time a phase has 
					committed recently. */

				if ( o->eventTimePermitted <= 0 )
				{
					blockObject = 1;
				}

				break;
			}

			case DYNWINTHROT:
			{
				/* Throttle optimism based on a dynamic virtual time window 
					calculated off recent GVT values. */

				if ( o->svt.simtime > 0. && gtVTime ( o->svt, dynWindow) )
				{
					blockObject = 1;
				}

				break;
			}

			case EVTHROT:
			{
				/* Throttle optimism based on a count of how many events a 
					phase has committed recently. */

				if (  eventsBeyondGVT( o )  > maxBeyondGvt )
				{
					blockObject = 1;
				}

				break;
			}

			case RBTHROT:
			{
				/* Throttle optimism by comparing the amount of work committed 
					in the last load interval to the amount rolled back.
					(The actual comparison is made in loads.c) */

				if ( blockObjects == TRUE  )
				{
					blockObject = 1;
				}

				break;
			}
		
			case STATICWINDOW:
			{
				/* Throttle optimism using a static virtual time window. */

				VTime window;

				window = newVTime ( gvt.simtime + time_window, 0, 0 );

				if ( gtVTime ( o->svt, window ) )
				{
					blockObject = 1;
				}

				break;
			}

			default:
			{
				twerror("load_obj: illegal value %d for throttling type\n",
							throttleType);
				tester();
			}

		}

		/* If the form of throttling in use decided to block this event,
			and we're after the GVT sync point, and the event isn't being
			performed at time GVT, block it.  (The test for GVT is made
			here, rather than earlier, as it will almost always be passed.
			Therefore, it should not be made until other tests more likely
			to fail have been tried.) */

		if ( blockObject && gtSTime ( gvt.simtime, gvt_sync ) && 
				neVTime ( o->svt, gvt ))
		{
			objectBlocked++;
			return FAILURE;
		}
	}

	if ( o->control != NONEDGE )
	{
		retval = objhead (o);
	}
	else        /* non_edge */
	{
		if ( o->sb == NULL )
		{
			twerror ("load_obj F o->sb is NULL %s", o->name );
			tester ();
		}

		if ( o->stk == NULL )
		{
			twerror ("load_obj F o->stk is NULL %s at %f",
				 o->name,o->svt.simtime );
			tester ();
		}

		xqting_ocb = o; /* set the object global */

#if RBC
		if ( o->uses_rbc )
		{
			setctx (o->footer, NULL, o->stk);
		}
		else
#endif
		setctx (o->sb + 1, NULL, o->stk);

		retval = SUCCESS;
	}

	return retval;
}


#if DISPATCH_STATS
int dispatch_calls;
int dispatch_total_search;
int dispatch_longest_search;

int dispatch_blkinf_cnt;
int dispatch_blkpkt_cnt;
int dispatch_blkpkt_qq_cnt;
int dispatch_arcp_cnt;
int dispatch_arlbk_cnt;
int dispatch_strange_cnt;

print_dispatch_stats ()
{
	int dispatch_average_search = 0;

	if ( dispatch_calls > 0 )
		dispatch_average_search = dispatch_total_search / dispatch_calls;

	printf ( "\n" );
	printf ( "Dispatch: Number of Calls = %d\n", dispatch_calls );
	printf ( "Dispatch: Number of Loops = %d\n", dispatch_total_search );
	printf ( "Dispatch: Longest Search = %d\n", dispatch_longest_search );
	printf ( "Dispatch: Average Search = %d\n", dispatch_average_search );
	printf ( "\n" );
	printf ( "Dispatch: BLKINF cnt = %d\n", dispatch_blkinf_cnt );
	printf ( "Dispatch: BLKPKT cnt = %d\n", dispatch_blkpkt_cnt );
	printf ( "Dispatch: BLKPKT and BLKQQ cnt = %d\n", dispatch_blkpkt_qq_cnt );
	printf ( "Dispatch: ARCP   cnt = %d\n", dispatch_arcp_cnt );
	printf ( "Dispatch: ARLBK  cnt = %d\n", dispatch_arlbk_cnt );
	printf ( "Dispatch: STRANGE cnt = %d\n", dispatch_strange_cnt );
	printf ( "\n" );
}
#endif

#if TIMING
#define SCHED_TIMING_MODE 7
#endif

extern int cancellation_reward;

FUNCTION dispatch ()
{
	Int		contFlag;
	register Ocb *o, *nxto;
	Ocb * first_skipped_o;

#if DISPATCH_STATS
	register int search = 0;
#endif

Debug

#if TIMING
	start_timing ( SCHED_TIMING_MODE );
#endif

	first_skipped_o = NULL;

	for ( o = fstocb_macro; ; o = nxto )
	{

#if DISPATCH_STATS
		search++;
#endif
		if ( o == NULL )
		{
			o = first_skipped_o;
			first_skipped_o = NULL;
		}
		if (o == NULL)
		{
			setnull ();
			xqting_ocb = NULLOCB;
			break;
		}

		nxto = nxtocb_macro (o);

		contFlag = FALSE;
		while ( o->runstat == GOFWD )
		{
			go_forward ( o );

			if ( nxto != NULL && ltVTime ( nxto->svt, o->svt ) )
				contFlag = TRUE;
				break;
		}
		if (contFlag) continue;

		if (o->runstat != READY)
		{

#if DISPATCH_STATS
			switch ( o->runstat )
			{
				case BLKINF:
					if ( ltSTime ( o->svt.simtime, posinf.simtime ) )
					{
						printf ( "Dispatch: Object BLKINF not at POSINF\n" );
						tester ();
					}
					dispatch_blkinf_cnt++;
					break;

				case BLKPKT:
					dispatch_blkpkt_cnt++;
					break;

				case ARCP:
					dispatch_arcp_cnt++;
					break;

				case ARLBK:
					dispatch_arlbk_cnt++;
					break;

				default:
					dispatch_strange_cnt++;
			}
#endif
			if ( o->runstat == BLKPKT )
			{
				if ( sv_doit ( o ) == SUCCESS )
				{
					o->runstat = READY;
				}
			}
		}

		if ( o->runstat == READY )
		{
			if ( o->cs->serror != NOERR )
				continue;

			if ( o->cancellations > 0 )
			{
				o->cancellations -= cancellation_reward;
			}
			if ( o->cancellations > 0 )
			{
				if ( first_skipped_o == NULL )
					first_skipped_o = o;
			}
			else
			if ( load_obj (o) == SUCCESS )
			{                   /* we found a ready object */
				break;          /* break, return to the MI, and the object
								 * will run */
			}
			else
			{
/*
				setnull ();
				xqting_ocb = NULLOCB;
				break;
*/
				continue;
			}
		}

		/*  STATESEND means that this object needs to send its final state to
				the next phase down the line, but was unable to get enough
				memory to do so, the last time it tried.  Simply call 
				send_state_copy() on the last state in its queue. */

		if ( o->runstat == STATESEND )
		{
			State * s;

			s = lststate_macro ( o );

			if ( s == NULL )
			{
				twerror ( "dispatch: %s in STATESEND with no state to send\n",
						o->name);
				tester();
			}
			
			send_state_copy ( s, o );
		}
		
	}

#if DISPATCH_STATS
	dispatch_calls++;
	dispatch_total_search += search;
	if ( search > dispatch_longest_search )
		dispatch_longest_search = search;
#endif

#if TIMING
	stop_timing ();
#endif
}

/* The code below here supports various conditionally compiled throttling
	options. */

FUNCTION turnThrottleOn ( type )
	int * type;
{
	char throttleName[20];
	int i;

	if ( *type < 0 || *type > MAXTHROTSTRAT )
	{
		if ( tw_node_num == 0 )
			_pprintf("illegal throttling strategy parameter %d; throttling not turned on\n",
				*type);

		return;
	}
	
	throttleType = *type;

	throttle = TRUE;

	if ( tw_node_num != 0 )
		return;

	for ( i = 0; i < 20; i++)
		throttleName[i] = 0;

	switch ( throttleType )
	{
		case NOTHROT:
			strcpy ( throttleName, "NOTHROT");
			break;

		case OMSGTHROT:	
			strcpy ( throttleName, "OMSGTHROT");
			break;

		case ETIMETHROT:	
			strcpy ( throttleName, "ETIMETHROT");
			break;

		case DYNWINTHROT:	
			strcpy ( throttleName, "DYNWINTHROT");
			break;

		case EVTHROT:	
			strcpy ( throttleName, "EVTHROT");
			break;

		case RBTHROT:	
			strcpy ( throttleName, "RBTHROT");
			break;

		case STATICWINDOW:	
			strcpy ( throttleName, "STATICWINDOW");
			break;

		default:
			_pprintf("illegal throttling type %d\n",throttleType);
			return;
	}

	_pprintf( "Throttling using %s\n",throttleName );

}

/* eventsBeyondGVT() counts how many events beyond GVT a given phase has
	gone. */

FUNCTION eventsBeyondGVT ( o )
	Ocb * o;
{
	State * s;
	int i;

	/* Set i to -1, because there's always one pre-gvt state. */

	i = -1;

	for ( s = fststate_macro ( o ); s != NULLSTATE; s = nxtstate_macro ( s ) )
	{
		i++;
	}
	
	return ( i ) ;
}


FUNCTION setEventThrottle ( events )
	int * events;
{

    if ( *events <= 0 )
	{
		_pprintf ( "illegal value for event throttling parameter %d; default not changed\n", 
			*events );
	}
	else
	{
		maxBeyondGvt = *events;

		if ( tw_node_num == 0 )
			_pprintf ( "event throttling parameter set to %d\n", maxBeyondGvt ) ;
	}
}

double windowMultiplier = 20;

FUNCTION setWindowMultiplier ( multiplier )
	double * multiplier;
{
	if ( *multiplier < 0 )
	{
		_pprintf("illegal value for throttling window multiplier %f; set to 1\n",
			*multiplier);
		return;
	}
		
	windowMultiplier = *multiplier;
	if ( tw_node_num == 0 )
		_pprintf ( "window throttling multiplier set to %f\n", windowMultiplier ) ;
}

FUNCTION setThrotMultFactor ( factor )
	double * factor;
{

    if ( *factor <= 0 )
	{
		_pprintf ( "illegal value for throttling multiplicative parameter %f; default not changed\n", 
			*factor );
	}
	else
	{
		throttleMultFactor = *factor;

		if ( tw_node_num == 0 )
			_pprintf ( "throttling multiplicative parameter set to %f\n", throttleMultFactor ) ;
	}
}

FUNCTION setThrotAddFactor ( factor )
	int * factor;
{

    if ( *factor <= 1 )
	{
		_pprintf ( "illegal value for throttling additive parameter %d; default not changed\n", 
			*factor );
	}
	else
	{
		throttleAddFactor = *factor;

		if ( tw_node_num == 0 )
			_pprintf ( "throttling additive parameter set to %d\n", throttleAddFactor ) ;
	}
}

FUNCTION countOutputMsgs ( ocb )
	Ocb *ocb;
{
	Msgh *m;
	int count = 0;

	for (m = fstomsg_macro (ocb); m; m = nxtomsg_macro (m))
	{
		/* Only count messages whose send time is past, so we don't get
			held up by lazily cancelled future messages. */

		if ( ltVTime (m->sndtim, ocb->svt ) )
			count++;
	}

	return ( count );
}
