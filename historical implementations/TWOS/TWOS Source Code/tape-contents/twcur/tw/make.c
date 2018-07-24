/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	make.c,v $
 * Revision 1.7  91/11/01  09:40:25  reiher
 * added code to initialize the truncated state queue header of an ocb, for
 * critical path computation purposes. (PLR)
 * 
 * Revision 1.6  91/08/08  13:02:22  reiher
 * Removed error msg from mkocb, since it wasn't always an error
 * 
 * Revision 1.5  91/07/17  15:09:49  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/06/03  12:24:51  configtw
 * Tab conversion.
 * 
 * Revision 1.3  91/05/31  13:27:34  pls
 * Clear out emergbuf after use.
 * 
 * Revision 1.2  91/04/01  15:40:19  reiher
 * A bug fix to make sure that TWOS doesn't try to clear a buffer if it 
 * failed to allocate the buffer.
 * 
 * Revision 1.1  90/08/07  15:40:09  configtw
 * Initial revision
 * 
*/
char make_id [] = "@(#)make.c   $Revision\t$Date\tTIMEWARP";


/*

Purpose:

		make.c contains several routines used in acquiring buffers of 
		various sorts, including sysbufs, output buffers, output message
		buffers, input message buffers, and new object control blocks.
		It also contains a routine that gets rid of an object control
		block cleanly.

Functions:

		sysbuf() - get a system message buffer
				Parameters - none
				Return - a pointer to the buffer

		output_buf() - get an output buffer
				Parameters - none
				Return - a pointer to the buffer

		mkomsg(o,rcvtim,rcvr,mtyp,txtlen) - get a buffer
				for an output message, and initialize it
				Parameters - Ocb *o, VTime rcvtim, Char *rcvr, Byte mtyp, 
						int txtlen
				Return - a pointer to the buffer, or a NULL pointer if no 
						memory was available

		mkimsg(txtlen) - get a buffer for an input message
				Parameters -  int txtlen
				Return - a pointer to the buffer, or a NULL pointer if no 
						memory was available

		mkocb() - get memory for an ocb, and initialize it
				Parameters - none
				Return - a pointer to the ocb, or a NULL pointer if no memory 
						was available

		nukocb(o) - release an ocb's memory
				Parameters - Ocb *o
				Return - always returns 0

Implementation:

		Generally speaking, these routines follow a common pattern.
		They make a call for memory, check to see if they got it,
		and do something if they didn't.  If they do get the memory,
		they clear out any pre-existing garbage in it. In some cases, 
		they also initialize some of the fields of the new entity, when 
		they succeed in getting memory for one.  nukocb() is a special
		cases, since it is a destroyer, rather than a maker.

		sysbuf() calls mkqelm() to get enough memory to hold a 
		system message.  If it fails to get the memory from the
		memory allocator, it tries to get hold of the single 
		emergency buffer devoted to system messages.  (System
		messages may cause memory to be freed, so they should be
		delivered even when there is no memory space availble.)
		The routine may have to wait for the machine interface to
		finish with the buffer.  If so, it simply runs in circles
		until the buffer is ready.  Once it has the buffer, it calls
		clear() to zero out all memory in it, so that garbage from
		previous uses of the memory does not corrupt the system
		message.  If the emergency buffer was obtained, that buffer
		is locked, so no one else will grab it until this message has
		gone through.

		output_buf() does very much the same thing as sysbuf().  
		Instead of trying to lay hands on the emergency buffer, though,
		it tries to get the report_buf, instead.

		mkomsg() calls m_create() to get a message-sized piece of memory.
		Failure simply causes a message to be printed, and a return of
		a NULL pointer.  If the memory was obtained, it is cleared with
		clear().  Then, several pieces of information obtained from
		the parameters are copied into the message.  A pointer to the
		message is returned.

		mkimsg() asks for an input message space of some particular size.
		For some reason, it can ask for a space that is too big, in
		which case the routine fails, returning a NULL pointer.  If the
		size is OK, and the space can be allocated, it is cleared with
		clear(), and a pointer to it is returned.

		mkocb() calls mkqlm() to get its memory for a new ocb.  clear()
		then clears it (before checking to see if the attempt was a 
		success; presumably, clear() won't do any harm if given a null
		address).  Then, several checks are made.  First, mkocb()
		checks to see if the memory was obtained.  If it was, it tries
		to get headers for the state queue, input queue, and output
		queue.  If any of those attempts fails, a twerror is printed,
		and NULL returned.  As they all succeed, their pointers are
		copied into the ocb.  (All four tests are in a single if
		statement, which is why the memory is cleared before checking
		to see if we have it: if it was cleared after, it would also
		destroy the pointers to these headers.)  If all went well,
		mkocb() returns a pointer to the new ocb.

		nukocb() destroys an ocb, by first deallocating its list
		headers for the state, input, and output queues.  (This code
		assumes that all other elements of the list are already 
		removed from the list, for, otherwise, l_hdestroy() fails,
		and the code here doesn't bother to check if it worked.  If
		it fails, the later attempt to destroy the ocb itself will
		leave portions of memory permanently unusable.)  Then call
		l_destroy() to get rid of the ocb itself.  

*/

#include "twcommon.h"
#include "twsys.h"

FUNCTION Msgh *sysbuf ()
{
	register Msgh  *m;
	register int   count = 0;

  Debug

	/* first try to allocate space */
	m = (Msgh *) l_create (msgdefsize);

	if (m == NULLMSGH)
	{  /* no space--get desperate */
		while (BITTST (emergbuf->flags, LOCKED))
		{
			if (count++ == 10000)
			{                   /* Wait for the MI to free the buffer..  */
				_pprintf ("Waiting for emergbuf to be unlocked... %x\n",
						emergbuf);
				send_e_from_q();        /* clear emergbuf */
				count = 0;
			}
		}
		m = emergbuf;   /* grab emergbuf */
	}

	clear ( m, sizeof (Msgh) ); /* clear it */

	if ( m == emergbuf )
		BITSET (m->flags, LOCKED); /* Tell rest of system not to use */

	return m;
}


FUNCTION Msgh *output_buf ()
{
	register Msgh  *m;
	register int    count = 0;

  Debug

	/* first try to allocate space */
	m = (Msgh *) l_create (msgdefsize);

	if (m == NULLMSGH)
	{  /* no space--get desperate */
		while (BITTST (report_buf->flags, LOCKED))
		{
			if (count++ == 10000)
			{                   /* Wait for the MI to free the buffer..  */
				dprintf ("Waiting for report buffer to be unlocked...\n");
				count = 0;
			}
		}
		m = report_buf; /* grab report_buf */
	}

	clear ( m, sizeof (Msgh) ); /* clear it */

	if ( m == report_buf )
		BITSET (m->flags, LOCKED); /* Tell rest of system not to use */

	return m;
}

FUNCTION Msgh  *mkomsg ( o, rcvtim, rcvr, mtyp, txtlen, selector)

	Ocb            *o;
	VTime           rcvtim;
	Char           *rcvr;
	Byte            mtyp;
	int             txtlen;
	long            selector;
{
	register Msgh *buf;

  Debug

	buf = (Msgh *) m_create ( msgdefsize, o->svt, NONCRITICAL );

	if (buf == NULL)
	{
		/* dprintf("mkomsg: no memory to make output message\n"); */
		return buf;
	}

	/* else, rest of routine */

	clear ( buf, sizeof (Msgh) );

	buf->sndtim = o->svt;
	strcpy (buf->snder, o->name);

	buf->rcvtim = rcvtim;
	strcpy (buf->rcver, rcvr);

	buf->mtype = mtyp;
	buf->stype = o->centry;
	buf->txtlen = txtlen;
	buf->selector = selector;

	buf->flags = NORMAL;

	return buf;
}

FUNCTION Ocb *mkocb ()
{
	register Ocb *buf;

  Debug

	buf = (Ocb *) l_create (sizeof (Ocb));      /* allocate object memory */

	if ( buf != NULL )
		clear ( buf, sizeof (Ocb) );

	/* this IF makes use of C's handling of OR-clauses; won't evaluate */
	/* next one unless prev one is true */

	if (buf == NULL ||  /* set up state, input & output queues */
		(buf->sqh = (State *) l_hcreate() ) == NULL ||
		(buf->tsqh =  (Msgh *) l_hcreate() ) == NULL ||
		(buf->iqh =  (Msgh *) l_hcreate() ) == NULL ||
		(buf->oqh =  (Msgh *) l_hcreate() ) == NULL
		)
	{
		return NULL;
	}

	return buf;
}

FUNCTION nukocb (o)

	register Ocb *o;
{
  Debug

	l_hdestroy ( (List_hdr *) (o->sqh));  /* release the (empty) headers */
	l_hdestroy ( (List_hdr *) (o->iqh));
	l_hdestroy ( (List_hdr *) (o->oqh));

	l_destroy ( (List_hdr *) o);        /* get rid of this object */
}
