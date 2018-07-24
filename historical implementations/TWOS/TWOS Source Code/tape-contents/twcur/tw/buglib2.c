/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	buglib2.c,v $
 * Revision 1.17  91/12/27  10:45:04  pls
 * 1.  Use l_size macro.
 * 2.  Add support for variable length address table (SCR 214).
 * 
 * Revision 1.16  91/12/27  08:39:16  reiher
 * Changed showocb to show how many events throttling permits
 * 
 * Revision 1.15  91/11/06  11:08:16  configtw
 * Fix merge error.
 * 
 * Revision 1.14  91/11/04  09:34:08  pls
 * Add sequence times to gvt printouts.
 * 
 * Revision 1.13  91/11/01  09:28:10  reiher
 * Added debugging info for critical path mechanism
 * 
 * Revision 1.12  91/08/08  15:17:49  reiher
 * Fixed showHomeNode() so it would actually print the home node number, not
 * just return it.
 * 
 * Revision 1.11  91/07/17  15:07:15  judy
 * New copyright notice.
 * 
 * Revision 1.10  91/06/04  13:44:41  configtw
 * Don't count system message with gid.num 0 as bad.
 * 
 * Revision 1.9  91/06/03  12:23:36  configtw
 * Tab conversion.
 * 
 * Revision 1.8  91/05/31  12:42:48  pls
 * 1.  Keep tester from bombing when bad msg is entered (bug 6).
 * 2.  Add tester routine to dump list headers.
 * 3.  Add validState() for PARANOID testing.
 * 
 * Revision 1.7  91/04/01  15:33:58  reiher
 * Added a number of new routines for dynamic load management data
 * gathering.  Also added output lines to some existing routines.  Some code
 * is conditionally compiled to support Tapas Som's work.
 * 
 * Revision 1.6  91/03/26  14:17:50  configtw
 * Enhance operation of dm() (from Steve).
 * 
 * Revision 1.5  90/12/10  10:39:23  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.4  90/11/27  09:43:11  csupport
 * have dumpstateAddrTablex() call dumpstateAddrTable(), not dumpstate()
 * 
 * Revision 1.3  90/11/27  09:23:35  csupport
 * 1.  fix bug in showschedq to handle deferred segments
 * 2.  add code to handle staddrt tester command
 * 
 * Revision 1.2  90/08/09  15:05:30  steve
 * dumpstate now prints ocb field
 * 
 * Revision 1.1  90/08/07  15:37:54  configtw
 * Initial revision
 * 
*/
char buglib2_id [] = "@(#)buglib2.c     1.67\t10/23/89\t11:02:54\tTIMEWARP";


/*

Purpose:

		This module contains routines that support debugging and other
		informatory output from the system.

Functions:

		get_enum(type,symbolic_constant) - convert a numeric condition
				code into a string
				Parameters - Char *type, int symbolic constant
				Return - a pointer to the string

		dumparray(u,n) - print an array, both in alpha and hex
				Parameters - char *u, int n
				Return -  Always returns zero

		mem_used_in_queues() - calculate and print the memory used in queues
				Parameters - none
				Return - Always returns zero

		showschedq() - print out a scheduler queue, in short form
				Parameters - none
				Return - Always returns zero

		showsq(o) - shwo an object's state queue
				Parameters - Ocb o
				Return - Always returns zero

		returnoqmem(o) - calculate the amount of memory in an object's output
				queue
				Parameters - Ocb *o
				Return - the amount of memory

		returniqmem(o) - calculate the amount of memory in an object's input
				queue
				Parameters - Ocb *o
				Return - the amount of memory

		returnsqmem(o) - calculate the amount of memory in an object's state
				queue
				Parameters - Ocb *o
				Return - the amount of memory

		countoqmem(o) - print the amount of memory in an object's output
				queue
				Parameters - Ocb *o
				Return - Always returns zero

		countiqmem(o) - print the amount of memory in an object's input
				queue
				Parameters - Ocb *o
				Return - Always returns zero

		countsqmem(o) - print the amount of memory in an object's state
				queue
				Parameters - Ocb *o
				Return - Always returns zero

		showiq(o) - print out an object's input queue
				Parameters - Ocb *o
				Return - Always returns zero

		showoq(o) - print out an object's output queue
				Parameters - Ocb *o
				Return - Always returns zero

		dumpmsg(m) - print out a message
				Parameters - Msgh *m
				Return - Always returns zero

		showmsg_head() - print a message heading
				Parameters - none
				Return - Always returns zero

		showmsg(m) - print a message in abbreviated form
				Parameters - Msgh *m
				Return - Always returns zero

		show_state_head() - print out a state heading
				Parameters - none
				Return - Always returns zero

		showstate(s) - print out a state in abbreviated form
				Parameters - State *s
				Return - Always returns zero

		dumpstate(s) - print out a state in long form
				Parameters - State *s
				Return - Always returns zero

		ttoc(string,time) - convert a virtual time to a user-readable string
				Parameters - char *string, VTime time
				Return - Always returns zero

		showocb_head() - print out an ocb heading
				Parameters - none
				Return - Always returns zero

		showocb(o) - print out an ocb
				Parameters - Ocb *o
				Return - Always returns zero

		general_ocb_by_name(name) - find an object's location from its name
				Parameters - char *name
				Return - a pointer to its location, or NULL if it
						doesn't exist, or isn't on this node

		show_iq_by_name(name) - show the input queue of the named object
				Parameters - char *name
				Return - Always returns zero

		show_oq_by_name(name) - show the output queue of the named object
				Parameters - char *name
				Return - Always returns zero

		show_sq_by_name(name) - show the state queue of the named object
				Parameters - char *name
				Return - Always returns zero

		dump_ocb_by_name(name) - dump the ocb of the named object
				Parameters - char *name
				Return - Always returns zero

		dumpocb(o) - make a complete dump of an ocb
				Parameters - Ocb *o
				Return - Always returns zero

		dprintf(s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) - 
				do a formatted print of the data provided
				Parameters - Char *s, Long a1 through a15
				Return - Always returns zero

		drawline() - print out a line of underscores
				Parameters - none
				Return - Always returns zero

Implementation:

		While buglib2.c contains a lot of routines, there is little
		code of much interest here.  Most of it is rather repetitive
		output formatting, or slightly differing interfaces for
		accessing the same data.  

		A couple of routines are worth noting.
		get_enum() is worthy of mention because
		it provides a method of converting condition codes to 
		strings that is used for several different types of data
		structures.  The static array "list" (a real loser of a
		variable name) contains the data that get_enum() needs to 
		translate the name of the type of the condition code field
		and a specific integer condition code type to a short string
		suitable for output.  This interface is used for the run status
		of objects; for their error status, if any; for their edge
		status; for the status of messages; and for message types.

		dumparray() is a generalized array dumper that puts its output
		in a format similar to the well-known od output format, except
		that dumparray() uses hex.

		These aside, there is little in this code that is tricky,
		interesting, important, or otherwise worthy of mention. 

*/


#include "twcommon.h"
#include "twsys.h"
#include "tester.h"

struct enum_dat
{
	char            * typefield;
	char            * constant_name;
	int               constant_value;
}
	list[] =
{
	{              "runstat", "READY", READY                    },
	{              "runstat", "BLKINF", BLKINF                  },
	{               "runstat", "BLKPKT", BLKPKT                 },
	{               "runstat", "GOFWD", GOFWD                   },
	{               "runstat", "ARCP", ARCP                     },
	{               "runstat", "ARLBK", ARLBK                   },
	{               "runstat", "STDOUT", ITS_STDOUT             },
	{               "runstat", "BLKSTA", BLKSTATE               },
	{               "runstat", "BLKPHA", BLKPHASE               },

	{               "centry", "INIT", INIT                      },
	{               "centry", "EVENT", EVENT                    },
	{               "centry", "TERM", TERM                      },
	{               "centry", "CREATE", DCRT                    },
	{               "centry", "DESTR", DDES                     },

	{               "mtype", "CMSG", CMSG                       },
	{               "mtype", "EMSG", EMSG                       },
	{               "mtype", "TMSG", TMSG                       },
	{               "mtype", "DYNCR", DYNCRMSG                  },
	{               "mtype", "DYNDS", DYNDSMSG                  },
	{               "mtype", "GVTSYS", GVTSYS                   },
	{               "mtype", "COMMAND", COMMAND                 },
	{               "mtype", "CRT_ACK", CRT_ACK                 },

	{               "node", "BCAST", BCAST                      },

	{               "control", "EDGE", EDGE                     },
	{               "control", "NON-EDGE", NONEDGE              },

	{               NULL, NULL, 0                               }
};

get_enum (type, symbolic_constant, p)

	Char           *type;
	int             symbolic_constant;
	char           *p;
{
	struct enum_dat *l; 
	int             n;

	if (type == NULL)
	{
		strcpy (p, " **> ERROR <**");
		return;
	}

	n = 0;

	for (l = list; l->typefield[0] != '\0'; l++)
	{
		if (strcmp (l->typefield, type) == SUCCESS)
		{
			n++;
			if (l->constant_value == symbolic_constant)
			{
				strcpy (p, l->constant_name);
				return;
			}
		}
		else
		if (n > 0)
			break;
	}

	itoa (symbolic_constant, p);
}

dumparray (u, n)
	char           *u;
	int             n;
{
	char            s[80];
	char            t[10],
					c;
	int             i,
					j;

	s[78] = '\n';
	s[79] = '\0';

#define clrs    {int i; for(i=0; i<78; i++) s[i] = ' ';}

	for (i = 0; i < n;)
	{
		clrs

		sprintf (t, "%3.3x:", i % 0x1000);

		s[0] = toupper (t[0]);
		s[1] = toupper (t[1]);
		s[2] = toupper (t[2]);
		s[3] = t[3];

		for (j = 0; j < 16 && i < n; j++, i++)
		{
			sprintf (t, "%2x ", 0377 & (c = 0377 & u[i]));
			s[5 + 3 * j] = toupper (t[0]);
			s[5 + 3 * j + 1] = toupper (t[1]);
			s[5 + 52 + j] = c < 32 || c > 127 ? '.' : c;
		}
		dprintf ("%s", s);
	}
}

#define OVERHEAD (sizeof(List_hdr)+sizeof(Mem_hdr))

extern List_hdr * free_pool;
extern int free_pool_size;

extern List_hdr * msg_free_pool;
extern int msg_free_pool_size;

mem_used_in_queues ()
{
	Ocb            *o;
	Long            count = OVERHEAD;
	List_hdr * free;

	for (o = fstocb_macro; o; o = nxtocb_macro (o))
	{
		count += sizeof(Ocb) + OVERHEAD;
		if ( o->sb != NULL )
		{
			count += l_size (o->sb) + OVERHEAD;
			count += l_size (o->stk) + OVERHEAD;
		}
		count += (long) returniqmem (o);
		count += (long) returnoqmem (o);
		count += (long) returnsqmem (o);
	}
	_pprintf ("Total memory used in queues = %ld bytes\n", count);

	if ( free_pool != NULL )
	{
		count = OVERHEAD;

		for ( free = l_next_macro ( free_pool ); free != free_pool;
			  free = l_next_macro ( free ) )
		{
			count += (free-1)->size + OVERHEAD;
		}

		_pprintf ( "Free pool size %d # of bytes %d\n", free_pool_size, count );
	}

	if ( msg_free_pool != NULL )
	{
		count = OVERHEAD;

		for ( free = l_next_macro ( msg_free_pool ); free != msg_free_pool;
			  free = l_next_macro ( free ) )
		{
			count += (free-1)->size + OVERHEAD;
		}

		_pprintf ( "Mesg pool size %d # of bytes %d\n", msg_free_pool_size, count );
	}
}

showschedq ()
{
	Ocb            *o;

	showocb_head ();
	for (o = fstocb_macro; o; o = nxtocb_macro (o))
		showocb (o);
}

showdeadq ()
{
	deadOcb            *o;

   printf("Name              phBegin                 phEnd\n");
   for (o = fstDocb_macro; o; o = nxtDocb_macro (o))
	   printf("%s      %f      %f\n",o->name,o->phaseBegin.simtime,
			  o->phaseEnd.simtime);

}

showsq (o)
	Ocb            *o;
{
	State          *s;

	showstate_head ();
	for (s = fststate_macro (o); s; s = nxtstate_macro (s))
	{
		showstate (s);
	}
}

returnoqmem (o)
	Ocb            *o;
{
	Msgh           *m;
	int             b;

	b = OVERHEAD;
	for (m = fstomsg_macro (o); m; m = nxtomsg_macro (m))
		b += l_size (m) + OVERHEAD;
	return b;
}


returniqmem (o)
	Ocb            *o;
{
	Msgh           *m;
	int             b;

	b = OVERHEAD;
	for (m = fstimsg_macro (o); m; m = nxtimsg_macro (m))
		b += l_size (m) + OVERHEAD;
	return b;
}


returnsqmem (o)
	Ocb            *o;
{
	State          *m;
	int             b;

	b = OVERHEAD;
	for (m = fststate_macro (o); m; m = nxtstate_macro (m))
	{
		b += sizeof (State) + o->pvz_len + 12 + OVERHEAD;

		b += dynamic_mem (m);
	}
	return b;
}

dynamic_mem (m)

	State          *m;
{
	int             b, i;

	if ( m->address_table == NULL )
		return 0;

	b = l_size(m->address_table) + OVERHEAD;

	for ( i = 0; i < l_size(m->address_table) / sizeof(Address); i++ )
	{
		Address addr = m->address_table[i];

		if ( (addr != NULL) & (addr != DEFERRED) )
		{
			b += l_size (addr) + OVERHEAD;
		}
	}

	return b;
}

showiq (o)
	Ocb            *o;
{
	Msgh           *m;

	showmsg_head ();
	for (m = fstimsg_macro (o); m; m = nxtimsg_macro (m))
	{
		showmsg (m);
	}
}

showoq (o)
	Ocb            *o;
{
	Msgh           *m;

	showmsg_head ();
	for (m = fstomsg_macro (o); m; m = nxtomsg_macro (m))
	{
		showmsg (m);
	}
}

dumpmsg (m)
	Msgh           *m;
{
	char           c[20];
	char            time[12];

    if ((m->gid.node >= tw_num_nodes) ||
        ((!(m->flags & SYSMSG)) && (m->gid.num == 0)))
		{
		dprintf("Bad message\n");
		return;
		}
	dprintf (" ****** MESSAGE *********[ %lx ]**********************\n", m);
	dprintf ("Sender:      %-20s\n", m->snder);
	ttoc (time, m->sndtim);
	dprintf ("Sendtime:    %-10s,%d,%d\n", time,m->sndtim.sequence1,m->sndtim.sequence2);
	dprintf ("Receiver:    %-20s\n", m->rcver);
	ttoc (time, m->rcvtim);
	dprintf ("Rcvtime:     %-10s\n", time);
	dprintf ("Selector:    %20d\n", m->selector);
	dprintf ("- Gid.Node:  %20d\n", m->gid.node);
	dprintf ("  Gid.Num:   %20d\n", m->gid.num);
	dprintf ("Bits (hex):  %20x\n", m->flags);
	get_enum ("mtype", m->mtype, c);
	dprintf ("Mtype:       %-20s\n", c);
	dprintf ("Txtlen:      %20d\n", m->txtlen);
#ifdef SOM
	dprintf ("Ept:    %20d\n", m->Ept);
#endif SOM
	dprintf ("\n");

/* Messages can be truncated in the same way as states, but there's no spare
	bit in their flag field to indicate that we did it.  Until we expand
	the flag field, we'll have to live with junk being printed when 
	dumpmsg() is done on a truncated message. */

/*
	if ( BITTEST ( m->flags, MSGTRUNC ) )
	{
		dprintf ("\nMESSAGE TRUNCATED\n");
		drawline ();
		return;
	}
*/

	dumparray ((Char *) (m + 1), m->txtlen);

	drawline ();
}

showmsg_head ()
{
	dprintf ( "\n" );
	dprintf (
"Message  Sender           Sndtim Receiver         Rcvtim --GID--  B Mtype\n" );
}

showmsg (m)
	Msgh           *m;
{
	char            mtype[20];
	char            sndtim[12];
	char            rcvtim[12];

	if (m == NULL)
		return;
/*
xx--Message  Sender           Sndtim Receiver         Rcvtim --GID--  B Mtype
xx--mmmmmmmm ssssssssssssssss tttttt rrrrrrrrrrrrrrrr tttttt gg nnnn bb ttttt
*/
	ttoc (sndtim, m->sndtim);
	ttoc (rcvtim, m->rcvtim);
	get_enum ("mtype", m->mtype, mtype);
	dprintf ( "%-8x %-16s %6s %-16s %6s %2d %4d %2x %-5s\n",
		m, m->snder, sndtim, m->rcver, rcvtim, m->gid.node, m->gid.num,
		m->flags, mtype );
}

showstate_head ()
{
	dprintf ( "\n" );
	dprintf (" Address  sndtim   Serror\n");
}

showstate (s)
	State          *s;
{
	char            sndtim[12];
	char * serror = "no err";

	if ( s->serror != NOERR )
		serror = s->serror;
/*
xx--mmmmmmmm tttttt tttttttttt
*/
	ttoc ( sndtim, s->sndtim );
	dprintf ( "%8x  %6s   %-10s  otype = %x, stype = %d, flag = %d, resEv = %d\n",
		s, sndtim, serror, s->otype->type, s->stype, s->sflag, s->resultingEvents );
}


dumpstate (s)
	State          *s;
{
	char            time[12];
	char * serror = "no error";
	Ocb       * o;

	if ( s->serror != NOERR )
		serror = s->serror;

	dprintf (" ******  STATE  *********[ %lx ]**********************\n", s);
	ttoc (time, s->sndtim);
	dprintf ("Sendtime:    %-20s, %d, %d\n", time,s->sndtim.sequence1,
		s->sndtim.sequence2);
	dprintf ("Serror:      %-20s\n", serror);
	dprintf ("Stype:       %d\n", s->stype);
	dprintf ("Ocb:         %x\n", s->ocb);
	o = (Ocb *) s->ocb;
	dprintf ("Object: %-20s\n",o->name);
#ifdef SOM
	dprintf("Ept:     %d\n",s->Ept);
	dprintf("Prev Ept:        %d\n",s->previousEpt);
	dprintf("Result Evnts:	%d\n",s->resultingEvents );
#endif SOM

	dprintf ("\n");

	if ( BITTEST ( s->sflag, STATETRUNC) )
	{
		dprintf("\nSTATE TRUNCATED\n");
		drawline ();
		return;
	}

	dumparray ((Char *) (s + 1), l_size(s) - sizeof(State));

	drawline ();

	if ( s->address_table != NULL )
	{
		register Address a;
		register int i;

		for ( i = 0; i < l_size(s->address_table) / sizeof(Address); i++ )
		{
			if ( ( a = s->address_table[i] ) )
			{
				if ( a == (Address)-1 )
					dprintf ("DEFERRED\n");
				else
					dumparray ( a, l_size(a) );
				drawline ();
			}
		}
	}
}


dumpstateAddrTablex (state)
	State ** state;
{
	dumpstateAddrTable ( *state );
}

dumpstateAddrTable (s)
	State *s;
{
	char            time[12];

	ttoc (time, s->sndtim);

	printf("dumping address table for object %s, time %f\n", ((Ocb *) s->ocb)->name,
				time);

	if ( s->address_table != NULL )
	{
		register Address a;
		register int i;

		dprintf("Max_Addresses is %d\n", Max_Addresses );
		dprintf("Entry  Size    Address\n");

		for ( i = 0; i < l_size(s->address_table) / sizeof(Address); i++ )
		{
			if ( ( a = s->address_table[i] ) )
			{
				if ( a == (Address)-1 )
					dprintf ("DEFERRED\n");
				else
					dprintf("%d %d      %x\n",i,l_size(a), a);
			}
		}
	}
	else
	{
		dprintf("no address table for state\n");
	}

}

dump_state_hdr ( state_ptr )
State   * state_ptr;

{

  dprintf ("st %f: node %d: seg# %d: #segs %d: pkt# %d: #pkts %d Tpkts %d\n",
			state_ptr->sndtim.simtime,
			state_ptr->segno,
			state_ptr->no_segs,
			state_ptr->pktno,
			state_ptr->no_pkts,
			state_ptr->tot_pkts
		  );

}
/*PJH#1 */

dump_state_migr_hdr (migr_hdr )
State_Migr_Hdr *migr_hdr;

{
	Ocb *o;

	o = (Ocb *) migr_hdr->state->ocb;

   _pprintf ("name %s: dt %f: node %d: ack %d: done %d: migr_flags %d\n",
			 o->name,
			 migr_hdr->time_to_deliver.simtime,
			 migr_hdr->to_node,
			 migr_hdr->waiting_for_ack,
			 migr_hdr->waiting_for_done,
			 migr_hdr->migr_flags
			);
}


ttoc ( string, time )           /* time to character */

	char * string;
	VTime time;
{
	if ( eqSTime ( time.simtime, posinf.simtime ) )
		strcpy ( string, "+inf  " );
	else
	if ( eqSTime ( time.simtime, posinfPlus1.simtime ) )
		strcpy ( string, "+inf+1" );
	else
	if ( eqSTime ( time.simtime, neginf.simtime ) )
		strcpy ( string, "-inf  " );
	else
	if ( eqSTime ( time.simtime, neginfPlus1.simtime ) )
		strcpy ( string, "-inf+1" );
	else
		sprintf ( string, "%.2f", time.simtime );
}

ttoc1 (string, time)            /* time to character */
	char           *string;
	VTime           time;
{
	if (time.simtime == POSINF)
		strcpy (string, "posInf");
	else
	if (time.simtime == POSINF+1)
		strcpy (string, "posInf");
	else
	if (time.simtime == NEGINF)
		strcpy (string, "negInf");
	else
	if (time.simtime == NEGINF+1)
		strcpy (string, "negInf");
	else
		sprintf (string, "%.2f", time.simtime);
}

showocb_head ()
{
	dprintf ( "\n" );
	dprintf (
"Name          Phase   Limit  Ocb Ptr   Svt    IQ mem OQ mem SQ mem entry rstat EvPerm\n" );
}

ashowocb (o)
	Ocb *o;
{

	  showocb (*o);
}

showtypes ()
{
	int i;

	dprintf("State hdr is %d bytes\n",sizeof ( State ));
	dprintf("Type       	size		type area pointer\n");
	for ( i = 0; i < MAXNTYP; i++)
	{  
	  if ( type_table[i].type != NULL)
	  {
		  dprintf("%s   	%d		%x\n", type_table[i].type, type_table[i].statesize,type_table[i].typeArea);
	  }
	}  
}

showocb (o)
	Ocb            *o;
{
	char phase[12];
	char limit[12];
	char            svt[12];
	int             iqmem;
	int             oqmem;
	int             sqmem;
	char            centry[20];
	char            runstat[20];
/*
Name          Phase   Limit  Ocb Ptr   Svt    IQ mem OQ mem SQ mem entry rstat
nnnnnnnnnnnn ppppppp lllllll oooooooo sssssss iiiiii oooooo ssssss ccccc rrrrrr
*/
	ttoc (phase, o->phase_begin );
	ttoc (limit, o->phase_limit );
	ttoc (svt, o->svt);
	iqmem = returniqmem (o);
	oqmem = returnoqmem (o);
	sqmem = returnsqmem (o);
	get_enum ("centry", o->centry, centry);
	get_enum ("runstat", o->runstat, runstat);
	dprintf ( "%-12s %7s %7s %8x %7s %6d %6d %6d %5s %-6s %5d\n",
		o->name, phase, limit, o, svt, iqmem, oqmem, sqmem,
		centry, runstat, o->eventsPermitted );
}

Ocb            *general_ocb_by_name (name)
	char           *name;
{
	Ocb            *o;
	Objloc         *location;
	char phase_name[20];
	STime phase_time = NEGINF+1;

	sscanf ( name, "%s %lf", phase_name, &phase_time );

	if ((location = GetLocation(phase_name,newVTime(phase_time,0,0))) == 
				(Objloc *) NULL || location->node == -1)
	{
		if (name_hash_function(phase_name,HOME_NODE) != miparm.me)
				dprintf ("%s not found; try on node %d\n", phase_name,
						name_hash_function(phase_name,HOME_NODE));
		else
				dprintf ("%s does not exist\n",phase_name);
		return NULL;
	}
	if ((o = location->po) == NULL)
	{
		dprintf ("%s is on node %d\n", phase_name, location->node);
		return NULL;
	}
	return o;
}

Ocb * general_ocb_by_phase ( name, ptime )

	char        *name;
	STime       ptime;
{
	Ocb         *o;
	Objloc      *location;
	VTime       vtime;

	vtime = newVTime ( ptime, 0, 0 );

	if ( (location = GetLocation ( name, vtime ) ) == NULL
	||    location->node == -1 )
	{
		if ( name_hash_function ( name, HOME_NODE ) != miparm.me )
			dprintf ( "%s not found; try on node %d\n", name,
				name_hash_function ( name, HOME_NODE ) );
		else
			dprintf ( "%s does not exist\n", name );

		return ( NULL );
	}

	if ( (o = location->po) == NULL )
	{
		dprintf ( "%s at %f is on node %d\n", name, vtime.simtime,
				location->node);

		return ( NULL );
	}

	return ( o );
}

dm ( msg )

	Msgh ** msg;
{
	Ocb * o;

	if ( ( o = general_ocb_by_phase ((*msg)->rcver, (*msg)->rcvtim) ) != NULL )
	{
		if ( o->typepointer->displayMsg )
		{
			o->typepointer->displayMsg ( (*msg)->selector, (*msg) + 1 );
		}
		else
			dumpmsg ( *msg );
	}
	else
		dumpmsg ( *msg );
}

dst ( state )

	State ** state;
{
	Typtbl * type = (*state)->otype;

	int offset = type - type_table;

	if ( offset < 0 || offset >= MAXNTYP )
		return;

	if ( (*state)->otype->displayState )
	{
		(*state)->otype->displayState ( (*state) + 1 );
	}
	else
		dumpstate ( *state );
}

show_iq_by_name ( name )

	char * name;
{
	Ocb * o;

	if ( ( o = general_ocb_by_name ( name ) ) != NULL )
	{
		showiq ( o );
	}
}

show_iq_by_phase ( name, ptime )

	char * name;
	STime * ptime;
{
	Ocb * o;
	VTime vtime;

	vtime = newVTime ( *ptime, 0, 0 );

	if ( ( o = general_ocb_by_phase ( name, vtime ) ) != NULL )
	{
		showiq ( o );
	}
}

show_oq_by_name ( name )

	char * name;
{
	Ocb * o;

	if ( ( o = general_ocb_by_name ( name ) ) != NULL )
	{
		showoq ( o );
	}
}

show_oq_by_phase ( name, ptime )

	char * name;
	STime * ptime;
{
	Ocb * o;
	VTime vtime;

	vtime = newVTime ( *ptime, 0, 0 );

	if ( ( o = general_ocb_by_phase ( name, vtime ) ) != NULL )
	{
		showoq ( o );
	}
}

show_sq_by_name ( name )

	char * name;
{
	Ocb * o;

	if ( ( o = general_ocb_by_name ( name ) ) != NULL )
	{
		showsq ( o );
	}
}

show_sq_by_phase ( name, ptime )

	char * name;
	STime * ptime;
{
	Ocb * o;
	VTime vtime;

	vtime = newVTime ( *ptime, 0, 0 );

	if ( ( o = general_ocb_by_phase ( name, vtime ) ) != NULL )
	{
		showsq ( o );
	}
}

dump_ocb_by_name ( name )

	char * name;
{
	Ocb * o;

	if ( ( o = general_ocb_by_name ( name ) ) != NULL )
	{
		dumpocb ( o );
	}
}

dump_ocb_by_phase ( name, ptime )

	char * name;
	STime * ptime;
{
	Ocb * o;
	VTime vtime;

	vtime = newVTime ( *ptime, 0, 0 );

	if ( ( o = general_ocb_by_phase ( name, vtime ) ) != NULL )
	{
		dumpocb ( o );
	}
}


show_miq_by_name ( name )

	char * name;
{
	Ocb * ocb;

	if ( sendOcbQ )
	for ( ocb = nxtocb_macro ( sendOcbQ ); ocb != NULL;
		  ocb = nxtocb_macro ( ocb ) )
	{
		if ( namecmp ( name, ocb->name ) == 0 )
		{
			dprintf ( "Object %s - \n", ocb->name );
			showiq ( ocb );
		}
	}
}

show_moq_by_name ( name )

	char * name;
{
	Ocb * ocb;

	if ( sendOcbQ )
	for ( ocb = nxtocb_macro ( sendOcbQ ); ocb != NULL;
		  ocb = nxtocb_macro ( ocb ) )
	{
		if ( namecmp ( name, ocb->name ) == 0 )
		{
			dprintf ( "Object %s - \n", ocb->name );
			showoq ( ocb );
		}
	}
}

show_msq_by_name ( name )

	char * name;
{
	Ocb * ocb;

	if ( sendOcbQ )
	for ( ocb = nxtocb_macro ( sendOcbQ ); ocb != NULL;
		  ocb = nxtocb_macro ( ocb ) )
	{
		if ( namecmp ( name, ocb->name ) == 0 )
		{    
			dprintf ( "Object %s - \n", ocb->name );
			showsq ( ocb );
		}
	}
}

/* Added function to dump ocbs while in the migration queue. PLRBUG */

show_mocb_by_name ( name )

	char * name;
{
	Ocb * ocb;

	if ( sendOcbQ )
	for ( ocb = nxtocb_macro ( sendOcbQ ); ocb != NULL;
		  ocb = nxtocb_macro ( ocb ) )
	{
		if ( namecmp ( name, ocb->name ) == 0 )
		{    
			dprintf ( "Object %s - \n", ocb->name );
			dumpocb ( ocb );
		}
	}
}

dumpocb ( o )

	Ocb * o;
{
	char phase[12];
	char phase_end[12];
	char phase_limit[12];
	char svt[12];
	char c[20];

	ttoc (phase, o->phase_begin );
	ttoc (phase_end, o->phase_end );
	ttoc (phase_limit, o->phase_limit );
	ttoc (svt, o->svt);
	dprintf (" ****** OCB     *********[ %lx ]**********************\n", o);
	dprintf ("Name:        %-20s\n", o->name);
	dprintf ("Phase Begin: %20s\n", phase);
	dprintf ("Phase End:   %20s\n", phase_end);
	dprintf ("Phase Limit: %20s\n", phase_limit);
	dprintf ("Svt:         %20s,%d,%d\n",svt,o->svt.sequence1,o->svt.sequence2);
	dprintf ("Sb           %20x\n", o->sb);
	dprintf ("Cs:          %20x\n", o->cs);
	dprintf ("Ecount:      %20d\n", o->ecount);
	dprintf ("Ci:          %20x\n", o->ci);
	dprintf ("Co:          %20x\n", o->co);
	dprintf ("Sqh:         %20x\n", o->sqh);
	dprintf ("Oqh:         %20x\n", o->oqh);
	dprintf ("Iqh:         %20x\n", o->iqh);
	get_enum ("centry", o->centry, c);
	dprintf ("Centry:      %-20s\n", c);
	get_enum ("runstat", o->runstat, c);
	dprintf ("Runstat:     %-20s\n", c);
	get_enum ("control", o->control, c);
	dprintf ("Control:     %-20s\n", c);
	dprintf ("Type:   %-20s\n",o->typepointer->type );
	dprintf ("Rstate: %20x\n",o->rstate);
	if ( o->migrStatus != 0 )
	{
		dprintf ( "Migr Stat: %20x",o->migrStatus );
		dprintf ( " imsgs %d, omsgs %d states %d\n",o->num_imsgs, o->num_omsgs,
					o->num_states );
	}
	dprintf ("\n");

	drawline ();
}


dprintf (s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)
	Char *s;
	Long a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15;
{
#ifdef MARK3
#define printf _pprintf
#endif


#ifdef TRANSPUTER
#define printf _pprintf
#endif

	printf (s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
}

drawline ()
{
	dprintf (
			 "--------------------------------------------------------------------------------\n");
}

/* Print a load array gathered for dynamic load management. */

extern int migrPerInt;

showLoadArray ( loadArray )

	int loadArray[];
{
	int i,entries, entriesPerLine;


	entriesPerLine = 0;

	_pprintf ( "        node    util    node    util    node    util    node    util\n");
	for ( i = 0; i < tw_num_nodes; i++ )
	{
		dprintf ( "     %d:     %d", i, loadArray[i] );
		entriesPerLine++;

		if ( entriesPerLine > 3)
		{
			dprintf ( "\n" );
			entriesPerLine = 0;
		}
	}

	if (entriesPerLine != 0)
		dprintf ( "\n");
}

showHomeNode ( name )

char * name;
{
	int HomeNode;

	HomeNode = name_hash_function(name,HOME_NODE);

	dprintf ( "	Object:	%s		Home Node: %d\n", name, HomeNode );

	return ( HomeNode );
}

showListHdr ( listElement )
	List_hdr ** listElement;
{
	List_hdr          *l;

	l = *listElement;

   dprintf("prev:       %x\n", l->prev );
   dprintf("next:       %x\n", l->next );
   dprintf("size:       %d\n", l->size );
}

#if PARANOID

validState ( state )

	State       *state;

{
	int         i;
	List_hdr    *l;

	l = (List_hdr*)state - 1;
	if ((l->next->prev != l) || (l->size == 0))
		{       /* bad state header */
		_pprintf("Invalid state header: %x\n",state);
		tester();
		}
	if (state->address_table == NULL)
		return;
	l = (List_hdr*)(state->address_table) - 1;
	if ((l->next->prev != l) || (l->size == 0))
		{       /* bad address table header */
		_pprintf("Invalid address table header: %x\n",state->address_table);
		tester();
		}
	for (i = 0; i < l_size(state->address_table / sizeof(Address)); i++)
		{       /* loop through address table */
		if (state->address_table[i] == NULL)
			continue;
		if (state->address_table[i] == DEFERRED)
			continue;
		l = (List_hdr*)(state->address_table[i]) -1;
		if ((l->next->prev != l) || (l->size == 0))
			{   /* bad segment header */
			_pprintf("Invalid segment header: %x\n",state->address_table[i]);
			tester();
			}
		}
}  /* validState */

#endif
