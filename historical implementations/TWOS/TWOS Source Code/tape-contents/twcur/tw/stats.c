/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	stats.c,v $
 * Revision 1.4  91/07/17  15:12:45  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/07/09  14:45:13  steve
 * Added MicroTime support
 * 
 * Revision 1.2  91/06/03  12:26:52  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:41:07  configtw
 * Initial revision
 * 
*/
char stats_id [] = "@(#)stats.c 1.29\t8/8/89\t13:21:20\tTIMEWARP";

/*

Purpose:

		The routines in this module support the statistics output.  These
		routines should run on both the Hypercube and the VAX.  The strategy
		of statistics gathering is as follows:  During normal running of the
		Executive, statistics collection is limited to incrementing a variable
		in a structure or to performing a simple if-elseif-else test and then
		incrementing a variable.  During the output of statistics, when the
		routine dump_stats() is called, statistics collection is expanded to
		include looping through queues (OCB lists, or input, output, and state
		queues of an object) in order to count messages, space, and the like.

		The function to call whenever you wish statistics output is 
		dump_stats ().  That function is NOT in this module, but is in the 
		modules VAXIO.C and CUBEIO.C.  The former module, as its name implies, 
		contains VAX-specific statistics output, which includes writing to a 
		file.  The latter module contains only bare-bones output for the cube,
		 which is currently dumped using the function dprintf. 

		NOTE TO HYPERCUBE USERS: Please read the introduction to CUBEIO.C 
		before attempting to use dump_stats.

Functions:

		smsg_stat(m) - increment send statistics for a message
				Parameters - Ocb *o
				Return - Always returns 0

		rmsg_stat(o,m) - increment the receive statistics for a message 
				Parameters - Ocb *o
				Return - Always returns 0

		stats_zap(o,m) - increment the zap statistic
				Parameters - Ocb *o
				Return - Always returns 0

		stats_garbtime(o,msg,nmsg) - do stats at garbage collection time
				Parameters - Ocb *o
				Return - Always returns 0

		stats_garbouttime(o,msg,nmsg) - do output queue stats at garbage
						collection time
				Parameters - Ocb *o
				Return - Always returns 0

		do_stats(o) - calculate stats for an object
				Parameters - Ocb *o
				Return - Always returns 0

Implementation:

		Most of these routines are very straightforward.  Typically,
		they compute a single statistic, then update a particular
		statistics field, either in an object, or in the tw_stats
		block.  A representative function will be discussed, and only
		those functions that differ greatly in form will get much 
		further attention here.

		smsg_stat() checks to see if the message is going in reverse.
		If it is, and its destination is on this node, one of several
		ocb fields is incremented, based on the message's type.

		rmsg_stat() increments a field in the ocb for any forward message.

		stats_zap() increments a field in the ocb based on the type of the
		message.

		stats_garbtime() runs through a queue, looking at the messages.
		Each message causes a counter to be incremented, based on msg type.
		(Multipacket msgs cause only one incrementation, not one per packet.)
		Also, each bundle in the queue causes a counter to be incremented.
		Multipacket msgs are also counted.

		stats_garbouttime() is similar to stats_garbtime(), but works on
		the output queue, and keeps different counters.

		do_stats() calls several statistics routines.

*/
#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"


FUNCTION smsg_stat ( o, m )

	Ocb *o;
	Msgh *m;
{
	if ( m->mtype != EMSG )
		return; /* keep stats only for event messages */

	if ( isforward_macro ( m ) )
	{   /* it's a forward message */
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

/* increment the count */

		if ( isposi_macro (m) )
			o->stats.eposfs++;
		else
			o->stats.enegfs++;
	}
	else
	{   /* reverse message */
		if ( isposi_macro ( m ) )
			o->stats.eposrs++;
		else
			o->stats.enegrs++;
	}
}

FUNCTION rmsg_stat ( o, m )

	Ocb *o;
	Msgh *m;
{
	if ( m->mtype != EMSG )
		return;

	if ( m->flags & MOVING )
		return;

	if ( isforward_macro ( m ) )
	{
		if ( isposi_macro ( m ) )
			o->stats.eposfr++;
		else
			o->stats.enegfr++;
	}
	else
	{
		if ( isposi_macro ( m ) )
			o->stats.eposrr++;
		else
			o->stats.enegrr++;
	}
}

FUNCTION stats_zaps ( o, m )    /* count number of annihilations */

	Ocb *o;
	Msgh *m;
{
	if ( m->mtype == EMSG )
		o->stats.ezaps++;
}

FUNCTION stats_garbtime ( o, msg, nmsg )        /* count committed events */

	Ocb            *o;
	Msgh           *msg;        /* pointer to current message (one we're
								 * considering counting) */
	Msgh           *nmsg;       /* pointer to next message in the queue */
{
	switch ( msg->mtype )
	{
		case EMSG:

			o->stats.cemsgs++;  /* increment event message count */

			/*
			 * if (we're at end of queue or the next message is at a different
			 * virtual time) count the message as a committed event 
			 */

			if ( nmsg == NULL || neVTime ( msg->rcvtim, nmsg->rcvtim ) )
				o->stats.cebdls++;

			break;

		case TMSG:

			if ( nmsg != NULL )
				o->stats.cebdls--;

			break;

		case DYNCRMSG:
		case CMSG:

			o->stats.ccrmsgs++;
			break;

		case DYNDSMSG:

			o->stats.cdsmsgs++;
			break;
	}
}

FUNCTION stats_garbouttime ( o, msg, nmsg )     /* count committed events */

	Ocb            *o;
	Msgh           *msg;        /* pointer to current message (one we're
								 * considering counting) */
	Msgh           *nmsg;       /* pointer to next message in the queue */
{
	switch ( msg->mtype )
	{
		case EMSG:

			o->stats.coemsgs++; /* increment event message count */

			/*
			 * if (we're at end of queue or the next message is at a different
			 * virtual time) count the message as a committed event 
			 */

			if ( nmsg == NULL || neVTime ( msg->sndtim, nmsg->sndtim ) )
				o->stats.coebdls++;

			break;
	}
}
