/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	mkmsg.c,v $
 * Revision 1.3  91/07/17  15:10:20  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:25:23  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:40:16  configtw
 * Initial revision
 * 
*/
char mkmsg_id [] = "@(#)mkmsg.c 1.9\t6/2/89\t12:44:35\tTIMEWARP";


/*

Purpose:

		mkmsg.c contains routines that request and initialize message
		buffers.  These routines are used only in Tester.c and flower.c.
		The first routine requests space, then sets up the message.
		The second routine works with an already-allocated space,
		and merely sets up the message.

Functions:

		make_message(mtype,snder,sndtim,rcver,rcvtim,txtlen,text) -
				request a message list element, and initialize it
				Parameters - Byte mtype, Name *snder, VTime sndtim,
						Name *rcver, VTime rcvtim, Int txtlen, Byte *text
				Return - a pointer to the message list element, or a NULL ptr, 
						if no space is available

		make_static_msg(message,mtype,snder,sndtim,rcver,rcvtim,txtlen,text) - 
				initialize the provided message list element
				Parameters - Msgh *message, Byte mtype, Name *snder,
						VTime sndtim, Name *rcver, VTime rcvtim, Int txtlen, 
						Byte *text
				Return - a pointer to the message list element

Implementation:

		make_message() calls l_create() to get memory to hold the 
		message.  Failure to get the memory causes a NULL pointer to 
		be returned.  If memory was obtained, call clear() to zero it.
		Copy the provided parameters into the appropriate place in the
		new message space.  Get a unique id for it by calling uniq().
		Return a pointer to the message.

		make_static_msg() is similar, but is provided with the memory
		space by a parameter.  Thus, it does not call l_create(), and
		it will never return a NULL pointer.  (It behooves a user of
		this routine to make very sure that he has the memory he thinks
		he does before calling this routine.)  Otherwise, it is the
		same as make_message().
*/

#include "twcommon.h"
#include "twsys.h"

Msgh *
FUNCTION make_message ( mtype, snder, sndtim, rcver, rcvtim, txtlen, text )

	Byte mtype;
	Name * snder;
	VTime sndtim;
	Name * rcver;
	VTime rcvtim;
	Int txtlen;
	Byte * text;
{
	Msgh * message;

  Debug  

	/* find space, invoking sendback if necessary */
	message =  (Msgh *) m_create ( sizeof ( Msgh ) + txtlen, sndtim, 
								CRITICAL );

	if ( message == NULL )
		return ( NULL );        /* no space found */

	clear ( message, sizeof ( Msgh ) + txtlen );        /* zero it */

	/* fill in the blanks */
	message->mtype = mtype;
	strcpy ( message->snder, snder );
	message->sndtim = sndtim;
	strcpy ( message->rcver, rcver );
	message->rcvtim = rcvtim;

	message->txtlen = txtlen;

	if ( txtlen > 0 )
		/* copy text in area just past message header */
		entcpy ( message + 1, text, txtlen );

	message->gid.node = miparm.me;
	message->gid.num  = uniq ();

	mark_macro ( message );     /* mark message as used */

	return ( message );
}


Msgh *
make_static_msg ( message, mtype, snder, sndtim, rcver, rcvtim, txtlen, text )

	Msgh * message;
	Byte mtype;
	Name * snder;
	VTime sndtim;
	Name * rcver;
	VTime rcvtim;
	Int txtlen;
	Byte * text;
{
	clear ( message, sizeof ( Msgh ) + txtlen );

	message->mtype = mtype;
	strcpy ( message->snder, snder );
	message->sndtim = sndtim;
	strcpy ( message->rcver, rcver );
	message->rcvtim = rcvtim;
	message->txtlen = txtlen;

	if ( txtlen > 0 )
		entcpy ( message + 1, text, txtlen );

	message->gid.node = miparm.me;
	message->gid.num  = uniq ();

	return ( message );
}
