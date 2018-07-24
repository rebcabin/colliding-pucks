/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	start.c,v $
 * Revision 1.6  91/11/01  12:38:51  pls
 * 1.  Change ifdef's and version id.
 * 2.  Add signal handling (SCR 164).
 * 
 * Revision 1.5  91/07/17  15:12:32  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/07/09  14:41:57  steve
 * changed 128 to IH_NODE
 * 
 * Revision 1.3  91/06/03  12:26:45  configtw
 * Tab conversion.
 * 
 * Revision 1.2  91/04/01  15:46:28  reiher
 * Code to set up the dead ocb list.
 * 
 * Revision 1.1  90/08/07  15:41:03  configtw
 * Initial revision
 * 
*/
char start_id [] = "@(#)start.c $Revision: 1.6 $\t$Date: 91/11/01 12:38:51 $\tTIMEWARP";


/*

Purpose:

		start.c contains initialization code for the system.  In addition,
		it contains a couple of utilities related to output and error
		messages.

Functions:

		tw_startup() - initialize Time Warp
				Parameters - none
				Return - Always returns zero

		install_output_object_types() - put output objects in the type table
				Parameters - none
				Return - Always returns zero

		send_to_IH(ptr,len,mtype) - route a message off the cube
				Parameters -  Byte *ptr, Int len, Int mtype
				Return - Always returns zero

		twerror(s,a1,a2,a3,a4,a5) - print a Time Warp error message
				Parameters -  Byte *s, Byte *a1, Byte *a2, Byte *a3,
						Byte *a4, Byte *a5
				Return - Always returns zero

Implementation:

		tw_startup() is composed mostly of assignment statements and
		calls to initialization functions.

		install_output_object_types() tries to put an entry for stdout 
		into the type table.  Commented out code also puts an unstdout
		type into that table.  

		send_to_IH() takes provided message text (pointed to by ptr),
		and creates and formats a message to be sent to the IH, for
		purpose of output from the cube.  It performs the typical
		operations of code that sends a message, calling output_buf()
		to get memory space for the message, initializing various fields 
		in the message space, and copying the text into the buffer.
		It has some minor fiddling to get proper strings into the snder
		and rcver fields, but nothing very complicated.  Finally, it
		calls sndmsg() to ship it off.

		twerror() is a error interface for Time Warp errors.  Error 
		messages sent here are reformatted, then printed out.

*/

#define DATAMASTER 1		/* needed for externs -- see globals.h */

#include <signal.h>

#include "twcommon.h"
#include "twsys.h"

void	initInterrupts();
int		sigHandler();

FUNCTION        tw_startup ()
{
	gvt = min_vt = neginf;      /* init to starting time */

	gvtcfg ();                  /* set up gvt graph */

	/* Make the procesor global data structures */

	_prqhd = (Byte *) l_hcreate ();             /* processor ready queue */

	DeadOcbList =  (struct deadOcb *) l_hcreate ();

	emergbuf = (Msgh *) l_create ( msgdefsize );

	report_buf = (Msgh *) l_create ( msgdefsize );

	install_output_object_types ();     /* set up stdout type */

	create_stdout ();   /* create & init the stdout object */

	initInterrupts();	/* set up error interrupt vectors */
}


FUNCTION        install_output_object_types ()
{
	Typtbl      *t;

	for (t = type_table; t->type; t++)
		/* skip ahead to first unused record */
	{
		;
	}

	t->type = "stdout";
}

FUNCTION void	initInterrupts()
{
	objectCode = FALSE;				/* not executing object code now */
	signal ( SIGBUS, sigHandler);	/* point to bus error handler */
	signal ( SIGEMT, sigHandler);	/* point to illegal inst. handler */
	signal ( SIGFPE, sigHandler);	/* point to floating pt. error handler */
	signal ( SIGILL, sigHandler);	/* point to illegal inst. handler */
	signal ( SIGSEGV, sigHandler);	/* point to segment violation handler */

}	/* initInterrupts */

FUNCTION send_to_IH (ptr, len, mtype)

	Byte           *ptr;
	Int             len,
					mtype;
{
	Msgh           *mptr;
	char           *cp;

	if (len > MINPKTL)
	{
		dprintf (" send_to_IH : Receive message over MAX size, truncating.\n");
		len = MINPKTL;
	}

	mptr = output_buf ();       /* grab some message space */

	/* mptr->flags |= SYSMSG; */
	mptr->mtype = mtype;
	mptr->txtlen = len;                         /* sndmsg also does this */
	strcpy (mptr->snder, "Node ");              /* going out from the node */
	cp = (char *) ((char *) mptr->snder + strlen ("Node "));
	cp = itoa (miparm.me, cp);                  /* store node number */
	strcpy (mptr->rcver, "$IH");
	mptr->sndtim = gvt;
	mptr->rcvtim = gvt;
	mptr->gid.num = (unsigned long) uniq ();    /* unique # for this node */
	mptr->gid.node = miparm.me;                 /* this node */
	entcpy (mptr + 1, ptr, len);                /* copy the contents */

	/* now put it in the queue and send it out to node IH_NODE */
	sndmsg (mptr, mptr->txtlen + sizeof (Msgh), IH_NODE);
}


/****************************************************************************
********************************   TWERROR   ********************************
*****************************************************************************/

FUNCTION twerror ( s )

	char *s;
{
	char **a = &s;
	char err_str[MINPKTL];

	strcpy  ( err_str, "TWERROR : " );
	sprintf ( err_str + strlen ( err_str ), s,
		a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10] );
	strcat  ( err_str, "\n" );
	_pprintf ( err_str );

  Debug 
}
