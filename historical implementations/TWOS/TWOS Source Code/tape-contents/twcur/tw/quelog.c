/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	quelog.c,v $
 * Revision 1.4  91/07/17  15:11:48  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/07/09  14:39:38  steve
 * Added MicroTime and Sun support
 * 
 * Revision 1.2  91/06/03  12:26:07  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:40:47  configtw
 * Initial revision
 * 
*/
char quelog_id [] = "@(#)quelog.c       1.5\t10/2/89\t16:52:53\tTIMEWARP";


#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"
#include "logdefs.h"


/* Dynamic Load Management utilization logging code. */


#ifdef DLM
extern int loadCount;
#endif DLM

Q_LOG_ENTRY *qlog, *qlogp, *qloge;
int num_que_entries;

quelog ( que_log_size )

	int *que_log_size;
{
	qlog = qlogp = (Q_LOG_ENTRY *) m_allocate ( *que_log_size * sizeof (Q_LOG_ENTRY) );
	if ( qlog == NULL )
	{
		_pprintf ( "can't allocate que log space %d\n",
				*que_log_size * sizeof (Q_LOG_ENTRY) );
		tester ();
	}
	qloge = qlog + *que_log_size;
}

quelog_entry ( utilization )
 
	float utilization;
{
	num_que_entries++;

	if ( qlogp == NULL )
	{
		static int print_once;
		if ( print_once == 0 )
		{
			print_once = 1;
			_pprintf ( "no que log space\n" );
		}
		return;
	}

	if ( qlogp >= qloge )
	{
		static int print_once;
		if ( print_once == 0 )
		{
			print_once = 1;
			_pprintf ( "que log full\n" );
		}
		return;
	}

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
	qlogp->loadCount = loadCount;
#endif DLM
	qlogp->gvt = gvt;
	qlogp->utilization = utilization;
	qlogp++;
}

dump_qlog ()
{
	register int i;

	_pprintf ( "num_que_entries = %d\n", num_que_entries );

	qlogp->gvt.simtime = POSINF+1;
	qlogp++;

	while ( qlog < qlogp )
	{
		i = qlogp - qlog;

		if ( i > 10 )
			i = 10;

		send_message ( qlog, sizeof(Q_LOG_ENTRY) * i, CP, QLOG_DATA );

		qlog += i;

		if ( i != 10 )
			_pprintf ( "sent %d qlog entries\n", i );

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
