/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	flowlog.c,v $
 * Revision 1.4  91/07/17  15:08:12  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/07/09  13:39:30  steve
 * Replaced 7 with FLOW_DATA. Added Sun support.
 * 
 * Revision 1.2  91/06/03  12:24:07  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:38:18  configtw
 * Initial revision
 * 
*/
char flowlog_id [] = "@(#)flowlog.c     1.7\t9/26/89\t15:27:42\tTIMEWARP";


#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"
#include "logdefs.h"


FLOW_LOG_ENTRY *flog, *flogp, *floge;
int num_flow_entries;

flowlog ( flow_log_size )

	int *flow_log_size;
{
	flog = flogp = (FLOW_LOG_ENTRY *) m_allocate
								( *flow_log_size * sizeof (FLOW_LOG_ENTRY) );
	if ( flog == NULL )
	{
		_pprintf ( "can't allocate flow log space\n" );
		tw_exit (0);
	}
	floge = flog + *flow_log_size;
}

flowlog_entry ()
{
	extern STime gvt_sync;

	if ( leSTime ( xqting_ocb->svt.simtime, gvt_sync )
	||   geSTime ( xqting_ocb->svt.simtime, posinf ) )
		return;

	num_flow_entries++;

	if ( flogp == NULL )
	{
		static int print_once;
		if ( print_once == 0 )
		{
			print_once = 1;
			_pprintf ( "no flow log space\n" );
		}
		return;
	}

	if ( flogp >= floge )
	{
		static int print_once;
		if ( print_once == 0 )
		{
			print_once = 1;
			_pprintf ( "flow log full\n" );
		}
		return;
	}

	flogp->start_time = object_start_time;
	flogp->end_time = object_end_time;
	flogp->svt = xqting_ocb->svt.simtime;
	strcpy ( flogp->object, xqting_ocb->name );
	flogp++;
}

dumplog ()
{
#ifdef BBN_SMALL
	char filename[10];
	FILE * fp;

	_pprintf ( "num_flow_entries = %d\n", num_flow_entries );

	sprintf ( filename, "flog%d", tw_node_num );

	fp = (FILE *) HOST_fopen ( filename, "w" );

	HOST_fwrite ( flog, (char *)flogp - (char *)flog, 1, fp );

	HOST_fclose ( fp );

	_pprintf ( "flowlog done\n" );
#else 
	register int i;

	_pprintf ( "num_flow_entries = %d\n", num_flow_entries );

	flogp->start_time = MAXINT;
	flogp++;

	while ( flog < flogp )
	{
		i = flogp - flog;

		if ( i > 20 )
			i = 20;
		
		send_message ( flog, sizeof(FLOW_LOG_ENTRY) * i, CP, FLOW_DATA );

		flog += i;

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
#endif
}
