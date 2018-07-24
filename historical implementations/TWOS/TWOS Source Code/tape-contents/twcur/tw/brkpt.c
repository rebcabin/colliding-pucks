/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	brkpt.c,v $
 * Revision 1.4  91/07/17  15:07:08  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/06/03  12:23:34  configtw
 * Tab conversion.
 * 
 * Revision 1.2  90/12/10  10:38:13  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.1  90/08/07  15:37:52  configtw
 * Initial revision
 * 
*/
char brkpt_id [] = "@(#)brkpt.c 1.7\t9/26/89\t15:40:41\tTIMEWARP";


/*

Purpose:

		brkpt.c contains code that allows users to interactively set
		breakpoints for debugging purposes.

Functions:

		breakpoint() - check to see if we've hit a breakpoint
				Parameters - none
				Return - TRUE or FALSE

		set_object_breakpoint(object_name) - set a breakpoint for the next
						time an object executes
				Parameters - Name * object_name
				Return - Always returns zero

		set_time_breakpoint(stime) - set a breakpoint for a simulation time
				Parameters -  STime * stime
				Return - Always returns zero

		clear_breakpoint() - clear any existing breakpoint
				Parameters - none
				Return - Always returns zero

		show_breakpoint() - print out the identity of the current breakpoint
				Parameters - none
				Return - Always returns zero

		watchpoint() - check to see if we've hit a watchpoint
				Parameters - none
				Return - TRUE or FALSE

		set_object_watchpoint(object_name) - set a watchpoint for the next
						time an object executes
				Parameters - Name * object_name
				Return - Always returns zero

		set_time_watchpoint(stime) - set a watchpoint for a simulation time
				Parameters -  STime * stime
				Return - Always returns zero

		clear_watchpoint() - clear any existing watchpoint
				Parameters - none
				Return - Always returns zero

		show_watchpoint() - print out the identity of the current watchpoint
				Parameters - none
				Return - Always returns zero

Implementation:

		The breakpoint() and watchpoint() facilities are very similar.
		The breakpoint() code checks to see if a breakpoint is set
		for the executing object, or for the current simulation
		time.  If so, it returns TRUE.  If not, FALSE.  set_object_breakpoint()
		and set_time_breakpoint() store breakpoint values in variables.
		Only one breakpoint can be set at a time, so, if either function
		is called, it clears any breakpoint set by the other.  Breakpoints
		can be explicitly cleared with clear_breakpoint().  show_breakpoint()
		displays the breakpoint that is currently set.  Watchpoints are 
		essentially a second, independent breakpoint.

*/

#include "twcommon.h"
#include "twsys.h"

int object_breakpoint = FALSE;
int time_breakpoint = FALSE;
Name breakpoint_object;
STime breakpoint_time;

breakpoint ()
{
	register int torf = FALSE;

	if ( object_breakpoint )
	{
		if ( strcmp ( xqting_ocb->name, breakpoint_object ) == 0 )
			torf = TRUE;
	}

	if ( time_breakpoint )
	{
		if ( geSTime ( xqting_ocb->svt.simtime, breakpoint_time ) )
			torf = TRUE;
	}

	if ( torf )
	{
		_pprintf ( "Breakpoint on Object %s at Time %.2f\n",
			xqting_ocb->name, xqting_ocb->svt.simtime );
	}
	return ( torf );
}

set_object_breakpoint ( object_name )

	Name * object_name;
{
	strcpy ( breakpoint_object, object_name );

	object_breakpoint = TRUE;

	time_breakpoint = FALSE;
}

set_time_breakpoint ( stime )

	STime * stime;
{
	breakpoint_time = *stime;

	time_breakpoint = TRUE;

	object_breakpoint = FALSE;
}

clear_breakpoint ()
{
	object_breakpoint = FALSE;

	time_breakpoint = FALSE;
}

show_breakpoint ()
{
	if ( object_breakpoint )
		_pprintf ( "Breakpoint on Object %s\n", breakpoint_object );
	else
	if ( time_breakpoint )
		_pprintf ( "Breakpoint at Time %.2f\n", breakpoint_time );
	else
		_pprintf ( "No Breakpoint Set\n" );
}

int object_watchpoint = FALSE;
int time_watchpoint = FALSE;
Name watchpoint_object;
STime watchpoint_time;

watchpoint ()
{
	register int torf = FALSE;

	if ( object_watchpoint )
	{
		if ( strcmp ( xqting_ocb->name, watchpoint_object ) == 0 )
			torf = TRUE;
	}

	if ( time_watchpoint )
	{
		if ( geSTime ( xqting_ocb->svt.simtime, watchpoint_time ) )
			torf = TRUE;
	}

	if ( torf )
	{
		_pprintf ( "Watchpoint on Object %s at Time %.2f\n",
			xqting_ocb->name, xqting_ocb->svt.simtime );
	}
	return ( torf );
}

set_object_watchpoint ( object_name )

	Name * object_name;
{
	strcpy ( watchpoint_object, object_name );

	object_watchpoint = TRUE;

	time_watchpoint = FALSE;
}

set_time_watchpoint ( stime )

	STime * stime;
{
	watchpoint_time = *stime;

	time_watchpoint = TRUE;

	object_watchpoint = FALSE;
}

clear_watchpoint ()
{
	object_watchpoint = FALSE;

	time_watchpoint = FALSE;
}

show_watchpoint ()
{
	if ( object_watchpoint )
		_pprintf ( "Watchpoint on Object %s\n", watchpoint_object );
	else
	if ( time_watchpoint )
		_pprintf ( "Watchpoint at Time %.2f\n", watchpoint_time );
	else
		_pprintf ( "No Watchpoint Set\n" );
}
