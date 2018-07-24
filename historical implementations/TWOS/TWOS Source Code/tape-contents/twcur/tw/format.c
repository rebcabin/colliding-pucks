
/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */
/*
 * $Log:	format.c,v $
 * Revision 1.5  91/11/01  09:35:18  reiher
 * added strings for system messages related to dynamic load management and
 * critical path computation (PLR)
 * 
 * Revision 1.4  91/07/17  15:08:17  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/06/03  12:24:09  configtw
 * Tab conversion.
 * 
 * Revision 1.2  90/12/10  10:41:10  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.1  90/08/07  15:38:21  configtw
 * Initial revision
 * 
*/
char format_id [] = "@(#)format.c       1.21\t9/27/89\t14:03:48\tTIMEWARP";

/*      Copyright (C) 1989, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

#ifndef TRANSPUTER

/*

Purpose:
		
		format.c contains code that takes pointers to data, and flags
		indicating the data's type, and converts the data to human-
		readable formats.  It is used primarily by the monitor.

Functions:

		tod() - return the contents of atime
				Parameters - none
				Return - a pointer to the atime character array

		formhdr(bp,name,caller,level) - set up a string indicating who called
				what function
				Parameters -  char **bp, char *name, char *caller, int level
				Return - Always returns zero

		format(bp,name,data,type,level) - format output of the appropriate type
				Parameters -  char **bp, char *name, int *data,
								int type, int level
				Return -  argument size (currently either 1 or 2)

		form_vtime(bp,vtime) - convert a virtual time to human-readable form
				Parameters - char **bp, VTime vtime
				Return - Always returns zero

Implementation:

		tod() simply returns a constant array address.

		formhdr() takes its last three parameters, produces a string
		describing their values, and puts the string into the place
		pointed to by its first parameter.

		format() has the bulk of the module's code.  formhdr() is
		usually called just before format().  It is called once
		per invocation of monitor() (or whatever other routine is
		formatting data), and produces a header on an output string,
		describing what routine called what other routine.  Then, for
		each parameter in the call being looked at by the monitor,
		format() is invoked once.  The point of format is to build up
		a string containing the name of the data item, and some reasonable
		formatting of its value.  Since the item in question may be a
		structure, this reasonable formatting may not be straightforward.

		format() puts the name it is provided into the string.  If its 
		data field is zero, it then puts "NULL" into the string.  size
		is set to 1.  The remainder of format is a large case statement
		on the type provided as a parameter.

		Going through the various cases in detail would be rather tedious.
		Most of them are fairly straightforward formattings of the
		data in question, printing decimal values for ints, the actual
		strings for strings, etc.  If the data is a virtual time,
		form_vtime() is called to handle it.  If the data is a list
		header, determine if it is a real list header, or just a list
		element header.  In the former case, just indicate that it is
		a list header.  In the latter case, print out several important
		list element fields, such as send time, receive time, sender's
		and receiver's names, etc.  (Apparently, this code is only meant
		to handle the message lists, not other lists, such as the state
		list.)  The message text is also printed, making sure that the
		leading escape character, if any, is converted to the string
		"<ESC>".  World map entries have their names and nodes copied.
		
		Whatever the data type, return size.

		form_vtime() looks to see if the virtual time in question is
		either POSINF or NEGINF.  If so, it produces the strings
		"POSINF" and "NEGINF", respectively.  Otherwise, the virtual
		time is simply converted into an integer.

*/

#include "twcommon.h"
#include "twsys.h"

formhdr ( bp, name, caller )

	char **bp;
	char *name;
	char *caller;
{
	sprintf ( *bp, "%d %s called by %s", miparm.me, name, caller );
	*bp += strlen ( *bp );
}

static char *mname[] = {
		"TYPE0",        /*  0 */
		"TYPE1",        /*  1 */
		"CMSG",         /*  2 */
		"DYNCRMSG",     /*  3 */
		"TYPE4",        /*  4 */
		"TMSG",         /*  5 */
		"EMSG",         /*  6 */
		"DYNDSMSG",     /*  7 */
		"CREATESYS",    /*  8 */
		"GVTSYS",       /*  9 */
		"COMMAND",      /* 10 */
		"CRT_ACK",      /* 11 */
		"SIM_END_MSG",  /* 12 */
		"MIGR_LOG",     /* 13 */
		"XL_STATS",     /* 14 */
		"MONINIT",      /* 15 */
		"TW_ERROR",     /* 16 */
		"TYPE17",       /* 17 */
		"STATEMSG",     /* 18 */
		"STATEACK",     /* 19 */
		"STATENAK",     /* 20 */
		"STATEDONE",    /* 21 */
		"MOVEPHASE",    /* 22 */
		"PHASEACK",     /* 23 */
		"PHASENAK",     /* 24 */
		"PHASEDONE",    /* 25 */
		"MOVEVTIME",    /* 26 */
		"VTIMEACK",     /* 27 */
		"VTIMENAK",     /* 28 */
		"VTIMEDONE",    /* 29 */
		"TYPE30",       /* 30 */
		"TYPE31",       /* 31 */
		"HOMENOTIF",    /* 32 */
		"HOMEASK",      /* 33 */
		"HOMEANS",      /* 34 */
		"HOMECHANGE",   /* 35 */
		"CACHEINVAL",   /* 36 */
		"LOADSYS",      /* 37 */
		"PCREATESYS",   /* 38 */
		"ADDSTATS",     /* 39 */
		"CRITPATH",		/* 40 */
		"CRITSTEP",		/* 41 */
		"CRITEND",		/* 42 */
		"CRIT_LOG",		/* 43 */
		"CRITRM"		/* 44 */
	};

static char *gname[6] = {
	"GVTINIT",          /* 71 */
	"GVTSTART",         /* 72 */
	"GVTACK",           /* 73 */
	"GVTSTOP",          /* 74 */
	"GVTLVT",           /* 75 */
	"GVTUPDATE" };      /* 76 */

extern int ok_to_monitor;
extern char monitor_object[20];

int format ( bp, name, data, type, level )

	char **bp;
	char *name;
	int *data;
	int type;
	int level;
{
	int size = 1;       /* default argument size */

	sprintf ( *bp, " %s =", name );
	*bp += strlen ( *bp );

	if ( data == 0L )
	{
		sprintf ( *bp, " NULL" );
		*bp += strlen ( *bp );
		return ( size );
	}

	switch ( type )
	{
		case 1:                 /* Byte         */

			if ( level > 0 )
			{
				sprintf ( *bp, " %lx", (Byte *)data );
				*bp += strlen ( *bp );
			}
			break;

		case 2:                 /* Int          */

			if ( level > 0 )
			{
				sprintf ( *bp, " %d", *(Int *)data );
				*bp += strlen ( *bp );
			}
			break;

		case 3:                 /* Name         */

			if ( level > 0 )
			{
				sprintf ( *bp, " %s", (Name *)data );
				*bp += strlen ( *bp );

				if ( strcmp ( monitor_object, (Name *)data ) == 0 )
					ok_to_monitor = TRUE;
			}
			break;

		case 4:                 /* VTime        */

			if ( level > 0 )
			{
				form_vtime ( bp, *(VTime *)data );
			}
			size = sizeof ( VTime ) / sizeof ( int );
			break;

		case 5:                 /* Msgh         */

			if ( level > 0 )
			{
				if ( (((List_hdr *)data)-1)->size == 0 )
				{
					sprintf ( *bp, " LIST_HDR" );
					*bp += strlen ( *bp );
				}
				else
				{
					sprintf ( *bp, " %s", mname[((Msgh *)data)->mtype] );
					*bp += strlen ( *bp );
					sprintf ( *bp, " %s", ((Msgh *)data)->snder );
					*bp += strlen ( *bp );
					form_vtime ( bp, ((Msgh *)data)->sndtim );
					sprintf ( *bp, " %s", ((Msgh *)data)->rcver );
					*bp += strlen ( *bp );
					form_vtime ( bp, ((Msgh *)data)->rcvtim );

					if ( strcmp ( monitor_object, ((Msgh *)data)->snder) == 0 )
						ok_to_monitor = TRUE;
					if ( strcmp ( monitor_object, ((Msgh *)data)->rcver) == 0 )
						ok_to_monitor = TRUE;
				}
			}
			break;

		case 6:                 /* Ocb          */

			if ( level > 0 )
			{
				sprintf ( *bp, " %s", ((Ocb *)data)->name );
				*bp += strlen ( *bp );
				form_vtime ( bp, ((Ocb *)data)->svt );

				if ( strcmp ( monitor_object, ((Ocb *)data)->name ) == 0 )
					ok_to_monitor = TRUE;
			}
			break;

		case 7:                 /* STime                */

			if ( level > 0 )
			{
				sprintf ( *bp, " %f", *(STime *)data );
				*bp += strlen ( *bp );
			}
			break;

		case 8:                 /* Gvttype              */

			if ( level > 0 )
			{
				sprintf ( *bp, " %s", gname[*((Gvttype *)data)-GVTINIT] );
				*bp += strlen ( *bp );
			}
			break;

		case 9:                 /* Flag         */

			break;

		case 10:                /* Long         */

			if ( level > 0 )
			{
				sprintf ( *bp, " %ld", *(Long *)data );
				*bp += strlen ( *bp );
			}
			break;

		case 11:                /* Uint         */

			if ( level > 0 )
			{
				sprintf ( *bp, " %x", *(Uint *)data );
				*bp += strlen ( *bp );
			}
			break;

		case 12:                /* Ulong                */

			if ( level > 0 )
			{
				sprintf ( *bp, " %lx", *(Ulong *)data );
				*bp += strlen ( *bp );
			}
			break;

		case 13:                /* Char         */

			if ( level > 0 )
			{
				sprintf ( *bp, " %s", (Char *)data );
				*bp += strlen ( *bp );
			}
			break;

		case 14:                /* State        */

			if ( level > 0 )
			{
				sprintf ( *bp, " %lx", (State *)data );
				*bp += strlen ( *bp );
			}
			break;

		case 15:                /* int          */

			if ( level > 0 )
			{
				sprintf ( *bp, " %d", *(int *)data );
				*bp += strlen ( *bp );
			}
			break;

		case 16:                /* Objloc       */

			if ( level > 0 )
			{
				sprintf ( *bp, " %s", ((Objloc *)data)->name );
				*bp += strlen ( *bp );
				sprintf ( *bp, " %d", ((Objloc *)data)->node );
				*bp += strlen ( *bp );

				if ( strcmp ( monitor_object, ((Objloc *)data)->name ) == 0 )
					ok_to_monitor = TRUE;
			}

		default:

			if ( level > 0 )
			{
				sprintf ( *bp, " type%d", type );
				*bp += strlen ( *bp );
			}
			break;

	}
	return ( size );
}

form_vtime ( bp, vtime )

	char **bp;
	VTime vtime;
{
	if ( eqSTime ( vtime.simtime, posinf.simtime ) )
	{
		sprintf ( *bp, " POSINF" );
		*bp += strlen ( *bp );
	}
	else
	if ( eqSTime ( vtime.simtime, posinfPlus1.simtime ) )
	{
		sprintf ( *bp, " POSINF+1" );
		*bp += strlen ( *bp );
	}
	else
	if ( eqSTime ( vtime.simtime, neginf.simtime ) )
	{
		sprintf ( *bp, " NEGINF" );
		*bp += strlen ( *bp );
	}
	else
	if ( eqSTime ( vtime.simtime, neginfPlus1.simtime ) )
	{
		sprintf ( *bp, " NEGINF+1" );
		*bp += strlen ( *bp );
	}
	else
	{
		sprintf ( *bp, " %.2f", vtime.simtime );
		*bp += strlen ( *bp );
	}
}

#endif
