/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	objhead.c,v $
 * Revision 1.10  91/12/27  08:47:35  reiher
 * Added code to support event count throttling
 * 
 * Revision 1.9  91/11/01  13:25:10  reiher
 * Started initial events' EPTs at zero, added timing code
 * (PLR)
 * 
 * Revision 1.8  91/11/01  09:51:46  pls
 * 1.  Change version id.
 * 2.  Add parm to loadstatebuffer() for bug 15.
 * 
 * Revision 1.7  91/07/17  15:11:19  judy
 * New copyright notice.
 * 
 * Revision 1.6  91/06/03  14:24:21  configtw
 * Change #ifdef to #if
 * 
 * Revision 1.5  91/06/03  12:25:50  configtw
 * Tab conversion.
 * 
 * Revision 1.4  91/05/31  14:57:40  pls
 * Change object type to NULL if destroyed.
 * 
 * Revision 1.3  91/04/01  15:44:31  reiher
 * Code to support Tapas Som's work, plus possibly incorrect code to permit
 * multiple create messages to be delivered to an object without error,
 * provided they are all the same type.
 * 
 * Revision 1.2  91/03/26  09:31:02  pls
 * Add Steve's RBC code.
 * 
 * Revision 1.1  90/08/07  15:40:36  configtw
 * Initial revision
 * 
*/
char objhead_id [] = "@(#)objhead.c     $Revision: 1.10 $\t$Date: 91/12/27 08:47:35 $\tTIMEWARP";


/*
Purpose:

		The code in objhead.c is intended to select the next message 
		from a running object's input queue, and perform the proper
		action to handle that message.  That action consists largely
		of arranging to have a state available, and calling the
		appropriate routine to handle the particular message type.
		In the current system, the only types of messages that
		objhead(), the main routine here, needs to handle are
		event messages.

Functions:

		objhead(o) - choose the next message from the current object's
				input queue and handle it
				Parameters - Ocb *o
				Returns - SUCCESS or FAILURE

Implementation:

		Look for an unprocessed message in the input queue, by consulting
		the ocb's ci pointer.  If there is no such message, return FAILURE.
		Otherwise, examine the type of the message.
		Try to load a state for this object.  If the attempt fails,
		return FAILURE.  Set the loaded state's send time to the ocb's
		simulation time, set the state type to NOERR, indicate in the
		ocb that an event is being processed,
		indicate that we are in a NONEDGE control mode, and set
		xqting_ocb to o, indicating that this object is the executing 
		one.  Call setctx() to prepare for a switch of execution to 
		the object, and update a counter keeping track of the number of 
		event starts.
*/

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"

extern FILE * cpulog;

#ifdef MSGTIMER
extern long msgstart, msgend;
extern long msgtime ;
extern long msgcount ;
extern int mlog;
#endif MSGTIMER

dummy_destroy () {}
dummy_init () {}

FUNCTION        objhead (o)
	Ocb            *o;
{
	register Msgh  *m;
	register int i;
	Typtbl * tempType = NULL;

  Debug

#ifdef PARANOID
	if ( gtVTime ( o->svt, o->phase_end ) )
	{
		twerror ( "objhead F object %s phase %f", o->name, o->phase_end.simtime );
		tester ();
	}
#endif

	m = o->ci;
/*
_pprintf("objhead %s msg is 0x%x\n", o->name, m);
_pprintf("prev 0x%x next 0x%x size %d pad %d\n",
		((List_hdr *) m - 1)->prev,
		((List_hdr *) m - 1)->next,
		((List_hdr *) m - 1)->size,
		((List_hdr *) m - 1)->pad );
*/
	if (m->mtype == DYNCRMSG)
	{
	  tempType = ChangeType ( o, m );

	  if ( tempType != (Typtbl *)-1 )
	  {
		  o->typepointer = tempType;
	  }
	  else
	  {
		  o->typepointer = o->cs->otype;
	  }
	}
	else
	if ( m->mtype == DYNDSMSG )
	{
		/*  type_table[1] contains the entry for the NULL type.   Dynamic
				destroy messages always change the object's type to NULL.*/

		o->typepointer = &(type_table[1]);

	}
	else
	{
		o->typepointer = o->cs->otype;
	}

#ifdef RBC
	if ( o->uses_rbc )
	{
		o->pvz_len = 0;
	}
	else
#endif
	o->pvz_len = o->typepointer->statesize;

	o->centry = m->mtype;
	if ( loadstatebuffer(o,tempType) == FAILURE )
	{
		return FAILURE;
	}
	o->sb->stype = o->centry;
	o->sb->sndtim = o->svt;
	o->sb->serror = NOERR;
	o->control = NONEDGE;
	o->sb->stdout_sequence = 0;
	for ( i = 0; i < MAX_TW_STREAMS; i++ )
		o->sb->stream[i].sequence = 0;
	xqting_ocb = o;
#ifdef SOM
/* Ept is calculated for any message type that calls message_vector().
	  The types of messages that don't call that routine are create
	  messages, and they set the Ept separately.  For static messages,
	  it's set to the time of their creation.  For dynamic create messages,
	  it's set to the ept of the create message.
*/
#endif SOM

	switch (m->mtype)
	{

	case CMSG:

		if ( cpulog )
			HOST_fprintf ( cpulog, "O %s\n", o->name );

		o->ecount = 0;
		o->sb->otype = o->typepointer;
#ifdef SOM
	  o->sb->Ept = 0;
#endif SOM
#ifdef RBC
		if ( o->uses_rbc )
		{
			setctx (o->footer, o->typepointer->init, o->stk);
		}
		else
#endif
		setctx (o->sb + 1, o->typepointer->init, o->stk);
		break;

	case EMSG:

		if ( message_vector (o) == FAILURE )
		{
#ifdef RBC
			if ( o->uses_rbc )
				l_destroy ( o->sb );
			else
			/* destroy_state and rollback chip don't mix */
#endif
			destroy_state ( o->sb );

			o->sb = NULL;
			l_destroy ( o->stk );
			o->stk = NULL;
			o->control = EDGE;
#ifdef RBC
			if ( o->uses_rbc && rollback_op ( o, 1, o->svt ) )
			{
				_pprintf ( "objhead weird rbc rollback failure\n" );
				tester();
			}
#endif 
			return FAILURE;
		}

		if ( cpulog )
			HOST_fprintf ( cpulog, "B %s %f %d\n", o->name,
				o->svt.simtime, o->stats.cputime );
#ifdef RBC
		if ( o->uses_rbc )
		{
			setctx (o->footer, o->typepointer->event, o->stk);
		}
		else
#endif
#ifdef MSGTIMER
        	if ( !mlog && m->mtype == EMSG )
			{
				msgstart = m->msgtimef;
			}
#endif MSGTIMER
		setctx (o->sb + 1, o->typepointer->event, o->stk);
		o->stats.numestart++;
		o->eventsPermitted--;
		break;

	case TMSG:

		o->ecount = 0;
		m = nxtigb (m);
		if (m != NULL)
		{
			o->ci = m;
			if ( message_vector (o) == FAILURE )
			{
#ifdef RBC
				if ( o->uses_rbc )
					l_destroy ( o->sb );
				else
				/* destroy_state and rollback chip don't mix */
#endif
				destroy_state ( o->sb );

				o->sb = NULL;
				l_destroy ( o->stk );
				o->stk = NULL;
				o->control = EDGE;
#ifdef RBC
				if ( o->uses_rbc && rollback_op ( o, 1, o->svt ) )
				{
					_pprintf ( "objhead weird rbc rollback failure\n" );
					tester();
				}
#endif 
				return FAILURE;
			}
		}
#ifdef RBC
		if ( o->uses_rbc )
		{
			setctx (o->footer, o->typepointer->term, o->stk);
		}
		else
#endif
		setctx (o->sb + 1, o->typepointer->term, o->stk);
		break;

	case DYNCRMSG:

		if ( cpulog )
			HOST_fprintf ( cpulog, "O %s\n", o->name );

		o->ecount = 0;
		o->sb->otype = o->typepointer;
#ifdef SOM
		o->sb->Ept = m->Ept;
#endif SOM
#ifdef RBC
		if ( o->uses_rbc )
		{
			setctx (o->footer, o->typepointer->init, o->stk);
		}
		else
#endif
		if ( tempType == (Typtbl *)-1 )
		{
			/* In this case, we don't want to rerun the init section, as
				we have a second create message for the object of the same
				kind as the first. */

			setctx (o->sb + 1, dummy_init, o->stk);
		}
		else
		{
			setctx (o->sb + 1, o->typepointer->init, o->stk);
		}
		o->stats.numcreate++;
		break;

	case DYNDSMSG:

		o->ecount = 0;
#ifdef RBC
		if ( o->uses_rbc )
		{
			setctx (o->footer, dummy_destroy, o->stk);
		}
		else
#endif
		setctx(o->sb + 1, dummy_destroy , o->stk);
		o->stats.numdestroy++;
		break;

	default:

		twerror ("objhead E input validity check failure");
		return FAILURE;
	}                           /* end of switch mtype */

	return SUCCESS;
}


FUNCTION message_vector (o)

	Ocb * o;
{
	register Msgh *m;
	register Byte *t;

	Msgh * temp[MAXMSGS];

	register Msgh **v = temp;

	if ( o->msgv != NULL )
	{
		_pprintf ( "message_vector: o->msgv != NULL for %s\n", o->name );
		destroy_message_vector ( o );
	}

	o->ecount = 0;

	for ( m = o->ci; m && m->mtype == o->ci->mtype; m = nxtigb (m) )
	{
		if ( o->ecount == MAXMSGS )
		{
			o->sb->serror = MSGVOFLOW;

			break;
		}

		if ( cpulog )
		{
			if ( o->stats.cputime < m->cputime )
				o->stats.cputime = m->cputime;
		}

		if ( m->txtlen > pktlen )
		{
			twerror ( "message_vector I txtlen > pktlen in objhead" );
			tester();
		}
		else
			t = m_create ( msgdefsize, o->svt, NONCRITICAL );

		if ( t == NULL )
		{
			while ( v != temp )
			{
				v--;

				l_destroy ( (List_hdr *) *v );
			}

			return FAILURE;
		}

		entcpy ( t, m, sizeof(Msgh) + m->txtlen );

		*v++ = (Msgh *) t;

#ifdef SOM

	/* loadstatebuffer() has initialized the new state's Ept to that of the
	  old state, so only the input messages' epts need to be maxed in
	  to find the new Ept. */

	  if ( o->sb->Ept < m->Ept )
	  {
		  o->sb->Ept = m->Ept;
	  }

#endif SOM

		o->ecount++;
	}

	o->msgv = (Msgh **) m_create ( o->ecount * 4, o->svt, NONCRITICAL );

	if ( o->msgv == NULL )
	{
		while ( v != temp )
		{
			v--;

			l_destroy ( (List_hdr *) *v );
		}

		return FAILURE;
	}

	entcpy ( o->msgv, temp, o->ecount * 4 );

	return SUCCESS;
}

destroy_message_vector (o)

	register Ocb * o;
{
	if ( o->msgv != NULL )
	{
		register Msgh ** v = o->msgv;
		register int i;

		for ( i = 0; i < o->ecount; i++ )
		{       /* release memory for each message vector */
			l_destroy ( (List_hdr *) (*v++) );
		}

		l_destroy ( (List_hdr *) o->msgv );     /* release ptr to table */
		o->msgv = NULL;
	}
}

FUNCTION Typtbl * ChangeType(o,m)

	Ocb         *o;
	Msgh        *m;
{
	Crttext *ChangeText;
	Typtbl *NewType;

	ChangeText = (Crttext *) (m + 1);

	NewType = find_object_type (ChangeText->tp);


	if (NewType == NULL)
	{
		NewType = NULL_TYPE;
	}
	else
	{  
	  if ( NewType == o->typepointer )
	  {
		  /*  In this case, we have a second create message for the
			  same object, of the same type.  We want to simply ignore
			  this message.  */

		  return ( (Typtbl *)-1 );
	  }
#if 0
	  else
		_pprintf("ChangeType changing type of %s from %s to %s at %f\n",
			o->name, o->typepointer->type, NewType->type,m->rcvtim.simtime );
#endif
	}

	return (NewType);
}
