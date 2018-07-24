/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	timewarp.c,v $
 * Revision 1.23  92/02/25  10:49:44  configtw
 * Changed to version 2.7.1
 * 
 * Revision 1.22  91/12/31  08:28:02  configtw
 * New version ID.
 * 
 * Revision 1.21  91/12/27  09:19:32  pls
 * 1.  Fix up TIMING code.
 * 2.  Make breakpoint code depend on DEBUG flag, not PARANOID.
 * 
 * Revision 1.20  91/11/06  11:12:49  configtw
 * Change version ID.
 * 
 * Revision 1.19  91/11/01  13:56:31  reiher
 * added timing code (PLR)
 * 
 * Revision 1.18  91/11/01  13:04:16  pls
 * 1.  Change ifdef's and version id.
 * 2.  Set objectCode flag when exceuting app code (SCR 164).
 * 3.  Create statesMovedQ.
 * 4.  Enlarge default memory usage (SCR 189).
 * 5.  Change signal stack allocation.
 * 
 * Revision 1.17  91/07/24  13:15:21  configtw
 * Allow for xqting_ocb = NULLOCB.
 * 
 * Revision 1.16  91/07/22  13:17:50  configtw
 * Fix new bug in Mark3 code.
 * 
 * Revision 1.15  91/07/17  15:13:26  judy
 * New copyright notice.
 * 
 * Revision 1.14  91/07/17  09:05:04  configtw
 * Change version #.
 * 
 * Revision 1.13  91/07/09  16:16:53  steve
 * 1. Support for multiple alarms with MicroTime
 * 2. Signal driven socket support
 * 3. object_timing_mode support
 * 4. changed numbers to constats (ie 9 to TIME_SYNC)
 * 
 * Revision 1.12  91/06/04  11:09:52  configtw
 * Fix Sun compile bugs.
 *
 * Revision 1.11  91/06/03  14:19:42  configtw
 * Make network version exit correctly.
 *
 * Revision 1.10  91/06/03  12:27:08  configtw
 * Tab conversion.
 *
 * Revision 1.9  91/05/31  15:41:15  pls
 * 1.  Execute object even with messages to send, if necessary.
 * 2.  Don't migrate out states if resendState flag is false.
 *
 * Revision 1.8  91/04/01  15:48:01  reiher
 * Code to support Tapas Som's work, plus permitting any node to run
 * check_alarm().  (Necessary for dynamic load management.)
 * 
 * Revision 1.7  91/03/28  09:58:39  configtw
 * Change timeoff() to tw_timeoff()--conflict with Sun libraries.
 * 
 * Revision 1.6  91/03/26  09:50:59  pls
 * Add Steve's RBC code.
 * 
 * Revision 1.5  90/12/12  10:06:20  configtw
 * 1.  fix #if 0 bug
 * 2.  change version number
 * 
 * Revision 1.4  90/12/10  10:53:42  configtw
 * 1.  add PARANOID code to detect off node memory
 * 2.  allow tester to break into looping node via signals
 * 3.  add comment for future TC2000 memory default change
 * 
 * Revision 1.3  90/08/27  10:46:05  configtw
 * Round objstksize to 8 byte boundary.
 * 
 * Revision 1.2  90/08/16  10:53:42  steve
 * 1. fixed command line `number_of_buffers'.
 * 2. increased default mem_size to 2 1/8M.
 * 3. changed page touching to read text pages and to be correctly
 *      aligned for the TC2000
 * The Above Changes apply only to the BF_MACH code.
 * 
 * Revision 1.1  90/08/07  15:41:23  configtw
 * Initial revision
 * 
*/
char timewarp_id [] = "@(#)timewarp.c   $Revision: 1.23 $\t$Date: 92/02/25 10:49:44 $\tTIMEWARP";


/*

Purpose:

		This module contains the main loop of Time Warp.  This loop, contained
		in main(), is the point at which decisions are made about what basic
		operations to perform next: running a user object, trying to empty
		low-level output queues, checking for input, etc.  The main()
		function also contains substantial initialization code.  

		timewarp.c also contains code related to timing, and a function
		that allows a clean exit.

Functions:

		main(argc, argv) - the main loop of Time Warp
				Parameters - int argc, char ** argv
				Return - never returns

		tw_exit() - exit Time Warp cleanly
				Parameters - int
				Return - Always returns zero

		timeon() - turn the timer on and set an alarm
				Parameters - none
				Return - Always returns zero

		tw_timeoff() - turn off the timer
				Parameters - none
				Return - Always returns zero

		timeval(interval) - do nothing
				Parameters - int * interval
				Return - Always returns zero

		timechg(interval_change_time) - do nothing
				Parameters - STime * interval_change_time
				Return - Always returns zero

		timint() - increment the timer
				Parameters - none
				Return - Always returns zero

		timer_interrupt() - call the function used when the timer expires
				Parameters - none
				Return - Always returns zero

Implementation:

		main() is the main loop of the Time Warp system.  It performs 
		certain initialization tasks for the Time Warp system, such 
		as starting the monitor, initializing queues, and creating 
		special buffers.  (Most of the actual work of these tasks 
		is performed in various subroutines.)  Next is the main loop, 
		which loops indefinitely.  First, check if we are working 
		from the monitor, and prompt for a command if we are.  Next, 
		see if a message is stored in rm_msg.  If it's a system message, 
		deal with it through command().  Otherwise, use msgproc() to 
		handle it.  Then loop.

		If there was no message in rm_msg, and there are either messages
		or acks to be sent out, call send_from_q() to send them.  After
		send_from_q() returns, check again for a message in rm_msg, 
		going to the top of the loop if one is found.  

		If there were no messages to be sent, and if there is an object 
		to be run, call check_mercury_queue(), going to a spot further 
		along in the loop if there is a message earlier than the one
		being processed.  Test for breakpoints, and call switch_over()
		to run the object.  If, when the object relinquishes control,
		rm_msg has something in it, go to the top of the loop.  Otherwise,
		read_the_mail().  This is the point gone to if the check of the
		mercury queue showed an earlier message.  After the mail is read,
		go to the top of the loop.

		tw_exit() provides a clean exit, by sending a halt message to
		the CP.

		timeon(), tw_timeoff(), timeval(), and timechg() all have to
		do with the timing code, in the ways their names imply, mostly.
		So do timint() and timer_interrupt().  timint() handles a timer 
		interrupt by incrementing a counter, and checking if sufficent 
		ticks have gone by to call alarm().  timer_interrupt() calls a
		pre-supplied function when the timer expires.

*/

#define TDATAMASTER 1

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"
#if BF_MACH
#include <mach.h>
/*-------------------------------------------------------
   Shifts to get physical node numbers out of physical addresses
   NMASK is node mask for the tc2000
   NSHIFT is the node to memory shift and is different for the tc2000 and gp1000
   data from D. Rich and from BBN
   note that the below defaults to gp1000 unless TC2000 is defined
*--------------------------------------------------------*/

#if TC2000
#define NMASK 0x1f800000
#define NSHIFT 23
#define node_of_addr(addr) ((((unsigned)(addr)) & NMASK) >> NSHIFT)
#else
#define NSHIFT 24
#define node_of_addr(addr) ((( unsigned)(addr)) >> NSHIFT)
#endif

#endif

Byte * object_context;
Byte * object_data;

int ctrlc();
#if MICROTIME
int timed_out;
int object_timing_mode = WALLOBJTIME;
#else
int timint();
#endif
int ioint();

#if DLM
int dlmTimer = 0;
int dlmInterval = 4;
int dlmTimer_on = 0;
int dlmtimint () ;
#endif DLM

extern VTime    oldgvt1;
extern VTime    oldgvt2;
extern int resendState;

#if SOM

/* This variable is set at the first time that an object is created on the
	  node.  All subsequent creations use the first value as their initial
	  Est, as any object could, in theory, have been run first. */

long firstEst = 0;
#endif SOM

int timer = 0;
int timer_on = 0;
/*
#include <us.h>
*/

#if MARK3
#include <cube.h>
struct cubenv env;
#include <errno.h>
int abortint();
int cubeint();

int time_sync();
int time_sync_flag = 0;
int rtc_tick ();
int rtc_tick_cnt = 0;
extern char host_wd[60];
int delta = 1000;               /* 1 second */
Msgh * inuse_messages;
int mercury_msgs;
#define MEM_SIZE        0x300000        /* 3M */
char * mem;
char * malloc ();
int max_stdout_acks = 20;
VTime stdout_sent_time = { NEGINF, 0, 0 };
VTime stdout_ok_time = { NEGINF, 0, 0 };
#endif

#if BF_PLUS  

#define MEM_SIZE        0x2c0000        /* 2.75M */
char mem_array[MEM_SIZE + 4];
char * mem = mem_array;
int delta = 1;                  /* 1 second */
int max_stdout_acks = 20;
VTime stdout_sent_time = { NEGINF, 0, 0 };
VTime stdout_ok_time = { NEGINF, 0, 0 };
Msgh * rmq;
#endif

#if BF_MACH  

#include <signal.h>

#if 0

#if TC2000
#define MEM_SIZE        0x600000        /* Default  6M  */
#else
#define MEM_SIZE        0x220000        /* Default  2.125M */
#endif TC2000

#endif

/* as long as the TC2000 only has 4 meg per node: */
#define MEM_SIZE        0x244000        /* Default  2.265625M */

extern int tester();
struct sigvec testervec = { (int *)tester, 0, 1 };
char * mem;
char * malloc ();
int delta = 1;                  /* 1 second */
int max_stdout_acks = 20;
VTime stdout_sent_time = { NEGINF, 0, 0 };
VTime stdout_ok_time = { NEGINF, 0, 0 };
Msgh * rmq;



#endif

#if SUN
#if SUN4
#define MEM_SIZE        0x400000        /* 4M */
#else
#define MEM_SIZE        0x200000        /* 2M */
#endif
char * mem;
char * malloc ();
int delta = 1;                  /* 1 second */
char standalone;
char * usage = "usage: program [ node ] configFile [ memsize ]\n";

double sarea[1024];       /* signal stack area */
struct sigstack ss = { (char *)&sarea[1023], 0 };
#ifndef MICROTIME
struct sigvec alarmvec = { (void *)timint, 0, 1 };
#endif
struct sigvec iovec = { (void *)ioint, 0, 1 };
Msgh * rmq;
int maybe_socket_io;
extern int messages_received;
extern int partial_send;
#endif

#if TRANSPUTER
#define MEM_SIZE        0x080000        /* .5M */
int  mem_array [ MEM_SIZE / 4 ];     /* div. by 4 since this is an int array. */
char * mem = mem_array;
#define ONE_SEC 15625
int delta       = ONE_SEC;
extern int host_input_waiting ;
#else
int host_input_waiting = 0;
#endif

int interval = 1;
STime interval_change_time = POSINF;

int mem_stats_enabled = FALSE;
int no_gvtout = FALSE;
int object_ended = TRUE;

#ifndef BRDCST_ABLE
char brdcst_flag[MAX_NODES];
#endif

#if SUN
int max_acks = 20;
#else
int max_acks = 2;
#endif
#if BBN
int max_neg_acks = 10;
/*
 *	note that recv_q_limit has replaced the use of max_neg_acks
 *	to limit the receive queues length in the bbn (and sun) version.
 */
#else
int max_neg_acks = MAX_ACKS;
#endif

int mem_size = MEM_SIZE;

extern int states_to_send;

Msgh * command_queue;
STime command_queue_time = POSINF+1;

char * config_file;

FUNCTION init_args ( argc, argv )

/* these args are passed by brun */

	int argc;
	char ** argv;
{
	double meg;
	char * memarg;

	pktlen = MAXPKTL;
	msgdefsize = sizeof ( Msgh ) + pktlen;

#if SUN

	printf ( "%s\n", timewarp_id+4 );

	if ( argc < 2 )
	{
		printf ( "%s", usage );
		exit (0);
	}

	if ( *argv[1] >= '0' && *argv[1] <= '9' )
	{
		tw_node_num = atoi ( argv[1] );

		if ( tw_node_num == 0 )
		{
			if ( argc < 3 || argc > 4 )
			{
				printf ( "%s", usage );
				tw_exit (0);
			}

			config_file = argv[2];
			memarg = argv[3];
		}
		else
		{
			if ( argc > 3 )
			{
				printf ( "%s", usage );
				tw_exit (0);
			}

			memarg = argv[2];
		}

		init_node ( tw_node_num );

		if ( tw_num_nodes > MAX_NODES )
		{
			printf ( "MAX_NODES is %d\n", MAX_NODES );
			tw_exit ( 0 );
		}
	}
	else
	{
		if ( argc < 2 || argc > 3 )
		{
			printf ( "%s", usage );
			tw_exit (0);
		}

		config_file = argv[1];
		memarg = argv[2];

		standalone = 1;
		tw_num_nodes = 1;
		signal ( SIGINT, ctrlc );
	}

	if ( memarg )
	{
		if ( sscanf ( memarg, "%lf", &meg ) == 0 )
		{
			printf ( "%s", usage );
			tw_exit (0);
		}

		mem_size = meg * 1024 * 1024;
	}

	mem = malloc ( mem_size );

	if ( mem == 0 )
	{
		printf ( "can't allocate %d bytes\n", mem_size );
		tw_exit (0);
	}

#if MONITOR
	moninit ();
#endif

	sigstack ( &ss, 0 );
	sigvec ( SIGIO, &iovec, 0 );
	maybe_socket_io = 1;
#if MICROTIME
	SunMicroTimeInit (); /* must be done after sigstack(?) */
#else
	sigvec ( SIGALRM, &alarmvec, 0 );
#endif
	rmq = (Msgh *) l_hcreate ();
	rm_buf = (char *) l_create ( msgdefsize );

	objstksize = 6000;

#endif SUN
#if TRANSPUTER

	kernel_main ( argc, argv );

	tw_node_num  = get_node_num ();
	tw_num_nodes = get_num_nodes ();

	transputer_time_init ( 0 );

	if ( tw_node_num == 0 )
	{
		_pprintf ( "%s\n", timewarp_id+4 );
	}

	rm_buf = l_create ( msgdefsize );
	signal ( TPSIGALRM, timint );

	objstksize = 2500;

#endif TRANSPUTER

#if MARK3

	set_param ( SP_GET_Q_SIZ, 1024 );

	async_comm ( INF );

	now_async = 1;

	printf ( "%s\n", timewarp_id+4 );

	sscanf ( argv[3], "%lf", &meg );

	if ( meg > 0 )
		mem_size = meg * 1024 * 1024;

	mem = malloc ( mem_size );

	if ( mem == 0 )
	{
		printf ( "can't allocate %d bytes\n", mem_size );
		tw_exit (0);
	}

#if MONITOR
	moninit ();
#endif

	indep ();

	config_file = argv[1];
	stats_name = argv[2];
	cparam ( &env );
	tw_node_num = env.procnum;
	tw_num_nodes = atoi ( argv[4] );

	if ( argv[5] != 0 )
		strcpy ( host_wd, argv[5] );

	node_offset = 0;
	node_limit = tw_num_nodes - 1;

	sigglob ( 2, ctrlc );
	sigglob ( 3, time_sync );
	sigglob ( RTCGLOB, rtc_tick );

	send.source = tw_node_num;
	send.blen = recv.blen = 512;

	LEDS ( 15 );

	send_message ( 0, 0, CP, TIME_SYNC );       /* request time sync */

	while ( time_sync_flag == 0 )
		;

	mark3time_init ();

	stdout_q = (Msgh *) l_hcreate ();
	inuse_messages = (Msgh *) l_hcreate ();

	signal ( SIGMALRM, timint );
	signal ( SIGMSG, ioint );
	signal ( SIGABRT, abortint );
	signal ( SIGCUBE, cubeint );

	objstksize = 3000;

#endif MARK3



#if BF_PLUS
	config_file = argv[1];
	stats_name = argv[2];
	tw_num_nodes = atoi ( argv[4] );
	tw_node_num = atoi ( argv[5] );
	if ( tw_node_num == 0 )
	{
		printf ( "%s nodes = %d\n", timewarp_id+4, tw_num_nodes );
	}
	sscanf ( argv[6] , "%x",  &goid );
	butterfly_node_init ( goid );
	n_initialize_net_files ();

#if MONITOR
	moninit ();
#endif

	if ( tw_node_num == 0 )
	{
		butterfly_sigalarm ( timint );
#if DLM
		butterfly_dlmAlarm ( dlmtimint );
#endif DLM
	}
	butterfly_sigint (ctrlc);
	stdout_q = l_hcreate ();
	rmq = l_hcreate ();
	rm_buf = (char *) l_create ( msgdefsize );

	objstksize = 3000;

#endif BF_PLUS
}

#if BF_MACH

extern int number_of_buffers;

BF_MACH_init_args()

{

	pktlen = MAXPKTL;
	msgdefsize = sizeof ( Msgh ) + pktlen;

	if ( tw_node_num == 0 )
	{
		printf (
	"Time Warp 2.7.1  %d nodes, memory/node = %.3fMB, msg buffs/node = %d\n", 
			tw_num_nodes,
			( ((float) mem_size)/ (1024 * 1024) ),
			number_of_buffers
			 );

		printf ( "%s\n", timewarp_id+4 ); 
	}

#if MONITOR
	moninit ();
#endif

	if ( tw_node_num == 0 )
	{
		butterfly_sigalarm ( timint );
#if DLM
		butterfly_dlmAlarm ( dlmtimint );
#endif DLM
	}
	signal ( SIGINT, tester);
	butterfly_sigint (ctrlc);
	stdout_q = l_hcreate ();
	rmq = l_hcreate ();
	rm_buf = (char *) l_create ( msgdefsize );
	objstksize = 3000;
}

#endif BF_MACH

int its_a_feature = 2;


#if RBC
extern int rbc_present;
#endif

#ifndef BF_MACH



FUNCTION main ( argc, argv )

	int argc;
	char ** argv;
{
	int         execObj = FALSE;
		/* handle arguments & init rm_buf & msgdefsize */
	init_args ( argc, argv );
#else

extern Node_Arg_Str Node_Args;
extern char etext,end;
#ifdef MSGTIMER
extern long msgstart, msgend;
extern long msgtime ;
extern long msgcount ;
extern int mlog;
extern int onNodeTime;
#endif MSGTIMER

FUNCTION  Main_Node_Execution_Loop (my_node_num)
int my_node_num;

{

	int         execObj = FALSE;
	int i;
	char *p;
	char temp;
	char *mem_end;

#if PARANOID
	int MyNode;
	char *q;
	int t_on, t_off, d_on, d_off, m_on, m_off;
#endif

	tw_node_num = my_node_num;
	config_file = Node_Args.config_path;
	stats_name = Node_Args.stats_path; 

	if ( Node_Args.meg != 0 )
	{
		mem_size = Node_Args.meg * 1024 * 1024;
	}
	else
	{
		mem_size = MEM_SIZE;
	}
	mem = malloc( mem_size );

	if ( mem == 0 )
	{
		printf ( "can't allocate %d bytes\n", mem_size );
		tw_exit (0);
	}

	mem_end = (char *)((unsigned long)mem + mem_size);

	/* read and write back Time Warp's heap */
	for ( p = mem; p < mem_end; p = (char *)((unsigned long)p + vm_page_size) )
	{
		temp = *p;
		*p = temp;
	}

	/* read the text area */
	for ( p = (char *) 0; p < &etext;
		p = (char *)((unsigned long)  p + vm_page_size) )
	{
		temp = *p;
	}

	/* note that p >= &etext so it points to the data area now */

	/* read and write back data area */
	for ( ; p < &end; p = (char *)((unsigned long)p + vm_page_size) )
	{
		temp = *p;
		*p = temp;
	}

	butterfly_node_init();

#if PARANOID
/*  This PARANOID code causes the system to crash.  It has been if'd out,
		but left in in case someone wants to fix it up some day. PLR */

#if 0
	t_on = t_off = d_on = d_off = m_on = m_off = 0;
	q = 0;
	MyNode = node_of_addr(getphysaddr(0));
	for ( p = (char *) 0; p <= &etext; p += vm_page_size )
		(node_of_addr(getphysaddr ( p ))) == MyNode ? t_on++ : t_off++;
	_pprintf("Node: %x text: %x\n",MyNode,getphysaddr(p-vm_page_size));

	for ( p = &etext + vm_page_size - 1; p <= &end; p += vm_page_size )
		(node_of_addr(getphysaddr ( p ))) == MyNode ? d_on++ : d_off++;
	_pprintf("data: %x\n",getphysaddr(p-vm_page_size));

	for ( p = mem; p < mem_end; p += vm_page_size )
		{
		(node_of_addr(getphysaddr ( p ))) == MyNode ? m_on++ : m_off++;
		if (!q && m_off)
			q = p;
		}
	_pprintf("mem: %x\n",getphysaddr(q));

	if ( t_off || d_off || m_off )
	{
		_pprintf("Program is using OFF node memory!!!\n" );
		_pprintf("text: %don %doff data: %don %doff malloc: %don %doff\n",        
			t_on, t_off, d_on, d_off, m_on, m_off );
	}
#endif 
#endif PARANOID

	BF_MACH_init_args (  );

#endif

	objstksize = (objstksize + 7) & ~7; /* round to 8 byte boundary */
	miparm.me = tw_node_num;            /* this node's # */
	miparm.maxnprc = tw_num_nodes;      /* total number of nodes */

	init_types ();      /* set up the object type table */

	/* Initialize object location structures. */

	CacheInit();        /* init object location cache */
	HomeInit();         /* init node's home list */
	PendingListInit();  /* init pending requests list */

	sendOcbQ = (Ocb *) l_hcreate ();
	sendStateQ = (State_Migr_Hdr *) l_hcreate ();
    statesMovedQ = (State_Migr_Hdr *) l_hcreate ();

	tw_startup ();      /* do some initialization (eg for stdout obj) */

	command_queue = (Msgh *) l_hcreate ();      /* set up command queue */

	pmq = (Msgh *) l_hcreate ();        /* set up node's output queue */

	brdcst_buf = (Msgh *) l_create ( msgdefsize );

#if RBC
	if ( rbc_init_start() == SUCCESS )
	{
		rbc_present = TRUE;
		rbc_init_done ();
	}
#endif
#if SUN
#if SELECT
	init_select();
#endif
#endif

	if ( tw_node_num == 0 )
	{  /* read in & handle config commands */
		init_command ( config_file );
	}

#if TIMING
	timing_mode = 0;
	start_time = clock ();
#endif

	for ( ;; )
	{
#if MARK3
		register Msgh * msg;

		msg = (Msgh *) l_next_macro ( inuse_messages );

		if ( ! l_ishead_macro ( msg ) )
		{
			if ( *((int *)(&msg->low.type)) == DONE )
			{
				l_remove ( msg );
				destroy_msg ( msg );
			}
			continue;
		}
#endif

#if BBN
#if STANDALONE
		if ( ChannelHasInput ( stdin ) )
		{   
			char    buff[80];
			gets ( buff );
			host_input_waiting = TRUE;
		}
#endif
		if ( host_input_waiting == 0 )
		{  /* check for alarms & interrupts */
			check_alarm();
			check_for_events ();
		}
#endif

		if ( host_input_waiting )
		{
			if ( manual_mode )
				go ();
			else
			{  /* get commands manually */
				command ( "Tester" );
			}
			continue;
		}

		if ( rm_msg != NULL )
		{  /* message received by node */
			if ( rm_msg->low.to_node == IH_NODE )
			{  /* handle message from host */
				ih_msgproc ( rm_msg );
			}
			else
			if ( rm_msg->mtype == COMMAND )
			{  /* prompt for commands */
				command ( "COMMAND" );
			}
			else
			{
#if TIMING
				start_timing ( TIMEWARP_TIMING_MODE );
#endif
				msgproc ();
#if TIMING
				stop_timing ();
#endif
			}

			continue;
		}

		if ( ltSTime ( command_queue_time, gvt.simtime ) )
		{
			exec_commands_in_queue ();
			continue;
		}

#if MICROTIME
		if ( timed_out )
			check_timeouts ();
#else
		if ( timer >= interval )
		{
			timer_interrupt ();
			continue;
		}
#if DLM
		if ( dlmTimer >= dlmInterval )
		{
			dlmTimer_interrupt ();
			continue;
		}
#endif DLM
#endif

		if ( states_to_send && resendState)
		{
			send_state_from_q ();
		}


		if ( ( messages_to_send || acks_queued ) && !execObj)
		{
			if ( eqVTime ( gvt, oldgvt2 ) &&
				xqting_ocb &&
				eqVTime ( gvt, xqting_ocb->svt) &&
				(oldgvt2.simtime != NEGINF)  &&
				(nxtocb_macro(sendOcbQ) == NULL) )
			{
				execObj = TRUE; /* force object execution next time */
			}
			send_from_q ();

			if ( rm_msg != NULL )
				continue;
		}
#if SUN
		else if ( partial_send )
			resend_msg();
#endif
		else

		if ( object_context != NULL )
		{
			execObj = FALSE;
#if BBN
			read_the_mail ( TRUE );     /* check_only */

			if ( rm_msg != NULL )
				continue;
#endif
#if MARK3
			cnt_msgs ( &mercury_msgs );

			if ( mercury_msgs )
			{
				check_mercury_queue ();

				if ( rm_msg != NULL )
					continue;
			}

			LEDS ( 4 );
#endif
#if DEBUG
			watchpoint ();
			if ( breakpoint () )
				tester ();
#endif
#if TIMING
			start_timing ( OBJECT_TIMING_MODE );
#endif
#if MICROTIME
			switch ( object_timing_mode )
			{
			case WALLOBJTIME:
				MicroTime ();
				object_start_time = node_cputime;
				break;
			case USEROBJTIME:
				UserDeltaTime(); /* start clock */
				/* object_start_time is still zero */
				break;
			case NOOBJTIME:
			default:
				/* no measure */
				break;
			}
#else
#if MARK3
			mark3time ();
#endif
#if BBN
			butterflytime ();
#endif
			object_start_time = node_cputime;
#endif
#ifdef MSGTIMER
/*  This version of MSGTIMER will time from send to start of the resulting
	event.  */
/*
					msgend = clock();
					if ( onNodeTime == TRUE )
					{
						if ( msgend - msgstart > 0  &&
							msgend - msgstart < 1000 )
						{
							msgtime += msgend - msgstart;
							msgcount++;
						}
					}
					else
						onNodeTime = TRUE;
*/
#endif MSGTIMER
			objectCode = TRUE;	/* will execute object code */
			switch_over ( object_context, object_data );
/*	tobjend is called by switch_over upon return, & objectCode is reset there */

			if ( rm_msg != NULL )
				continue;
		}

#if MARK3
		else
		{
			cnt_msgs ( &mercury_msgs );

			if ( mercury_msgs == 0 )
				idle ();
		}
#endif

#if MARK3
		cnt_msgs ( &mercury_msgs );

		if ( mercury_msgs )
		{
#else
#ifndef BBN
#if SUN
		if ( !standalone )
#else
		if ( tw_num_nodes > 1 )
#endif
#endif
		{
#endif
			read_the_mail ( FALSE );
		}
#if SUN
		/*start_idle*/
		if ( ( object_context != NULL ) || (rm_msg != NULL)
			|| messages_received || messages_to_send || states_to_send
			|| acks_queued || partial_send )
		{
				continue;
		}
		else
		{
			sigblock ( sigmask(SIGIO) | sigmask(SIGALRM) );

			if ( ( maybe_socket_io == 0 ) && ( timed_out == 0 ) )
				sigpause(0);

			sigsetmask(0);
		}
#endif

	}  /* for ( ;; ) */
}

#if MICROTIME
/* microTime is like mark3time, butterflytime it counts in machine ticks with
each tick a handfull of micro-seconds. It returns the number of ticks since
the last call. The global variable node_cputime keeps a running total count. */

static void (*spare_routine) ();
int spare_interval;

static unsigned int next_timeout;
static unsigned int gvt_time;
static unsigned int dlm_time;
static unsigned int spare_time;

static time_counter;
check_timeouts ()
{
	if ( ! next_timeout )
		return;

	MicroTime();

	if ( next_timeout > node_cputime )
	{
		return;
	}

	timed_out = 0;

	if ((gvt_time != 0) && (node_cputime >= gvt_time) )
	{
		gvt_time = 0;

		gvtinterrupt ();
	}

	if ((dlm_time != 0) && (node_cputime >= dlm_time) )
	{
		dlm_time = 0;

#if DLM
		loadinterrupt ();
#endif
	}


	if ((spare_time != 0) && (node_cputime >= spare_time) )
	{
		spare_time = 0;

		if ( spare_routine )
			(*spare_routine) ();
		else
		{
			printf ( "spare_signal called with void signal handler\n" );
			tester ();
		}
	}

	schedule_time_out();
}

schedule_time_out()
{
	int min_time;
	int delta_time;

	if ( gvt_time )
	{
		min_time = gvt_time;
		if ( dlm_time && (dlm_time < min_time) )
			min_time = dlm_time;
		if ( spare_time && (spare_time < min_time) )
			min_time = spare_time;
	}
	else if ( dlm_time )
	{
		min_time = dlm_time;
		if ( spare_time && (spare_time < min_time) )
			min_time = spare_time;
	}
	else if ( spare_time )
	{
		min_time = spare_time;
	}
	else
	{
		min_time = 0;
	}

	next_timeout = min_time;

	if ( min_time )
	{
		MicroTime();
		delta_time = min_time - node_cputime;
		delta_time /= TICKS_PER_MILLISECOND;

		if ( delta_time > 0 )
			SetMicroAlarm ( delta_time );
		else 
			timed_out = 1;
	}
}

schedule_next_gvt()
{
	MicroTime();

	gvt_time = node_cputime + TICKS_PER_SECOND * interval;
									    /* interval = gvtInterval */

	schedule_time_out();
}
#if DLM
schedule_next_dlm()
{
	MicroTime();

	dlm_time = node_cputime + TICKS_PER_SECOND * dlmInterval;

	schedule_time_out();
}
#endif
schedule_next_spare()
{
	MicroTime();

	spare_time = node_cputime + TICKS_PER_SECOND * spare_interval;

	schedule_time_out();
}

void (*spare_signal ( routine ))()
void (*routine) ();
{
	void (*temp_fun) ();

	temp_fun = spare_routine;

	spare_routine = routine;

	return temp_fun;
}
#endif MICROTIME

#if MARK3
idle ()
{
	LEDS ( 8 );

#if TIMING
	start_timing ( IDLE_TIMING_MODE );
#endif
	for ( ;; )
	{
		if ( host_input_waiting )
			break;

		if ( timer >= interval )
			break;

		cnt_msgs ( &mercury_msgs );

		if ( mercury_msgs )
			break;
	}
#if TIMING
	stop_timing ();
#endif
}
#endif

/* here's where we exit from timewarp */
#if SUN
int triedToExitOnce;
#endif

tw_exit ( code )
	int code;
{
#if MARK3
	send_message ( 0, 0, CP, EXIT );       /* tell CP to exit */
#endif

#if BBN
	if (mem != NULL )
		free (mem);

	butterfly_node_term();
#endif

#if SUN
	if ( standalone || tw_node_num || triedToExitOnce )
		exit(code);
	else
	{
		if ( /* batch mode */ 1 )
			send_message ( 0, 0, CP, EXIT );
		triedToExitOnce = 1;
	}
#endif

}

/* put "msg" in the command_queue in order of rcvtim */
enq_command ( msg )

	Msgh * msg;
{
	Msgh * last;

	for ( last = (Msgh *) l_prev_macro ( command_queue );
			   ! l_ishead_macro ( last );
		  last = (Msgh *) l_prev_macro ( last ) )
	{  /* loop through command_queue */
		if ( leVTime ( last->rcvtim, msg->rcvtim ) )
			break;      /* found the insertion spot */
	}

	l_insert (  (List_hdr *) last, (List_hdr *) msg );  /* insert */

	if ( ltSTime ( msg->rcvtim.simtime, command_queue_time ) )
		command_queue_time = msg->rcvtim.simtime;       /* update c_q_time */
}

exec_commands_in_queue ()
{
	Msgh * command_msg;

	for ( ;; )
	{
		command_msg = (Msgh *) l_next_macro ( command_queue );

		if ( command_msg == command_queue )
		{
			command_queue_time = POSINF+1;
			break;
		}

		if ( gtSTime ( command_msg->rcvtim.simtime, gvt.simtime ) )
		{
			command_queue_time = command_msg->rcvtim.simtime;
			break;
		}

		l_remove ( (List_hdr *) command_msg );
		rm_msg = command_msg;
		command ( "QCOMMAND" );
	}
}

FUNCTION timeon ()
{
  Debug

	timer_on = 1;

#if MARK3
	malarm ( delta );
#else
	alarm ( delta );    /* set timer for 1 second interrupts */
#endif

}

#if DLM
#if SUN
dlmAlarm()
{
	printf ("dlmAlarm called\n");
	tester();
}
#endif
FUNCTION timelon ()
{
  Debug

	dlmTimer_on = 1;

#if MARK3

/* mdlmalarm ( ) hasn't been written yet; obviously, this stuff won't work 
		on the MARK3 without it.  It should look pretty much like malarm (). */

	mdlmalarm ( delta );
#else
	dlmAlarm ( delta );
#endif

}
#endif DLM

FUNCTION tw_timeoff ()
{
  Debug

	timer_on = 0;

	timer = 0;
}

#if DLM
FUNCTION dlmTimeoff ()
{
  Debug

	dlmTimer_on = 0;

	dlmTimer = 0;
}
#endif


timeval ( t )

	int * t;
{
	interval = * t;
}

timechg ( t )

	STime * t;
{
	interval_change_time = *t;
}

FUNCTION timint ()
{
  Debug

	if ( timer_on )
	{
		timer++;

		if ( timer < interval )
		{
#if MARK3
			malarm ( delta );
#else
			alarm ( delta );
#endif
		}
	}
}

#if DLM

FUNCTION dlmtimint ()
{
  Debug

	if ( dlmTimer_on )
	{
		dlmTimer++;

		if ( dlmTimer < dlmInterval )
		{
#if MARK3
			mdlmalarm ( delta );
#else
			dlmAlarm ( delta );
#endif
		}
	}    
}
#endif DLM

#if TIMING
#define GVT_TIMING_MODE 11
#endif

FUNCTION timer_interrupt ()
{
	int ( *tempf ) ();

  Debug

	if ( timrproc != NULL )
	{
		tw_timeoff ();

#if TIMING
		start_timing ( GVT_TIMING_MODE );
#endif
		tempf = timrproc;
		timrproc = NULL;
		( * tempf ) ();

#if TIMING
		stop_timing ();
#endif
	}
}

#if DLM
FUNCTION dlmTimer_interrupt ()
{
	int ( *dlmtempf ) ();

  Debug

	if ( timlproc != NULL )
	{
		dlmTimeoff ();

#if TIMING
/*
		start_timing ( LOAD_TIMING_MODE );
*/
#endif
		dlmtempf = timlproc;
		timlproc = NULL;
		( * dlmtempf ) ();

#if TIMING
/*
		stop_timing ();
*/
#endif
	}
}
#endif 

#if MARK3
time_sync ()
{
	time_sync_flag = 1;
}

rtc_tick ()
{
	rtc_tick_cnt++;
}

abortint ()
{
	_pprintf ( "abortint: errno = %d\n", errno );
}

cubeint ()
{
	_pprintf ( "cubeint: errno = %d\n", errno );
}
#endif

