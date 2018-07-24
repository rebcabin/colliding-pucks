/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	mproc.c,v $
 * Revision 1.15  91/12/27  11:07:59  pls
 * Make EVTLOG output the same as the Simulator's.
 * 
 * Revision 1.14  91/12/27  08:46:20  reiher
 * Added code to object creation routines to support event and event time
 * throttling.
 * 
 * Revision 1.13  91/11/06  11:12:12  configtw
 * Fix DLM link errors on Suns.
 * 
 * Revision 1.12  91/11/01  09:46:08  reiher
 * Code to support critical path messages and changes to permit migration
 * acks and naks to pass a message to their handling routines (PLR)
 * 
 * Revision 1.11  91/08/08  13:04:55  reiher
 * Added tests for failure of mkocb() to createproc(), makephase(), and
 * create_stdout()
 * 
 * Revision 1.10  91/07/17  15:10:41  judy
 * New copyright notice.
 * 
 * Revision 1.9  91/07/09  14:28:23  steve
 * Replaced 128 with IH_NODE and miparm.me with tw_node_num
 * 
 * Revision 1.8  91/06/03  14:24:46  configtw
 * Fix code for DLM off
 * 
 * Revision 1.7  91/06/03  12:25:31  configtw
 * Tab conversion.
 * 
 * Revision 1.6  91/06/03  09:49:35  pls
 * Add dispatch() call to error return of msgproc().
 * 
 * Revision 1.5  91/05/31  14:26:35  pls
 * 1.  Add parameter to FindObject() call.
 * 
 * Revision 1.4  91/04/01  15:42:01  reiher
 * Changes to handle phase creation and statistics stored in dead ocbs.
 * 
 * Revision 1.3  91/03/26  09:26:59  pls
 * Add Steve's RBC code.
 * 
 * Revision 1.2  90/08/09  15:55:41  steve
 * set state->ocb at create time
 * 
 * Revision 1.1  90/08/07  15:40:23  configtw
 * Initial revision
 * 
*/
char mproc_id [] = "@(#)mproc.c 1.55\t9/28/89\t15:05:09\tTIMEWARP";


/* 

Purpose:

		mproc.c contains routines for handling messages incoming to
		a node.  The hardware and low level non-TW software handles
		routing between nodes, and delivery of messages to this
		module.  The code here is responsible for delivering the
		message to the right place on this node.

		Two basic types of messages are handled by this routine.
		User messages are delivered to the appropriate object.
		System messages are transferred to the appropriate
		operating system routine for handling.  The routine
		msgproc() is responsible for dealing with messages at
		this level.  For systems messages, it consults a switch
		statement based on the system message type.  For user
		messages, it calls the routine deliver(), which will
		deliver the message to the correct object.

		Several auxiliary routines are also kept in mproc.c.
		These include some of the routines called from
		msgproc()'s switch for systems messages.  (But not
		all of those routines are kept here.)  Also, some
		small support functions are kept here.

Functions:

		msgproc() - determine what to do with a message delivered
				to this node
				Parameters - none
				Return - Always returns 0

		find_object_type(type) - consult the local type table
				to find the entry for the requested type
				Parameters - Type type
				Return - Address of the correct entry in the
						type table, or NULL ptr if not found


		f(len) - performs the work of creating a context for
				non-stdout objects.
				Parameters -  int len
				Return - Byte * to the newly created state for
						the object

		createproc(m) - set up (almost) everything needed by an object:
				an ocb, pointer to code for its object type,input/output 
				queues, etc.
				Parameters - Msgh *m
				Returns - Always returns 0

		create_stdout() - similar to createproc(), but specific to
				stdout objects
				Parameters - none
				Returns - Always returns 0

		cinfproc(cinf) - create and install world map entries
				Parameters - Wmr *cinf
				Returns - Always returns 0

Implementation:

		The main code in mproc.c, msgproc(), is fairly simple.  If
		an object is executing, call savout() for it.  Then take a
		look at the newly arrived message.  (msgproc() is only 
		called when a new message has arrived.)  If the message is
		a system message, use a switch statement to determine what
		action to take for each particular type of system message.
		Generally speaking, the action is to call an appropriate
		handling routine, then to clear the message out of the
		buffer.

		If the message is a user message, call deliver() to get it
		into the right object's input queue.  

		In either case, choose a new object to run, now that the
		operating system has finished its work.

		The rest of the code in mproc.c is related to various of
		the system message handling routines.  

		find_object_type() runs through the type table, trying to 
		find a match on the object type provided.  The purpose is 
		to locate the code that this type of object runs, information 
		that is kept in the type table.  This routine is used by
		createproc() and create_stdout().

		createproc() is the routine used to handle a CREATSYS
		system message.  Its purpose is to create a new object.
		It allocates space for the object's ocb, and sets up
		its input and output queues.  It copies the name of the
		object from the CREATSYS message to the ocb.  It finds
		the object's type (using find_object_type()), and copies
		the returned pointer into the type table into the ocb,
		thus linking the ocb with the code that implements the 
		object.  This routine then sets the object's scheduler
		time and local virtual time to POSINF.  Then the object's
		state is set to 'blocked waiting rollback", its send time
		field is set to NEGINF, and its serror field is set to
		indicate that this state is not an error state.  save_state()
		is called to save its starting state, so that it will always
		have something to roll back to.  If the object is of type
		stdout, set some state information, but do not allocate it
		a private variable zone.  For any type of object, send
		its "status" variable (local to this routine) to IH, whatever
		that may be.  Insert the new object into the object list,
		and increment the counter for number of local objects.

		create_stdout() mostly duplicates the stdout portion of
		the code in createproc().  In addition, it calls mkwmr()
		and nqwmr() to put an entry for the local stdout object
		into the local world map.

		cinfproc() finds out if a specified object is locally
		stored.  If it is, it changes the requesting message's 
		po field to show where the object is.  It then calls 
		mkwmr() and nqwmr() to put an entry for it into the 
		local world map, whether or not the object is local.  
		This routine is called as the result of a system message 
		arrival.

*/

#include "twcommon.h"
#include "twsys.h"

#define INITIAL_EVENTS_PERMITTED 30
#define INITIAL_EVENT_TIME_PERMITTED 15000
#ifdef SOM
extern long firstEst;
extern unsigned int node_cputime;
#endif SOM
extern int migrGraph;

int finishAddStats ();
Ocb * makephase ();

FUNCTION        msgproc ()

{
	Msgh           *peek;
	Msgh           *peekmsg ();
	Gvtmsg          g;
	Loadmsg         l;
	Critmsg			q;
	Crttext       c;
	Stattext      s;
	HLmsg           locmsg;
	VTime           Ctime;
	Crttext         createtext;
	Name            name;
	Type            type;

	extern int mlog;

Debug

	peek = peekmsg ();  /* lock and return pointer to message */
	if (peek == NULLMSGH)
	{  /* no message */
		twerror ("msgproc E MI gave a NULL peek msg pointer");
		dispatch();
		return;
	}
	if (issys_macro (peek))
	{
		if ( mlog )
			msglog_entry ( peek );      /* log the message */

		switch (peek->mtype)
		{  /* handle the message type */

		case GVTSYS:                    /* GVT message */

			g = ((Gvtmsg *) (peek + 1))[0];     /* point to msg body */
			acceptmsg (NULLMSGH);       /* unlock & discard message */
			gvtproc (&g);
			break;

#if DLM
		case LOADSYS:
			l = ((Loadmsg *) (peek + 1))[0];
			acceptmsg (NULLMSGH);
			loadproc (&l);
			break;
#endif

		case CRITMSG:
			q = ( ( Critmsg * ) ( peek + 1 )) [0];
			acceptmsg ( NULLMSGH );
			checkCritPath ( &q );
			break;

		case CRITSTEP:
			q = ( ( Critmsg * ) ( peek + 1 )) [0];
			acceptmsg ( NULLMSGH );
			takeCritStep ( &q );
			break;

		case CRITEND:
			q = ( ( Critmsg * ) ( peek + 1 )) [0];
			acceptmsg ( NULLMSGH );
			outputCritPath ( );
			break;

		case CRITRM:
			q = ( ( Critmsg * ) ( peek + 1 )) [0];
			acceptmsg ( NULLMSGH );
			successorNotOnCP ( &q );
			break;


		case CREATESYS:
			strcpy ( name, peek->rcver );
			strcpy ( type, ((Crttext *)(peek+1))->tp );
			acceptmsg (NULLMSGH);
			createproc (name,type,neginf);
			break;

	  case PCREATESYS:
		  strcpy ( name, peek->rcver );
		  c = (( Crttext * ) ( peek + 1 ))[0];
		  acceptmsg (NULLMSGH);
		  makephase ( name, &c );
		  break;

	  case ADDSTATS:
		  strcpy ( name, peek->rcver );
		  addStats ( name, peek );
		  acceptmsg ( NULLMSGH );
		  break;

		case MONINIT:
#ifdef MONITOR
			moninit ();
#endif
			acceptmsg (NULLMSGH);
			break;

		case STATEMSG:
			recv_state ( peek );
			acceptmsg (NULLMSGH);
			break;

		case STATEACK:
			recv_state_ack ( peek );
			acceptmsg (NULLMSGH);
			break;

		case STATENAK:
			recv_state_nak ( peek );
			acceptmsg (NULLMSGH);
			break;

		case STATEDONE:
			recv_state_done ( peek );
			acceptmsg (NULLMSGH);
			break;

		case MOVEPHASE:
			recv_phase ( peek );
			acceptmsg (NULLMSGH);
			break;

		case PHASEACK:
			recv_phase_ack ( peek );
			acceptmsg (NULLMSGH);
			break;

		case PHASENAK:
			recv_phase_nak ( peek );
			acceptmsg (NULLMSGH);
			break;

		case PHASEDONE:
			acceptmsg (NULLMSGH);
			recv_phase_done ();
			break;

		case MOVEVTIME:
			recv_vtime ( peek );
			acceptmsg (NULLMSGH);
			break;

		case VTIMEACK:
			recv_vtime_ack ( peek );
			acceptmsg (NULLMSGH);
			break;

		case VTIMENAK:
			recv_vtime_nak ( peek );
			acceptmsg (NULLMSGH);
			break;

		case VTIMEDONE:
			recv_vtime_done ( peek );
			acceptmsg (NULLMSGH);
			break;

		case HOMENOTIF:
			createtext = ((Crttext *) (peek + 1))[0];
			strcpy ( name, peek->rcver );
			acceptmsg (NULLMSGH);
			AddToHomeList ( name, createtext.phase_begin,
				createtext.phase_end,createtext.node );
			break;

		case HOMEASK:
			Ctime = peek->rcvtim;
			locmsg = ((HLmsg *) (peek + 1))[0];
			acceptmsg (NULLMSGH);
			ServiceHLRequest ( &locmsg, Ctime );
			break;

		case HOMEANS:
			locmsg = ((HLmsg *) (peek + 1))[0];
			acceptmsg (NULLMSGH);
			ObjectFound ( &locmsg );
			break;

		case HOMECHANGE:
			locmsg = ((HLmsg *) (peek + 1))[0];
			acceptmsg (NULLMSGH);
			ChangeHLEntry( locmsg.object, locmsg.time,locmsg.newloc,locmsg.generation);
			break;

		case CACHEINVAL:
			locmsg = ((HLmsg *) (peek + 1))[0];
			acceptmsg(NULLMSGH);
			RemoveFromCache(locmsg.object, locmsg.time);
			break;

		default:
			acceptmsg (NULLMSGH);
			twerror ("mproc: received unknown TW system message");
			tester();
			break;
		}

	}

	else
	{                           /* a user message */
		lock_macro ( peek );
		deliver ( peek );
	}
	dispatch ();
}



FUNCTION Typtbl *find_object_type ( type )
	char * type;

/* return pointer to the entry in type_table which matches "type" */

{
	register int i;

	for ( i = 0; type_table[i].type && i < MAXNTYP; i++)
	{
		if ( namecmp ( type, type_table[i].type ) == SUCCESS )
		{
			return &type_table[i];
		}
	}

	return (Typtbl *) NULL;
}

#ifdef RBC
extern int rbc_present;
#endif

FUNCTION        Ocb * createproc (oname,type,time)
	Name        oname;
	Type        type;
	VTime       time;
{
	Ocb            *o;
	Msgh           *msg;
	State          *state;
	Int             status;

Debug

	o = mkocb ();       /* allocate space for the new object */

	if ( o == NULL )
	{
		twerror ( "createproc unable to set up ocb for object %s\n",
					oname );
		tester();
	}

	strcpy (o->name, oname);    /* copy its name */

	if ( strcmp ( type, "NULL" ) != 0 )
	{  /* store its type if not null */
		o->typepointer = find_object_type ( type );
	}
	else
	{
		/* type_table [1] contains the NULL object type. */

		o->typepointer = &type_table[1];
	}

	if (o->typepointer == NULL)
	{
		twerror ("createproc E object type %s not found",type);
		nukocb (o);     /* deallocate memory */
		return ((Ocb *) NULL);
	}

	/* generate the unique object id */
	o->oid = ( tw_node_num << 22 )        /* 10 bits for 1024 nodes */
		   + ( unique_oid++ << 12 )     /* 10 bits for 1024 objects per node */
		   + 0;                         /* 12 bits for 4096 sequence numbers */

	o->svt = posinf;
	o->control = EDGE;

	/* phase boundaries */
	o->phase_begin = neginfPlus1;
	o->phase_end = posinfPlus1;
	o->phase_limit = o->phase_end;

	o->cycletime = 0;
	o->eventTimePermitted = INITIAL_EVENT_TIME_PERMITTED;
	o->eventsPermitted = INITIAL_EVENTS_PERMITTED;
#ifdef SOM

/*  This code initializes the ept information for the object.  Ept is
	  taken off of the system clock, in 62.5 microsecond clock ticks,
	  and it does not start at 0 at system initialization time.  This
	  code currently only works for the Butterfly, and probably would
	  bomb on any other machine. */

	if ( firstEst == 0 )
	{  
#ifdef BBN
	  butterflytime ();
#endif
	  firstEst = node_cputime;
	}  

/* I'm dubious that this is the correct thing to do for dynamically
	  created objects.  Maybe it is, but consider more fully if we intend
	  to use this code for that purpose. */

		  o->Ept = firstEst;
		  o->comEpt = o->Ept;
		  o->lastComEpt = o->Ept;
		  o->comWork = 0;
		  o->lastComWork = 0;
		  o->work = 0;
#endif SOM


	/* insert into the active object queue for this node */
	l_insert ( l_prev_macro ( _prqhd ), o );

	if ( namecmp (o->typepointer->type, "stdout") != 0 )
	{  /* it's not stdout */
		o->runstat = ARLBK;     /* activate on rollback */

#ifdef RBC
		if ( rbc_present )
		{
			o->uses_rbc = TRUE;
			o->pvz_len = 0;
		}
		else
#endif
		o->pvz_len = o->typepointer->statesize;

		/* set up a state for the object */
		state = (State *) m_create ( sizeof(State) + o->pvz_len + 12,
								time, CRITICAL );       /* allocate it */

		clear ( state, sizeof(State) + o->pvz_len );    /* clear it */
		strcpy ( ((Byte *)(state+1)) + o->pvz_len, "state limit" );
		state->ocb = o;
		state->resultingEvents = 1;

#ifdef RBC
		if ( o->uses_rbc )
		{
			int size_in_bytes = o->typepointer->statesize;

			init_op ( o, size_in_bytes );
			clear ( o->footer, size_in_bytes );
		}
#endif

		/* Even if the create message itself is at a non-NEGINF virtual time,
				the NULL state for the object should be created at NEGINF.    
				In principle, the NULL version of the object has existed since
				that time, and the NULL version hasn't done anything since     
				that time, so a state at NEGINF is the "most current" state
				of the NULL object. */

		state->sndtim = neginf;
		state->serror = NOERR;  /* state error code */

		state->otype = o->typepointer;  /* type of the state's object */

		l_insert ( o->sqh, state );     /* put into the state queue */

		/* In the case of a CREATESYS message, we now want to run its init
				section.  We do so by sending a CMSG to it, causing objhead()
				to run its init section.  If the object was created because   
				of a home list request, however, there is no point in running
				its init section, at this time, as it is a NULL object.  When 
				the user-level create message comes through, its init section 
				will be scheduled. */

		if ( strcmp ( type, "NULL" ) != 0 )
		{  /* not a null type */
			Byte mtype;
			Name *snder, *rcver;
			VTime sndtim, rcvtim;

			mtype = CMSG;       /* create message */
			snder = (Name *) "TW";
			sndtim = neginf;
			rcver = (Name *) o->name;   /* send to this new object */
			rcvtim = neginfPlus1;

			msg = make_message ( mtype, snder, sndtim, rcver, rcvtim, 0, 0 );
#ifdef SOM
			msg->Ept = 0;
#endif SOM
			nq_input_message ( o, msg );        /* put in the input queue */
		}
	}
	else
	{  /* it's type is stdout */
		o->runstat = ITS_STDOUT ;

		if ( namecmp ( o->name, "stdout" ) == 0 )
			stdout_ocb = o;     /* it's name is stdout */
		else
			o->co = (Msgh *) open_output_file ( o->name );  /* it's a file */
	}

	status = SUCCESS;

	if ( tw_node_num != 0 )
		/* send create acknowledgement to node IH_NODE */
		send_to_IH ( (Byte*) &status, sizeof(status), CRT_ACK);

#ifdef DLM
	if ( migrGraph )
	{  
	  char buff[MINPKTL];

	  sprintf ( buff, "Create %s negInf posInf %d\n", o->name, tw_node_num );

/*
	  send_to_IH ( buff, strlen ( buff ) + 1, MIGR_LOG );
*/
	}
#endif

#ifdef EVTLOG
	if ( chklog )
	{
		char name[20];
		VTime vtime;
		int evtcnt;
		Evtlog *temp;
		Byte * cp;
		int tp;
		Byte **lp;
		int namelen;
		static Byte ** logindex;
		extern Byte * evtlog_area;

#define MAX_EVTS 1000

		if ( strncmp ( o->name, "init", 4 ) == 0
		||   strncmp ( o->name, "user", 4 ) == 0 )
			return NULL;

		cp = evtlog_area;
		if ( cp == 0 )
		{
			return NULL;
		}
		if ( logindex == 0 )
		{
			tp = 0;
			while ( *cp != 0 )
			{
				while ( *cp++ != '\n' )
					;
				tp++;
			}
			logindex =  (Byte **) m_allocate ( (tp+1)*sizeof(*logindex));
			if ( logindex == 0 )
			{
				_pprintf ( "can't allocate logindex of %d\n", tp );
				tw_exit (0);
			}
			lp = logindex;
			cp = evtlog_area;
			while ( *cp != 0 )
			{
				*lp++ = cp;
				while ( *cp++ != '\n' )
					;
			}
			*lp = 0;
		}
		tp = 0;
		lp = logindex;
		namelen = strlen ( o->name );
		temp = (Evtlog *) m_allocate ( (MAX_EVTS+1)*sizeof(*temp) );
		if ( temp == 0 )
		{
			_pprintf ( "can't allocate temp of %d\n", MAX_EVTS );
			tw_exit (0);
		}
		while ( *lp != 0 )
		{
			if ( strncmp ( o->name, *lp, namelen ) == 0 )
			{
				sscanf ( *lp, "%s %lf %d %d %d",
					name, &vtime.simtime,&vtime.sequence1,
					&vtime.sequence2,&evtcnt );

				if ( name[namelen] == 0 )
				{
					if ( tp >= MAX_EVTS )
					{
						_pprintf ( "too many events for %s\n", o->name );
						tw_exit (0);
					}

					if ( vtime.simtime == 0.0 )
						vtime.simtime = 0.0;    /* Guess why */

					temp[tp].vtime = vtime;
					temp[tp++].cnt = evtcnt;
				}
			}
			lp++;
		}
		temp[tp].vtime = posinfPlus1;
		temp[tp++].cnt = 0;
		o->evtlog = (Evtlog *) m_allocate ( tp*sizeof(*temp) );
		if ( o->evtlog == 0 )
		{
			_pprintf ( "can't allocate evtlog for %s\n", o->name );
			tw_exit (0);
		}
		entcpy ( o->evtlog, temp, tp*sizeof(*temp) );
		m_release ( (Mem_hdr *) temp );
		/*_pprintf ( "%s has %d events\n", o->name, tp-1 );*/
	}
#endif

	return (o);
}

/* This function creates one phase of an object.  */

FUNCTION        Ocb * makephase ( name, c )
	Name        * name;
	Crttext     * c;
{
	Ocb            *o;
	Msgh           *msg;
	State          *state;
	Int             status;
	VTime           phaseStart, phaseEnd;
	Type            type;


Debug

	strcpy ( type, c->tp );

	o = mkocb ();       /* allocate space for the new object */

	if ( o == NULL )
	{
		twerror ( "makephase unable to set up ocb for phase %s,%f \n",
					name, c->phase_begin.simtime  );
		tester();
	}

	strcpy (o->name, name);     /* copy its name */

	if ( strcmp ( type, "NULL" ) != 0 )
	{  
		/* store its type if not null */

		o->typepointer = find_object_type ( type );
	}
	else
	{
		/* type_table [1] contains the NULL object type. */

		o->typepointer = &type_table[1];
	}

	if (o->typepointer == NULL)
	{
		twerror ("makephase E object type %s not found",type);
		nukocb (o);     /* deallocate memory */
		return ((Ocb *) NULL);
	}



	/* No attempt is made to ensure that different phases of the same object
		have the same oid.  The only purpose of the oid is to ensure that
		all file output/stdout output from a given object for a given event
		is kept together.  Since each event is handled by one and only one
		phase, and since oids from two events at different times are never 
		compared, it doesn't matter whether two phases of the same object
		have the same oid.  The oid field should probably be scrapped,
		anyway, in favor of a fully deterministic method of keeping file
		output together. */

	o->oid = ( tw_node_num << 22 )        /* 10 bits for 1024 nodes */
		+ ( unique_oid++ << 12 )/* 10 bits for 1024 objects per node */
		+ 0;                    /* 12 bits for 4096 sequence numbers */

	o->svt = posinf;
	o->control = EDGE;

	/* phase boundaries */
	o->phase_begin = c->phase_begin;
	o->phase_end = c->phase_end;
	o->phase_limit = o->phase_end;
	o->eventTimePermitted = INITIAL_EVENT_TIME_PERMITTED;
	o->eventsPermitted = INITIAL_EVENTS_PERMITTED;

	/* insert into the active object queue for this node */

	l_insert ( l_prev_macro ( _prqhd ), o );

	if ( namecmp (o->typepointer->type, "stdout") != 0 )
	{  
		/* it's not stdout */

		o->runstat = ARLBK;     /* activate on rollback */

		o->pvz_len = o->typepointer->statesize;

		/* If this isn't the first phase for the object, don't create
				a state for it, yet.  Instead, leave it in a BLKSTATE
				run status, indicating that it's waiting for a state from
				an earlier phase. */

		if ( eqVTime ( o->phase_begin, neginfPlus1 ) )
		{
			/* set up a state for the object */

			state = (State *) m_create ( sizeof(State) + o->pvz_len + 12,
								neginf, CRITICAL );     /* allocate it */

			clear ( state, sizeof(State) + o->pvz_len );        /* clear it */
			strcpy ( ((Byte *)(state+1)) + o->pvz_len, "state limit" );
			state->ocb = o;

			/* Even if the create message itself is at a non-NEGINF virtual 
				time, the NULL state for the object should be created at 
				NEGINF.  In principle, the NULL version of the object has 
				existed since that time, and the NULL version hasn't done 
				anything since that time, so a state at NEGINF is the "most 
				current" state of the NULL object. */

			state->sndtim = neginf;
			state->serror = NOERR;      /* state error code */

			state->otype = o->typepointer;      /* type of the state's object */

			l_insert ( o->sqh, state ); /* put into the state queue */

			/* In the case of a CREATESYS message, we now want to run its init
				section.  We do so by sending a CMSG to it, causing objhead()
				to run its init section.  If the object was created because   
				of a home list request, however, there is no point in running
				its init section, at this time, as it is a NULL object.  When 
				the user-level create message comes through, its init section 
				will be scheduled. */

			if ( strcmp ( type, "NULL" ) != 0 )
			{  
				/* not a null type */

				Byte mtype;
				Name *snder, *rcver;
				VTime sndtim, rcvtim;

				mtype = CMSG;   /* create message */
				snder = (Name *) "TW";
				sndtim = neginf;
				rcver = (Name *) o->name;       /* send to this new object */
				rcvtim = neginfPlus1;

				msg = make_message ( mtype, snder, sndtim, rcver, rcvtim, 
									    0, 0 );
				nq_input_message ( o, msg );    /* put in the input queue */
			}
		}
		else
		{
			o->runstat = BLKSTATE;

			/* If this is not the first phase of the object, set this phase's
				create count to 1, indicating that it was properly created.
				The first phase gets its create count set to 1 by the
				CMSG sent to start up its init section.  This raises a
				possibility that must be investigated - if we send a split
				object a dynamic destroy message to its earlier phase,
				will the later phase be properly destroyed?  */

			o->crcount = 1;
		}
	}
	else
	{  
		/* its type is stdout, but you can't divide a stdout type object
				into phases */

		twerror("makephase: Cannot divide object %s of type stdout into phases\n", 
				o->name);  
		tester();

	}

	status = SUCCESS;

	if ( tw_node_num != 0 )
		/* send create acknowledgement to node IH_NODE */
		send_to_IH ( (Byte*) &status, sizeof(status), CRT_ACK);

#ifdef EVTLOG
/* There's no reason to believe event logging will work with static phase
		creation. */

	if ( chklog )
	{
		char name[20];
		VTime vtime;
		int evtcnt;
		Evtlog *temp;
		Byte * cp;
		int tp;
		Byte **lp;
		int namelen;
		static Byte ** logindex;
		extern Byte * evtlog_area;

#define MAX_EVTS 1000

		if ( strncmp ( o->name, "init", 4 ) == 0
		||   strncmp ( o->name, "user", 4 ) == 0 )
			return NULL;

		cp = evtlog_area;
		if ( cp == 0 )
		{
			return NULL;
		}
		if ( logindex == 0 )
		{
			tp = 0;
			while ( *cp != 0 )
			{
				while ( *cp++ != '\n' )
					;
				tp++;
			}
			logindex =  (Byte **) m_allocate ( (tp+1)*sizeof(*logindex));
			if ( logindex == 0 )
			{
				_pprintf ( "can't allocate logindex of %d\n", tp );
				tw_exit (0);
			}
			lp = logindex;
			cp = evtlog_area;
			while ( *cp != 0 )
			{
				*lp++ = cp;
				while ( *cp++ != '\n' )
					;
			}
			*lp = 0;
		}
		tp = 0;
		lp = logindex;
		namelen = strlen ( o->name );
		temp = (Evtlog *) m_allocate ( (MAX_EVTS+1)*sizeof(*temp) );
		if ( temp == 0 )
		{
			_pprintf ( "can't allocate temp of %d\n", MAX_EVTS );
			tw_exit (0);
		}
		vtime.sequence1 = vtime.sequence2 = 0;
		while ( *lp != 0 )
		{
			if ( strncmp ( o->name, *lp, namelen ) == 0 )
			{
				sscanf ( *lp, "%s %lf %d %d %d",
					name, &vtime.simtime,&vtime.sequence1,
					&vtime.sequence2,&evtcnt );

				if ( name[namelen] == 0 )
				{
					if ( tp >= MAX_EVTS )
					{
						_pprintf ( "too many events for %s\n", o->name );
						tw_exit (0);
					}

					if ( vtime.simtime == 0.0 )
						vtime.simtime = 0.0;    /* Guess why */

					temp[tp].vtime = vtime;
					temp[tp++].cnt = evtcnt;
				}
			}
			lp++;
		}
		temp[tp].vtime = posinfPlus1;
		temp[tp++].cnt = 0;
		o->evtlog = (Evtlog *) m_allocate ( tp*sizeof(*temp) );
		if ( o->evtlog == 0 )
		{
			_pprintf ( "can't allocate evtlog for %s\n", o->name );
			tw_exit (0);
		}
		entcpy ( o->evtlog, temp, tp*sizeof(*temp) );
		m_release ( (Mem_hdr *) temp );
		/*_pprintf ( "%s has %d events\n", o->name, tp-1 );*/
	}
#endif

	return (o);
}

create_stdout ()
{       /* create the stdout object */
	Ocb            *o;

	o = mkocb ();       /* create an object & init its queues */

	if ( o == NULL )
	{
		twerror ( "create_stdout unable to set up ocb\n" );
		tester();
	}

	if (o == NULL)
	{
		twerror ("create_stdout E couldn't allocate Ocb");
	}

	strcpy (o->name, "stdout"); /* name it */

	o->typepointer = find_object_type ("stdout");  /* find it in the list */
	if (o->typepointer == NULL)
	{
		twerror ("createproc E object type %s not found","stdout");
		nukocb (o);     /* get rid of the object & its queues */
		return;
	}

/* init some object fields */
	o->svt = posinf;
	o->runstat = ITS_STDOUT ;

	o->phase_begin = neginfPlus1;
	o->phase_end = posinfPlus1;

	stdout_ocb = o;     /* init this global */

	l_insert ( l_prev_macro ( _prqhd ), o );    /* add to processor ready q */
}

/* When an object is divided into several phases, the earlier phases will
	  eventually be garbage collected.  As GVT catches up with a phase's
	  end of interval, that phase packages its statistics in a system
	  message and sends it to the next later phase.  That later phase
	  adds them to its own statistics.  If the later phase has itself
	  been garbage collected, then forward the message to the next
	  phase.  Since the final phase is never garbage collected, eventually
	  the message will find a phase.  A special "dead ocb" queue containing
	  only the name of the object and the phase begin and end of garbage
	  collected ocbs is kept for this purpose. */

FUNCTION addStats ( object, statsMsg )

	Name      * object;
	Msgh    * statsMsg;
{
	Objloc    * location;
	Stattext  * text;

	text = ( Stattext * ) ( statsMsg + 1 );

	location = GetLocation ( object, text->phaseEnd );

	/* If the object isn't here, it could be a problem of the phase being
	  migrated, or it could be that the phase has been garbage collected.
	  If the object is here, all is well, so just add in the stats. */

	if ( location == NULL || location->node != tw_node_num )
	{  
	  Int     newNode;
	  Msgh    * tw_msg;

	  /* The reason the actual make_message() calls are done below,
			  rather than consolidated here, is that some of them have
			  different times associated with the message. */

	  if  ( location == NULL )
	  {
		  deadOcb     * dead;

		  dead = findPhaseInDeadQ ( object, text->phaseEnd );

		  if ( dead == NULL )
		  {


			  /* How inconvenient.  Someone migrated the phase elsewhere.
					  Call FindObject() to figure out where, coming
					  back to finishAddStats() when you do. */

			  tw_msg = make_message ( ADDSTATS, object, statsMsg->sndtim,
							  object, statsMsg->rcvtim, sizeof ( Stattext ),
							  text );

			  tw_msg->flags |= SYSMSG;

			  /* We'll have to call FindObject. */

			  FindObject ( object, statsMsg->rcvtim, tw_msg,
							  finishAddStats,NOTMSG );
		  }
		  else
		  {

			  /* If the destination phase is dead, we'd better send the
					  stats on to the next phase down the line. */

			  tw_msg = make_message ( ADDSTATS, object, dead->phaseBegin,
					  object, dead->phaseEnd, sizeof ( Stattext ), text );

			  tw_msg->flags |= SYSMSG;

			  /* We'll have to call FindObject. */

			  FindObject (object, dead->phaseEnd, tw_msg,finishAddStats,NOTMSG);
		  }
	  }
	  else
	  {
		  /* Package up a new ADDSTATS message for whoever hosts the next
			  phase. */

		  tw_msg = make_message ( ADDSTATS, object, statsMsg->sndtim,
							  object, statsMsg->rcvtim, sizeof ( Stattext ),
							  text );

		  tw_msg->flags |= SYSMSG;

		  sndmsg ( tw_msg, tw_msg->txtlen + sizeof (Msgh), location->node );
	  }
	}      
else
	{  
	  finishAddStats ( statsMsg, location );
	}  
}

/* finishAddStats() is called either from addStats() or after a request to
	  the home node to find the phase in question returns.  It either
	  sends the message off to another node, or it adds all the stats
	  into the local phase's statistics. */

FUNCTION finishAddStats ( m, location )
	Msgh      * m;
	Objloc    * location;
{
	Stattext  * statsPtr;
	struct stats_s * earlyStats, * lateStats;
	Ocb       * o;

	o = location->po;

	statsPtr = ( Stattext * ) ( m + 1 );

  if ( location->node != tw_node_num )
	{  

	  _pprintf("finishAddStats forwarding msg to %d\n", location->node );
	  /* Send message to wherever it goes. */

	  sndmsg (m, m->txtlen + sizeof (Msgh), location->node);
	}  
	else
	if ( location->po == NULL )
	{  
	  twerror("finishAddStats: cannot find supposedly local phase %s %f\n",
					  location->name, location->phase_begin);
	  tester();
	}  
	else
	{  
	  earlyStats = & (statsPtr->stats);
	  lateStats = & (o->stats);

	  lateStats->numestart += earlyStats->numestart;
	  lateStats->numecomp += earlyStats->numecomp;
	  lateStats->numcreate += earlyStats->numcreate;
	  lateStats->numdestroy += earlyStats->numdestroy;
	  lateStats->ccrmsgs += earlyStats->ccrmsgs;
	  lateStats->cdsmsgs += earlyStats->cdsmsgs;
	  lateStats->cemsgs += earlyStats->cemsgs;
	  lateStats->cebdls += earlyStats->cebdls;
	  lateStats->coemsgs += earlyStats->coemsgs;
	  lateStats->coebdls += earlyStats->coebdls;
	  lateStats->nssave += earlyStats->nssave;
	  lateStats->nscom += earlyStats->nscom;
	  lateStats->eposfs += earlyStats->eposfs;
	  lateStats->eposfr += earlyStats->eposfr;
	  lateStats->enegfs += earlyStats->enegfs;
	  lateStats->enegfr += earlyStats->enegfr;
	  lateStats->ezaps += earlyStats->ezaps;
	  lateStats->evtmsg += earlyStats->evtmsg;
	  lateStats->eposrs += earlyStats->eposrs;
	  lateStats->eposrr += earlyStats->eposrr;
	  lateStats->enegrs += earlyStats->enegrs;
	  lateStats->enegrr += earlyStats->enegrr;
	  lateStats->cputime += earlyStats->cputime;
	  lateStats->rbtime += earlyStats->rbtime;
	  lateStats->comtime += earlyStats->comtime;
	  lateStats->nummigr += earlyStats->nummigr;
	  lateStats->numstmigr += earlyStats->numstmigr;
	  lateStats->numimmigr += earlyStats->numimmigr;
	  lateStats->numommigr += earlyStats->numommigr;

	/* Just adding is OK for these queue length stats, since these stats
	  will be divided by the gvt count to get an average. */

	  lateStats->sqlen += earlyStats->sqlen;
	  lateStats->iqlen += earlyStats->iqlen;
	  lateStats->oqlen += earlyStats->oqlen;

	  if ( lateStats->sqmax < earlyStats->sqmax )
		  lateStats->sqmax += earlyStats->sqmax;

	  if ( lateStats->iqmax < earlyStats->iqmax )
		  lateStats->iqmax += earlyStats->iqmax;

	  if ( lateStats->oqmax < earlyStats->oqmax )
		  lateStats->oqmax += earlyStats->oqmax;

	  lateStats->stforw += earlyStats->stforw;

	}  
}


