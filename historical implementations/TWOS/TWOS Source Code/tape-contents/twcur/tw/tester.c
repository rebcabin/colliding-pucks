/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	tester.c,v $
 * Revision 1.13  91/12/27  09:18:24  pls
 * Fix up TIMING code.
 * 
 * Revision 1.12  91/11/01  13:51:19  reiher
 * Added batch mode support (PLR)
 * 
 * Revision 1.11  91/11/01  12:55:33  pls
 * 1.  Change ifdef's and version id.
 * 2.  Add support code for signal handling (SCR 164).
 * 3.  GVT display now shows sequence 1 and 2.
 * 4.  Change proportional delay routine.
 * 
 * Revision 1.10  91/07/17  15:13:14  judy
 * New copyright notice.
 * 
 * Revision 1.9  91/07/09  15:28:16  steve
 * MicroTime & object_timing_mode support. 2 changed to SIGNAL_THE_CUBE
 * 
 * Revision 1.8  91/06/03  14:24:01  configtw
 * Make network version exit correctly.
 * 
 * Revision 1.7  91/06/03  12:27:03  configtw
 * Tab conversion.
 * 
 * Revision 1.6  91/04/01  15:47:45  reiher
 * Code to support Tapas Som's work
 * 
 * Revision 1.5  91/03/26  09:50:01  pls
 * 1.  Modify hoglog code to refer to hlog.
 * 2.  Remove gvt_position().
 * 
 * Revision 1.4  91/03/25  16:41:58  csupport
 * Implement hoglog.
 *
 * Revision 1.3  90/12/10  10:53:26  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.2  90/08/27  10:45:29  configtw
 * Split cycle time from committed time.
 * 
 * Revision 1.1  90/08/07  15:41:18  configtw
 * Initial revision
 * 
*/
char tester_id [] = "@(#)tester.c       $Revision: 1.13 $\t$Date: 91/12/27 09:18:24 $\tTIMEWARP";


/*

Purpose:

		tester.c is something of a catchall module.  Any low level function
		that does not belong somewhere else is kept here.  The bulk of the
		routines stored here are small routines that deal with commands
		given to the system in tester mode, such as manually sending
		messages.  But several other miscellaneous functions have found
		their way into this module, as well.

Functions:

		tobjend() - handle the timings for an object's end
				Parameters - none
				Return - Always returns zero

		start_timing(new_timing_mode) - start timing in a new mode, saving
						the previous timing mode's information
				Parameters -  int new_timing_mode
				Return - Always returns zero

		stop_timing() - clean up for the current timing mode, and revert
						to the previous mode
				Parameters - none
				Return - Always returns zero

		tester() - invoke the system testing software
				Parameters - none
				Return - Always returns zero

		go() - set host_input_waiting to 0
				Parameters - none
				Return - Always returns zero

		stop() - exit the simulation
				Parameters - none
				Return - Never returns

		dumpmsgx(msgh) - dump a message
				Parameters - Msgh **msgh
				Return - Always returns zero

		dumpstatex(state) - dump a state
				Parameters - int ** state
				Return - Always returns zero

		manual_lvt() - print the processor's virtual time
				Parameters - none
				Return - Always returns zero

		manual_gvt() - print the gvt
				Parameters - none
				Return - Always returns zero

		gvt_position(row,column) - set the output format for gvt messages
				Parameters -  int * row, int * column
				Return - Always returns zero

		manual_init(f) - call the function provided in the parameter
				Parameters - Byte * (*f)()
				Return - Always returns zero

		manual_event(state,p) - ask for a command until an object ends
				Parameters -  char * state, int (*p) ()
				Return - Always returns zero

		manual_term(state) - do nothing
				Parameters - char * state
				Return - Always returns zero

		manual_objend() - indicate that an object has ended
				Parameters - none
				Return - Always returns zero

		crash() - crash and burn
				Parameters - none
				Return - Never returns

		debug() - do nothing
				Parameters - none
				Return - Always returns zero

Implemenation:

		start_timing() and stop_timing() maintain a stack of timing
		modes.  The system can time various different modes of Time
		Warp operation.  There is a timing mode for the tester, for
		Time Warp, for objects, and for the system.  A call to 
		start_timing() pushes the existing timing mode (if any) onto the
		stack, along with any time that has been accumulated for it,
		and starts in the new mode.  stop_timing() completes the timing
		of the old mode, and restores the topmost timing mode (if any) from
		the stack.

		tester() iteratively prompts for commands.

		clear_screen() does what it says.

		go() sets host_input_waiting to 0, which allows the system to 
		continue without traps to the tester.

		stop() calls tw_exit().

		dumpmsgx() and dumpstatex() call the corresponding functions without
		the x on the ends of their names.  manual_lvt() prints the local
		virtual time, and manual_gvt() prints the global virtual time.
		gvt_position() sets up some variables for the next printout of
		gvt.  manual_init() calls its parameter, with 100 as a parameter.
		manual_event() calls command(), telling it to perform MANUAL_EVENT.
		manual_objend() simply indicates that an object has ended.

		crash() kills the machine by calling a null function.

		debug() does nothing. but you can put a breakpoint in it.
*/

#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

extern FILE * cpulog;
extern int host_input_waiting;
extern int object_ended;
extern Byte * object_context;

#if DLM
extern int batchRun;
#endif DLM
#if TIMING
int * timing_mode_sp = timing_mode_stack;
#endif

#if TIMING
#define OBJEND_TIMING_MODE 10
#endif

extern int      hlog;
extern VTime    hlogVTime;
extern int      maxSlice;
extern Ocb*     maxSliceObj;
extern VTime    maxSliceTime;
extern int      sliceTime;

FUNCTION tobjend ()
{
	extern int flog;

	objectCode = FALSE;		/* no longer executing object code */

#if TIMING
	stop_timing ();
#endif

	if ( prop_delay )
		delay_object ();

#if MICROTIME
	switch ( object_timing_mode )
	{
	case WALLOBJTIME:
		MicroTime ();
		object_end_time = node_cputime;
		break;
	case USEROBJTIME:
		object_end_time = UserDeltaTime(); /* end clock */
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
	object_end_time = node_cputime;
#endif

	if (hlog)
		{
		sliceTime = object_end_time - object_start_time;
		if ((sliceTime > maxSlice) && (geVTime(gvt,hlogVTime)) &&
			(ltSTime(xqting_ocb->svt, posinf.simtime))) /* skip term */
			{ /* this slice was bigger than max so far */
			maxSlice = sliceTime;
			maxSliceObj = xqting_ocb;
			maxSliceTime = xqting_ocb->svt;
			}
		}

	xqting_ocb->stats.cputime += object_end_time - object_start_time;
	xqting_ocb->cycletime += object_end_time - object_start_time;
	xqting_ocb->stats.comtime += object_end_time - object_start_time;
	xqting_ocb->sb->effectWork += object_end_time - object_start_time;

#if SOM
	/*  Calculate the ept for the state of the event just completing. */

	xqting_ocb->sb->Ept += object_end_time - object_start_time;
	xqting_ocb->work += object_end_time - object_start_time;
#endif SOM

	if ( cpulog )
	{
		register Ocb * o = xqting_ocb;

		if ( gtSTime ( o->svt.simtime, neginfPlus1.simtime ) )
			HOST_fprintf ( cpulog, "E %s %f %d\n", o->name,
				o->svt.simtime, o->stats.cputime );
		else
			o->stats.cputime = 0;
	}

	if ( flog )
		flowlog_entry ();

#if TIMING
	start_timing ( OBJEND_TIMING_MODE );
#endif

	objend ();

#if TIMING
	stop_timing ();
#endif
}       /* tobjend */

#if TIMING
start_timing ( new_timing_mode )

	int new_timing_mode;
{
	if ( timing_mode_sp >= timing_mode_stack + 20 )
	{
		_pprintf ( "Timing Mode Stack Overflow\n" );
		tester();
	}

#if MICROTIME
	timing[timing_mode] += MicroTime ();
#else
#if MARK3
	timing[timing_mode] += mark3time ();
#endif

#if BBN
	end_time = clock ();
	timing[timing_mode] += ( ( end_time - start_time ) *62.5);
	start_time = end_time;
#endif


#if SUN
	end_time = clock ();
	timing[timing_mode] += end_time - start_time;
	start_time = end_time;
#endif
#if TRANSPUTER
	end_time = clock ();
	timing[timing_mode] += end_time - start_time;
	start_time = end_time;
#endif
#endif

	*timing_mode_sp++ = timing_mode;
	timing_mode = new_timing_mode;
}

stop_timing ()
{
	if ( timing_mode_sp <= timing_mode_stack )
	{
		_pprintf ( "Timing Mode Stack Underflow\n" );
		crash ();
	}
#if MICROTIME
	timing[timing_mode] += MicroTime ();
#else
#if MARK3
	timing[timing_mode] += mark3time ();
#endif

#if BBN
	end_time = clock ();
	timing[timing_mode] += ( ( end_time - start_time ) *62.5);
	start_time = end_time;
#endif 

#if SUN
	end_time = clock ();
	timing[timing_mode] += end_time - start_time;
	start_time = end_time;
#endif
#if TRANSPUTER
	end_time = clock ();
	timing[timing_mode] += end_time - start_time;
	start_time = end_time;
#endif
#endif
	timing_mode = *--timing_mode_sp;
}
#endif

tester ()
{

#ifdef DLM

	/* If we're running in batch mode, when tester() is called abandon
			the run so we can go on to the next. */

	if ( batchRun == TRUE)
			tw_exit ( 0 );
#endif DLM


#if MARK3
	send_message ( 0, 0, CP, SIGNAL_THE_CUBE );/* tell CP to signal nodes */
#endif

#if BBN
		interrupt_nodes ();
#endif
#if TRANSPUTER 
	 if ( tw_node_num == 0 ) 
		{
		 close_command_file (); 
		}

	   brdcst_interrupt ();

#else
	close_command_file ();
#endif

	host_input_waiting = 1;

	while ( host_input_waiting )
		command ( "Tester" );
}

ctrlc ()
{
	host_input_waiting = 1;

	object_ended = TRUE;
}

clear_screen ()
{
	printf ( "\f" );
}

go ()
{
	host_input_waiting = 0;
}

stop ()
{
#if SUN
	extern int triedToExitOnce;

	triedToExitOnce = 1;
#endif
	tw_exit ( 0 );
}

dumpmsgx ( msgh )

	Msgh ** msgh;
{

#if TRANSPUTER

	* msgh =  0x80000000 | (int) *msgh ;

#endif

	dumpmsg ( *msgh );
}

dumpstatex ( state )

	State ** state;
{
	dumpstate ( *state );
}

manual_lvt ()
{
	char pvts[12];
	char min_vts[12];
#if DLM
	char local_min_vts[12];
#endif DLM

	ttoc ( pvts, pvt );
	ttoc ( min_vts, min_vt );
#if DLM
	ttoc ( local_min_vts, local_min_vt );
#endif DLM

#if DLM
	_pprintf ( "PVT = %s LOCAL_MIN_VT = %s MIN_VT = %s\n", pvts, 
				local_min_vts, min_vts );
#else
	_pprintf ( "PVT = %s MIN_VT = %s\n", pvts, min_vts );
#endif DLM
}

manual_gvt ()
{
	char gvts[12];

	ttoc ( gvts, gvt );

	_pprintf ( "GVT = %s, %d, %d\n", gvts,gvt.sequence1,gvt.sequence2 );
}

manual_init ( state )

	char * state;
{
}

manual_event ( state )

	char * state;
{
	for ( object_ended = FALSE; object_ended == FALSE; )
	{
#if MARK3
		extern int object_running;

		object_running = 0;
#endif
		command ( "MANUAL_EVENT" );
	}
}

manual_term ( state )

	char * state;
{
}

manual_objend ()
{
	object_ended = TRUE;

	_pprintf ( "Manual Object Ended\n" );
}

crash ()
{
	int (*z)() = 0;

	_pprintf ( "CRASH\n" );

#if MARK3
	debug ();
#endif

#if TRANSPUTER
	tester ();
#endif

	(*z)();
}

#if SUN
debug ()
{
}
#endif

delay_object ()
{
	int elapsed_time, delay_time, end_delay_time;

#if MICROTIME
	MicroTime ();
#else
#if MARK3
	mark3time ();
#endif
#if BBN
	butterflytime ();
#endif 
#endif 
	elapsed_time = node_cputime - object_start_time;

	delay_time = elapsed_time * delay_factor + 0.5;

	end_delay_time = node_cputime + delay_time;

#if MICROTIME
	while ( node_cputime < end_delay_time )
		MicroTime ();
#else
#if MARK3
	while ( node_cputime < end_delay_time )
		mark3time ();

	object_running = 0;
#endif
#if BBN
	while ( node_cputime < end_delay_time )
		butterflytime ();
#endif
#endif

	xqting_ocb->sb->cputime = 0;
}
