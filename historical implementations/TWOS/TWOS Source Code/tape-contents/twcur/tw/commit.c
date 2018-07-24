/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	commit.c,v $
 * Revision 1.5  91/07/17  15:07:41  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/07/09  13:26:12  steve
 * replace 128 with IH_NODE
 * 
 * Revision 1.3  91/06/03  12:23:56  configtw
 * Tab conversion.
 * 
 * Revision 1.2  90/12/10  10:40:50  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.1  90/08/07  15:38:07  configtw
 * Initial revision
 * 
*/
char commit_id [] = "@(#)commit.c       1.20\t9/26/89\t16:48:05\tTIMEWARP";


/*

Purpose:

		The only routine in this module does fossil collection for
		the input queue of the stdout object.  It determines what 
		virtual time is safe to commit on, and removes all messages 
		with rcvtimes less than that time from the input queue.  
		If standard output is being collected, the messages are sent to
		the IH object for output.  Otherwise, they are simply removed from 
		the queue and their memory reclaimed.

Functions:

		commit() - clear fossils from the input queue
				Parameters - none
				Return - always returns 0

Implementation:

		Find the stdout object on this node.  If it exists, set 
		outtime to a safe commit time.  This time is not necessarily
		gvt, because, apparently, the gvt calculation does not take
		stdout objects into account.  Therefore, compare gvt and
		the local simulation time, svt.  Choose the minimum of the
		two for outtime.  (If gvt is POSINF, we can commit everything,
		because no messages that will cause rollback are coming.)

		Run through the messages in the input queue one at a time,
		until you run out of messages or a message with a receive
		time >= outtime is found.  If you find an anti-message, there
		is a problem, as they should have been annihilated by now.
		Print an error message in this case, and dump the anti-message
		to the screen.

		Get a pointer to the next message, call a stats routine on
		this message, and check its type.  If the message is an
		event message, then it was meant to be user-level output.
		So package it up and send it off to node IH_NODE, which will take
		care of really outputting it.  If standard out isn't really being
		output, just call l_remove() and l_destroy() to get the
		message out of the input queue and reclaim its memory.
		Otherwise, remove it from the queue, then send it off to
		node IH_NODE, as a system message.  Presumably, the system will
		get it to IH, which then knows what to do with it.  After
		the message goes out, sndmsg() will take care of reclaiming
		the memory.  Go on to the next message.
*/

#include "twcommon.h"
#include "twsys.h"


FUNCTION commit ( ocb )

	register Ocb * ocb; /* OCB to check for physical output to commit */
{
	register Msgh * imsg, * nxtmsg;
	VTime outtime;

Debug

	if( ocb != 0 ) {

		/* Send to the application standard output all input
		 * messages whose receive times are less than the 
		 * object's svt
		 * assign object output time (outtime) to the minimum of GVT or SVT.
		 * this is the time such that for all messages less than this time,
		 * we will commit output (FPW  4/86). We must use SVT for stdout because
		 * the GVT calculation has been changed to ignore STDOUT. Thus if 
		 * svt < gvt we cannot commit output in the time range [svt, gvt]
		 * because stdout may be in the middle of rollback or annihilation.
		 * HOWEVER, if GVT = infinity, we want to go ahead and commit all 
		 * of the remaining output.
		 */

		if ( ltVTime ( gvt, ocb->svt ) )
			outtime = gvt;
		else
			outtime = ocb->svt;

		if ( geSTime ( gvt.simtime, posinf.simtime ) )
			outtime = gvt;

		imsg = fstimsg_macro(ocb);

		while ( (imsg != NULL) && ltVTime(imsg->rcvtim, outtime) ) {

			/* cycle through the input message queue */
#ifdef PARANOID
			if ( isanti_macro (imsg) ) {
				twerror ( "comout F committed antimessage" );
				dumpmsg ( imsg );
				tester ();
				}
#endif
			nxtmsg = nxtimsg_macro(imsg);       /* line up the next msg */

			stats_garbtime(ocb, imsg, nxtmsg);  /* count committed events */

			if ( ocb != stdout_ocb )
			{  /* it's going to a file */
				HOST_fwrite ( (char *) (imsg + 1), imsg->txtlen, 1, 
								ocb->co );      /* output the message */
				HOST_fflush ( ocb->co );

				if ( nxtmsg == NULL && 
					 eqSTime ( gvt.simtime, posinfPlus1.simtime ) 
				   )
					HOST_fclose ( ocb->co );    /* close if we're done */

				l_remove ( (List_hdr *) imsg ); /* dump the message */
				l_destroy ( (List_hdr *) imsg );
			}
			else
			if ( no_stdout )
			{  /* no stdout--just dump the message */
				l_remove ( (List_hdr *) imsg );
				l_destroy ( (List_hdr *) imsg );
			}
			else
			{  /* send the message to stdout on node IH_NODE */
				l_remove ( (List_hdr *) imsg );
				imsg->flags |= SYSMSG;
				sndmsg ( imsg, imsg->txtlen+sizeof(Msgh), IH_NODE);
			}

			imsg = nxtmsg;      /* go to the next message */

			}
		}
	}


