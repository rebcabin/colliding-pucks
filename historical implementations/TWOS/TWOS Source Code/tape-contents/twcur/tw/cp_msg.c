/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	cp_msg.c,v $
 * Revision 1.3  91/07/17  15:07:47  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/07/11  08:56:57  steve
 * fixed objno with mlog.
 * 
 * Revision 1.1  91/07/09  12:50:07  steve
 * Initial revision
 * 
 */


#include  <stdio.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "twcommon.h"
#include "BBN.h"
#include "logdefs.h"

extern  int tester();

#define ALL -1
#define ANY -1


#define READ_THE_CONSOLE        0
#define EXIT                    1
#define SIGNAL_THE_CUBE         2
#define STDOUT_DATA             3
#define STDOUT_TIME             4
#define STATS_DATA              5
#define GVT_DATA                6
#define FLOW_DATA               7
#define MSG_DATA                8
#define TIME_SYNC               9
#define ISLOG_DATA              10
#define QLOG_DATA               11

int tw_num_nodes, tw_node_num;
int node_cputime;

int sync_cnt;
int num_times;
int low_node;
VTime low_time, next_low_time;

int stdout_fd;
int stats_fd;

/* Globals used for the assignment of object names to integers */

#define MAX_OBJS 1000

typedef struct
{
	Name name;
	int node;
} CP_ObjStruct;

/* Old Sytle -- Pre-Malloc
CP_ObjStruct obj[MAX_OBJS];
*/
CP_ObjStruct *obj;

int nobjs;

#define UNKNOWN_NODE -1

objno ( name, node, sim_time )

	char * name;
	int node;
	float sim_time;
{
	register int i;
	static int error = 0;

	if ( error )
		return 0;

	if ( !obj )
	{
		obj = (CP_ObjStruct *) malloc ( MAX_OBJS * sizeof(CP_ObjStruct) );
		if ( !obj )
		{
			error = 1;
			printf ( "objno malloc failed\n" );
			return 0;
		}
	}

	if ( *name == 0 )
		strcpy ( name, "null" );

	for ( i = 0; i < nobjs; i++ )
	{
		if ( strcmp ( name, obj[i].name ) == 0 )
			break;
	}
	if ( i == nobjs )
	{
		if ( i == MAX_OBJS )
		{
			printf ( "exceeded %d objects\n", i );
		}
		else
		{
			strcpy ( obj[i].name, name );
			nobjs++;
		}
	}

	return ( i );
}
#define BLOCKSIZE 128


handle_cp_msg ( m )
MSG_STRUCT m;
{

	switch ( m.type )
	{
	case TIME_SYNC:
		sync_cnt++;
		printf ("BF_MACHrun: received TIME_SYNC\n");

		if ( sync_cnt == tw_num_nodes )
		{
			sync_cnt = 0;
		}
		break;

	case GVT_DATA:
		printf ("BF_MACHrun: received GVT_DATA\n");
		printf ( "%s", m.buf );
		break;

	case STDOUT_DATA:
		fwrite (m.buf,m.mlen,1, stdout_fd );

		m.dest = m.source;
		m.source = CP;
		m.mlen = 0;
		m.type = 0;
		send_msg_w ( &m );
		m.type = STDOUT_DATA;
		break;

	case STDOUT_TIME:
		cp_stdouttime_msg ( m );
		break;

	case FLOW_DATA:
		cp_flog_msg ( m );
		break;

	case MSG_DATA:
		cp_mlog_msg ( m );
		break;

	case ISLOG_DATA:
		cp_islog_msg ( m );
		break;

	case QLOG_DATA:
		cp_qlog_msg ( m );
		break;

	case STATS_DATA:
		cp_hist_msg ( m );
		break;

	case READ_THE_CONSOLE:
		printf ("BBNrun: received READ_THE_CONSOLE\n");
		break;

	case EXIT:
#ifdef BBN
		butterfly_node_term ();
#else
		exit();
#endif
		break;
	default:
		printf ( "CP got unknown message type %d\n", m.type );
		break;

	}
}

/*::::::: NOTE about malloc's ::::::*/

/*
		At first glance this seems to be the hard way to make array's.
However all these big array's were also included in the data segment
of the tw nodes. By malloc'ing them they only take space on the CP node.
*/


/*::::::::: msglog :::::::*/
/* Declarations for message logging (msglog)  */

FILE * mlog_file;
int mno, mitems;
int msg_num_times;
int msg_low_node;
int msg_low_time;

int * mx;
int * msg_time;
char * msg_first_time;

/* Old Style Definitions -- Pre-Malloc
int mx[MAX_NODES];
int msg_time[MAX_NODES];
char msg_first_time[MAX_NODES];

MSG_LOG_ENTRY * CPmlogp;
MSG_LOG_ENTRY CPmlog[MAX_NODES][8];
MSG_COBJ mobj[BLOCKSIZE];
*/

MSG_LOG_ENTRY * CPmlogp;
typedef MSG_LOG_ENTRY x8_MSG_LOG_ENTRY[8];
x8_MSG_LOG_ENTRY * CPmlog;
MSG_COBJ * mobj;


/* Allocations for message logging (msglog)  */
malloc_mlog_variables()
{
	mx = (int *) malloc ( tw_num_nodes * sizeof(int) );
	msg_time = (int *) malloc ( tw_num_nodes * sizeof(int) );
	msg_first_time = (char *) malloc ( tw_num_nodes * sizeof(char) );

	CPmlog = (x8_MSG_LOG_ENTRY *)
		malloc ( tw_num_nodes * sizeof(x8_MSG_LOG_ENTRY) );
	mobj = (MSG_COBJ *) malloc ( BLOCKSIZE * sizeof(MSG_COBJ) );

	return ( !mx || !msg_time || !msg_first_time || !CPmlog || !mobj);
}

cp_mlog_msg ( m )
MSG_STRUCT m;
{
	register int node = m.source;
	int i;
    static int need_malloc = 1;
    static int error = 0;

	if ( error )
		return;

	if ( need_malloc )
    {
		if ( error = malloc_mlog_variables() )
			printf ( "CP malloc failed\n" );
		need_malloc = 0;
	}

	if ( msg_first_time[node] == 0 )
	{
		msg_first_time[node] = 1;

		msg_num_times++;
	}
/*
	entcpy ( CPmlog[node], m.buf, m.mlen );
*/
	bcopy ( m.buf, CPmlog[node], m.mlen );

	msg_time[node] = CPmlog[node][0].twtimet;

	mx[node] = 0;

	if ( msg_time[node] == MAXINT )
	{
		m.buf  = &msg_time[node];
		m.mlen = sizeof(msg_time[node]);
		m.dest = node;
		m.source = CP;
		m.type = 0;
		send_msg_w ( &m );
		m.type = MSG_DATA;
	}

	if ( msg_num_times == tw_num_nodes )
	{
msg_loop:
		msg_low_node = 0;
		msg_low_time = MAXINT;

		for ( node = 0; node < tw_num_nodes; node++ )
		{
			if ( msg_time[node] < msg_low_time )
			{
				msg_low_time = msg_time[node];
				msg_low_node = node;
			}
		}

		if ( mlog_file == 0 )
		{
			mlog_file = (FILE *) fopen ( "mmlog", "w" );

			if ( mlog_file == 0 )
			{
				printf ( "can't open mmlog file\n" );
				error = 1;
				return;
			}
		}

		if ( msg_low_time == MAXINT )
		{
			FILE * fp;

			if ( mno > 0 )
			{
				fwrite ( mobj, sizeof(MSG_COBJ) * mno,
						1, mlog_file );
			}
			fclose ( mlog_file );

			printf ( "%d mlog items\n", mitems );

			fp = (FILE *) fopen ( "mname", "w" );

			if ( fp == 0 )
			{
				printf ( "can't open mname file\n" );
				error = 1;
				return;
			}

			for ( i = 0; i < nobjs; i++ )
			{
				fprintf ( fp, "%-20s %d\n", obj[i].name,
						obj[i].node );
			}

			fclose ( fp );

			printf ( "%d mlog names\n", nobjs );

			nobjs = 0;

			m.buf  = &msg_low_time;
			m.mlen = sizeof(msg_low_time);
			m.dest = 0;
			m.source = CP;
			m.type = 0;
			send_msg_w ( &m );
			m.type = MSG_DATA;
		}

		node = msg_low_node;

		CPmlogp = &CPmlog[node][mx[node]];

/* We don't write these out in ASCII anymore. It takes too
much room on the disks.
		fprintf ( mlog_file,
			"%d %d %d %d %d %s %.2f %s %.2f %d %d %x %d\n",
				node,
				CPmlogp->twtimef, CPmlogp->twtimet,
				CPmlogp->hgtimef, CPmlogp->hgtimet,
				CPmlogp->snder, CPmlogp->sndtim,
				CPmlogp->rcver, CPmlogp->rcvtim,
				CPmlogp->id_num, CPmlogp->mtype,
				CPmlogp->flags, CPmlogp->len );
*/
		mitems++;

		mobj[mno].twtimef = CPmlogp->twtimef;
		mobj[mno].twtimet = CPmlogp->twtimet;
		mobj[mno].hgtimef = CPmlogp->hgtimef;
		mobj[mno].hgtimet = CPmlogp->hgtimet;
		mobj[mno].sndtim = CPmlogp->sndtim;
		mobj[mno].rcvtim = CPmlogp->rcvtim;
		mobj[mno].id_num = CPmlogp->id_num;
		mobj[mno].snder = objno ( CPmlogp->snder,
			UNKNOWN_NODE, CPmlogp->sndtim);
		i = objno( CPmlogp->rcver, node, CPmlogp->rcvtim );
		mobj[mno].rcver = i;
		obj[i].node = node;
		mobj[mno].len = CPmlogp->len;
		mobj[mno].mtype = CPmlogp->mtype;
		mobj[mno].flags = CPmlogp->flags;

		mno++;

		if ( mno == BLOCKSIZE )
		{
			mno = 0;

			fwrite ( mobj, sizeof(MSG_COBJ) * BLOCKSIZE,
				1, mlog_file );
		}

		mx[node]++;

		if ( mx[node] < 8 )
			msg_time[node] = CPmlog[node][mx[node]].twtimet;

		if ( mx[node] == 8 || msg_time[node] == MAXINT )
		{
			m.buf  = &msg_low_time;
			m.mlen = sizeof(msg_low_time);
			m.dest = msg_low_node;
			m.source = CP;
			m.type = 0;
			send_msg_w ( &m );
			m.type = MSG_DATA;
		}

		if ( mx[node] < 8 )
			goto msg_loop;
	}
}
/*::::::::: flow flog :::::::*/
/* Declarations for event logging (flowlog)  */

FILE * flow_file;
int fno, fitems;
int flow_num_times;
int flow_low_node;
int flow_low_time;

int * flow_time;
int * fx;
char * flow_first_time;

/* Old Sytle Definitions -- Pre-Malloc
int flow_time[MAX_NODES];
int fx[MAX_NODES];
char flow_first_time[MAX_NODES];

FLOW_LOG_ENTRY * flogp;
FLOW_LOG_ENTRY flog[MAX_NODES][20];
FLOW_COBJ fobj[BLOCKSIZE];
*/

FLOW_LOG_ENTRY * flogp;
typedef FLOW_LOG_ENTRY x20_FLOW_LOG_ENTRY[20];
x20_FLOW_LOG_ENTRY * flog;
FLOW_COBJ * fobj;

/* Allocations for event logging (flowlog)  */
malloc_flog_variables()
{
	flow_time = (int *) malloc ( tw_num_nodes * sizeof(int) );
	fx = (int *) malloc ( tw_num_nodes * sizeof(int) );
	flow_first_time = (char *) malloc ( tw_num_nodes * sizeof(char) );

	flog = (x20_FLOW_LOG_ENTRY *)
		malloc ( tw_num_nodes * sizeof(x20_FLOW_LOG_ENTRY) );
	fobj = (FLOW_COBJ *) malloc ( BLOCKSIZE * sizeof(FLOW_COBJ) );

	return ( !flow_time || !fx || !flow_first_time || !flog || !fobj );
}

cp_flog_msg ( m )
MSG_STRUCT m;
{
	register int node = m.source;
	int i;
    static int need_malloc = 1;
    static int error = 0;

	if ( error )
		return;

	if ( need_malloc )
    {
		if ( error = malloc_flog_variables() )
			printf ( "CP malloc failed\n" );
		need_malloc = 0;
	}

	if ( flow_first_time[node] == 0 )
	{
		flow_first_time[node] = 1;
		flow_num_times++;
	}
/*
	entcpy ( flog[node], m.buf, m.mlen );
*/
	bcopy ( m.buf, flog[node], m.mlen );

	flow_time[node] = flog[node][0].start_time;

	fx[node] = 0;

	if ( flow_time[node] == MAXINT )
	{
		m.buf  = &flow_time[node];
		m.mlen = sizeof(flow_time[node]);
		m.dest = node;
		m.source = CP;
		m.type = 0;
		send_msg_w ( &m );
		m.type = FLOW_DATA;
	}

	if ( flow_num_times == tw_num_nodes )
	{
flow_loop:
		flow_low_node = 0;
		flow_low_time = MAXINT;

		for ( node = 0; node < tw_num_nodes; node++ )
		{
			if ( flow_time[node] < flow_low_time )
			{
				flow_low_time = flow_time[node];
				flow_low_node = node;
			}
		}

		if ( flow_file == 0 )
		{
			flow_file = (FILE *) fopen ( "cflow", "w" );

			if ( flow_file == 0 )
			{
				printf ( "can't open cflow file\n" );
				error = 1;
				return;
			}
		}

		if ( flow_low_time == MAXINT )
		{
			FILE * fp;

			if ( fno > 0 )
			{

				fwrite ( fobj, sizeof(FLOW_COBJ) * fno,
						1, flow_file );
			}
			fclose ( flow_file );

			printf ( "%d flow items\n", fitems );

			fp = (FILE *) fopen ( "cname", "w" );

			if ( fp == 0 )
			{
				printf ( "can't open cname file\n" );
				error = 1;
				return;
			}

			for ( i = 0; i < nobjs; i++ )
			{
				fprintf ( fp, "%-20s %d\n", obj[i].name,
						obj[i].node );
			}

			fclose ( fp );

			printf ( "%d flow names\n", nobjs );

			nobjs = 0;

			m.buf  = &flow_low_time;
			m.mlen = sizeof(flow_low_time);
			m.dest = 0;
			m.source = CP;
			m.type = 0;
			send_msg_w ( &m );
			m.type = FLOW_DATA;
		}

		node = flow_low_node;

		flogp = &flog[node][fx[node]];
/*We dont write the file in ASCII anymore. It takes up too much
room on the disks.
		fprintf ( flow_file, "%d %d %d %s %f\n", node,
				flogp->start_time, flogp->end_time,
				flogp->object, flogp->svt );
*/
		fitems++;

		fobj[fno].cpuf = flogp->start_time;
		fobj[fno].cput = flogp->end_time;
		fobj[fno].vt = flogp->svt;
		i = objno(flogp->object, node, flogp->svt);
		fobj[fno].objno = i;

		fno++;

		if ( fno == BLOCKSIZE )
		{
			fno = 0;

			fwrite ( fobj, sizeof(FLOW_COBJ) * BLOCKSIZE,
				1, flow_file );
		}

		fx[node]++;

		if ( fx[node] < 20 )
			flow_time[node] = flog[node][fx[node]].start_time;

		if ( fx[node] == 20 || flow_time[node] == MAXINT )
		{
			m.buf  = &flow_low_time;
			m.mlen = sizeof(flow_low_time);
			m.dest = flow_low_node;
			m.source = CP;
			m.type = 0;
			send_msg_w ( &m );
			m.type = FLOW_DATA;
		}

		if ( fx[node] < 20 )
			goto flow_loop;
	}
}
/*::::::::: islog :::::::*/
/* Declarations for Instantaneous Speedup logging (islog)  */

FILE * ISfile;
int isitems;
int IS_num_times;
int IS_low_node;
double IS_low_time;

int * IS;
double * IS_time;
char * IS_first_time;

/* Old Style Definitions -- Pre-Malloc
int IS[MAX_NODES];
double IS_time[MAX_NODES];
char IS_first_time[MAX_NODES];

IS_LOG_ENTRY * IS_logp;
IS_LOG_ENTRY IS_log[MAX_NODES][16];
*/

IS_LOG_ENTRY * IS_logp;
typedef IS_LOG_ENTRY x16_IS_LOG_ENTRY[16];
x16_IS_LOG_ENTRY * IS_log;

malloc_islog_variables()
{
/* Allocations for Instantaneous Speedup logging (islog)  */

	IS = (int *) malloc ( tw_num_nodes * sizeof(int) );
	IS_time = (double *) malloc ( tw_num_nodes * sizeof(double) );
	IS_first_time = (char *) malloc ( tw_num_nodes * sizeof(char) );

	IS_log = (x16_IS_LOG_ENTRY *)
		malloc ( tw_num_nodes * sizeof(x16_IS_LOG_ENTRY) );

	return ( !IS || !IS_time || !IS_first_time || !IS_log );
}

cp_islog_msg ( m )
MSG_STRUCT m;
{
	register int node = m.source;
    static int need_malloc = 1;
    static int error = 0;

	if ( error )
		return;

	if ( need_malloc )
    {
		if ( error = malloc_islog_variables() )
			printf ( "CP malloc failed\n" );
		need_malloc = 0;
	}

	if ( IS_first_time[node] == 0 )
	{
		IS_first_time[node] = 1;

		IS_num_times++;
	}    
/*
	entcpy ( IS_log[node], m.buf, m.mlen );
*/
	bcopy ( m.buf, IS_log[node], m.mlen );

	IS_time[node] = IS_log[node][0].cputime;

	IS[node] = 0;

	if ( IS_time[node] == 1000000. )
	{
		m.buf  = &IS_time[node];
		m.mlen = sizeof(IS_time[node]);
		m.dest = node;
		m.source = CP;
		m.type = 0;
		send_msg_w ( &m );
		m.type = ISLOG_DATA;
	}

	if ( IS_num_times == tw_num_nodes )
	{
IS_loop:
		IS_low_node = 0;
		IS_low_time = 1000000.;

		for ( node = 0; node < tw_num_nodes; node++ )
		{
			if ( IS_time[node] < IS_low_time )
			{
				IS_low_time = IS_time[node];
				IS_low_node = node;
			}
		}

		if ( ISfile == 0 )
		{
			ISfile = fopen ( "ISLOG", "w" );

			if ( ISfile == 0 )
			{
				printf ( "can't open ISLOG file\n" );
			}
		}    

		if ( IS_low_time == 1000000. )
		{
			fclose ( ISfile );

			printf ( "%d islog items\n", isitems );

			m.buf  = &IS_low_time;
			m.mlen = sizeof(IS_low_time);
			m.dest = 0;
			m.source = CP;
			m.type = 0;
			send_msg_w ( &m );
			m.type = ISLOG_DATA;

			goto IS_done;
		}

		node = IS_low_node;

		IS_logp = &IS_log[node][IS[node]];

		fprintf ( ISfile, "%d %d %f %f\n",
				node,
				IS_logp->seqnum,
				IS_logp->cputime,
				IS_logp->minvt.simtime
				);

		isitems++;

		IS[node]++;

		if ( IS[node] < 16 )
			IS_time[node] = IS_log[node][IS[node]].cputime;

		if ( IS[node] == 16 || IS_time[node] == 1000000. )
		{
			m.buf  = &IS_low_time;
			m.mlen = sizeof(IS_low_time);
			m.dest = IS_low_node;
			m.source = CP;
			m.type = 0;
			send_msg_w ( &m );
			m.type = ISLOG_DATA;
		}

		if ( IS[node] < 16 )
			goto IS_loop;
	}
IS_done:			;
}
/*::::::::: histogram time :::::::*/
#define HLENGTH 100
typedef int hist_array[HLENGTH+1];

/* Old Style Defintion -- Pre-Malloc
hist_array sum_gram[20];
*/
hist_array * sum_gram;

FILE * stats_fp;

/* if ( m.type == STATS_DATA )*/
cp_hist_msg ( m )
MSG_STRUCT m;
{
	register int i, j;
	static int hist_nodes;
	hist_array * histogram;
	int node = m.source;
    static int error = 0;

	if ( error )
		return;

	printf ("BF_MACHrun: received STATS_DATA\n");

	if ( ! sum_gram )
    {
		sum_gram = (hist_array *) malloc ( 20 * sizeof(hist_array) );
		if ( ! sum_gram )
		{
			error = 1;
			printf ( "histogram malloc failed\n" );
			return;
		}
		stats_fp = fopen ( "SUM", "w" );
		if ( stats_fp )
		{
			error = 1;
			printf ( "open failed\n" );
			return;
		}
	}

	histogram = m.buf;

	for ( j = 0; j <= HLENGTH; j++ )
	{
		fprintf (
		stats_fp, 
		"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
		node, j,
		histogram[0][j],
		histogram[1][j],
		histogram[2][j],
		histogram[3][j],
		histogram[4][j],
		histogram[5][j],
		histogram[6][j],
		histogram[7][j],
		histogram[8][j],
		histogram[9][j],
		histogram[10][j],
		histogram[11][j],
		histogram[12][j] );

		for ( i = 0; i < 13; i++ )
			sum_gram[i][j] += histogram[i][j];
	}
	hist_nodes++;
	if ( hist_nodes == tw_num_nodes )
	{
/*sfb
		fclose ( stats_fp );
		stats_fp = fopen ( "SUM", "w" );
*/
		fprintf (
		stats_fp, 
		"Node\tBucket\tTester\tTW\tObjects\tMercury\tIdle\tGo_Fwd\tQueue\tSched\tDeliver\tServe\tObjend\tGvt\tGcpast\n");
		for ( j = 0; j <= HLENGTH; j++ )
		{
			fprintf ( stats_fp, "ALL\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
			j,
			sum_gram[0][j],
			sum_gram[1][j],
			sum_gram[2][j],
			sum_gram[3][j],
			sum_gram[4][j],
			sum_gram[5][j],
			sum_gram[6][j],
			sum_gram[7][j],
			sum_gram[8][j],
			sum_gram[9][j],
			sum_gram[10][j],
			sum_gram[11][j],
			sum_gram[12][j] );
		}
		fclose ( stats_fp );
	}
}
/*::::::::: stdout time :::::::*/

/* Old Style Defintions -- Pre-Malloc
VTime stdout_time[MAX_NODES];
char first_time[MAX_NODES];
*/
VTime * stdout_time;
char * first_time;

VTime CPposinfPlus1 = { POSINF+1, 0, 0 };


cp_stdouttime_msg ( m )
MSG_STRUCT m;
{
	register int node = m.source;
    static int need_malloc = 1;
    static int error = 0;

	if ( error )
		return;

	if ( need_malloc )
    {
		stdout_time = (VTime *) malloc ( tw_num_nodes * sizeof(VTime) );
		first_time = (char *) malloc ( tw_num_nodes * sizeof(char) );

		if ( error = !stdout_time || !first_time )
			printf ( "CP malloc failed\n" );
		need_malloc = 0;
	}

	if ( first_time[node] == 0 )
	{
		first_time[node] = 1;
		num_times++;
	}
	stdout_time[node] = * (VTime *) m.buf;

	if ( num_times == tw_num_nodes )
	{
		low_node = 0;
		low_time = CPposinfPlus1;
		next_low_time = low_time;

		for ( node = 0; node < tw_num_nodes; node++ )
		{
			if ( ltVTime ( stdout_time[node], low_time ) )
			{
				next_low_time = low_time;
				low_time = stdout_time[node];
				low_node = node;
			}
			else
			if ( ltVTime ( stdout_time[node], next_low_time ) )
			{
				next_low_time = stdout_time[node];
			}
		}
		next_low_time = low_time;
		m.buf  = &next_low_time;
		m.mlen = sizeof(next_low_time);
		m.dest = low_node;
		m.source = CP;
		m.type = 0;
		send_msg_w ( &m );
		m.type = STDOUT_TIME;
	}
}

/*::::::::: qlog :::::::*/
/* Declarations for queue logging (qlog)  */

FILE * Qfile;
int Q_num_times;
int Q_low_node;
double Q_low_time;

int * Q;
double * Q_time;
char * Q_first_time;

/* Old Style Definitions -- Pre-Malloc
int Q[MAX_NODES];
double Q_time[MAX_NODES];
char Q_first_time[MAX_NODES];

Q_LOG_ENTRY * Q_logp;
Q_LOG_ENTRY Q_log[MAX_NODES][10];
*/

Q_LOG_ENTRY * Q_logp;
typedef Q_LOG_ENTRY x10_Q_LOG_ENTRY[10];
x10_Q_LOG_ENTRY * Q_log;

/* Allocations for queue logging (qlog)  */
malloc_qlog_variables()
{

	Q = (int *) malloc ( tw_num_nodes * sizeof(int) );
	Q_time = (double *) malloc ( tw_num_nodes * sizeof(double) );
	Q_first_time = (char *) malloc ( tw_num_nodes * sizeof(char) );

	Q_log = (x10_Q_LOG_ENTRY *)
		malloc ( tw_num_nodes * sizeof(x10_Q_LOG_ENTRY) );

	return ( !Q || !Q_time || !Q_first_time || !Q_log );
}


cp_qlog_msg ( m )
MSG_STRUCT m;
{
	register int node = m.source;
    static int need_malloc = 1;
    static int error = 0;

	if ( error )
		return;

	if ( need_malloc )
    {
		if ( error = malloc_qlog_variables() )
			printf ( "CP malloc failed\n" );
		need_malloc = 0;
	}

	if ( Q_first_time[node] == 0 )
	{
		Q_first_time[node] = 1;
		Q_num_times++;
	}    

	if ( m.mlen != ( sizeof(Q_LOG_ENTRY) * 10 ) )
		printf ( " --recv %d bytes from %d\n", m.mlen, node );
/*
	entcpy ( Q_log[node], m.buf, m.mlen );
*/
	bcopy ( m.buf, Q_log[node], m.mlen );

	Q_time[node] = Q_log[node][0].gvt.simtime;

	Q[node] = 0;

	if ( Q_time[node] == POSINF+1 )
	{
		m.buf  = &Q_low_time;
		m.mlen = sizeof(Q_low_time);
		m.dest = node;
		m.source = CP;
		m.type = 0;
		send_msg_w ( &m );
		m.type = QLOG_DATA;
	}

	if ( Q_num_times == tw_num_nodes )
	{
Q_loop:
		Q_low_node = 0;
		Q_low_time = POSINF+1;

		for ( node = 0; node < tw_num_nodes; node++ )
		{
			if ( Q_time[node] < Q_low_time )
			{
				Q_low_time = Q_time[node];
				Q_low_node = node;
			}
		}

		if ( Qfile == 0 )
		{
			Qfile = fopen ( "QLOG", "w" );

			if ( Qfile == 0 )
				printf ( "can't open QLOG file\n" );
			else
				printf ( "QLOG opened\n" );
		}    

		if ( Q_low_time == POSINF+1 )
		{
			fclose ( Qfile );
			printf ( "QLOG closed\n" );

			m.buf  = &Q_low_time;
			m.mlen = sizeof(Q_low_time);
			m.dest = 0;
			m.source = CP;
			m.type = 0;
			send_msg_w ( &m );
			m.type = QLOG_DATA;
/*
			event_type = END_EVENT;
			break;
*/
			return;
		}

		node = Q_low_node;

		Q_logp = &Q_log[node][Q[node]];

#ifdef DLM
		fprintf ( Qfile, "%d %d %f %f\n",
				node,
				Q_logp->loadCount,
				Q_logp->gvt.simtime,
				Q_logp->utilization
				);

#else
		fprintf ( Qfile, "%d %f %f\n",
				node,
				Q_logp->gvt.simtime,
				Q_logp->utilization
				);

#endif DLM
		Q[node]++;

		if ( Q[node] < 10 )
			Q_time[node] = Q_log[node][Q[node]].gvt.simtime;

		if ( Q[node] == 10 || Q_time[node] == POSINF+1 )
		{
			m.buf  = &Q_low_time;
			m.mlen = sizeof(Q_low_time);
			m.dest = Q_low_node;
			m.source = CP;
			m.type = 0;
			send_msg_w ( &m );
			m.type = QLOG_DATA;
		}

		if ( Q[node] < 10 )
			goto Q_loop;
	}
}
