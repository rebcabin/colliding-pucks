/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	objifc.c,v $
 * Revision 1.12  91/12/27  09:10:27  pls
 * Fix up TIMING code.
 * 
 * Revision 1.11  91/11/01  13:32:23  reiher
 * Added timing code (PLR)
 * 
 * Revision 1.10  91/11/01  09:56:00  pls
 * 1.  Change ifdef's, version id.
 * 2.  Add speculative computing interface (SCR 172).
 * 3.  Change "NULL" error message.
 * 
 * Revision 1.9  91/07/17  15:11:26  judy
 * New copyright notice.
 * 
 * Revision 1.8  91/07/09  14:38:03  steve
 * Added MicroTime and object_timing_mode support
 * 
 * Revision 1.7  91/06/07  13:48:58  configtw
 * Handle non CRT_ACK messages.
 * 
 * Revision 1.6  91/06/03  12:25:53  configtw
 * Tab conversion.
 * 
 * Revision 1.5  91/04/01  15:45:22  reiher
 * Phase creation routines, plus support for Tapas Som's work.
 * 
 * Revision 1.4  91/03/26  09:36:15  pls
 * Change hoglog implementation.
 * 
 * Revision 1.3  91/03/25  16:27:27  csupport
 * 1.  Implement hoglog.
 * 2.  Change tell() to schedule().
 *
 * Revision 1.2  90/08/27  10:44:30  configtw
 * Split cycle time from committed time.
 * 
 * Revision 1.1  90/08/07  15:40:39  configtw
 * Initial revision
 * 
*/
char objifc_id [] = "@(#)objifc.c       $Revision: 1.12 $\t$Date: 91/12/27 09:10:27 $\tTIMEWARP";


/*

Purpose:

		objifc.c contains the object interface for Time Warp.  Objects
		wishing to communicate with Time Warp typically go through this
		code to get to the system.  Since many object actions can be faked
		through the tester, much of this code is set up to permit manual
		execution of the commands.

Functions:

		obcreate_b(rcver,rcvtim,objtype,node) - do the actual work of creating 
						an object and informing all nodes of its creation
				Parameters - Name * rcver, VTime rcvtim, Name * objtype, 
						int node
				Return - Always returns zero

		obcreate(rcver,rcvtim,objtype,node) - prepare to switch context
						for an object creation
				Parameters - Name * rcver, VTime rcvtim, Name * objtype, 
						int node
				Return - Always returns zero
				
		newObj(rcver,rcvtim,objtype) - prepare for object creation
				Parameters - Name *rcver, VTime rcvtim, Name *objtype
				Return - msgRef structure for use by cancel()
		
		delObj(rcver,rcvtim) - prepare for object deletion
				Parameters - Name *rcver, VTime rcvtim
				Return - msgRef structure for use by cancel()

		schedule(rcver,rcvtim,selector,txtlen,message) - prepare to switch
					context for a schedule message call
				Parameters - Name * rcver, VTime rcvtim,
						Long selector, int txtlen, String * message
				Return - msgRef structure for use by cancel()
				
		cancel(msgID) - cancel a previously sent message
				Parameters - msgRef msgID
				Return -
				
		guess(sndtime,rcvtime,rcvr,selector,txtlen,text) - send a
					speculative messsage
				Parameters - VTime sndtime, VTime rcvtime, Name *rcvr,
					Long selector, Int txtlen, String *text
				Return -
				
		unguess(sndtime,rcvtime,rcvr,selector,txtlen,text) - unsend a
					speculative messsage
				Parameters - VTime sndtime, VTime rcvtime, Name *rcvr,
					Long selector, Int txtlen, String *text
				Return -									

		tw_interrupt() - switch context to give Time Warp control

Implementation:

		obcreate_b() provides a manual way to create an object,
		by calling create_object() and create_inform() on the
		parameters provided to it. obcreate() is the system version,
		which basically just calls switch_back() with obcreate_b() as a 
		parameter (thus causing execution to switch to that routine.)

		tw_interrupt was implemented in version 2.5.  It's sole purpose
		is to allow objects which take large time slices to make dummy
		calls to Time Warp in order to check for such things as incoming
		messages and higher priority objects to execute.  
*/

#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

extern int object_ended;

extern Byte * object_context;
extern Byte * object_data;

#if TIMING
#define SERVE_TIMING_MODE 9
#endif

extern int      hlog;
extern VTime    hlogVTime;
extern int      maxSlice;
extern Ocb*     maxSliceObj;
extern VTime    maxSliceTime;
extern int      sliceTime;


FUNCTION create_object ( object, objtype, node )

	Name * object;
	Name * objtype;
	int node;
{
	char * snder;
	VTime sndtim;
	VTime rcvtim;
	Int txtlen;
	Msgh * tw_msg;
	Crttext create_text;
	int n;
	Int home;

  Debug
	clear ( &create_text, sizeof ( Crttext ) );

	strcpy ( create_text.tp, objtype );

	n = node % tw_num_nodes;

	create_text.node = n;

	snder = "IH";
	sndtim = rcvtim = neginf;
	txtlen = sizeof ( Crttext );

	create_text.phase_begin = neginf;
	create_text.phase_end = posinfPlus1;


	/* Find this object's home node and send a message to it notifying it of
		the object's initial location. */

	home = name_hash_function(object,HOME_NODE);

	tw_msg = make_message ( HOMENOTIF, snder, sndtim, object,
				newVTime ( 0.0, 0, 0 ), txtlen, &create_text);

	tw_msg->flags |= SYSMSG;


	sndmsg ( tw_msg, sizeof ( Msgh ) + txtlen, home );

	/* If the local node is the home node, call msgproc() to handle the
		home node notification right away. */

	if (home == tw_node_num)
	{
		msgproc();
	}

	tw_msg = make_message ( (Byte) CREATESYS, snder, sndtim, object, 
				rcvtim, txtlen, &create_text );

	tw_msg->flags |= SYSMSG ;


	sndmsg ( tw_msg, sizeof ( Msgh ) + txtlen, n );

	if ( n == tw_node_num )
	{
		msgproc ();
	}
	else /* Node 0 cheat for performance for schedules from config file */
	{
		CacheReplace ( object, neginf, posinfPlus1, n, 0,
			name_hash_function ( object, CACHE ) );
	}
}


/* Create a phase.  Send a notification message to its home node, and a
		create message to the node hosting it. */

/* Things needing more work -
		2.  Need to write code to handle create phase sys msg in mproc.c
		3.  Need to write code to actually do phase creation
		4.  Probably need some VTime utility routines to allow users to
				specify positive and negative infinity
*/

FUNCTION create_phase ( object, objtype, node, begin, end )

	Name * object;
	Name * objtype;
	int node;
	double begin;
	double end;
{
	char * snder;
	VTime sndtim;
	VTime rcvtim;
	Int txtlen;
	Msgh * tw_msg;
	Crttext create_text;
	int n;
	Int home;

  Debug
	clear ( &create_text, sizeof ( Crttext ) );

	strcpy ( create_text.tp, objtype );

	n = node % tw_num_nodes;

	create_text.node = n;

	snder = "IH";
	sndtim = rcvtim = neginf;
	txtlen = sizeof ( Crttext );

	create_text.phase_begin = newVTime ( begin, 0, 0 );
	create_text.phase_end = newVTime ( end, 0, 0 );


	/* Find this object's home node and send a message to it notifying it of
		the object's initial location. */

	home = name_hash_function(object,HOME_NODE);

/* Make sure home notification code knows that there's a begin and end time
		to deal with in this message. */

	tw_msg = make_message ( HOMENOTIF, snder, sndtim, object,
				newVTime ( 0.0, 0, 0 ), txtlen, &create_text);

	tw_msg->flags |= SYSMSG;


	sndmsg ( tw_msg, sizeof ( Msgh ) + txtlen, home );

	/* If the local node is the home node, call msgproc() to handle the
		home node notification right away. */

	if (home == tw_node_num)
	{
		msgproc();
	}

/* Be sure to set things up so everyone knows about the PCREATESYS message
		type, and handles it correctly. */


	tw_msg = make_message ( (Byte) PCREATESYS, snder, sndtim, object, 
				rcvtim, txtlen, &create_text );

	tw_msg->flags |= SYSMSG ;


	sndmsg ( tw_msg, sizeof ( Msgh ) + txtlen, n );

	if ( n == tw_node_num )
	{
		msgproc ();
	}
	else /* Node 0 cheat for performance for tells from config file */
	{
		CacheReplace ( object, neginf, posinfPlus1, n, 0,
			name_hash_function ( object, CACHE ) );
	}
}

now_cmd ()

{
	VTime time;

	if ( manual_mode )
	{
		time = obj_now ();
		_pprintf ( "NOW = %f,%d,%d\n", time.simtime, time.sequence1, time.sequence2 );
	}
	else
		_pprintf ( "now can only be called from manual mode\n" );
}

myName_cmd ()
{
	Name object_name;

	if ( manual_mode )
	{
		obj_myName (object_name);
		_pprintf ( "OBJECT NAME = %s\n", object_name );
	}
	else
		_pprintf ( "myName can only be called from manual mode\n" );
}

FUNCTION obcreate_cmd ( rcver, objtype, node )

	Name * rcver;
	Name * objtype;
	int * node;
{


	if ( strcmp ( rcver, "stdout" ) == 0 )
	{
		_pprintf ( "You don't need to create stdout any more\n" );
		return;
	}

	if ( manual_mode )
		obcreate ( rcver, objtype, *node );
	else
		obcreate_b ( rcver, objtype, * node );
}

FUNCTION obcreate_b ( rcver, objtype, node )

	Name * rcver;
	Name * objtype;
	int node;
{
	int n;

  Debug

#if TIMING
	start_timing ( TESTER_TIMING_MODE );
#endif

	create_object ( rcver, objtype, node );


	n = node % tw_num_nodes;


	if ( n != 0 )
	{
		while ( rm_msg != NULL && rm_msg->mtype != CRT_ACK )
			msgproc ();
		while ( rm_msg == NULL )
		{
			send_from_q();	/* in case CREATESYS still in pmq */
			read_the_mail ( 0 );        /* wait for CRT_ACK */

			while ( rm_msg != NULL && rm_msg->mtype != CRT_ACK )
				msgproc ();
		}
		if ( rm_msg->mtype != CRT_ACK )
		{
			_pprintf ( "obcreate got a bad response\n" );
			dumpmsg ( rm_msg );
			tw_exit (0);
		}
		acceptmsg ( NULL );
	}

#if TIMING
	stop_timing ();
#endif
}

FUNCTION phcreate_cmd ( rcver, objtype, node, begin, end )

	Name * rcver;
	Name * objtype;
	int * node;
	STime * begin;
	STime * end;
{



	if ( strcmp ( rcver, "stdout" ) == 0 )
	{
		_pprintf ( "You don't need to create stdout any more\n" );
		return;
	}

	if ( manual_mode )
		phcreate ( rcver, objtype, *node, *begin, *end );
	else
		phcreate_b ( rcver, objtype, * node, *begin, *end );
}

/* Create a phase from the config file.  The parameters are the phase's name,
		type, node hosting it, phase begin time, and phase end time.  This
		command will only create phases on simulation time boundaries, as
		the begin and end do not accept full VTime structures.  Also, this
		routine does no error checking to determine that the object has all
		phases necessary to make up its full interval, nor that there are
		no overlapping phases, nor that all phases are of the same type.
		Therefore, this command is currently an "experts-only" command. */

FUNCTION phcreate_b ( rcver, objtype, node, begin, end )

	Name * rcver;
	Name * objtype;
	int node;
	double begin;
	double end;
{
	int n;

  Debug

#if TIMING
	start_timing ( TESTER_TIMING_MODE );
#endif

	create_phase ( rcver, objtype, node, begin, end );


	n = node % tw_num_nodes;


	if ( n != 0 )
	{
		while ( rm_msg != NULL && rm_msg->mtype != CRT_ACK )
			msgproc ();
		while ( rm_msg == NULL )
		{
			read_the_mail ( 0 );        /* wait for CRT_ACK */

			while ( rm_msg != NULL && rm_msg->mtype != CRT_ACK )
				msgproc ();
		}
		if ( rm_msg->mtype != CRT_ACK )
		{
			_pprintf ( "phcreate got a bad response\n" );
			dumpmsg ( rm_msg );
			tw_exit (0);
		}
		acceptmsg ( NULL );
	}

#if TIMING
	stop_timing ();
#endif
}

FUNCTION obcreate ( rcver, objtype, node )

	Name * rcver;
	Name * objtype;
	int node;
{
  Debug

	if ( object_context != NULL )
	{
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

		xqting_ocb->stats.cputime += object_end_time - object_start_time;


		switch_back ( obcreate_b, object_context,
			rcver, objtype, node );
	}
	else
	{
		obcreate_b ( rcver, objtype, node );
	}

}  /* obcreate */

FUNCTION phcreate ( rcver, objtype, node, begin, end )

	Name * rcver;
	Name * objtype;
	int node;
	double begin;
	double end;
{
  Debug

	if ( object_context != NULL )
	{  
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

	  xqting_ocb->stats.cputime += object_end_time - object_start_time;


	  switch_back ( phcreate_b, object_context,
		  rcver, objtype, node, begin, end );
	}  
	else
	{  
	  phcreate_b ( rcver, objtype, node, begin, end );
	}  

}



FUNCTION msgRef newObj ( rcver, rcvtim, objtype )

	Name *rcver;
	VTime rcvtim;
	Name *objtype;
{
	msgRef	retVal;
	int sv_create ();

  Debug

	retVal.gid.node = tw_node_num;
	retVal.gid.num = 0;		/* error value */
	objectCode = FALSE;		/* not executing object code now */
	if ( object_context != NULL )
	{
		if ( prop_delay )
			delay_object ();

#if TIMING
		stop_timing();
#endif

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
			if ((sliceTime > maxSlice) && (geVTime(gvt,hlogVTime)))
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
	/*  Calculate the ept for the state of the event just interrupted. */

	xqting_ocb->sb->Ept += object_end_time - object_start_time;
	xqting_ocb->work += object_end_time - object_start_time;
#endif SOM

		switch_back ( sv_create, object_context, rcver, rcvtim, objtype );
	}
	else
		_pprintf ( "You must call newObj() from within an object.\n" );
	return(retVal);
}  /* newObj */

FUNCTION msgRef delObj ( rcver, rcvtim )

	Name *rcver;
	VTime rcvtim;
{
	msgRef	retVal;

	int sv_destroy ();

  Debug

	retVal.gid.node = tw_node_num;
	retVal.gid.num = 0;		/* error value */
	objectCode = FALSE;		/* not executing object code now */

	if ( object_context != NULL )
	{
		if ( prop_delay )
			delay_object ();

#if TIMING
        stop_timing();
#endif

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
			if ((sliceTime > maxSlice) && (geVTime(gvt,hlogVTime)))
				{ /* this slice was bigger than max so far */
				maxSlice = sliceTime;
				maxSliceObj = xqting_ocb;
				maxSliceTime = xqting_ocb->svt;
				}
			}  /* if hlog */

		xqting_ocb->stats.cputime += object_end_time - object_start_time;
		xqting_ocb->cycletime += object_end_time - object_start_time;
		xqting_ocb->stats.comtime += object_end_time - object_start_time;
		xqting_ocb->sb->effectWork += object_end_time - object_start_time;

#if SOM
	/*  Calculate the ept for the state of the event just interrupted. */

	xqting_ocb->sb->Ept += object_end_time - object_start_time;
	xqting_ocb->work += object_end_time - object_start_time;
#endif SOM

		switch_back ( sv_destroy, object_context, rcver, rcvtim );
	}
	else
		_pprintf ( "You must call delObj() from within an object.\n" );
	return(retVal);
}  /* delObj */

int config_eposfs;

#ifdef MSGTIMER
long msgstart, msgend;
long msgtime = 0;
long msgcount = 0;
int onNodeTime = TRUE;
#endif MSGTIMER

FUNCTION tell_cmd ( rcver, rcvtim, selector, message )

	Name * rcver;
	STime * rcvtim;
	Long * selector;
	String * message;
{
	register int txtlen = strlen ( message ) + 1;
	extern int mlog, node_cputime;

  Debug

#ifdef MSGTIMER
	msgstart = clock ();
#endif MSGTIMER

	if ( manual_mode )
		schedule(rcver,newVTime ( *rcvtim, 0, 0 ), *selector, txtlen, message );
	else
	{
		register Msgh * msg;
		register char * snder;
		VTime sndtim;

		snder = "IH";
		sndtim = neginf;

		msg = make_message ( EMSG, snder, sndtim, rcver,
						newVTime ( *rcvtim, 0, 0 ), txtlen, message );

		msg->selector = *selector;
#ifdef MSGTIMER
	if ( !mlog )
		msg->msgtimef = msgstart;
#endif MSGTIMER

		if ( mlog )
		{
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
			msg->cputime = node_cputime;
		}

		deliver ( msg );

		config_eposfs++;
	}
}

FUNCTION msgRef schedule ( rcver, rcvtim, selector, txtlen, message )

	Name * rcver;
	VTime rcvtim;
	Long selector;
	int txtlen;
	String * message;
{
	msgRef	retVal;

	int sv_tell ();

	extern STime cutoff_time;

	retVal.gid.node = tw_node_num;
	retVal.gid.num = 0;		/* error value */
	if ( gtSTime ( rcvtim.simtime, cutoff_time ) )
		return(retVal);

  Debug
#ifdef MSGTIMER
	gstart = clock ();
	NodeTime = FALSE;
#endif MSGTIMER

	objectCode = FALSE;		/* not executing object code now */

	if ( object_context != NULL )
	{
		if ( prop_delay )
			delay_object ();

#if TIMING
	stop_timing ();
#endif

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
			if ((sliceTime > maxSlice) && (geVTime(gvt,hlogVTime)))
				{ /* this slice was bigger than max so far */
				 maxSlice = sliceTime;
				maxSliceObj = xqting_ocb;
				maxSliceTime = xqting_ocb->svt;
				}
			}  /* if hlog */

		xqting_ocb->stats.cputime += object_end_time - object_start_time;
		xqting_ocb->cycletime += object_end_time - object_start_time;
		xqting_ocb->stats.comtime += object_end_time - object_start_time;
		xqting_ocb->sb->effectWork += object_end_time - object_start_time;

#if SOM
	/*  Calculate the ept for the state of the event just interrupted. */

	xqting_ocb->sb->Ept += object_end_time - object_start_time;
	xqting_ocb->work += object_end_time - object_start_time;
#endif SOM

		switch_back ( sv_tell, object_context,
			rcver, rcvtim, selector, txtlen, message );

	}
	else
		_pprintf ( "You must call schedule() from within an object.\n" );
	return(retVal);
}  /* schedule */

FUNCTION unschedule ( rcver, rcvtim, selector, txtlen, message )

	Name * rcver;
	VTime rcvtim;
	Long selector;
	int txtlen;
	String * message;
{
}	/* unschedule */

FUNCTION cancel(msgID)
	msgRef		msgID;
{
}	/* cancel */

FUNCTION guess(sndtime,rcvtime,rcvr,selector,txtlen,text)
	VTime		sndtime;
	VTime		rcvtime;
	Name		*rcvr;
	Long		selector;
	Int			txtlen;
	String		*text;
{
}	/* guess */

FUNCTION unguess(sndtime,rcvtime,rcvr,selector,txtlen,text)
	VTime		sndtime;
	VTime		rcvtime;
	Name		*rcvr;
	Long		selector;
	Int			txtlen;
	String		*text;
{
}	/* unguess */

numMsgs_cmd ()

{
	int msg_count;

	if ( manual_mode )
	{
		msg_count = obj_numMsgs ();
		_pprintf ( "MESSAGE COUNT = %d\n", msg_count );
	}
	else
		_pprintf ( "numMsgs can only be called from manual mode\n" );
}

msg_cmd ( msgnum )

	int *msgnum;
{
	char * msgbuf;

	if ( manual_mode )
	{
		msgbuf = msgText ( *msgnum );
		_pprintf ( "MESSAGE = %s\n", msgbuf );
	}
	else
		_pprintf ( "msg can only be called from manual mode\n" );
}

FUNCTION tw_interrupt ()

{
	int sv_interrupt ();

  Debug

	objectCode = FALSE;		/* not executing object code now */

	if ( object_context != NULL )
	{
		if ( prop_delay )
			delay_object ();

#if TIMING
	stop_timing ();
#endif

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
			if ((sliceTime > maxSlice) && (geVTime(gvt,hlogVTime)))
				{ /* this slice was bigger than max so far */
				maxSlice = sliceTime;
				maxSliceObj = xqting_ocb;
				maxSliceTime = xqting_ocb->svt;
				}
			}  /* if hlog */

		xqting_ocb->stats.cputime += object_end_time - object_start_time;
		xqting_ocb->cycletime += object_end_time - object_start_time;
		xqting_ocb->stats.comtime += object_end_time - object_start_time;
		xqting_ocb->sb->effectWork += object_end_time - object_start_time;

		switch_back ( sv_interrupt, object_context);

	}
	else
		_pprintf ( "You must call tw_interrupt() from within an object.\n" );
}  /* tw_interrupt */
