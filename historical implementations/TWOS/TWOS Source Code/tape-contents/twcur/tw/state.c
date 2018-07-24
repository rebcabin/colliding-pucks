/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	state.c,v $
 * Revision 1.15  91/12/27  09:57:34  reiher
 * Added code to support event count throttling
 * 
 * Revision 1.14  91/12/27  09:15:54  pls
 * 1.  Fix up TIMING code.
 * 2.  Combine newBlockWithPtrs_b and newBlockPtr_b.
 * 3.  Add support for variable length address table (SCR 214).
 * 
 * Revision 1.13  91/11/01  13:47:14  reiher
 * Added timing code and critical path code (PLR)
 * 
 * Revision 1.12  91/11/01  12:50:39  pls
 * 1.  Change ifdef's and version id.
 * 2.  Make sflag a bit flag.
 * 3.  Put in changes for signal support (SCR 164).
 * 4.  loadstatebuffer() uses new parm for bug 15.
 * 
 * Revision 1.11  91/07/17  15:12:38  judy
 * New copyright notice.
 * 
 * Revision 1.10  91/07/09  14:43:48  steve
 * Added support for MicroTime and object_timing_mode
 * 
 * Revision 1.9  91/06/03  12:26:47  configtw
 * Tab conversion.
 * 
 * Revision 1.8  91/06/03  09:50:12  pls
 * Use message buffer pool for state error message space.
 * 
 * Revision 1.7  91/05/31  15:30:51  pls
 * 1.  Copy state error messages into TW space.
 * 2.  Check for bad block size request.
 * 3.  Add code to handle state copying for object migration.
 * 
 * Revision 1.6  91/04/01  15:46:44  reiher
 * Code to support Tapas Som's work, and a routine for state comparison to
 * be used with the limited jump forward optimization.
 * 
 * Revision 1.5  91/03/26  09:47:23  pls
 * 1.  Add Steve's RBC code.
 * 2.  Modify hoglog code to refer to hlog.
 * 
 * Revision 1.4  91/03/25  16:39:36  csupport
 * Implement hoglog.
 *
 * Revision 1.3  90/08/27  10:44:53  configtw
 * Split cycle time from committed time.
 * 
 * Revision 1.2  90/08/09  16:17:09  steve
 * set o->sb->ocb field twice
 * 
 * Revision 1.1  90/08/07  15:41:05  configtw
 * Initial revision
 * 
*/
char state_id [] = "@(#)state.c $Revision: 1.15 $\t$Date: 91/12/27 09:57:34 $\tTIMEWARP";


/*
		Functions:

		loadstatebuffer(o) - set up an active state for the object to run in
				Parameters - Ocb *o
				Return - SUCCESS or FAILURE

		loadstatebuffer() checks to see if the object's state buffer is
		already loaded, returning success if it is.  Otherwise, the
		routine looks into the cs pointer to find a state.  If there
		is no state there either, an error has occurred.  If a state
		is found, m_create() is used to allocate space for the temporary
		state buffer, and for the associated stack.  If space is unavailable,
		then any work done in this routine is undone, and FAILURE is 
		returned.  If the m_create()s succeed, the current state is copied
		into the temporary state with entcpy(), and the stack is initialized.
		SUCCESS is returned.
*/

#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

extern Byte * object_context;
extern int prop_delay;

extern int      hlog;
extern VTime    hlogVTime;
extern int      maxSlice;
extern Ocb*     maxSliceObj;
extern VTime    maxSliceTime;
extern int      sliceTime;
#ifdef STATETIME
long statestart, statend;
long statetime = 0;
long statecount = 0;
#endif STATETIME

FUNCTION int loadstatebuffer (o,tempType)

	register Ocb *o;
	Typtbl		*tempType;	/* -1 if same type on DYNCRMSG */
{
	register State * old_state, * new_state;
	register Byte * old_addr, * new_addr;
	register int i, size;
#if RBC
	int rv;
#endif

  Debug

#ifdef STATETIME
	statestart = clock();
#endif STATETIME

	if (o->sb != NULL)
	{
		twerror ( "loadstatebuffer F o->sb %x for %s\n", o->sb, o->name );
		tester ();
	}

	if (o->cs == NULL)
	{
		twerror ("loadstatebuffer F object %s cs == NULL", o->name);
		tester ();
	}

	o->sb = (State *) m_create ( sizeof(State) + o->pvz_len + 12, o->svt,
				NONCRITICAL );

	if ( o->sb == NULL )
		return FAILURE;

	o->sb->ocb = o;

	o->stk = m_create ( objstksize, o->svt, NONCRITICAL );

	if ( o->sb == NULL )
	{
		_pprintf ( "loadstatebuffer: o->sb == NULL after create stk\n" );
		tester ();
	}

	if ( o->stk == NULL )
	{
		l_destroy ( o->sb );
		o->sb = NULL;
		return FAILURE;
	}

	strcpy ( ((Byte *)(o->sb+1)) + o->pvz_len, "state limit" );

	strcpy ( o->stk, "stack limit" );

	if (o->centry == CMSG ||
		(o->centry == DYNCRMSG && tempType != (Typtbl *)-1))
	{
#if RBC
		if ( o->uses_rbc )
		{
			int size_in_bytes = o->typepointer->statesize;

			if ( rv = mark_op ( o, 1, o->svt ) )
			{
				_pprintf ( "mark overflow on new object in loadstatebuff\n" );
				tester();
			}
			clear ( o->footer, size_in_bytes );
		}
#endif
		clear ( o->sb, sizeof(State) + o->pvz_len);
		o->sb->ocb = o;
#ifdef STATETIME
		statend = clock();
		statetime += statend - statestart;
		statecount++;
#endif STATETIME

	/* Set the resulting event field to 1, to account for the next event
		to occur in this phase. */

		o->sb->resultingEvents = 1;

		return SUCCESS;
	}

    if ( BITTST ( o->cs->sflag, STATERR ) )
		{       /* no code is here to handle error msg copying */
		_pprintf("loadstatebuffer: error in state %x\n",o->cs);
		tester();
		}

	entcpy ( o->sb, o->cs, sizeof(State) + o->pvz_len );

	o->sb->effectWork = 0;

	old_state = o->cs;
	new_state = o->sb;

#if SOM

	/*  The previous ept is stored here so that, on rollback, the load
		management code can easily determine what the maximum non-rolled
		back ept should be changed to.  It should always be the ept of the
		previous event.  previousEpt is never adjusted during an event,
		while Ept has the amount of work added to it done in each segment
		of the event.  Ept is used to put timestamps on messages going out. */

	o->sb->previousEpt = o->sb->Ept;

	/* Set the resulting event field to 1, to account for the next event
		to occur in this phase. */

	o->sb->resultingEvents = 1;

#endif SOM

#if RBC
	if ( o->uses_rbc && (rv = mark_op ( o, 1, o->svt ) ) )
	{
		_pprintf ( "rbc is limiting memory\n" );

		l_destroy ( o->sb );
		o->sb = NULL;
		l_destroy ( o->stk );
		o->stk = NULL;
		return FAILURE;
	}
	else
		return SUCCESS;
#endif

	if ( old_state->address_table == NULL )
	{
#ifdef STATETIME
		statend = clock();
		statetime += statend - statestart;
		statecount++;
#endif STATETIME
		return SUCCESS;
	}

	new_addr = m_create ( l_size(old_state->address_table),
		o->svt, NONCRITICAL );

	if ( o->sb == NULL )
	{
		_pprintf ( "loadstatebuffer: o->sb == NULL after create addr\n" );
		tester ();
	}

	new_state->address_table = (Address *) new_addr;

	if ( new_addr == NULL )
	{
		destroy_state ( o->sb ); /* RBC can't get here */
		o->eventsPermitted++;
		o->sb = NULL;
		l_destroy ( o->stk );
		o->stk = NULL;
		return FAILURE;
	}

	clear ( new_addr, l_size(new_addr) );

	for ( i = 0; i < l_size(new_addr) / sizeof(Address); i++ )
	{
		if ( old_state->address_table[i] != NULL )
		{
			new_state->address_table[i] = DEFERRED;
		}
	}
#ifdef STATETIME
	statend = clock();
	statetime += statend - statestart;
	statecount++;
#endif STATETIME


	return SUCCESS;
}

FUNCTION destroy_state ( state )

	register State * state;
{
	register Byte * addr;
	register int i;

  Debug

	/* this function and the rollback chip don't mix! */
	/* RBC code doesn't call this function */

	if ( state->address_table != NULL )
	{
		for ( i = 0; i < l_size(state->address_table) / sizeof(Address); i++ )
		{
			addr = state->address_table[i];

			if ( addr != NULL && addr != DEFERRED )
			{   /* release all memory in the address table */
				l_destroy ( addr );
			}
		}

		/* release the address table */
		l_destroy ( state->address_table );
	}

    if ( BITTST ( state->sflag, STATERR ) )
		l_destroy(state->serror);       /* release error message */

		/* release the state space */
	l_destroy ( state );
}	/* destroy_state */

FUNCTION newBlockPtr_b ( size )

	Uint size;
{
	register Ocb * ocb = xqting_ocb;
	register State * state = ocb->sb;
	register Byte * addr;
	register int i;
	int			j;

  Debug

	if ((size > 100000) || (size == 0))
		{
		object_error ( NEWBLKSZ );
		}
	else if (state->segCount == Max_Addresses)
		{
		object_error ( ADDROFLOW );
		}
	else
		{
		if (( state->address_table == NULL ) || 
			(l_size(state->address_table) == state->segCount * sizeof(Address)))
			{  /* need to add to segment table size */
			j = (state->segCount + SEG_TABLE_INCREMENT) * sizeof ( Address );

#if RBC
			if ( ocb->uses_rbc )
				addr = rbc_malloc ( ocb, j );
			else
#endif
				addr = m_create ( j, ocb->svt, NONCRITICAL );

			if ( ocb->sb == NULL )
				{
				_pprintf ( "newBlock: o->sb == NULL after create addr\n" );
				tester ();
				}

			if ( addr == NULL )
				{
				rollback ( ocb, ocb->svt );
				dispatch ();
				return;
				}

			clear ( addr, j );	/* clear out the new table area */

			if (state->address_table != NULL)
				{	/* we're enlarging the size, so copy old stuff */
				entcpy(addr,state->address_table,l_size(state->address_table));
				l_destroy (state->address_table);	/* dump old one */
				}

			state->address_table = (Address *) addr;
			}  /* if ( state->address_table == NULL ) */

		j = l_size(state->address_table) / sizeof(Address);
		for ( i = 0; i < j; i++ )
			{
			if ( state->address_table[i] == NULL )
				break;
			}
#if RBC
		if ( ocb->uses_rbc )
			addr = rbc_malloc ( ocb, size );
		else
#endif
			addr = m_create ( size, ocb->svt, NONCRITICAL );

		if ( ocb->sb == NULL )
			{
			_pprintf ( "newBlock: o->sb == NULL after create size\n" );
			tester ();
			}

		if ( addr == NULL )
			{
			rollback ( ocb, ocb->svt );
			}
		else
			{
			state->address_table[i] = addr;
			if (++state->segCount > ocb->stats.segMax)
				ocb->stats.segMax = state->segCount;
			ocb->argblock.address_table_offset = i + 1;
			clear ( addr, size );
			}
		}  /* if no NEWBLKSZ or ADDROFLOW error */
	dispatch ();
}	/* newBlockPtr_b */

FUNCTION Pointer newBlockPtr ( size )

	Uint size;
{
	objectCode = FALSE;		/* not executing object code now */

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

	switch_back ( newBlockPtr_b, object_context, size );

	return ( xqting_ocb->argblock.address_table_offset );
}	/* newBlockPtr */



/* This function returns a Pointer value for the pointer to */
/* the top of the State. Since it is not really an          */
/* entity, it does not need to be stored in the address     */
/* table. We just pass back Max_Addresses +1                */

FUNCTION Pointer statePtr ( )

{
	return ( (Pointer) (Max_Addresses +1) );
}

#if 0

FUNCTION newBlockWithPtrs_b ( size )

	Uint size;
{
	register Ocb * ocb = xqting_ocb;
	register State * state = ocb->sb;
	register Byte * addr;
	register int i;

  Debug


	if ( state->address_table == NULL )
	{
		i = Max_Addresses * sizeof ( Address );

#if RBC
		if ( ocb->uses_rbc )
			addr = rbc_malloc ( ocb, i );
		else
#endif
		addr = m_create ( i, ocb->svt, NONCRITICAL );

		if ( ocb->sb == NULL )
		 {
		   _pprintf ( "newBlockWithPtrs: o->sb == NULL after create addr\n" );
		   tester ();
		 }

		if ( addr == NULL )
		{
			rollback ( ocb, ocb->svt );
			dispatch ();
			return;
		}

		state->address_table = (Address *) addr;
		clear ( addr, i );
	}

	for ( i = 0; i < Max_Addresses; i++ )
	{
		if ( state->address_table[i] == NULL )
			break;
	}

	if ( i == Max_Addresses )
	{
		object_error ( ADDROFLOW );
	}
	else
	{
#if RBC
		if ( ocb->uses_rbc )
			addr = rbc_malloc ( ocb, size );
		else
#endif
		addr = m_create ( size, ocb->svt, NONCRITICAL );

		if ( ocb->sb == NULL )
		{
			_pprintf (
				 "newBlockWithPtrs: o->sb == NULL after create size\n" );
			tester ();
		}

		if ( addr == NULL )
		{
			rollback ( ocb, ocb->svt );
		}
		else
		{
			state->address_table[i] = addr;
			if (++state->segCount > ocb->stats.segMax)
				ocb->stats.segMax = state->segCount;
			ocb->argblock.address_table_offset = i + 1;
			clear ( addr, size );
		}
	}

	dispatch ();
}	/* newBlockWithPtrs_b */
#endif

FUNCTION void * newBlockWithPtrs ( size, Ptr )

	Uint size;
	Pointer * Ptr;

{
	objectCode = FALSE;		/* not executing object code now */

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

	switch_back ( newBlockPtr_b, object_context, size );

	*Ptr =  xqting_ocb->argblock.address_table_offset ;
	return ( xqting_ocb->sb->address_table[(*Ptr)-1] );

}	/* newBlockWithPtrs */

FUNCTION disposeBlockPtr_b ( offset )

	int offset;
{
	register Ocb * ocb = xqting_ocb;
	register State * state = ocb->sb;
	register Byte * addr;

  Debug

	offset--;

	if ( offset < 0 ||
		state->address_table == NULL ||
		offset >= l_size(state->address_table) / sizeof(Address) ||
		state->address_table[offset] == NULL )
	{
		object_error ( STATE_DISPOSAL );
	}
	else
	{
		addr = state->address_table[offset];

#if RBC
		if ( ocb->uses_rbc )
			rbc_free ( ocb, addr );
		else
#endif
		if ( addr != DEFERRED )
			l_destroy ( addr );

		state->address_table[offset] = NULL;
		state->segCount--;
	}

	dispatch ();
}	/* disposeBlockPtr_b */

FUNCTION disposeBlockPtr ( offset )

	int offset;
{
	objectCode = FALSE;		/* not executing object code now */

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

	switch_back ( disposeBlockPtr_b, object_context, offset );
}       /* disposeBlockPtr */

FUNCTION pointerPtr_b ( offset )

	register int offset;
{
	register Ocb * ocb = xqting_ocb;
	register State * state = ocb->sb;
	register State * old_state = ocb->cs;
	register Byte * old_addr, * new_addr;
	register int size;

  Debug

	offset--;

	while ( old_state->address_table[offset] == DEFERRED )
		old_state = (State *) l_prev_macro ( old_state );

	old_addr = old_state->address_table[offset];

	size = l_size(old_addr);

	new_addr = m_create ( size, ocb->svt, NONCRITICAL );

	if ( new_addr == NULL )
	{
		rollback ( ocb, ocb->svt );
	}
	else
	{
		state->address_table[offset] = new_addr;
		entcpy ( new_addr, old_addr, size );
	}

	dispatch ();
}

FUNCTION void * pointerPtr ( offset )

	int offset;
{
	void * addr;

/* The offset value of Max_Addresses +1  is reserved    */
/* for the pointer to the top of State.                 */

	objectCode = FALSE;		/* not executing object code now */

	if ( offset == (Max_Addresses +1) )
	{
#if RBC
		if ( xqting_ocb->uses_rbc )
			{
			objectCode = TRUE;
			return ( (void *) (xqting_ocb->footer) );
			}
		else
#endif
			{
			objectCode = TRUE;
			return ( (void *) (xqting_ocb->sb +1) );
			}
	}

	if ( offset <= 0 ||
		xqting_ocb->sb->address_table == NULL ||
		offset > l_size(xqting_ocb->sb->address_table) / sizeof(Address) ||
		xqting_ocb->sb->address_table[offset-1] == NULL )
	{
#if PARANOID
		_pprintf("Bad TW pointer: %d for %s\n",offset,xqting_ocb->name);
		tester();
#endif
		addr = NULL;
	}
	else
	{
		addr = (void *) (xqting_ocb->sb->address_table[offset-1]);

		if ( addr == DEFERRED )
		{
#if RBC
			if ( xqting_ocb->uses_rbc )
			{
				_pprintf ( "rbc using object with deferred offset\n" );
				tester();
			}
#endif

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

			switch_back ( pointerPtr_b, object_context, offset );

			addr = (void *) (xqting_ocb->sb->address_table[offset-1]);
		}
	}

	objectCode = TRUE;
	return ( addr );
}       /* pointerPtr */

#if PARANOID
#define OK FALSE
#define NG TRUE

FUNCTION int objectAddressError ( addr, size )

	Byte * addr;
	int size;
{
	Ocb * o = xqting_ocb;
	int i;

	if ( addr >= o->stk+12 && addr+size <= o->stk+objstksize )
		return OK;

	if ( addr >= (Byte *)(o->sb+1) && addr+size <= (Byte *)(o->sb+1)+o->pvz_len)
		return OK;

	if ( o->sb->address_table == NULL )
		return NG;

	for ( i = 0; i < l_size(o->sb->address_table) / sizeof(Address); i++ )
	{
		Byte * saddr = (Byte *) o->sb->address_table[i];

		if ( saddr != NULL && saddr != DEFERRED )
		{
			int ssize = l_size(saddr);

			if ( addr >= saddr && addr+size <= saddr+ssize )
				return OK;
		}
	}

	return ( NG );
}
#endif

FUNCTION userError_b ( error )

	char * error;
{
	char * cerror;
	int         errLength;

	errLength = strlen(error)+1;
	if (errLength > msgdefsize)
		errLength = msgdefsize;
	cerror = (char *)m_create(msgdefsize,xqting_ocb->svt,NONCRITICAL);
	if (cerror == NULL)
		object_error ( error ); /* not enough room--use original */
	else
		{
        BITSET ( xqting_ocb->sb->sflag , STATERR );
		entcpy(cerror,error,errLength); /* copy original */
		object_error(cerror);
		}

	dispatch ();
}

FUNCTION userError ( error )

	char * error;
{
	objectCode = FALSE;		/* not executing object code now */

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

	switch_back ( userError_b, object_context, error );
}       /* userError */

FUNCTION State * copystate ( old_state )

	register State * old_state;
{
	char        *cerror;
	int         errLength;
	register State * new_state;
	register Byte * old_addr, * new_addr;
	register int i, size;
	int checkOk;

  Debug

	size = l_size(old_state);

	new_state = (State *) m_create ( size, old_state->sndtim, NONCRITICAL );

	if ( new_state == NULL )
	{
		checkOk = check_old_state ( old_state );

		if ( checkOk )
			return ( NULL );
		else
		{

#if 0
			_pprintf("copystate: can't get state to send (1)\n");
#endif

			return ( (State *) -1 );
		}
	}

	entcpy ( new_state, old_state, size );

	if ( old_state->address_table == NULL )
		return ( new_state );

	size = l_size(old_state->address_table);

	new_addr = m_create ( size, old_state->sndtim, NONCRITICAL );

	new_state->address_table = (Address *) new_addr;

	if ( new_addr == NULL )
	{
		l_destroy ( new_state );

		checkOk = check_old_state ( old_state );

		if ( checkOk )
			return ( NULL );
		else
		{

#if 0
			_pprintf("copystate: can't get state to send (2)\n");
#endif
			return ( ( State * ) -1 );
		}
	}

	clear ( new_addr, size );

	for ( i = 0; i < l_size(old_state->address_table) / sizeof(Address); i++ )
	{
		if ( old_state->address_table[i] != NULL )
		{
			State * back_state = old_state;

			while (  ! ( l_ishead_macro ( back_state ) ) &&
						back_state->address_table[i] == DEFERRED )
				back_state = (State *) l_prev_macro ( back_state );

			if ( l_ishead_macro ( back_state ) )
			{
				twerror ( "copystate F No non-deferred copy of segment %d of state %x\n", i,old_state );
				tester();
			}

			old_addr = back_state->address_table[i];

			size = l_size(old_addr);

			new_addr = m_create ( size, old_state->sndtim, NONCRITICAL );

			if ( new_addr == NULL )
			{
				destroy_state ( new_state );

				checkOk = check_old_state ( old_state );

				if ( checkOk )
					return ( NULL );
				else
				{

#if 0
					_pprintf("copystate: can't get state to send (3)\n");
#endif

					return ( ( State * ) -1 );
				}
			}

			entcpy ( new_addr, old_addr, size );

			new_state->address_table[i] = new_addr;
		}
	}

	if (old_state->serror)
		{       /* copy error message */
		errLength = strlen(old_state->serror) + 1;
		if (errLength > msgdefsize)
			errLength = msgdefsize;
		cerror = (char *)m_create(msgdefsize,
			old_state->sndtim,NONCRITICAL);
		if (cerror == NULL)
			/* not enough room--use original */
            BITCLR ( new_state->sflag , STATERR );
		else
			{
			new_state->serror = cerror;         /* save pointer */
			entcpy(cerror,old_state->serror,errLength); /* copy original */
			}
		}
	return ( new_state );
}

/*  Try to convert the existing old state into a state that can be stolen
		out of the state queue and shipped to the next phase.  If it has
		an address table, check all entries to make sure none are deferred.
		Deferred entries must be replaced with actual copies of the memory
		segments from the last state that accessed them.  If an attempt to
		make a copy of a deferred segment fails due to lack of memory, our
		best efforts have failed and we'll give up.  This function returns
		a 1 if the old state can be shipped off, and a zero if it cannot.  */

FUNCTION check_old_state ( old_state )

	State * old_state;
{
	int i, size;

#if 1
	return(NULL);
#else

	if ( old_state == NULL )
	 {
	   twerror ("check_old_state F passed a NULL State Pointer" );
	   tester ();
	   return ( NULL );
	 }

	if ( old_state->address_table != NULL )
	 {

		for ( i = 0; i < l_size(old_state->address_table) / sizeof(Address);i++)
		{
		  if ( old_state->address_table[i] == DEFERRED )
		  {
			State * back_state = l_prev_macro ( old_state );
			Byte * old_addr, * new_addr;

			/*  Look for an actual copy of this memory segment in earlier
				states.  Then grab memory and copy it. */

			while (  ! ( l_ishead_macro ( back_state )) &&
						back_state->address_table[i] == DEFERRED ) 
				back_state = (State *) l_prev_macro ( back_state );

			if ( l_ishead_macro ( back_state ) )
			{
				twerror ( "check_old_state F No non-deferred copy of segment %d of state %x\n", i,old_state );
				tester();
			}

			old_addr = back_state->address_table[i];

			size = l_size(old_addr);

			new_addr = m_create ( size, old_state->sndtim, NONCRITICAL );

			if ( new_addr == NULL )
			{
				_pprintf("check_old_state: can't find memory to copy deferred segment\n");
				return ( NULL );
			}

			entcpy ( new_addr, old_addr, size );

			old_state->address_table[i] = new_addr;
		   }
		}
	 }

	return ( 1 );
#endif
}

/* This function compares two states to determine if the jump forward
	optimization can be applied.  At the moment, only the actual body of
	the state is compared - no dynamically allocated segments are compared.
	If either state has dynamically allocated segments, the states are
	judged to be different.  In fact, if either state has an address table
	to hold dynamic memory pointers, the states are judged to be different.
	A future version of this function will also compare dynamic memory
	segments. */

FUNCTION state_compare ( old, new, length )
	State * old;
	State * new;
	int length;
{
	int compare;

	if (old->address_table != NULL || new->address_table != NULL )
	{  
	  /* Here's where the code to compare dynamic memory segments should
			  go.  When written, this whole code segment should probably
			  be relocated below the basic text comparison.*/
	  return ( -1 );
	}  

	compare = bytcmp ( (Byte *) ( old + 1 ), (Byte *) ( new + 1 ), length );

	return ( compare );
}

