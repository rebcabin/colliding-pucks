/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	hostifc.c,v $
 * Revision 1.6  91/11/01  09:38:11  reiher
 * Added code to print out the CRIT_LOG (PLR)
 * 
 * Revision 1.5  91/07/17  15:08:40  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/07/09  13:47:12  steve
 * replaced 3, 4 with STDOUT_DATA, STDOUT_TIME
 * 
 * Revision 1.3  91/06/03  12:24:20  configtw
 * Tab conversion.
 * 
 * Revision 1.2  91/03/26  09:26:10  pls
 * Add time parameter to brdcst_command().
 * 
 * Revision 1.1  90/08/07  15:38:29  configtw
 * Initial revision
 * 
*/
char hostifc_id [] = "@(#)hostifc.c     1.47\t9/12/89\t17:08:43\tTIMEWARP";


/*

Purpose:

		hostifc.c contains code for managing the interface to the computer
		hosting the parallel processor.  Most of this code has to do with
		file output.

Functions:

		send_stdout_msg() - on a Mark3, send a stdout message to the
						Counterpoint host
				Parameters - none
				Return - Always returns zero

		ih_msgproc(tw_msg) - call the appropriate handler for an output msg
				Parameters - Msgh * tw_msg
				Return - Always returns zero

		stdout_msg(tw_msg) - send a stdout message off-cube for output
				Parameters - Msgh * tw_msg
				Return - Always returns zero

		twerror_msg(tw_msg) - print an error message
				Parameters - Msgh * tw_msg
				Return - Always returns zero

		tw_stats_msg(tw_msg) - do nothing
				Parameters - Msgh * tw_msg
				Return - Always returns zero

		xl_stats_msg(tw_msg) - send a statistics message to the XL_STATS file
				Parameters - Msgh * tw_msg
				Return - Always returns zero

		stream_msg(tw_msg) - send a stream message to the STREAM file
				Parameters - Msgh * tw_msg
				Return - Always returns zero

		sim_end_msg(tw_msg) - shut down the simulation
				Parameters - Msgh * tw_msg
				Return - zero, or exits

Implementation:

		send_stdout_msg() is only defined for the Mark3 or BBN.  It looks
		at the stdout_q.  If there are no messages in it, and gvt has
		exceeded the time of the last time stdout was committed, call
		send_time_update().  If there are messages in the queue, and
		their rcvtime exceeds both stdout_ok_time and stdout_sent_time,
		call send_time_update(), as well.  On the other hand, if a 
		message's rcvtim does not exceed stdout_ok_time, and there aren't
		too many stdout acks pending, call send_message(), and remove
		the message from the queue.

		ih_msgproc() calls a routine appropriate to the type of its
		parameter message.

		stdout_msg() puts a message into the stdout_q.  If we're not on a Mark3,
		it simply write()s it to the appropriate file.

		twerror_msg() prints an error message.

		tw_stats_msg() does nothing at all.

		xl_stats_msg() writes a message to the XLSTATS file, opening that file
		if it is not already open. stream_msg() is the same, except for the
		name of the file written to.

		sim_end_msg() waits till all nodes have sent a simulation end message.
		Then it prints some messages to the XLSTATS file, closes that file,
		prints a simulation_end message, and calls tw_exit().

*/

#include <stdio.h> 
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

FILE * HOST_fopen ();

FILE * xl_stats = 0;
int nodes_ended;

#ifdef MARK3_OR_BBN

extern VTime stdout_sent_time;
extern VTime stdout_ok_time;
extern int max_stdout_acks;

send_stdout_msg ()
{
	register Msgh * msg;

	/* get the next stdout message in the queue */
	msg = (Msgh *) l_next_macro ( stdout_q );

	if ( l_ishead_macro ( msg ) )
	{  /* empty queue */
		if ( gtVTime ( gvt, stdout_sent_time ) )
			send_time_update ( gvt );   /* update stdout_sent_time */
		else
		if ( save_sim_end_msg )
		{  /* handle save_sim_end_msg */
			enq_msg ( save_sim_end_msg, 0 );
			save_sim_end_msg = NULL;
			messages_to_send--;
		}
	}
	else        /* something in the queue */
	if ( gtVTime ( msg->rcvtim, stdout_ok_time ) )
	{  /* can't yet commit msg */
		if ( gtVTime ( msg->rcvtim, stdout_sent_time ) )
			send_time_update ( msg->rcvtim );
	}
	else
	if ( stdout_acks_pending < max_stdout_acks )
	{  /* not too many ack's pending */
#ifdef MARK3
		send_message ( msg + 1, msg->txtlen - 1, CP, STDOUT_DATA );
#else
		if ( send_message ( msg + 1, msg->txtlen - 1, CP, STDOUT_DATA ) != -1 )
#endif
		{  /* msg was successfully sent */
			l_remove ( msg );           /* remove from queue */
			destroy_msg ( msg );        /* deallocate it */
			stdout_messages_to_send--;
			stdout_acks_pending++;
		}
	}
}


send_time_update ( time )

	VTime time;
{
#ifdef MARK3
	send_message ( &time, sizeof(time), CP, STDOUT_TIME );
#else
	/* send new stdout time to CP */
	if ( send_message ( &time, sizeof(time), CP, STDOUT_TIME ) != -1 )
#endif
		stdout_sent_time = time;        /* update stdout_sent_time */
}
#endif

/* handle messages from the host */

FUNCTION ih_msgproc ( tw_msg )

	Msgh * tw_msg;
{
  Debug

	switch ( tw_msg->mtype )
	{
		case EMSG:                      /* event message */

			stdout_msg ( tw_msg );      /* put in stdout_q */
			acceptmsg ( NULL );         /* throw away message */
			break;

		case TW_ERROR:                  /* time warp error message */

			twerror_msg ( tw_msg );     /* print the message */
			acceptmsg ( NULL );
			break;

		case XL_STATS:                  /* Excel formatted statistics */

			xl_stats_msg ( tw_msg );    /* copy message into stats */
			acceptmsg ( NULL );
			break;

		case MIGR_LOG:                  /* migration log */

			migr_log_msg ( tw_msg );    /* write to the log */
			acceptmsg ( NULL );
			break;

		case SIM_END_MSG:               /* end of simulation */

			acceptmsg ( NULL );
			sim_end_msg ();             /* exit if all nodes done */
			break;

		case CRT_ACK:                   /* create acknowledge message */

			acceptmsg ( NULL );
			break;      /* ignored */

		case CRIT_LOG:
			crit_log_msg ( tw_msg );
			acceptmsg ( NULL );
			break;
	}
}

/*  open the migration log file if necessary, and write this message to it */

migr_log_msg ( tw_msg )

	Msgh * tw_msg;
{
	static FILE * fp = 0;

	if ( fp == NULL )
		fp = HOST_fopen ( "MIGR_LOG", "w" );    /* open file if not yet open */

	HOST_fputs ( (char *) (tw_msg + 1), fp );   /* write out the message */
	HOST_fflush ( fp );
}

stdout_msg ( tw_msg )

	Msgh * tw_msg;
{
	static FILE * fp = 0;

	if ( no_stdout )
		return;

#ifdef MARK3_OR_BBN
	/* put this message in the stdout_q */
	l_insert ( l_prev_macro ( stdout_q ), tw_msg );
	stdout_messages_to_send++;
#else
	if ( fp == NULL )
		fp = HOST_fopen ( "STDOUT", "w" );
	HOST_fputs ( (char *) (tw_msg + 1), fp );
	HOST_fflush ( fp );
#endif
}

/* print an error message */

twerror_msg ( tw_msg )

	Msgh * tw_msg;
{
	_pprintf ( "\n%s", tw_msg + 1 );
}

extern List_hdr * free_pool;
extern int free_pool_size;

char * xl_stats_area;
char * xl_stats_ptr;

#define MAX_OBJS 1600
#define MAX_LINE 120
#define XL_STATS_AREA_SIZE ( MAX_OBJS * MAX_LINE )

extern int subcube_num;

xl_stats_msg ( tw_msg )

	Msgh * tw_msg;
{
	List_hdr * free;
	int i;

	if ( xl_stats == 0 )

#ifdef MARK3_OR_BBN

	{  /* file not yet open, so do it */
		if ( subcube_num != 0 )
		{
			char new_stats_name[80];

			sprintf ( new_stats_name, "%s%d", stats_name, subcube_num );

			xl_stats = HOST_fopen ( new_stats_name, "w" );
		}
		else
		 {
		   xl_stats = HOST_fopen ( stats_name, "w" );
		 }

		/* allocate space for the stats */
		xl_stats_area = (char *) m_allocate ( XL_STATS_AREA_SIZE );

		if ( xl_stats_area == NULL )
		{  /* space not available--get from free_pool */
			for ( free = l_next_macro ( free_pool ); free != free_pool;
				  free = l_next_macro ( free_pool ) )
			{
				l_remove ( free );
				m_release ( ((List_hdr *)free) - 1 );
			}
			xl_stats_area = (char *) m_allocate ( XL_STATS_AREA_SIZE );
/*
			if ( xl_stats_area == NULL )
				_pprintf ( "failed to allocate xl_stats_area again\n" );
			else
				_pprintf ( "got it after freeing free_pool!\n" );
*/
		}

		xl_stats_ptr = xl_stats_area;   /* init the pointer */
	}
#else
		xl_stats = HOST_fopen ( "XL_STATS", "w" );
#endif

	if ( xl_stats_ptr != NULL )
	{   /* ??? check before we copy */
		strcpy ( xl_stats_ptr, tw_msg+1 );      /* copy message */
		xl_stats_ptr += tw_msg->txtlen - 1;
		if ( xl_stats_ptr > xl_stats_area + XL_STATS_AREA_SIZE )
		  { 
				for (i=0; i<10000; i++) /* Wait a little while */
				{}
				/*_pprintf ( "xl_stats_area overflowed\n" );*/ 

		  }
	}
	else
	{  /* put out to file if necessary */
		HOST_fputs ( (char *) (tw_msg + 1), xl_stats );
		HOST_fflush ( xl_stats );
	}
}  /* xl_stats_msg */

sim_end_msg ()
{
	int xl_stats_size;
	extern int mlog, qlog, flog, IS_log;

	if ( ++nodes_ended == tw_num_nodes )
	{  /* if all nodes have stopped */
		if ( xl_stats_area != NULL ) /* should be non-NULL for Mk3 & BBN */
		{  /* finish up Excel stats file */
			xl_stats_size = xl_stats_ptr - xl_stats_area;
			HOST_fwrite ( xl_stats_area, xl_stats_size, 1, xl_stats );
		}

		HOST_fprintf ( xl_stats, "\n\n\n" );

		/* flush id_list to file */
		dump_id_list ( xl_stats );

		HOST_fclose ( xl_stats );

		_pprintf ( "\n%d-- Stats Dumped\n\n", subcube_num );

		/* dump the rest of the logs and exit */
		if ( IS_log )
		{
			brdcst_command ( "IS_dumplog\n",gvt );
			IS_dumplog ();
		}

		if ( qlog )
		{
			brdcst_command ( "dumpqlog\n",gvt );
			dump_qlog ();
		}

		if ( flog )
		{
			brdcst_command ( "dumplog\n",gvt );
			dumplog ();
		}

		if ( mlog )
		{
			brdcst_command ( "dumpmlog\n",gvt );
			dump_mlog ();
		}

		tw_exit ( 0 );  /* exit timewarp */
	}
}

FUNCTION crit_log_msg ( tw_msg )
	Msgh * tw_msg;
{
	static FILE * cp = 0;


	if ( cp == NULL )
		cp = HOST_fopen ( "CRIT_LOG", "w" );    /* open file if not yet open */

	HOST_fputs ( (char *) (tw_msg + 1), cp );   /* write out the message */
	HOST_fflush ( cp );
}
