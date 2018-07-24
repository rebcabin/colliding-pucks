/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	int.c,v $
 * Revision 1.5  91/12/27  09:05:55  pls
 * Fix up TIMING code.
 * 
 * Revision 1.4  91/11/01  09:24:41  pls
 * 1.  Change version id.
 * 2.  Add signal handling (SCR 164).
 * 
 * Revision 1.3  91/07/17  15:08:50  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:24:24  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:38:34  configtw
 * Initial revision
 * 
*/
char int_id [] = "@(#)int.c     $Revision: 1.5 $\t$Date: 91/12/27 09:05:55 $\tTIMEWARP";

/*      Copyright (C) 1989, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*

Purpose:

		int.c contains a set of functions that put a site's currently
		executing object into a blocked state, to handle an error
		condition.  Such error conditions include division by zero,
		overflow, stack overflow, addressing exceptions, and similar
		errors.  Once the object has been blocked, they call dispatch()
		to choose a new object for execution.

Functions:

		sigHandler() - initial routine triggered by signal
				Parameters -	int  sig,code
								struct sigcontext *scp
								char *addr
				Return - void
				
		objdiv0() - set an error condition for division by zero
				Parameters - none
				Return - Always returns 0

		objoflow() - set an error condition for arithmetic overflow
				Parameters - none
				Return - Always returns 0

		objsub() - set an error condition for subscript out of range
				Parameters - none
				Return - Always returns 0

		objadr() - set an error condition for addressing fault
				Parameters - none
				Return - Always returns 0

		objstk() - set an error condition for stack overflow
				Parameters - none
				Return - Always returns 0

		objins() - set an error condition for illegal instruction
				Parameters - none
				Return - Always returns 0

		objtout() - set an error condition for timeout
				Parameters - none
				Return - Always returns 0

		twmfu() - set an error condition for mfu (??)
				Parameters - none
				Return - Always returns 0

Implementation:

		All of these routines are identical, except for the error
		condition they set, so only one will be discussed.  objdiv0()
		sets the serror field of the state in the currently executing
		object's state buffer to OBJDIV0.  It sets the executing object's
		run status to ARLBK, so that it will not execute.  Then it calls
		dispatch(), to select a new object for execution.

*/

#include <signal.h>

#include "twcommon.h"
#include "twsys.h"
#include "tester.h"

extern Byte * object_context;

extern int      hlog;
extern VTime    hlogVTime;
extern int      maxSlice;
extern Ocb*     maxSliceObj;
extern VTime    maxSliceTime;
extern int      sliceTime;

FUNCTION		sigHandler_b(sig,code,scp,addr)

	int					sig,code;
	struct sigcontext	*scp;
	char				*addr;
	
{
	switch (sig)
		{
		case SIGBUS:
		case SIGEMT:
		case SIGILL:
		case SIGSEGV:
			check_stack_and_state(xqting_ocb);
			objins();	/* illegal instruction */
			break;
		case SIGFPE:
#if BFLY1
			if (code == FPE_INTDIV0)
				objdiv0();	/* divide by 0 */
			else
#endif
				objoflow();	/* numeric overflow */
			break;
		default:
			_pprintf ("Unknown signal: %d, code: %d @%x\n",sig,code,addr);
			tester();
			break;
		}	/* switch (sig) */
}	/* sigHandler_b */

FUNCTION		sigHandler(sig,code,scp,addr)

	int					sig,code;
	struct sigcontext	*scp;
	char				*addr;
	
{
	if (objectCode)
		{	/* interrupted while executing object code */
		objectCode = FALSE;
#if TIMING
		stop_timing();
#endif
		 
#if MARK3
		mark3time ();
#endif
#if BF_MACH
		butterflytime ();
#endif
		object_end_time = node_cputime;

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
	
		switch_back ( sigHandler_b, object_context, sig, code, scp, addr );
		
		}
	else
		{
		twerror("sigHandler F Fatal error in Time Warp: %d, %d",sig,code);
		tester();
		}
}	/* sigHandler */

FUNCTION        objdiv0 ()
{
	object_error ( OBJDIV0 );
	dispatch ();
}


FUNCTION        objoflow ()
{
	object_error ( OBJOFLOW );
	dispatch ();
}


FUNCTION        objsub ()
{
	object_error ( OBJSUB );
	dispatch ();
}


FUNCTION        objadr ()
{
	object_error ( OBJADR );
	dispatch ();
}


FUNCTION        objstk ()
{
	object_error ( OBJSTK );
	dispatch ();
}


FUNCTION        objins ()
{
	object_error ( OBJINS );
	dispatch ();
}


FUNCTION        objtout ()
{
	object_error ( OBJTOUT );
	dispatch ();
}


FUNCTION        twmfu ()
{
	object_error ( TWMFU );
	dispatch ();
}


FUNCTION object_error ( error )

	char * error;
{
	xqting_ocb->sb->serror = error;

	objend ();
}
