/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	msglog.c,v $
 * Revision 1.5  91/07/17  15:10:55  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/07/09  14:35:54  steve
 * Add MicroTime and Sun support
 * 
 * Revision 1.3  91/06/03  12:25:41  configtw
 * Tab conversion.
 * 
 * Revision 1.2  90/12/10  10:51:40  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.1  90/08/07  15:40:28  configtw
 * Initial revision
 * 
*/
char msglog_id [] = "@(#)msglog.c       1.21\t9/26/89\t15:35:30\tTIMEWARP";


#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"
#include "logdefs.h"



MSG_LOG_ENTRY *mlog, *mlogp, *mloge;
int num_msg_entries;

msglog ( msg_log_size )

	int *msg_log_size;
{
	mlog = mlogp = (MSG_LOG_ENTRY * ) m_allocate
						( *msg_log_size * sizeof (MSG_LOG_ENTRY) );
	if ( mlog == NULL )
	{
		_pprintf ( "can't allocate msg log space %d\n",
				*msg_log_size * sizeof (MSG_LOG_ENTRY) );
		tester ();
	}
	mloge = mlog + *msg_log_size;
}

msglog_entry ( msg )

	Msgh * msg;
{
	if ( leSTime( msg->sndtim.simtime, neginfPlus1.simtime ) || 
				geSTime( msg->rcvtim.simtime, posinf.simtime ) )
		return;

	num_msg_entries++;

	if ( mlogp == NULL )
	{
		static int print_once;
		if ( print_once == 0 )
		{
			print_once = 1;
			_pprintf ( "no msg log space\n" );
		}
		return;
	}

	if ( mlogp >= mloge )
	{
		static int print_once;
		if ( print_once == 0 )
		{
			print_once = 1;
			_pprintf ( "msg log full\n" );
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
	butterflytime();
#endif
#endif
	mlogp->twtimet = node_cputime;      /* nq_input_message */
	mlogp->twtimef = msg->cputime;      /* smsg_stat */
	mlogp->hgtimet = msg->msgtimet;     /* read_the_mail */
	mlogp->hgtimef = msg->msgtimef;     /* send_from_q */
	strcpy ( mlogp->snder, msg->snder );
	strcpy ( mlogp->rcver, msg->rcver );
	mlogp->sndtim = msg->sndtim.simtime;
	mlogp->rcvtim = msg->rcvtim.simtime;
	mlogp->id_num = msg->gid.num;
	mlogp->mtype = msg->mtype;
	mlogp->flags = msg->flags;
	mlogp->len = sizeof(Msgh)+msg->txtlen;
	mlogp++;
}

dump_mlog ()
{
#ifdef BBN_SMALL
	char filename[10];
	FILE * fp;

	_pprintf ( "num_msg_entries = %d\n", num_msg_entries );

	sprintf ( filename, "mlog%d", tw_node_num );

	fp = (FILE *) HOST_fopen ( filename, "w" );

	HOST_fwrite ( mlog, (char *)mlogp - (char *)mlog, 1, fp );

	HOST_fclose ( fp );

	_pprintf ( "msglog done\n" );

#else

	int i;

	_pprintf ( "num_msg_entries = %d\n", num_msg_entries );

	mlogp->twtimet = MAXINT;
	mlogp++;

	while ( mlog < mlogp )
	{
		i = mlogp - mlog;

		if ( i > 8 )
			i = 8;

		send_message ( mlog, sizeof(MSG_LOG_ENTRY) * i, CP, MSG_DATA );

		mlog += i;

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
