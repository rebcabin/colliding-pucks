/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	gvt2.c,v $
 * Revision 1.17  91/12/27  08:41:59  reiher
 * Added code to support dynamic window throttling
 * 
 * Revision 1.16  91/11/06  11:10:52  configtw
 * Add sequence output for Suns.
 * 
 * Revision 1.15  91/11/04  10:15:40  pls
 * Force gvt display to print sequence numbers.
 * 
 * Revision 1.14  91/11/01  09:35:58  reiher
 * Removed all code relating to the memory out poll flag, changed all 
 * occurrences of miparm.me to tw_node_num, and added a call to the critical
 * path calculation module, if needed
 * 
 * Revision 1.13  91/07/17  15:08:33  judy
 * New copyright notice.
 * 
 * Revision 1.12  91/07/09  13:44:56  steve
 * Added MicroTime support for Sun version. Replace 5 with STATS_DATA and
 * 6 with GVT_DATA.
 * 
 * Revision 1.11  91/06/07  15:25:23  configtw
 * Put oldgvt outside of #if DLM.
 *
 * Revision 1.10  91/06/03  14:27:12  configtw
 * Fix code for DLM off.
 * 
 * Revision 1.9  91/06/03  12:24:16  configtw
 * Tab conversion.
 * 
 * Revision 1.8  91/05/31  13:15:24  pls
 * 1.  Add resendState flag for migration control.
 * 2.  Add PARANOID code to check for scheduler queue misordering.
 * 3.  Don't depend on queue ordering at term time.
 * 
 * Revision 1.7  91/04/01  15:36:45  reiher
 * Added some code to look for gvt going backwards, and to support migration
 * graphics.
 * 
 * Revision 1.6  91/03/28  16:15:53  configtw
 * Put hoglog output in XL_STATS.
 * 
 * Revision 1.5  91/03/25  16:14:46  csupport
 * Add hoglog support.
 * 
 * Revision 1.4  90/12/10  10:41:31  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.3  90/11/27  09:32:32  csupport
 * 1.  check memory_out_time before signaling out-of-memory
 * 2.  go to tester if out of memory
 * 
 * Revision 1.2  90/08/16  10:32:19  steve
 * added page fault counting to systems which support getrusage()
 * the page counting use to be in faults.c which is now unneeded.
 * 
 * Revision 1.1  90/08/07  15:38:26  configtw
 * Initial revision
 * 
*/
char gvt_id [] = "@(#)gvt2.c    1.9\t10/23/89\t08:53:45\tTIMEWARP";



#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"

extern VTime min_msg_time;
int gvtCount;

int     resendState = TRUE;     /* set false when state is nak'd */
long run_time, run_time_2;
double seconds;

VTime MinLvt;

int myNum;
int numNodes;
int numFrom, numTo, from[2], to[2];
int Out0, Out1;

int numArrive;
#ifdef GETRUSAGE
static int init_faults = 0;
int total_faults;
#endif
#ifdef DLM
extern int migrGraph;
#endif

int     hlog = 0;
VTime   hlogVTime;
int     maxSlice = 0;           /* maximum object time slice for this node */
Ocb*    maxSliceObj = NULL;     /* object corresponding to maxSlice */
VTime   maxSliceTime;           /* time for above object */
int     sliceTime;              /* time for current slice */

extern long highEpt ;
extern long highestEpt ;
extern long critNode ;
extern long nodesReporting;
extern Name critObject ;
extern long critEnabled ;

/*
   Calculate which nodes send us a GVT start message.  Put the number
   of such nodes into numFrom, and their node numbers into from[].
   Also calculate which nodes we send the GVT start message to, put
   the number of such nodes into numTo, and the node numbers into to[].
   See Steve Bellenot's paper on "Global Virtual Time Algorithms".
*/

FUNCTION gvtcfg()
{
	int Mid0, Mid1, myInv, OutFrom, In0, In1, InTo;
	int pen0, pen1;

	nodesReporting = tw_num_nodes;

	myNum = tw_node_num;          /* this nodes number */
	numNodes = miparm.maxnprc;  /* total number of nodes */

	MinLvt = posinfPlus1;

	numArrive = 0;

	Mid0 = ( numNodes - 1 )/2;
	Mid1 = ( numNodes & 0x01 )? Mid0 : Mid0 + 1;

	/* matching node on other tree */
	myInv = numNodes - myNum - 1;

	/* for binary tree with root 0 */
	Out0 = 2 * myNum + 1;               /* upper child number */
	Out1 = Out0 + 1;                    /* lower child number */
	OutFrom = ( myNum - 1 )/2;          /* parent number */

	/* for binary tree with root numNodes-1 */
	In0 = 2 * myNum - numNodes;         /* upper child number */
	In1 = In0 - 1;                      /* lower child number */
	InTo = ( numNodes + myNum + 1 )/2;  /* parent number */

	for ( pen0 = 1; pen0 < Mid0 + 1; pen0 = 2 * pen0 + 1 )
	{
		;
	}
	pen0 = pen0/2 - 1;
	pen1 = numNodes - pen0 -1;

	/* the two trees can share the bottom row */
	if ( pen1 <= 2 * pen0 + 3 )
	{
		Mid0 = pen1 - 1;
		Mid1 = pen0 + 1;
	}

  Debug

	if ( myNum == 0 )
	{   /* left root */
		numFrom = 0;
	}
	else if ( myNum <= Mid0 )
	{   /* in left tree */
		numFrom = 1;
		from[0] = OutFrom;
	}
	else if ( Mid1 <= In1 )
	{   /* right tree */
		numFrom = 2;
		from[0] = In0;
		from[1] = In1;
	}
	else if ( Mid1 == In0 )
	{   /* extra element between trees (eg 7->11 with 15 nodes) */
		numFrom = 1;
		from[0] = In0;
	}
	else
	{   /* input from matching node on left tree */
		numFrom = 1;
		from[0] = myInv;
	}

	if ( myNum == numNodes - 1 )
	{   /* right root */
		numTo = 0;
	}
	else if ( myNum >= Mid1 )
	{   /* in right tree */
		numTo = 1;
		to[0] = InTo;
	}
	else if ( Mid0 >= Out1 )
	{   /* in left tree */
		numTo = 2;
		to[0] = Out0;
		to[1] = Out1;
	}
	else if ( Mid0 == Out0 )
	{   /* extra element between trees (eg 3->7 with 15 nodes) */
		numTo = 1;
		to[0] = Out0;
	}
	else
	{   /* output to matching node on right tree */
		numTo = 1;
		to[0] = myInv;
	}

}

FUNCTION gvtinterrupt ()
{
  Debug

	gvtstart();
	dispatch();
}

FUNCTION gvtinit ()
{
  Debug

	if ( tw_node_num== 0 )
	{  /* if node 0 initiate gvtinterrupt routine every second */
#ifdef MICROTIME
		schedule_next_gvt();
#else
		timrint ( gvtinterrupt );
#endif
	}
}

FUNCTION gvtmessage ( type, time, dest )

	Int         type;
	VTime       time;
	Int         dest;
{
	Msgh        *p;
	Gvtmsg      *q;

  Debug

	p = sysbuf (); /*SFB maybe sometimes output_buf()??? */
	q = (Gvtmsg *) (p + 1);
	q->msgtype = type;
	q->sender = tw_node_num;
	q->time = time;

	sprintf ( p->snder, "GVT%d", tw_node_num );
	sprintf ( p->rcver, "GVT%d", dest );
	p->sndtim = gvt;
	p->rcvtim = time;

	sysmsg ( GVTSYS, p, sizeof (Gvtmsg), dest );
}

FUNCTION gvtproc ( gvt_message )

	Gvtmsg      *gvt_message;
{

  Debug

#ifdef TIMING
#define GVT_TIMING_MODE 11
	start_timing ( GVT_TIMING_MODE );
#endif

	switch ( gvt_message->msgtype )
	{
		case GVTSTART:          /* command to start a local GVT calculation */
			gvtstart ();
			break;

		case GVTLVT:            /* incoming minimum LVT value */
			gvtlvt (gvt_message->time );
			break;

		case GVTUPDATE:         /* announcement of new GVT value */
			gvtupdate ( gvt_message->time );
			break;
	}

#ifdef TIMING
	stop_timing ();
#endif
}

/* start the gvt calculation process */

FUNCTION gvtstart ( )

{
	int i;

  Debug


	if ( numFrom == 2 )
	{  /* 2 inputs expected--wait for both */
		if ( numArrive == 0 ) 
		{
			numArrive = 1;
			return;             /* if only 1 is here yet */
		}
		else
		{
			numArrive = 0;      /* both have arrived--continue */
		}
	}

	/* lvtstart (); replace with: */
	logmsg ();  /* calculate min_msg_time */

	if ( numTo == 2 )
	{  /* pass this message on */
		gvtmessage ( GVTSTART, gvt, to[0] );
		gvtmessage ( GVTSTART, gvt, to[1] );
	}
	else if ( numTo == 1 )
	{
		gvtmessage ( GVTSTART, gvt, to[0] );
	}
	else /* we are the last one */
	{
		gvtlvt ( MinLvt );
	}
}

FUNCTION gvtlvt ( lvt )

	VTime       lvt;    /* minimum LVT value */
{

  Debug


/* from old gvtstop */
	if ( numArrive == 0 ) 
	{   /* this is the first pass through gvtlvt() */
		lvtstop ( &MinLvt );    /* calculate our local virtual time */
	}

/* from old gvtlvt */
	if ( ltVTime ( lvt, MinLvt ) )
	{  /* passed in time is less */
		MinLvt = lvt;   /* remember the passed in time */
	}


	if ( numTo == 2 )
	{  /* we're expecting 2 of these messages */
		if ( numArrive == 0 ) 
		{

			numArrive = 1;
			return;     /* if we've only received the first */
		}
		else
		{  /* both messages have arrived */

			numArrive = 0;
		}
	}


	if ( numFrom == 2 )
	{  /* we need to send 2 of these messages (in reverse) */
		gvtmessage ( GVTLVT, MinLvt, from[0] );
		gvtmessage ( GVTLVT, MinLvt, from[1] );
	}
	else if ( numFrom == 1 )
	{  /* only send 1 */
		gvtmessage ( GVTLVT, MinLvt, from[0] );
	}
	else /* we are the last one */
	{

		gvtupdate ( MinLvt );   /* start the update cycle */

	}
}

STime gvt_sync = NEGINF+1;

VTime oldgvt1 = NEGINF;
VTime oldgvt2 = NEGINF;
VTime dynWindow = POSINF;
extern double windowMultiplier;

FUNCTION gvtupdate ( newgvt )

	VTime       newgvt;         /* new value of GVT */
{
	char        pvts[20];
	char        min_vts[20];
	char        newgvts[20];
	char        loggvts[20];

	extern int mem_stats_enabled;
	extern int no_gvtout;
	extern int qlog;
	char buff[MINPKTL];

#ifdef TIMING
	extern int timing[20];
	extern int timing_mode;
	int i;

#ifdef MARK3
	static char stats[200];

	extern int inttime;
#endif
#endif

  Debug

/* instead of the B'Cast, do a tree output */
	if ( Out0 < numNodes )
	{
		gvtmessage ( GVTUPDATE, newgvt, Out0, 0 );
	}

	if ( Out1 < numNodes )
	{
		gvtmessage ( GVTUPDATE, newgvt, Out1, 0 );
	}

#ifdef GETRUSAGE
	if ( init_faults  )
	{
/*
		if ( 0 )
			print_faults();
*/
	}
	else
	{
		set_faults();
		init_faults = 1;
	}
#endif

/* from gvtcalc */
	/* reset these */
	MinLvt = posinfPlus1;

	resendState = TRUE; /* ok to resend previously nak'd states */

/* from gvtupdate */
	if ( ltSTime ( gvt.simtime, posinf.simtime ) )
	{  /* our last gvt was not posinf */
		if ( gtSTime ( newgvt.simtime, gvt_sync ) )
		{  /* all nodes are past the initial starting time */
			if ( leSTime ( gvt.simtime, gvt_sync ) )
			{  /* our last gvt was not past the initial start time */

				run_time = clock ();    /* start the run time clock */
#ifdef TIMING
				for ( i = 0; i < 20; i++ )
					timing[i] = 0;
#endif
			}
			else
				do_timing();    /* cumulate timing figures */
		}

		if ( tw_node_num == 0 )
		{  /* convert times to strings on node 0 */
			ttoc ( pvts, pvt );
			ttoc ( min_vts, min_vt );
			ttoc ( newgvts, newgvt );

			if ( ! no_gvtout )
			{  /* print the new gvt */
#ifdef SUN
				_pprintf ( "Seconds: %6.2f   Gvt: %s%8d%8d\n", seconds, 
					newgvts,newgvt.sequence1,newgvt.sequence2 );
#endif

#ifdef TRANSPUTER
				_pprintf ( "Seconds: %6.2f   Gvt: %s\n", seconds, newgvts );
#endif

#ifdef BBN
				printf ( "Seconds:\t%6.2f\t\tGvt:\t%s%8d%8d\n", seconds,
					newgvts,newgvt.sequence1,newgvt.sequence2 );
#endif

#ifdef MARK3
				extern int subcube_num;

				printf ( "%d-- Seconds:\t%6.2f\t\tGvt:\t%s\n", subcube_num,
						seconds, newgvts );
/*
				{
					char gvt_line[80];

					sprintf ( gvt_line, " -- Pvt: %s MinVt: %s Gvt: %s\n",
						pvts, min_vts, newgvts);

					send_message ( gvt_line, strlen ( gvt_line ) + 1, CP, GVT_DATA );
				}
*/
#endif
#ifdef DLM
				if ( migrGraph )
				{
					ttoc1 (loggvts, newgvt );
					sprintf ( buff, "Gvt  %s \n", newgvts);

					send_to_IH ( buff, strlen ( buff ) + 1, MIGR_LOG );
				}
#endif
			}
		}
	}

#ifdef TIMING
#ifdef MARK3
	if ( eqSTime ( newgvt.simtime, posinfPlus1.simtime ) )
	{
		timing[timing_mode] += mark3time ();

	sprintf ( stats, "Node %d Gvt %d Pvt %d MinVt %d Tester %3.2f Timewarp %3.2f Objects %3.2f System %3.2f Idle %3.2f Rollback %3.2f Queue %3.2f Sched %3.2f Dlvr %3.2f Serve %3.2f Objend %3.2f Gvt %3.2f Ints %3.2f\n",
		tw_node_num, gvt, pvt, min_vt,
		((float) timing[0]) / 500000.0,
		((float) timing[1]) / 500000.0,
		((float) timing[2]) / 500000.0,
		((float) timing[3]) / 500000.0,
		((float) timing[4]) / 500000.0,
		((float) timing[5]) / 500000.0,
		((float) timing[6]) / 500000.0,
		((float) timing[7]) / 500000.0,
		((float) timing[8]) / 500000.0,
		((float) timing[9]) / 500000.0,
		((float) timing[10]) / 500000.0,
		((float) timing[11]) / 500000.0,
		((float) inttime ) * 4.34 / 1000000.0 );

	send_message ( stats, strlen ( stats ), CP, STATS_DATA );
	}
#endif
#ifdef SUN
	_pprintf ( "Tester %3.2f Timewarp %3.2f Objects %3.2f System %3.2f\n",
		((float) timing[0]) / 1000000.0,
		((float) timing[1]) / 1000000.0,
		((float) timing[2]) / 1000000.0,
		((float) timing[3]) / 1000000.0 );
#endif
#endif

	/* If there has been an increase in simtime over the last gvt tick, 
		bump up the window.  Otherwise, leave the window alone. */

	if ( newgvt.simtime - gvt.simtime > 0 )
	{
		dynWindow.simtime = gvt.simtime +  windowMultiplier * (
					(newgvt.simtime - gvt.simtime ) ) ;
	}
	dynWindow.sequence1 = dynWindow.sequence2 = 10000000;


	if ( dynWindow.simtime > POSINF )
		dynWindow.simtime = POSINF;


	oldgvt2 = oldgvt1;
	oldgvt1 = gvt;

	/* now set the new time */
	gvt = min_vt = newgvt;      /* must be done before gcpast */

if ( ltVTime ( min_msg_time, gvt ) )
{
	  twerror("gvtupdate:  min_msg_time %f below new gvt %f\n",
			  min_msg_time.simtime, gvt.simtime);
	  tester();
}


#ifdef DLM
/*
	if ( tw_node_num == 0 && 
		oldgvt2.simtime != NEGINF  && 
		eqVTime ( gvt, oldgvt1 ) && 
		eqVTime ( oldgvt1, oldgvt2 ) )
	{
		_pprintf ( "GVT repeats three times\n");
		tester();
	}
*/
#endif DLM


	gvtcount++;

	gcpast ();

	if ( mem_stats_enabled )
	{  /* do memory stats */
		mem_stats ();
	}



	if ( tw_node_num == GVTNODE )
	{
		if ( eqSTime ( newgvt.simtime, posinfPlus1.simtime ) )
		{       /* final time has been reached */
#ifdef BBN

		printf
		(
			"\nSimulation Over!\n\nElapsed time %.2f seconds\n\n",
			seconds
		);
#endif
#ifdef MARK3

		printf 
		(
			"\n%d-- Simulation Over!\n\n%d-- Elapsed time %.2f seconds\n\n",
			subcube_num, subcube_num, seconds
		);
#endif
#ifdef SUN
		printf 
		(
			"\n-- Simulation Over!\n\n-- Elapsed time %.2f seconds\n\n",
			seconds
		);
#endif
#ifdef TRANSPUTER
		_pprintf 
		(
			"\n-- Simulation Over!\n\n-- Elapsed time %.2f seconds\n\n",
			seconds
		);
#endif
		}
		else
		{
			extern int interval, delta;
			extern STime interval_change_time;

			if ( geSTime ( newgvt.simtime, interval_change_time ) )
			{
				interval = 1;
#ifdef MARK3
				delta = 250;
#endif
			}
#ifdef MICROTIME
			schedule_next_gvt();
#else
			timrint ( gvtinterrupt );
#endif
		}
	}

	if ( eqSTime ( newgvt.simtime, posinf.simtime ) )
	{
		extern STime cutoff_time;

#ifdef GETRUSAGE
		total_faults = num_faults();
/*
		_pprintf ( "Total Faults %d\n", total_faults );
*/
#endif

		cutoff_time = POSINF;
		term_objects ();
	}
	else
	if ( eqSTime ( newgvt.simtime, posinfPlus1.simtime ) )
	{
		dump_stats ( seconds );
		if ( critEnabled == TRUE )
		{
			calculateCritPath();
		}
		else
		{	
			send_to_IH ( "Simulation End\n", 16, SIM_END_MSG );
		}
	}
}

FUNCTION term_objects ()
{
	Ocb * o, * n;
	Msgh * msg;

	for ( o = fstocb_macro; o; o = n )
	{
		n = nxtocb_macro (o);

		if ( eqSTime ( o->svt.simtime, posinfPlus1.simtime ) )
			continue;

		if ( o->typepointer->term && 
				eqSTime ( o->phase_end.simtime, posinfPlus1.simtime ) )
		{
			if ( eqSTime ( o->svt.simtime, posinf.simtime )
			&&   o->runstat == BLKINF )
			{
				msg = make_message ( TMSG, "TW", o->svt, o->name, 
									    o->svt, 0, 0 );

				nq_input_message ( o, msg );
			}
		}
		else
		{
			l_remove ( o );
			o->svt = posinfPlus1;
			l_insert ( l_prev_macro ( _prqhd ), o );
		}
	}
}

/* cumulate run time in "seconds" */

do_timing()
{
#ifdef SUN
	run_time_2 = clock();
	seconds += (run_time_2 - run_time) / (double) TICKS_PER_SECOND;
#endif
#ifdef MARK3
	run_time_2 = clock();
	seconds += (run_time_2 - run_time) / 1000000.;
#endif
#ifdef TRANSPUTER
	run_time_2 = clock();
	seconds += (run_time_2 - run_time) / 1000000.;
#endif
#ifdef BBN
	run_time_2 = clock();
	seconds += ( (run_time_2 - run_time) * 62.5 ) / 1000000.;
#endif
	run_time = run_time_2;
}

/* calculate our local virtual time */

FUNCTION lvtstop ( t )

	VTime       *t;
{
	Ocb         *o;
	VTime       min;

#if PARANOID
	Ocb         *firstOcb = NULL;
#endif

	pvt = posinfPlus1;  /* init to the max */

	for ( o = fstocb_macro; o; o = nxtocb_macro (o) )
	{  /* cycle through the object list for this node */
		if ( o->runstat == ITS_STDOUT )
			continue;           /* skip stdout objects */

		/*  This code is in place to test for cases in which the first
				ocb in the scheduler queue is not the one with the earliest
				svt. */

#if PARANOID
		if ( firstOcb == NULL )
			firstOcb = o;
#endif

		if ( ltVTime ( o->svt, pvt ) )
		{
			pvt = o->svt;       /* new minimum--save it */

#if PARANOID
			if ( o != firstOcb )
			{
				_pprintf("lvtstop: scheduler misordering during gvt calculation\n");
				_pprintf("      %s at %f before %s at %f\n", firstOcb->name,
						firstOcb->svt.simtime, o->name, o->svt.simtime );
				tester();
			}
#endif

		}
#if !PARANOID
		break;
#endif

	}


	/* calculate minimum message & object migration time */
	minmsg ( &min_vt );

	if ( ltVTime ( pvt, min_vt ) )
	{  /* pvt is earlier--use it */
		min_vt = pvt;
	}

if ( ltVTime ( min_vt, gvt ) )
{
_pprintf("min_vt %f less than gvt %f after checking pvt\n",
	  min_vt.simtime, gvt.simtime);
tester();
}

	/* calculate the minimum time of pending actions */
	min = MinPendingList ();

	if ( ltVTime ( min, min_vt ) )
	{  /* min is earlier--use it */
		min_vt = min;
	}

if ( ltVTime ( min_vt, gvt ) )
{
_pprintf("min_vt %f less than gvt %f after checking pending list\n",
	  min_vt.simtime, gvt.simtime);
tester();
}


	if ( eqSTime ( min_vt.simtime, posinfPlus1.simtime ) )
	{  /* no minimum time found */
		extern int states_to_send, ocbs_to_send;

		if ( states_to_send || ocbs_to_send )
		{
		   _pprintf("Migrations not complete at POSINF+1, %d states, %d ocbs\n",
				states_to_send, ocbs_to_send );

			min_vt.simtime = POSINF;
		}
	}

	*t = min_vt;        /* send time back to the caller */
#ifdef DLM
	local_min_vt = min_vt;
#endif DLM

  Debug

	if ( ltVTime ( min_vt, gvt ) )
	{  /* uh oh, we're earlier than we should be */
		twerror ( "lvtstop E min_vt %.2f less than old gvt %.2f",
			min_vt.simtime, gvt.simtime );
		tester ();
	}

	return;
}       /* lvtstop */

#ifdef GETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>

int initFaults;
int lastFaultCount;

set_faults()
{
	struct rusage u;

	getrusage (0, &u);
	lastFaultCount = initFaults =  u.ru_majflt;
}

num_faults()
{
	struct rusage u;

	getrusage (0, &u);
	return  u.ru_majflt - initFaults;
}

/* debug code
int printCount;

print_faults()
{
	int thisCount;
	struct rusage u;


	printCount++;
	getrusage (0, &u);
	thisCount =  u.ru_majflt;

	if ( thisCount > lastFaultCount )
		_pprintf
		( "%4d -- %d more faults -- total: %d\n", 
			printCount, thisCount - lastFaultCount,
			thisCount - initFaults );

	lastFaultCount = thisCount;
}
*/
#endif

