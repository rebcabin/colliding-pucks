/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	nq.c,v $
 * Revision 1.9  91/12/27  09:09:48  pls
 * Fix up TIMING code.
 * 
 * Revision 1.8  91/11/01  09:48:32  reiher
 * timing code and bug fixes (PLR)
 * 
 * Revision 1.7  91/07/17  15:11:01  judy
 * New copyright notice.
 * 
 * Revision 1.6  91/06/03  12:25:43  configtw
 * Tab conversion.
 * 
 * Revision 1.5  91/05/31  14:53:36  pls
 * Handle POS_REVERSE_ENQUEUEING, NEG_REVERSE_ANNIHILATING, and
 * NEG_REVERSE_ENQUEUEING cases.
 * 
 * Revision 1.4  91/04/01  15:43:44  reiher
 * A test for an unexpected error condition.
 * 
 * Revision 1.3  91/03/26  09:27:39  pls
 * Combine handling of NEGATIVE_AND_ENQUEUING & POSITIVE_AND_ENQUEUING
 * cases.
 * 
 * Revision 1.2  90/11/27  09:47:19  csupport
 * add error message if msg recv time too early for phase
 * 
 * Revision 1.1  90/08/07  15:40:30  configtw
 * Initial revision
 * 
*/
char nq_id [] = "@(#)nq.c       1.67\t10/2/89\t15:05:42\tTIMEWARP";


/*

Purpose:

		This module contains routines that do the working of enqueueing
		input and output messages.  Enqueued messages must be put in
		the proper place.  In addition, adding a message to a queue
		may cause annihilation or rollback, so these cases must be
		tested for, and properly handled.  Messages can arrive either
		in forward or reverse directions, adding further complications.
		(Some possible combinations of queues and directions are errors.) 
		The two main routines in this module are nq_input_message()
		and nq_output_message(), dealing with the input and output 
		queues, respectively.

Functions:

		nq_input_message(o,m) - put a message in an object's input queue
				Parameters - Ocb *o, Msgh *m
				Return - Always returns 0

		nq_output_message(o,m) - put a message in an object's output queue
				Parameters - Ocb *o, Msgh *m
				Return -  Always returns 0

		accept_or_destroy(m) - decide if a potential annihilation should be
				done
				Parameters - Msgh *m
				Return -  Always returns 0

		get_memory_or_denymsg ( m ) - try to grab space for a message to be
				queued
				Parameters - Msgh *m
				Return - OK_TO_ENQUEUE or NO_ROOM_FOR_MSG

Implementation:

		nq_input_message() calls find() to determine where to put
		the message in the queue.  find() will position in the input
		queue where the message belongs.  (imcmp() is the comparison
		function provided to find().)  If find() returns NULL, there's
		bad trouble, so print an error message.  Set "time" to the
		message's rcvtim.  Now check whether the message is positive or
		negative.  Also check the contents of cmpval, which was set
		by find(), to determine if this is an annihilating message or
		not.  Based on these comparisons, set a switch.

		The value of the switch just set determines what to do next.
		If it's POSITIVE_AND_ANNIHILATING, and if we're being careful,
		check to make sure that the matching message found in the queue
		is an anti-message.  Then increment the nmber of annihilations,
		remove the matching message from the queue, call accept_or_destroy()
		to get rid of the incoming message, and call rollback() to see
		if rolling back is necessary.

		If the switch is POSITIVE_AND_ENQUEUEING, call get_memory_or_denymsg()
		to find out if we have room for it.  If so, try to enqueue it,
		taking typical error-steps if we fail.

		If it was NEGATIVE_AND_ANNIHILATING, make sure that the ocb's
		ci pointer doesn't point to the queued message about to be 
		annihilated, nulling ci if it does.  (We didn't do this in
		POSITIVE_AND_ANNIHILATING because ci will never point to a
		negative message.)  Otherwise, behave just as if the message
		was POSTITIVE_AND_ANNIHILATING.

		If it was NEGATIVE_AND_ENQUEUEING, use get_memory_or_denymsg()
		to allocate space for it, then try to put it into the queue.

		nq_output_message() calls find() to find a place for the
		new output message in the output queue.  (omcmp() is used
		as the comparison function for find().)  Check whether
		the new output message is an anti-message, and whether it
		has been sent forward or backward.  Also, it may prove 
		necessary to check whether or not the message can annihilate
		an existing message in the output queue.  Set a variable 
		based on these tests, and switch on that variable.

		NEG_FORWARD_ANNIHILATING is an illegal case, so just print
		an error message.  For NEG_FORWARD_ENQUEUING (the most
		common case), try to insert it into its proper place in the
		output queue, printing an error message on failure.
		Any POSITIVE_FORWARD message is illegal, so it also gets
		an error message. 

		A POS_REVERSE_ANNIHILATING is sent when the receiver is trying 
		to free up space, and will cause this object to roll back, 
		potentially.  First, check to see if the ocb's oc pointer is 
		placed at the queued message about to be annihilated.  If it is, 
		try to set it to something reasonable, so that it will point to 
		some other message in the output queue with the same sendtime.  
		If no such message exists, null the co pointer.  Call delomsg()
		to get rid of the queued message, accept_or_destroy() to get
		rid of the incoming message, and call rollback().

		NEGATIVE_REVERSE and POSITIVE_REVERSE_RESENDING (the only
		other positive reverse case, besides annihilating) are
		treated the same way.  Call get_memory_or_denymsg().
		If it succeeds in getting some memory, we will try to send out
		this message again.  Turn it into a forward message, call a
		stat routine to keep track of the send, try to find its
		destination in the world map, and call deliver() to ship it
		out.  Sooner or later, the receiver will be able to handle
		this message, and won't ship it back again. 

		accept_or_destroy() is responsible for annihilating incoming
		messages.  If the message is locked, then the machine interface
		is told that the message has been freed, and the buffer can
		be reused.  Otherwise, simple call l_destroy() on the message,
		returning its storage to the system.

		get_memory_or_denymsg() tries to get space to hold a message
		to be enqueued.  The message might have come from off-node,
		or from on-node, and different procedures are needed for each.
		If it came from off-node, it's currently sitting in a special
		buffer for incoming messages, and that buffer cannot simply
		be linked into the appropriate message queue.  Instead,
		try to get a new buffer to serve for incoming messages by
		allocating some memory for that purpose.  If the allocation
		succeeds, call acceptmsg() to tell the machine interface the
		location of its new buffer, then return OK_TO_ENQUEUE.  If the
		allocation fails, there is no free memory, so call denymsg()
		to return the message to the sender, and return NO_ROOM_FOR_MSG
		to the calling routine.  If the message came from on-node, 
		there is nothing special about its buffer, so simply return 
		OK_TO_ENQUEUE.

*/

#include "twcommon.h"
#include "twsys.h"

#define POSITIVE_AND_ANNIHILATING       73
#define POSITIVE_AND_ENQUEUING          74
#define NEGATIVE_AND_ANNIHILATING       85
#define NEGATIVE_AND_ENQUEUING          86

#define NEG_FORWARD_ANNIHILATING        37
#define NEG_FORWARD_ENQUEUING           38
#define POSITIVE_FORWARD                40
#define POS_REVERSE_ANNIHILATING        41
#define POS_REVERSE_RESENDING           42
#define POS_REVERSE_ENQUEUEING          43
#define NEG_REVERSE_ANNIHILATING        44
#define NEG_REVERSE_ENQUEUEING          45

#ifdef TIMING
#define QUEUE_TIMING_MODE 6
#define OQ_TIMING_MODE 12
#define IQFIND_TIMING_MODE 13
#define IQACCEPT_TIMING_MODE 14
#endif

#ifdef MSGTIMER
extern long msgstart, msgend;
extern long msgtime ;
extern long msgcount ;
#endif MSGTIMER
FUNCTION nq_input_message ( o, m )

	Ocb            *o;
	Msgh           *m;
{
	Msgh           *f;
	Long             cmpval;
	int         which_case;
	int moving;

	extern int mlog;
	extern int qlog, find_count;

	moving = m->flags & MOVING;

	if ( ltVTime ( m->rcvtim, o->phase_begin ) )
	{   /* check for receive time too early */
		twerror ( "nq_input F object %s phase %f",
				o->name, o->phase_begin.simtime );
		_pprintf("message info: ptr %x  to %s   at %f\n",
						m, m->rcver, m->rcvtim.simtime );
		tester ();
	}

	if ( mlog )
		msglog_entry ( m );     /* log this entry */

  Debug

#ifdef TIMING
	start_timing ( QUEUE_TIMING_MODE );
#endif

#ifdef TIMING
	start_timing ( IQFIND_TIMING_MODE );
#endif

#if 0
	if (o->stats.iqmax > 2000)
		{
		_pprintf("nq_input: o = %s m = %x\n",o->name,m);
		tester();
		}
#endif

	f = (Msgh *) find ((Byte *) o->iqh, (Byte *) lstimsg_macro (o), 
						(Byte *) m, imcmp, &cmpval);
	/*         this q  starting place     target */

#ifdef TIMING
	stop_timing ();
#endif

	if (f == NULL) twerror ("nqimsg F iq not found");

	if (isposi_macro (m))
	{   /* positive message */
		if (cmpval == 0)
		{                       /* annihilation possible */
				which_case = POSITIVE_AND_ANNIHILATING;
		}                       /* end cmpval == 0 */
		else
		{                       /* annihilation NOT possible */
				which_case = POSITIVE_AND_ENQUEUING;
		}                       /* end NON annihilation */
	}                           /* end isposi_macro */
	else
	{                           /* m is anti */
		if (cmpval == 0)
		{                       /* annihilation possible */
				which_case = NEGATIVE_AND_ANNIHILATING;
		}                       /* end annhilation */
		else
		{                       /* annihilation NOT possible */
				which_case = NEGATIVE_AND_ENQUEUING;
		}
	}

	switch ( which_case )
	{
		case POSITIVE_AND_ANNIHILATING:

			if (!isanti_macro (f))
			{   /* identical positive message found */
				twerror ("nqimsg F duplicate POSImsg");
				showmsg ( m );
				showmsg ( f );
				tester ();
				accept_or_destroy ( m );
			}
			else
			{   /* f & m cancel each other */
				annihilate ( o, f, m );
			}
			break;


		case NEGATIVE_AND_ENQUEUING:
/*
			if ( moving )
			{
				_pprintf("moving negative msg enqueued, %x, obj %s\n",
						m, o->name );
				tester();
			}
*/
		case POSITIVE_AND_ENQUEUING:


#ifdef TIMING
			start_timing ( IQACCEPT_TIMING_MODE );
			m = get_memory_or_denymsg ( o, m );
			stop_timing();
			if (m)
#else
			if ( m = get_memory_or_denymsg ( o, m ) )
#endif
			{                           /* here if m is now local */
				l_insert ( f, m);       /* insert m after f */
#ifdef MSGTIMER
				/* This version of MSGTIMER will time from msg send to when
					it is enqueued. */

				if ( !mlog && m->mtype == EMSG )
				{
					msgend = clock();
					msgtime += msgend - m->msgtimef;
					msgcount++;
				}
#endif MSGTIMER

				if ( moving )
				{
					BITCLR ( m->flags, MOVING );        /* a moving object */
					m->flags |= MOVED ;
				}
				else
					rollback ( o, m->rcvtim );  /* rollback if necessary */
			}
			break;


		case NEGATIVE_AND_ANNIHILATING:

			annihilate ( o, f, m );     /* f & m cancel */
			break;


		default:
			twerror ("nqimsg F default");
			break;
	}

	if ( moving && m != NULL )
	if ( o->runstat != ITS_STDOUT )
	{   /* m was part of an object movement */
		o->num_imsgs--;

		if ( ( o->num_states + o->num_imsgs + o->num_omsgs ) == 0 )
		{  /* if phase transfer is complete, restart it if necessary */
			rollback_phase ( o, m );
		}

	  if (moving && m == NULL)
	  {
		_pprintf("nq_input_msg: NULL moving msg\n");
	  }

	}

#ifdef TIMING
	stop_timing ();
#endif
}


FUNCTION annihilate ( o, f, m )

	register Ocb * o;
	register Msgh * f, * m;
{
	VTime time;
	extern int aggressive;

  Debug

	time = f->rcvtim;

	stats_zaps (o, m);          /* increment annihilations */

	accept_or_destroy ( m );

	if ( o->ci == f )
		o->ci = NULL;

	delimsg ( f );              /* dequeue and destroy f */

	rollback ( o, time );       /* rollback 'o' to 'time' */
}

/*  NQ_OUTPUT_MESSAGE

	Enqueue an output msg and do annihilation and rollback for reverse
	(flow control) messages.
	Return SUCCESS if the input message is annihilated (see comment about
	memory exhaustion emergencies in nqimsg), failure otherwise.
*/

FUNCTION nq_output_message ( o, m )

	Ocb            *o;
	Msgh           *m;
{
	Msgh           *f;
	Long             cmpval;
	VTime           time;
	int         which_case;
	extern int mlog;

	int moving = m->flags & MOVING;

	if ( mlog )
		msglog_entry ( m );

  Debug

#ifdef TIMING
	start_timing ( OQ_TIMING_MODE );
#endif

	time = m->sndtim;   /* time assignment must be up here */

		/* find where to put this message */
	f = (Msgh *) find ( o->oqh, o->co, m, omcmp, &cmpval );

#   define CAN_ANNIHILATE (cmpval==0)   /* for readability */

	/* The following indented logic is meant to resemble the comment */
	/* at the head of this routine */

	if ( isanti_macro ( m ) )
	{
		if ( isforward_macro ( m ) )
		{
			if ( CAN_ANNIHILATE )
			{
				which_case = NEG_FORWARD_ANNIHILATING;
			}
			else
			{
				which_case = NEG_FORWARD_ENQUEUING;
			}
		}
		else
		{
			if ( CAN_ANNIHILATE )
			{   
				which_case = NEG_REVERSE_ANNIHILATING;
			}
			else
			{
				which_case = NEG_REVERSE_ENQUEUEING;
			}
		}
	}
	else        /* isposi_macro ( m ) */
	{
		if ( isforward_macro ( m ) )
		{
			which_case = POSITIVE_FORWARD;
		}
		else    /* isreverse ( m ) */
		{
			if ( CAN_ANNIHILATE )
			{
				which_case = POS_REVERSE_ANNIHILATING;
			}
			else        /* CAN'T_ANNIHILATE */
			{
				which_case = POS_REVERSE_RESENDING;

				/* Strange case - the object is being migrated in,
					and has not yet gotten to moving the sendtime of this
					message.  Presumably, the negative copy of the message
					will eventually be migrated and will annihilate this
					positive copy, if we only keep the positive copy in the
					output queue.  Problems may arise if the negative copy
					is somewhere in transit to the receiver, attempting to
					cancel this message.  This kind of problem will be fixed
					when commit time arrives and commitment notices the
					positive committed msg in its output queue. */
 
				if ( ltVTime ( o->phase_limit, o->phase_end )  &&
						geVTime ( m->sndtim, o->phase_limit ) )
				{
					which_case = POS_REVERSE_ENQUEUEING;
				}
			}
		}
	}

	switch ( which_case )
	{
		case NEG_FORWARD_ANNIHILATING:

			twerror("nqomsg E causality violated, universe will self-destruct");

				break;

		case NEG_FORWARD_ENQUEUING:

			if ( m = get_memory_or_denymsg ( o, m ) )
			{
				BITCLR ( m->flags, MOVING );
				m->flags |= MOVED ;
				l_insert ( f,  m );
			}
			break;

		case POSITIVE_FORWARD:

			twerror ("nqomsg E illegal positive forward msg");
			break;

		case POS_REVERSE_ANNIHILATING:

			accept_or_destroy ( m );

			if ( o->co == f )
				o->co = NULL;

			delomsg ( f );

			rollback ( o, time );

			break;


		case NEG_REVERSE_ANNIHILATING:
			
			/* This case should only arise if a positive msg was earlier
				sent back to this object while it was migrating.  If the
				corresponding negative msg hadn't been migrated yet, then
				the sent back positive copy was enqueued.  When the negative
				copy arrives, it will annihilate the positive copy.  If the
				annihilating negative msg is not moving, then something is
				wrong and we should examine it.  If things are as expected,
				then the case is essentially the same as a positive reverse
				annihilation. */

			if ( !moving )
			{
				twerror("nq_output_message: neg reverse annihil msg %x for %s that is not moving\n", m, o->name );
				tester();
			}

			accept_or_destroy ( m );

			if ( o->co == f )
				o->co = NULL;

			delomsg ( f );

			rollback ( o, time );

			break;

		case NEG_REVERSE_ENQUEUEING:

			if ( m = get_memory_or_denymsg ( o, m ) )
			{
				set_forward_macro ( m );

				if ( moving )
				{
					BITCLR ( m->flags, MOVING );
					m->flags |= MOVED ;
					l_insert ( f, m );
				}
				else
				{
					mark_macro ( m );

					smsg_stat ( o, m );

					deliver ( m );
				}
			}
			break;

		case POS_REVERSE_RESENDING:

			if ( m = get_memory_or_denymsg ( o, m ) )
			{
				set_forward_macro ( m );

				mark_macro ( m );

				smsg_stat ( o, m );

				deliver ( m );
			}
			break;

		case POS_REVERSE_ENQUEUEING:

			if ( m = get_memory_or_denymsg ( o, m ) )
			{
				l_insert ( f,  m );
			}
			break;

		default:
			break;

	}

	if ( moving && m != NULL )
	{  
		o->num_omsgs--;

		if ( ( o->num_states + o->num_imsgs + o->num_omsgs ) == 0 )
		{
			rollback_phase ( o, m );
		}
	}

#ifdef TIMING
	stop_timing ();
#endif
}


/* accept or destroy message is called when the message can
annihilate with another message which is already enqueued.
*/

FUNCTION accept_or_destroy ( m )

	Msgh * m;
{
  Debug

	if ( islocked_macro ( m ) ) /* this msg from mproc */
	{
		acceptmsg ( NULL ); /* tell MI it's OK to re-use m's space */
	}
	else
	{
		l_destroy ( m );    /* this msg from this node */
	}
}

/* get_memory_or_denymsg is called for msgs which don't
annihilate and hence need memory space so they can be enqueued
*/

FUNCTION Msgh * get_memory_or_denymsg ( o, m )

	Ocb * o;
	Msgh * m;
{
	Msgh * new;

  Debug

	if ( islocked_macro ( m ) ) /* this msg is from mproc */
	{
		/* create space for copy of this message */
		new = (Msgh *) m_create ( msgdefsize, m->rcvtim, 
									    NONCRITICAL );
		if ( new != NULL)
		{
			/* yes we can save this message & unlock it */
			acceptmsg ( new );  /* make the copy & release m's space */
			return ( new );
		}
		else
		{
			/* no memory for this message */
			if ( ! ( m->flags & MOVING ) )
			{   /* it's not an object migration, so change direction */
				if ( isreverse_macro ( m ) )
				{
					set_forward_macro ( m );
					mark_macro ( m );
				}
				else
					set_reverse_macro ( m );
				smsg_stat ( o, m );
			}
			denymsg ( m );      /* send message back */
			return ( NULL );
		}
	}
	/* message was from this node */
	return ( m );
}
