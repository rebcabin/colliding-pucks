/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	serve.c,v $
 * Revision 1.10  91/11/04  10:33:09  pls
 * 1.  Check for messages at time now (SCR 199).
 * 2.  Add selector info to error message.
 * 
 * Revision 1.9  91/11/01  09:51:15  reiher
 * timing stuff and critical path stuff (PLR)
 * 
 * Revision 1.8  91/07/17  15:12:21  judy
 * New copyright notice.
 * 
 * Revision 1.7  91/06/03  12:26:40  configtw
 * Tab conversion.
 * 
 * Revision 1.6  91/05/31  15:23:06  pls
 * Change message ordering.
 * 
 * Revision 1.5  91/04/15  10:00:28  pls
 * Use allocated memory for arg.text (bug 4)
 * 
 * Revision 1.4  91/04/01  15:46:13  reiher
 * Code to support Tapas Som's work.
 *
 * Revision 1.3  91/03/25  16:37:30  csupport
 * Add the service routine for tw_interrupt()
 *
 * Revision 1.2  90/12/10  10:52:57  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.1  90/08/07  15:40:58  configtw
 * Initial revision
 * 
*/
char serve_id [] = "@(#)serve.c 1.65\t9/26/89\t16:40:59\tTIMEWARP";


/*
Purpose:  

		serve.c is supposed to contain the code necessary to handle
		user calls that trap to the operating system.  There are
		also several user services available that do not trap to
		the operating system.  These are kept in services.c.
		Thus, these two modules should contain all code for handling
		user interactions with the operating system.

Functions:

		sv_interrupt() - Give control to Time Warp

		sv_tell(xrcvr,xrcvtim,selector,xlen,xtext) - Handle an event
				message send request.
				Parameters - Ocb *o, Char *xrcvr, VTime xrcvtime,
						Long selector, Int xlen, Char *xtext
				Return - Always returns 0

		sv_doit(o) - Try to clear an object blocked in the middle
				of an event message send.
				Parameters - Ocb *o
				Return - SUCCESS if it unblocked the object, FAILURE
						otherwise

		msgfind(o) - Look for
				a particular message in an object's output queue.
				Parameters - Ocb *o
				Return - NULL if the message is found, themsg otherwise

		uniq() - generate a message id unique to this node
				Parameters - none
				Return - the id (as a long)

Implementation:

		also calls chk_blocked() and dispatch() after the service
		routine returns.  chk_blocked() will see if a blocked object
		can unblock, and dispatch() will choose a new object to run.
		The latter routine may, then, cause the object that called
		serve() to relinquish control of the processor.  (But only
		after its request has been serviced.)

		The service routines themselves tend to be straightforward.
		Most of their work (beyond some sanity checks) involves setting
		up fields in the ocb's argblock.pkt_blk structure.  After all
		fields are appropriately set, a blocking status is set.  After
		the return from the service routine, serve() will call a routine
		that checks the blocking status, and handles the situation
		appropriately.

		sv_interrupt() was added in version 2.5.  It is called from
		tw_interrupt and is used to allow Time Warp to regain control
		from an object that is taking a very long time to execute.

		sv_doit() is called by chk_blocked().  It extracts the 
		information put into the argblock.pkt_blk structure.
		Then it handles the nitty-gritty details of outputting the
		message. It calls routines to allocate the memory for both the
		message and anti-message, calls uniq() to get ids for the message,
		sends out the message, and puts the anti-message in the
		output queue, with appropriate marks to indicate their status.
		Throughout, care is taken to ensure that things aren't done
		halfway, and don't otherwise go wrong.  sv_doit() may well
		end up blocked, because memory for the message and anitmessage
		can't be found.

		msgfind() tries to match a message in the ocb's output queue
		that matches the one being searched for.  There are complications
		based on marked and unmarked output messages, but, those aside,
		the routine merely attempts to match parameters to message fields,
		trying the easiest tests, and those most likely to fail, first.

		uniq() simply keeps a static counter that it increments on each
		call, returning the new value.  In combination with a node number,
		this value can comprise an identifier unique to the entire 
		simulation.

*/

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"

extern Int		allowNowFlag;
extern FILE * cpulog;

#   define arg o->argblock.pkt_blk

/***************************************************************************/

FUNCTION sv_interrupt ()

{
	Debug

	dispatch();
}  /* sv_interrupt */

/***************************************************************************/

FUNCTION sv_tell ( rcver, rcvtim, selector, txtlen, text )

	Name * rcver;
	VTime rcvtim;
	Long selector;
	int txtlen;
	Byte * text;
{
	register Ocb * o = xqting_ocb;

  Debug

	o->stats.evtmsg++;

	if ( txtlen > pktlen )
	{
		object_error ( MSGLEN );
		goto sv_tell_end;
	}

	/* error if receive time < object's time or
		receive time = now & recipient is self or
		receive time > +inf or
		receive time = now & allowNowFlag is false & 
			not in term section &
			receiver is not stdout type object */

	if ( ltVTime ( rcvtim, o->svt )
	|| ( eqVTime ( rcvtim, o->svt ) && namecmp (rcver, o->name) == 0)
	||   gtSTime ( rcvtim.simtime, posinf.simtime )
	|| ( eqVTime ( rcvtim, o->svt ) && !allowNowFlag &&
		!eqSTime ( rcvtim.simtime, posinf.simtime) &&
		(namecmp ( rcver, "stdout") != 0) &&
		fileNum(rcver) == MAX_TW_FILES)
		)
		{
#if PARANOID
		twerror ("sv_tell I illegal %s at %f,%d,%d to %s at %f,%d,%d s: %d",
				 o->name, o->svt.simtime, o->svt.sequence1, o->svt.sequence2,
				rcver, rcvtim.simtime, rcvtim.sequence1, rcvtim.sequence2,
				selector);
#endif
		object_error ( MSGTIME );
		goto sv_tell_end;
		}

	if ( *(Byte *)rcver == 0 )
	{
#ifdef PARANOID
		twerror ( "sv_tell I null receiver from %s at %f",
				o->name, o->svt.simtime );
#endif
		object_error ( MSGRCVER );
		goto sv_tell_end;
	}

	arg.selector = selector;

	arg.mtype = EMSG;

	arg.rcver = rcver;
	arg.rcvtim = rcvtim;
	arg.len = txtlen;
	arg.text = text;

	arg.gid.node = miparm.me;
	arg.gid.num = uniq ();

	o->runstat = BLKPKT;        /* requests transmission */

sv_tell_end:

	dispatch ();
}


#ifdef MSGTIMER
extern long msgstart;
#endif MSGTIMER
FUNCTION sv_doit ( o )

	Ocb *o;
{
	Msgh *buf, *antibuf, *themsg;
	register int txtlen;
	Byte * text;
	extern int aggressive;

Debug

	if ( aggressive )
	{
		if ( ( themsg = o->co ) == NULL )
		{
			_pprintf ( "sv_doit null o->co for %s\n", o->name );
			tester();
		}
	}    
	else
	if ( ( themsg = msgfind ( o ) ) == NULL )
	{
		return SUCCESS;
	}

	text = arg.text,
	txtlen = arg.len;

	buf = mkomsg ( o, arg.rcvtim, arg.rcver, arg.mtype, txtlen, arg.selector );

	if ( buf == NULL )
	{
		/* OK, be that way!  We'll try again later */

		return FAILURE;
	}

	if ( ltVTime ( arg.rcvtim, gvt ) )
	{
		/* don't send the message -- if this condition is reached,       */
		/* then you should have gotten a HIT in the output queue,        */
		/* but didn't because user left some stack garbage in the        */
		/* message that is different THIS time through from what it      */
		/* was LAST time through.  So, the message the user is   */
		/* sending now should have been found in the output queue,       */
		/* but was MISSED.  What should we set o->co to be  ?  @@@       */
		/* could be a bug sliming around here, but for now @@@   */
		/* I won't disturb o->co since there is nothing @@@              */
		/* reasonable to set it equal to. @@@                    */

		entcpy ( buf + 1, text, txtlen );
		twerror ( "SV_DOIT E Message receive time earlier than GVT" );
		showmsg ( buf );
		tester ();
		l_destroy ( (List_hdr *) buf ); /* and don't produce antimessage */
		return FAILURE;
	}

	if ( ltVTime ( o->svt, gvt ) )
	{
		entcpy ( buf + 1, text, txtlen );
		twerror ( "SV_DOIT E Message send time earlier than GVT" );
		showmsg ( buf );
		tester ();
		l_destroy ( (List_hdr *) buf );
		return FAILURE;
	}

	if ( (antibuf =
			mkomsg ( o, arg.rcvtim, arg.rcver, arg.mtype, txtlen, arg.selector
		)) == NULL )
	{
		l_destroy ( (List_hdr *) buf );

		return FAILURE;
	}

	entcpy ( buf + 1, text, txtlen );
	entcpy ( antibuf + 1, text, txtlen );

	buf->gid = arg.gid;
	antibuf->gid = arg.gid;
#ifdef SOM
	buf->Ept = o->sb->Ept;
#endif SOM

	mark_macro ( buf );
	mark_macro ( antibuf );
	negate_macro ( antibuf );

	l_insert ( themsg, antibuf );

	if ( aggressive || o->co == o->oqh || neVTime ( o->co->sndtim, o->svt ) )
	{
		o->co = antibuf;
	}

	smsg_stat ( o, buf );

	if ( cpulog && arg.mtype == EMSG )
	{
		HOST_fprintf ( cpulog, "M %s %f %d %s %f\n", o->name, o->svt.simtime,
						o->stats.cputime, arg.rcver, arg.rcvtim.simtime );

		buf->cputime = o->stats.cputime;
	}

	if ((arg.mtype == DYNCRMSG) || (arg.mtype == DYNDSMSG))
		{       /* this is our own memory */
		l_destroy((List_hdr *)arg.text);
		}

#ifdef MSGTIMER
	if ( !mlog )
		buf->msgtimef = msgstart;
#endif MSGTIMER
	deliver ( buf );


	/* Keep track of the fact that this event sent one more message. */

	o->sb->resultingEvents++;

	return SUCCESS;
}


FUNCTION Msgh * msgfind ( o )

	Ocb         *o;
{
	register Msgh  *p, *last_p;

  Debug

	if ( o->co == NULL )
	{
		_pprintf ( "msgfind: you blew it dummy!\n" );
		tester ();
	}

	if ( o->co == o->oqh || neVTime ( o->co->sndtim, o->svt ) )
	{
		return ( o->co );
	}

	p = fstomb ( o->co );

	last_p = (Msgh *) l_prev_macro ( p );

	for ( ; p; last_p = p, p = nxtomb (p) )
	{
		if ( p->stype < o->centry ) break;

		/*  In this case, the other message in the bundle already in the
				output queue came from another node, due to migration.
				If that node had a higher node number than the current node,
				the new message should be queued before the message p currently
				points to. */

		if ( p->gid.node > arg.gid.node ) break;

/*
		if ( p->gid.node < arg.gid.node
		|| ( p->gid.node == arg.gid.node && p->gid.num < arg.gid.num ) )
			last_p = p;
*/

		if ( p->stype > o->centry ) continue;

		/* only look at unmarked messages in the output queue so that  */
		/* if the user produces several identical messages they will */
		/* be "msgfound" once each                                   */

		if ( isposi_macro ( p ) ) continue;
		if ( ismarked_macro ( p ) ) continue;

		if ( p->txtlen != arg.len ) continue;
		if ( neVTime ( p->rcvtim, arg.rcvtim ) ) continue;
		if ( p->mtype != arg.mtype ) continue;
		if ( p->selector != arg.selector ) continue;
		if ( namecmp ( p->rcver, arg.rcver ) != 0 ) continue;
		if ( bytcmp ( (Byte *) (p + 1), arg.text, arg.len ) != 0 ) continue;

		mark_macro ( p );
		return ( NULL );
	}

	return ( last_p );
}

FUNCTION long uniq ()
{
	static long count = 1000;   /* Start at 1000 for initialization */

	count++;    /* just a counter */

	return count;
}


/* This code is probably redundant, and can be replaced by calling sv_event()
		for dynamic creation messages.  */

FUNCTION sv_create ( rcver, rcvtim, objtype )

	Name *rcver;
	VTime rcvtim;
	Name *objtype;
{
	register Ocb * o = xqting_ocb;
	int txtlen = sizeof ( Crttext );
	Crttext *create_text;

  Debug

	if ((create_text = (Crttext *)m_create(sizeof(Crttext),o->svt,NONCRITICAL))
		== NULL)
		_pprintf("sv_create: out of memory");
	clear ( create_text, sizeof ( Crttext ) );

	strcpy ( create_text->tp, objtype );
	create_text->node = -1;

	if ( ltVTime ( rcvtim, o->svt )
	|| ( eqVTime ( rcvtim, o->svt ) && namecmp (rcver, o->name) == 0)
	||   gtSTime ( rcvtim.simtime, posinf.simtime ) )
	{
#ifdef PARANOID
		twerror ("sv_create I illegal %s at %f,%d,%d to %s at %f,%d,%d",
				 o->name, o->svt.simtime, o->svt.sequence1, o->svt.sequence2,
				rcver, rcvtim.simtime, rcvtim.sequence1, rcvtim.sequence2 );
#endif
		object_error ( MSGTIME );
		goto sv_create_end;
	}

	arg.mtype = DYNCRMSG;

	arg.rcver = rcver;
	arg.rcvtim = rcvtim;
	arg.len = txtlen;
	arg.text = (Byte *) create_text;

	arg.gid.node = miparm.me;
	arg.gid.num = uniq ();

	o->runstat = BLKPKT;        /* requests transmission */

sv_create_end:

	dispatch ();
}

FUNCTION sv_destroy ( rcver, rcvtim )

	Name *rcver;
	VTime rcvtim;
{
	register Ocb * o = xqting_ocb;
	int txtlen = sizeof ( Crttext );
	Crttext *create_text;

  Debug

	if ((create_text = (Crttext *)m_create(sizeof(Crttext),o->svt,NONCRITICAL))
		== NULL)
		_pprintf("sv_destroy: out of memory");
	clear ( create_text, sizeof ( Crttext ) );

	strcpy ( create_text->tp, "NULL" );
	create_text->node = -1;

	if ( ltVTime ( rcvtim, o->svt )
	|| ( eqVTime ( rcvtim, o->svt ) && namecmp (rcver, o->name) == 0)
	||   gtSTime ( rcvtim.simtime, posinf.simtime ) )
	{
#ifdef PARANOID
		twerror ("sv_destroy I illegal %s at %f,%d,%d to %s at %f,%d,%d",
				 o->name, o->svt.simtime, o->svt.sequence1, o->svt.sequence2,
				rcver, rcvtim.simtime, rcvtim.sequence1, rcvtim.sequence2 );
#endif
		object_error ( MSGTIME );
		goto sv_destroy_end;
	}

	arg.mtype = DYNDSMSG;

	arg.rcver = rcver;
	arg.rcvtim = rcvtim;
	arg.len = txtlen;
	arg.text = (Byte *) create_text;

	arg.gid.node = miparm.me;
	arg.gid.num = uniq ();

	o->runstat = BLKPKT;        /* requests transmission */

sv_destroy_end:

	dispatch ();
}
