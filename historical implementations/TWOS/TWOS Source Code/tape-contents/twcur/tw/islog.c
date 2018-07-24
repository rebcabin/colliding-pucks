/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	islog.c,v $
 * Revision 1.6  91/07/17  15:08:55  judy
 * New copyright notice.
 * 
 * Revision 1.5  91/07/09  13:48:47  steve
 * Added Sun support
 * 
 * Revision 1.4  91/06/03  12:24:26  configtw
 * Tab conversion.
 * 
 * Revision 1.3  91/04/01  15:37:29  reiher
 * Cosmetic change to output statement for IS_LOG_ENTRY, setting the
 * send_message() type field to a symbol, rather than an integer constant.
 * 
 * Revision 1.2  90/12/10  10:41:58  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.1  90/08/07  15:38:36  configtw
 * Initial revision
 * 
*/
char islog_id [] = "@(#)islog.c 1.7\t9/26/89\t15:27:03\tTIMEWARP";


#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"
#include "logdefs.h"


IS_LOG_ENTRY *IS_log, *IS_logp, *IS_loge;
int num_IS_entries;

#define MIN_IS_DELTA    100             /* .1 sec       */
#define MAX_IS_DELTA    10000           /* 10 secs      */
 
int     IS_delta;
int     IS_clock1, IS_clock2;

extern int host_input_waiting;

FUNCTION I_speedup ()
{
	Ocb         *o;
	VTime       t;
	VTime       minreg;
	double      cputime;

  Debug

	minreg = posinfPlus1;

	if ( ltSTime ( gvt.simtime, posinfPlus1.simtime ) && !host_input_waiting )
	{
		t = posinfPlus1;
		minmsg ( &t );

		for (o = fstocb_macro; o; o = nxtocb_macro (o))
		{ 
			if ( o->runstat == ITS_STDOUT )
				continue;

			if ( ltVTime ( o->svt, minreg ) )
			{
				minreg = o->svt;
			}
		}
 
		if ( ltVTime ( minreg, t ) )
		{
			t = minreg;
		}

		IS_clock2 = clock();
#ifdef BBN
		cputime = ( ( (IS_clock2 - IS_clock1 ) *62.5 ) /1000000.);
		record_min_vt ( cputime, t);
		malarm ( IS_delta );
#endif
	 }
}

init_islog ( IS_log_size, delta )

	int *IS_log_size;
	int *delta;
{
	if ( *delta < MIN_IS_DELTA || *delta > MAX_IS_DELTA )
	{
		_pprintf (" IS_delta setting out of bounds %d msecs\n", *delta );
		return;
	}

	IS_log = IS_logp = (IS_LOG_ENTRY *) m_allocate
						( *IS_log_size * sizeof (IS_LOG_ENTRY) );
	if ( IS_log == NULL )
	{
		_pprintf ( "can't allocate IS_log space\n" );
		tw_exit (0);
	}

	if ( tw_node_num == 0 )
	{
		_pprintf ("IS_logsize == %ld IS_delta == %ld\n",
						*IS_log_size,
						*delta
						);
	}

	IS_loge = IS_log + *IS_log_size;
	IS_delta = *delta;
	IS_clock1 = IS_clock2 =clock();
#ifdef BBN
	butterfly_msigalarm ( I_speedup );
	malarm ( IS_delta );
#endif
}

FUNCTION record_min_vt ( cputime, vt )

	double  cputime;
	VTime   vt;
{
	extern STime gvt_sync;

	if ( eqVTime (neginf, vt ) )
	{  
	  /* Don't record an instantaneous speedup log entry until we're
			  past initialization. */
	  return;
	}  

	num_IS_entries++;

	if ( IS_logp == NULL )
	{
		static int print_once;
		if ( print_once == 0 )
		{
			print_once = 1;
			_pprintf ( "no IS log space\n" );
		}
		return;
	}

	if ( IS_logp >= IS_loge )
	{
		static int print_once;
		if ( print_once == 0 )
		{
			print_once = 1;
			_pprintf ( "IS log full\n" );
		}
		return;
	}

	IS_logp->seqnum = num_IS_entries;
	IS_logp->cputime = cputime;
	IS_logp->minvt = vt;
	IS_logp++;
}
 
IS_dumplog ()
{
	register int i;

	_pprintf ( "num_IS_entries = %d\n", num_IS_entries );

	IS_logp->cputime = 1000000.;
	IS_logp++;

	while ( IS_log < IS_logp )
	{
		i = IS_logp - IS_log;

		if ( i > 16 )
			i = 16;

#define ISLOG_DATA 10
		/* Msg type 10 is ISLOG_DATA.  But using a "10" instead of the
				symbol makes it mighty hard to find where these messages
				get sent. */

		send_message ( IS_log, sizeof(IS_LOG_ENTRY) * i, CP, ISLOG_DATA );

		IS_log += i;

#ifdef SUN
		recv.buf = rm_buf;
#endif
#ifdef BBN
		recv.buf = rm_buf;
#endif
		recv.source = ANY;
		get_msg_w ( &recv );
#ifdef MARK3
		give_buf ( &recv );
#endif
	}

	if ( tw_node_num == 0 )
	{
#ifdef SUN
		recv.buf = rm_buf;
#endif
#ifdef BBN
		recv.buf = rm_buf;
#endif
		recv.source = ANY;
		get_msg_w ( &recv );
#ifdef MARK3
		give_buf ( &recv );
#endif
	}
}
