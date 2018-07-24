/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	loadman.c,v $
 * Revision 1.7  91/11/01  09:38:33  reiher
 * added code to deal with keep explicit track of undone work (PLR)
 * 
 * Revision 1.6  91/07/17  15:09:07  judy
 * New copyright notice.
 * 
 * Revision 1.5  91/07/09  13:50:04  steve
 * Added MicroTime support
 * 
 * Revision 1.4  91/06/03  12:24:32  configtw
 * Tab conversion.
 * 
 * Revision 1.3  91/04/01  15:38:14  reiher
 * Several bug fixes related to dynamic load management.
 * 
 * Revision 1.2  90/08/27  10:43:45  configtw
 * Split cycle time from committed time.
 * 
 * Revision 1.1  90/08/07  15:39:55  configtw
 * Initial revision
 * 
*/
char loadman_id [] = "@(#)loadman.c     1.1\t10/2/89\t17:00:55\tTIMEWARP";


/* loadman.c contains dynamic load management code.  Much of the dynamic
		load management code also resides in gvt.c, but those parts of it
		not inextricably linked into gvt functions are kept here.  Eventually,
		all major load management code will be located in this module.
*/

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"


/* This define describes what an unused entry in a load array looks like. */

#define EMPTY_ENTRY -5000

#define MAX_MIGR_SIZE 50000


/* mindiff controls the minimum amount of utilization that will be migrated
		to seek a balance.  At a setting of .1, a phase will not be moved
		unless it is predicted to account for at least 10% of the difference
		in utilization between the high and low nodes.  In the future, this
		parameter should be settable from the config file.  Also, it might
		want to be changed to an absolute quantity of utilization, rather
		than a proportion of the difference. */

float mindiff = .1;

/* This variable defines the maximum number of migrations load management
		will permit off a node at any given time. */

int maxMigr = 1;

/* loadStartTime stores the clock time of the start of the current load
		management cycle.  It's used to determine the elapsed time when
		calculating effective utiliation. */

int loadStartTime;
extern int migrPerInt;

/* chooseStrat defines what method will be used to choose a candidate object
		for splitting and migration. */

char chooseStrat = NEXT_BEST_FIT;

/* splitStrat defines what method will be used to split objects for migration
		purposes. */

char splitStrat = NEAR_FUTURE;

/* undoneWork keeps track of how much work was undone on this node during the
	last cycle.  This variable can be compared to the overall effective
	work contribution to determine if throttling is in order.  blockObjects
	is a flag that is set true if too much work has been undone.  It prevents
	the node from running any events until the next load interval. */

int undoneWork;
float undoneUtil = 0.;
int blockObjects = FALSE;

/* This routine calculates the effective utilization for the local node.
		It is found by adding up all of the individual utilizations of
		the local objects, except the standard out object.  All other
		objects' utilizations are then reset to zero. 
*/

FUNCTION float calculateUtilization ()
{
	Ocb *ocb;
	long cputime = 0;
	long elapsed;

#ifdef MICROTIME
	MicroTime ();
#else
#ifdef BBN
	butterflytime ();
#endif BBN
#endif 

	elapsed = node_cputime - loadStartTime ;

	/* Run through every phase stored locally.  For each, calculate its
		effective utilization.  Also, add its cycle time into a running
		total of cycle time. */

	for ( ocb = fstocb_macro; ocb; ocb = nxtocb_macro ( ocb ) )
	{
		if ( ocb->runstat == ITS_STDOUT )
				continue;

		cputime += ocb->cycletime;

		/* Don't divide by 0. */

		if ( elapsed != 0 )
		{
			ocb->stats.utilization = ( float ) ocb->cycletime/elapsed;
		}
		else
		{
			ocb->stats.utilization = 0;
		}
		

		/* Reset the statistics field for the next load cycle. */

		ocb->cycletime = 0;
	}


	/* Find the node's overall effective utilization by dividing the sum of
		the local phases' cycle times by the total elapsed time. */

	if ( elapsed != 0 )
	{
		myUtilization = ( float ) cputime/elapsed;
		undoneUtil = ( float ) undoneWork/elapsed;
	}
	else
	{
		myUtilization = 0;
	}

	/* Reset loadStartTime for the next load cycle. */

	loadStartTime = node_cputime;

	/* EMPTY_ENTRY is a flag value for an unfilled utilization table entry.  
		If, by some malign influence of fate, a node actually has a utilization
		that is lower than EMPTY_ENTRY, we have a problem, and had better not
		proceed.  At the moment, this fits into the category of things that
		shouldn't happen.  If it ever does, we'll fix it. */

	if ( myUtilization < EMPTY_ENTRY + 1 )
	{
		_pprintf( "Very low utilization %f\n",myUtilization );
		tester();
	}

	return ( myUtilization );
}


/* Subtract the effective work for a state about to be thrown away from
		the effective work for this cycle for the appropriate ocb. */

FUNCTION adjustEffectWork(o,state)
	Ocb *o;
	State *state;
{
	int work;

/*
printf("adjusting effective work for object %s\n", o->name);
*/
		work = state->effectWork;
		o->cycletime -= work;
		o->stats.comtime -= work;
		undoneWork += work;
}

/* Given that this node is overloaded, and has an underloaded partner, try
		to find an object to migrate.  Choose the object that minimizes the
		difference between the two nodes' utilizations. */


FUNCTION Ocb *chooseObject( lowutil )
   float        lowutil;
{
	Ocb *o,*chosen;
	int memory;
	VTime splitTime;
	State * lastState;
	int migrating;

	/* If this node is in the middle of migrating more phases than maxMigr 
		permits, do not migrate during this cycle. */

/*
_pprintf("CHOOSEOBJ: lowutil %f\n",lowutil);
*/

	migrating = numMigrating () ;

	if ( migrating >= maxMigr )
{
		return ( NULL ) ;
}



	/* Use the variable chooseStrat to determine which method to use to choose
		the candidate for migration. */

	switch ( chooseStrat )
	{
		/* BEST_FIT finds the phase whose migration would bring the two nodes'
			utilizations closest together. */

		case BEST_FIT:

				chosen = bestFit ( lowutil );
				break;

		/* NEXT_BEST_FIT skips over the phase that BEST_FIT would have chosen,
			if that phase is the one with the highest effective utilization
			on this node.  It chooses the next best one instead.  The theory 
			is that the most utilized phase probably has a lot of good work
			to do, and should not be slowed down by being migrated. */
 
		case NEXT_BEST_FIT:
/*
_pprintf("calling nextBestFit\n");
*/

				chosen = nextBestFit ( lowutil );
				break;

		default:
				_pprintf ("chooseObject gets default!\n");
				chosen = NULL;
			
	}

	/* After finding the best phase to move, decide whether to split it before
		moving it.  Choose the splitting strategy dictated by the variable 
		splitStrat. */

/*
_pprintf("about to switch on split for %s\n",chosen->name);
*/
	switch ( splitStrat )
	{

		/* NEAR_FUTURE causes the phase to split at the point of the last
			piece of work it did.  Presumably, then, the next event run by
			the object will happen at the new node. */

		case NEAR_FUTURE:
				chosen = splitNearFuture ( chosen );
				break;

		/* LIMIT_EMSGS attempts to prevent the phase to migrate from having
				very many input messages requiring forwarding.  Within those
				limits, it acts like NEAR_FUTURE. */

		case LIMIT_EMSGS:
				chosen = splitLimitEMsgs ( chosen );
				break;

		/* MINIMAL_SPLIT causes the phase to split in two, with the part 
			being migrated to have as little data to move as possible. */

		case MINIMAL_SPLIT:
				chosen = splitMinimal ( chosen );
				break;

		/* NO_SPLIT causes the entire chosen phase to migrate. */

		case NO_SPLIT:

				break;

		}

/*
if ( chosen != NULL )
	_pprintf("returning pointer to %s\n",chosen->name);
else
	_pprintf("returning NULL\n");
*/
	return ( chosen );

}

/*memUsedOK() checks to see how much memory is used by an object being
		considered for migration.  If too much memory is being used by
		it, then it will not be migrated.  memUsedOK() returns 1 if the
		object is small enough, 0 otherwise.  At the moment, this function
		is not actually used in making migration decisions. */

FUNCTION memUsedOK(o,memAvail)
	Ocb *o;
	int memAvail;
{
	int iqmem, oqmem, sqmem;
	int overhead = 0;
	int totalmem;

	iqmem = returniqmem (o);
	oqmem = returnoqmem (o);
	sqmem = returnsqmem (o);
	totalmem = iqmem + oqmem + sqmem + overhead;
	if (totalmem >= memAvail)
	{
		return totalmem;
	}
	else
	{
		return 0;
	}
}


/* Various strategies for choosing candidate objects. */

/* bestFit finds the object which, if moved to the underloaded node, would
		bring the two nodes' utilizations most closely together.  It assumes
		that the object would do an equal amount of committed work on either 
		node, and that the source node would still do just as much work if
		the object is moved off. */

Ocb * bestFit ( lowutil )

	float lowutil;
{
	float diff;
	Ocb *o, *chosen;
	float lowChange,highChange,predict;

	/* Run through the list of local phases to find the best one to migrate. */

	chosen = NULL;

	diff = myUtilization - lowutil;

	for ( o = fstocb_macro; o; o = nxtocb_macro (o) )
	{

		/* Don't try to migrate this node's STDOUT object. */

		if ( o->runstat == ITS_STDOUT )
				continue;

		/* Don't try to migrate a phase that has not been completely 
				migrated to this node, yet. */

		/* Testing that phase_limit is equal to phase_end is a more reliable
				way of telling whether a phase has completed migration.
				PLRBUG */

		if ( neVTime ( o->phase_limit, o->phase_end ) )
				continue;

		/*  If the utilization of this object is so low that moving it would
				not correct at least mindiff fraction of the differences,
				it isn't worth moving, so consider it no further. */

		if ( o->stats.utilization < mindiff * ( myUtilization - lowutil ) )
			continue;

		/* lowChange is a prediction of how utilized the underloaded node
				would be if we migrated this object.  highChange is a 
				prediction of the utilization on this node if the object
				was moved off. */

		lowChange = lowutil + o->stats.utilization;
		highChange = myUtilization - o->stats.utilization;

/*
_pprintf("%s:lowchange = %2f, highchange = %2f\n",o->name, lowChange, highChange);
*/
		/* Will migrating this object bring the two utilizations closer
				together than the previous choice? 
		*/

		predict = highChange - lowChange;

		if ( predict < 0 )
			predict *= -1;

		if ( predict < diff )
		{
				diff = predict;
				chosen = o;
		}
	}

	/* If the phase chosen is the only phase to have done useful work on
		this node during the last interval, don't migrate it. */

	if ( chosen != NULL && chosen->stats.utilization >= myUtilization )
	{
		chosen = NULL;
	}

	return ( chosen );
}

/* nextBestFit is like bestFit, but will never select the object with the
		highest effective utilization on the source node.  The idea is that
		moving that object is bad, as it might be a critical path object
		that should not have its progress delayed by a migration. */

Ocb * nextBestFit ( lowutil )

	float lowutil;
{
	float diff;
	Ocb *o, *chosen, *highest, *next;
	float lowChange,highChange,predict,highUtil,nextUtil;

	/*  Initialize variables for the search. */

	highUtil = EMPTY_ENTRY;
	nextUtil = EMPTY_ENTRY + 1;
	highest = NULL;
	next = NULL;

	chosen = NULL;

/*
_pprintf("myUtil = %f, lowutil = %f\n",myUtilization, lowutil);
*/
	diff = myUtilization - lowutil;

	/* Run through the list of local phases to find the best one to migrate,
		keeping track of the phase with the highest effective utilization and 
		the phase with the second highest utilization.  If the highest
		phase was the one chosen, return the second highest, instead. */

	for ( o = fstocb_macro; o; o = nxtocb_macro (o) )
	{

		/* Don't try to migrate this node's STDOUT object. */

		if ( o->runstat == ITS_STDOUT )
				continue;

		/* Don't try to migrate a phase that has not been completely 
				migrated to this node, yet. */

		/* Testing that phase_limit is equal to phase_end is a more reliable
				way of telling whether a phase has completed migration.
				PLRBUG */

		if ( neVTime ( o->phase_limit, o->phase_end ) )
		{
/*
				_pprintf("object %s rejected because migrating\n",o->name);
*/
				continue;
		}

		/*  If the utilization of this object is so low that moving it would
				not correct at least mindiff fraction of the differences,
				it isn't worth moving, so consider it no further. */

		if ( o->stats.utilization < mindiff * ( myUtilization - lowutil ) )
			continue;
/*
_pprintf("      difference %f too small to migrate\n", myUtilization - lowutil );
_pprintf ("     diff = %f, mindiff = %f, myUtil = %f, lowutil = %f\n",
				diff, mindiff, myUtilization, lowutil );
_pprintf("      chosen = %s, util %f\n", chosen->name, chosen->stats.utilization);
*/
		
		/* lowChange is a prediction of how utilized the underloaded node
				would be if we migrated this object.  highChange is a 
				prediction of the utilization on this node if the object
				was moved off. */


/*
if ( o->stats.utilization > 0.003 )
_pprintf("      name: %s        util: %f\n", o->name, o->stats.utilization);
*/

		lowChange = lowutil + o->stats.utilization;
		highChange = myUtilization - o->stats.utilization;
		
		/* Check if this object is the one with the highest utilization on
			this node, or the second highest. */

		if ( o->stats.utilization > highUtil )
		{
				nextUtil = highUtil;
				next = highest;

				highUtil = o->stats.utilization;
				highest = o;
		}
		else
		if ( o->stats.utilization > nextUtil )
		{
				nextUtil = o->stats.utilization;
				next = o;
		}


/*
_pprintf("%s:lowchange = %2f, highchange = %2f\n",o->name, lowChange, highChange);
*/
		/* Will migrating this object bring the two utilizations closer
				together than the previous choice? 
		*/

		predict = highChange - lowChange;

		if ( predict < 0 )
			predict *= -1;

		if ( predict < diff )
		{
				diff = predict;
				chosen = o;
		}
	}
	
	/* If the phase chosen was the one on the node with the highest 
		effective utilization, don't choose it.  Choose the next highest,
		instead. */

	if ( chosen == highest  && chosen != NULL )
	{
		if ( next != NULL )
		{
/*
_pprintf("taking %s (util %f) instead of %s (util %f)\n",
				next->name, next->stats.utilization, chosen->name,
				chosen->stats.utilization);
*/

			chosen = next;
		}
		else
		{
				chosen = NULL;
		}
	}

	return ( chosen );
}

/* Various strategies for splitting objects. */

/* Find the last piece of work a phase did, and split it at that point, with
		the intention being to migrate the part of the phase likely to do
		more work in the near future. */

Ocb * splitNearFuture ( chosen )

Ocb * chosen;
{
	VTime maxtime;
	State *s;
	Msgh *m;

	if ( chosen == NULL )
		return ( chosen );


	maxtime = neginfPlus1;

	s = lststate_macro ( chosen );

	/* If there is a last state, its virtual time is the time of the split. 
		Otherwise, find the last input message, and make that the time of
		the split.*/

	if ( neVTime ( chosen->svt, chosen->phase_begin )  && 
		 ltVTime ( chosen->svt, posinf) )
	{
		maxtime = chosen->svt;
	}
	else
	if ( s != NULL && neVTime ( s->sndtim, chosen->phase_begin ) )
	{
		maxtime = s->sndtim;
	}
	else
	{

		m = lstimsg_macro ( chosen );

		if ( m != NULL && gtVTime ( m->rcvtim, maxtime ) )
		{
				maxtime = m->rcvtim;
		}
	}


	/*  If the chosen split time is within the boundaries of this phase, split
		it.  Otherwise, don't do anything with this phase.  Don't split phases
		at posinf or neginfPlus1. */

	if ( ltVTime ( maxtime, chosen->phase_end )  && 
			gtVTime ( maxtime, chosen->phase_begin ) &&
			neVTime ( maxtime, posinf ) &&
			neVTime ( maxtime, neginfPlus1 ) )
	{
		chosen = split_object ( chosen, maxtime );
	}
	else
	{
/*
		twerror ( "splitNearFuture: proposed splitTime %f after phase_end %f for %s\n",
				maxtime.simtime, chosen->phase_end.simtime, chosen->name);
		tester ();
*/
		chosen = NULL;
	}


	return ( chosen );
}

#define EMSG_LIM 10

Ocb * splitLimitEMsgs ( chosen )

Ocb * chosen;
{
	VTime maxtime, splitTime;
	State *s;
	Msgh *m;
	int msgCount;

	if ( chosen == NULL )
		return ( chosen );

	maxtime = neginfPlus1;

	s = lststate_macro ( chosen );

	/* If there is a last state, its virtual time is the time of the split. 
		Otherwise, find the last input message, and make that the time of
		the split.*/

	if ( s != NULL )
	{
		maxtime = s->sndtim;
	}
	else
	{

		m = lstimsg_macro ( chosen );

		if ( m != NULL && gtVTime ( m->rcvtim, maxtime ) )
		{
				maxtime = m->rcvtim;
		}
	}


	/*  Count the number of input messages that would be migrated if we
		split the phase at this point.  If it's greater than EMSG_LIM,
		split further along to migrate less data. */

	msgCount = 0;

	for ( m = lstimsg_macro ( chosen ); m != NULL; m = prvimsg_macro ( m ))
	{
		if ( geVTime ( m->rcvtim, maxtime ) )
				msgCount++;
		else
				break;

		/* This code might still permit the phase to be migrated with more
				than EMSG_LIM input messages, but making it really work
				would be a lot more code.  See if this does anything 
				interesting first. */

		if ( msgCount >= EMSG_LIM )
		{
			_pprintf("changing split time from %f to %f\n", maxtime.simtime,
				m->rcvtim.simtime);
			maxtime = m->rcvtim;
			break;
		}
	}


	/*  If the chosen split time is within the boundaries of this phase, split
		it.  Otherwise, don't do anything with this phase. */

	if ( ltVTime ( maxtime, chosen->phase_end )  && 
			gtVTime ( maxtime, chosen->phase_begin ) &&
			neVTime ( maxtime, neginfPlus1 ) )
	{
		chosen = split_object ( chosen, maxtime );
	}
	else
	{
	/*
		twerror ( "chooseObject: proposed splitTime %f after phase_end %f for %s\n",
				maxtime.simtime, chosen->phase_end.simtime, chosen->name);
		tester ();
	*/
		chosen = NULL;
	}


	return ( chosen );
}

/* Find the latest virtual time item held by a phase, and split at that time,
		with the intention of migrating the minimum possible amount of data. */

Ocb * splitMinimal ( chosen )

Ocb * chosen;
{
	VTime maxtime;
	State *s;
	Msgh *m;

	if ( chosen == NULL )
		return ( NULL );


	maxtime = neginfPlus1;

	/* Find the latest time for a state at this phase. */

	s = lststate_macro ( chosen );

	if ( s != NULL )
	{
		maxtime = s->sndtim;
	}

	/* Find the latest time for an input message at this phase. */

	m = lstimsg_macro ( chosen );

	if (m != NULL && gtVTime ( m->rcvtim, maxtime ) )
	{
		maxtime = m->rcvtim;
	}

	/* Find the latest time for an output message at this phase. */

	m = lstomsg_macro ( chosen );

	if (m != NULL && gtVTime ( m->sndtim, maxtime ) )
	{
		maxtime = m->sndtim;
	}
	
	/* If the proposed split time is within the interval of this phase, split
		the phase.  Otherwise, do nothing with this phase. */

	if ( ltVTime ( maxtime, chosen->phase_end )  && 
			gtVTime ( maxtime, chosen->phase_begin ) &&
			neVTime ( maxtime, neginfPlus1 ) )
	{
		chosen = split_object ( chosen, maxtime );
	}
	else
	{
/*
		twerror ( "chooseObject: proposed splitTime %f after phase_end %f for %s\n",
					maxtime.simtime, chosen->phase_end.simtime, chosen->name);
		tester ();
*/
		chosen = NULL;
	}

	return ( chosen );
}


