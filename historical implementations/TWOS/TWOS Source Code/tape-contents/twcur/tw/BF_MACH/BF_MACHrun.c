/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	BF_MACHrun.c,v $
 * Revision 1.10  91/11/01  10:01:17  reiher
 * added variables for critical path computation
 * 
 * Revision 1.9  91/07/17  15:51:55  judy
 * New copyright notice.
 * 
 * Revision 1.8  91/07/17  09:08:51  configtw
 * Get rid of redundant defines.
 * 
 * Revision 1.7  91/07/09  16:25:42  steve
 * Moved `log-ing' code to file cp_msg.c (now shared with sun version)
 * 
 * Revision 1.6  91/06/03  12:16:27  configtw
 * Tab conversion.
 * 
 * Revision 1.5  91/03/25  16:44:33  csupport
 * Add support for longer names for mlog.
 * 
 * Revision 1.4  90/12/10  11:10:48  configtw
 * 1.  allow tester to interrupt looping node via signals
 * 2.  retype buff[] to double for TC2000 alignment
 * 
 * Revision 1.3  90/11/27  10:21:24  csupport
 * fix tester quit bug
 * 
 * Revision 1.2  90/08/16  11:04:23  steve
 * Major Revision
 * 1. removed the Uniform System.
 * 2. changed the large arrays from global data to malloc-ed storage
 *      (over 1/4 Meg saved from
 *      the data segment)
 * 3. the command line parameter -B fixed, it now changes the number
 *      of message buffers.
 * 4. The loging routines work with the no data case. (The files are
 *      created before they are closed.)
 * 5. It prints `.' as each node is started.
 * 
 * Revision 1.1  90/08/06  13:52:48  configtw
 * Initial revision
 * 
*/

#include  <stdio.h>
#include <sys/ioctl.h>
#include <mach.h>
#include <signal.h>
#include "twcommon.h"
#include "BBN.h"
#include "logdefs.h"
#include <sys/cluster.h>
#include <sys/kern_return.h>



extern  int tester();


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

extern int stdout_fd;
extern int stats_fd;
extern int stats_fp;


/* Global data structures that everyone will get a copy of */

extern  FIFO_QUEUE **event_queue;        /* Event Queue  */
extern  FIFO_QUEUE **recv_queue;         /* Message Queue */
extern  FIFO_QUEUE **recv_pqueue;        /* Prioirity Message Queue */
extern  char **buffer_pool;
extern  MESSAGE * send_buff[];

long endWhenCritDone = 0;
long critDone = 0;
extern long critEnabled;
#define BLOCKSIZE 128

/* This routine is here so that the CP process just stays */
/* out of the way in terms of the interruptable tester()  */
/* feature. This is very kludgy right now but for the     */
/* moment it will do.                                     */

CP_Tester()
{

  signal ( SIGINT, SIG_DFL);
  return;

}

Main_CP_Execution_Loop()
{

MSG_STRUCT m;
char line[100];
/* char buff [512]; */
double buff[64];        /* to force alignment on double boundary */
int  iret, navail;
int event_type;
int  i,j,node;
char *msg;
static int cmd_node;
char prompt[20];
int  nodes;


	butterfly_host_wait_for_nodes ();

	for ( ;; )
	{
		for ( ;; )
		{

/* Poll the input to see if any chars are there */

			ioctl ( fileno(stdin), FIONREAD, &navail );
			if ( navail >0 )
			{
				gets ( line );
				break;  
			} 


			event_type = check_for_events ();
			if ( event_type )
				break;  

			m.source = ANY;
			m.type   = ANY;
			m.buf = buff;

			iret = get_msg ( &m  );

			if ( iret )
				handle_cp_msg ( m );
		}

		if ( event_type == END_EVENT )
		{
			printf ( "All Done\n" );
			break;
		}

		interrupt_nodes ();

		butterfly_get_prompt ( prompt, ALL );

		for ( ;; )
		{
			printf ( "%d--%s", cmd_node, prompt );

			gets ( line );

			if ( strcmp ( line, "stop" ) == 0
			||   strcmp ( line, "quit" ) == 0 )
			{
				printf ( "byebye\n" );
				broadcast_event (END_EVENT);
				exit(0);
			}

			if ( line[0] >= '0' && line[0] <= '9' )
			{
				i = atoi ( line );

				if ( i >= 0 && i < tw_num_nodes )
					cmd_node = i;
				else
				{
					printf ( "Max node is %d\n", tw_num_nodes-1 );
					continue;
				}
			}

			if ( line[0] == '*'
			||  strcmp ( line, "go" ) == 0 )
				node = ALL;
			else
				node = cmd_node;

			msg = line;

			if ( *msg == '*' )
				msg++;
			else
			while ( *msg >= '0' && *msg <= '9' )
				msg++;

			if ( *msg == 0 )
				continue;

			butterfly_send_command ( msg, node );

			if ( strcmp ( line, "go" ) == 0 )
			 {
				signal(SIGINT, CP_Tester);
				break;
			 }

			if ( strcmp ( line, "*dumpqlog" ) == 0 )
				break;

			butterfly_get_prompt ( prompt, node );
		}
	}
	broadcast_event (END_EVENT);
	fclose ( stdout_fd );
	printf ( "Nodes shutting down\n" );
	while ( -1 != wait ( (union wait *) 0 ) )
	{
		printf ( "." );
		fflush ( stdout );
	}
	printf ( "\n" );
	printf ( "CP: cluster node %d exiting\n", tw_num_nodes );
	exit (0);
}



static char  working_dir[60];
static char zero[2] = "0";

int rtc_sync;

Node_Arg_Str Node_Args;
extern int number_of_buffers;

main ( argc, argv )

	int argc;
	char * argv[];
{

	char   c;
	int nodes;
	char * config;
	char * stats = "XL_STATS";  /* default for -S */
	char * usage = 
		"usage: TW nodes config [ -S stats] [ -M memsiz ] [ -B numbuffs ]\n";
	kern_return_t rv;
	int nodes_requested;
	cluster_type_t mach_type = 0;
	cluster_id_t  cluster_id;
	int nodes_allocated;
	int child_pid;


	signal (SIGINT, CP_Tester);

	if ((argc < 3) || (argc > 9) || (sscanf(argv[1], "%d", &nodes) == 0))
		{  /* wrong number of args or bad "nodes" argument */
		printf("%s", usage);
		exit(0);
		}
	Node_Args.num_nodes = tw_num_nodes = nodes;/* handle arg 1 */
	nodes_requested = nodes + 1;

	config = argv[2];   /* name of configuration file */

	argc -= 2;
	argv += 2;  /* set up for the loop */

	while (--argc > 0 && (*++argv)[0] == '-')
		{  /* loop through optional args */
		argc--;
		c = *++argv[0];         /* get option letter */
		switch (c)
			{
			case 'S':           /* stats file name */
				stats = *++argv;
				break;
			case 'M':           /* megs of memory */
				if (sscanf(*++argv, "%lf", &Node_Args.meg ) == 0)
					argc = -1;  /* force error exit */
				break;
			case 'B':           /* # of bufs */
				if (sscanf(*++argv, "%d", &number_of_buffers ) == 0)
					argc = -1;  /* force error exit */
				break;
			default:
				argc = -1;      /* force error exit */
				break;
			}  /* switch (c) */
		}  /* while (--argc ...) */
	if (argc != 0)
		{  /* an error in the arguments */
			printf("%s", usage);
			exit(0);
		}  /* if (argc != 0) */

	if ( number_of_buffers < MIN_NUM_BUFFS )
	{
		printf ( "Too few buffers (min is %d) --resetting to %d (default)\n",
			MIN_NUM_BUFFS, DEFAULT_NUM_BUFFS );
		number_of_buffers = DEFAULT_NUM_BUFFS;
	}
	else if ( number_of_buffers > MAX_NUM_BUFFS )
	{
		printf ( "Too many buffers (max is %d) --resetting to %d (default)\n",
			MAX_NUM_BUFFS, DEFAULT_NUM_BUFFS );
		number_of_buffers = DEFAULT_NUM_BUFFS;
	}


	make_path ( Node_Args.stdout_path, "STDOUT", "w" );
	make_path ( Node_Args.config_path, config, "r" );

	printf ( "Config File: %s\n", Node_Args.config_path );


	make_path ( Node_Args.stats_path, stats, "w" );
	printf ( "Stats File: %s\n", Node_Args.stats_path );
	stdout_fd = 
	   (int) fopen ( Node_Args.stdout_path, "w" );

	if ( 0 != ( rv = cluster_create(nodes_requested, mach_type,
		&cluster_id, &nodes_allocated) ) )
	{
		printf ( "cluster_create returned %d\n", rv );
		if ( rv == 6 )
		{
			printf ( "Too few nodes in free cluster\n" );
		}
		exit ( 1 );
	}

	if ( 0 != ( rv = fork_and_bind( tw_num_nodes, cluster_id, &child_pid) ) )
	{
		printf ( "fork_and_bind returned %d\n", rv );
		exit ( 1 );
	}

	if ( child_pid ) /* I am the parent */
	{
		wait ( (union wait *) 0 );
		printf ( "Cluster creator exiting\n" );
		exit ( 0 );
	}
	/* I am the child running on cluster node tw_num_nodes `the CP' */

	butterfly_host_init();
	rtc_sync = getrtc();

	printf ( "Starting Nodes\n" );
	for ( nodes = 0; nodes < tw_num_nodes; nodes++ )
	{
		if ( 0 != ( rv = fork_and_bind( nodes, cluster_id, &child_pid) ) )
		{
			printf ( "fork_and_bind returned %d\n", rv );
			exit ( 1 );
		}
		if ( ! child_pid ) /* I am the child running on cluster node `nodes' */
		{
			Main_Node_Execution_Loop( nodes );
			/* never returns */
		}
		printf ( "." );
		fflush ( stdout );
	}

	printf ( "\n" );
	/* cluster node: tw_num_nodes --CP stuff */
	tw_node_num = CP;
	Main_CP_Execution_Loop();
	/* never returns */
}


global ()
{
	printf ( "GLOBAL\n" );
	broadcast_event ( INTERRUPT_EVENT); 

}

quit ()
{
	butterfly_node_term ();
}

CP_ctrlc ()
{
	exit (0);
}

term ()
{
	printf ( "TERM\n" );
	exit (0);
}






make_path ( path_name, file_name, mode )

	char * path_name, * file_name, * mode;
{
	FILE * fp;

	if ( *file_name == '/' )
	{
		strcpy ( path_name, file_name );
	}
	else
	{
		strcpy ( path_name, working_dir );
		if ( path_name[0] != 0 )
			strcat ( path_name, "/" );
		strcat ( path_name, file_name );
	}

	fp = fopen ( path_name, mode );

	if ( fp == 0 )
	{
		printf ( "Can't Open File: %s\n", path_name );
		exit (0);
	}

	fclose ( fp );
}
