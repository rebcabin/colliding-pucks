/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	tstrinit.c,v $
 * Revision 1.18  91/11/06  11:13:13  configtw
 * Fix DLM link errors on Suns.
 * 
 * Revision 1.17  91/11/04  10:47:48  pls
 * 1.  Implement allowNow(), setMaxFreeMsgs().
 * 2.  Add string parm to typeInit().
 * 3.  Change sense of propDelay().
 * 
 * Revision 1.16  91/11/01  09:53:22  reiher
 * critical path switching code, code to control migration by ratio, and an
 * option to run TWOS in a batch mode (no traps to tester, exit instead) (PLR)
 * 
 * Revision 1.15  91/08/08  13:02:55  reiher
 * added test for failure from mkocb to typeinit
 * 
 * Revision 1.14  91/07/22  15:43:15  configtw
 * Fix misspelling of MARK3.
 * 
 * Revision 1.13  91/07/22  14:33:43  configtw
 * Remove recv_q_limit stuff for Mark3
 * 
 * Revision 1.12  91/07/17  15:13:44  judy
 * New copyright notice.
 * 
 * Revision 1.11  91/07/09  15:33:27  steve
 * support for receive queue limit, object timing mode, and the changable
 * its a feature. miparm.me replaced with tw_node_num.
 * 
 * Revision 1.10  91/06/07  13:49:45  configtw
 * Don't allow pktlen to change.
 *
 * Revision 1.9  91/06/04  08:57:02  configtw
 * Set default number of migrations to 8.
 * 
 * Revision 1.8  91/06/03  13:36:18  configtw
 * Put lower limit on stack size.
 * 
 * Revision 1.7  91/06/03  12:27:18  configtw
 * Tab conversion.
 * 
 * Revision 1.6  91/05/31  14:23:05  pls
 * Set dlm on by default.
 * 
 * Revision 1.5  91/04/01  15:48:54  reiher
 * Fixes to routines controlling DLM parameters, and a new routine for a new
 * capability (toggling dlm graphics).
 * 
 * Revision 1.4  91/03/26  09:53:47  pls
 * 1.  Add support for TYPEINIT.
 * 2.  Modify hoglog code to use hlog.
 * 3.  Add hook for library support code.
 * 
 * Revision 1.3  90/08/27  10:46:29  configtw
 * Round objstksize to 8 byte boundary.
 * 
 * Revision 1.2  90/08/16  10:58:31  steve
 * put in a check to make sure max_acks was not larger than 1/2 num of buffs
 * 
 * Revision 1.1  90/08/07  15:41:31  configtw
 * Initial revision
 * 
*/
char tstrinit_id [] = "@(#)tstrinit.c   1.44\t10/2/89\t16:28:12\tTIMEWARP";


/*

Purpose:

		This module contains routines that initialize certain variables
		and tables used by the tester.  

Functions:

		set_max_acks(number) - set a limit on the number of acks allowed
						to be outstanding
				Parameters - int * number
				Return - Always returns zero

		set_objstksize(number) - set the size of objects' stacks
				Parameters - int * number
				Return - Always returns zero

		disable_message_sendback() - turn off message sendback
				Parameters - none
				Return - Always returns zero

		set_nostdout() - disable output of stdout messages
				Parameters - none
				Return - Always returns zero

		set_nogvtout() - disable output of gvt msgs
				Parameters - none
				Return - Always returns zero

		enable_mem_stats() - start keeping track of memory statistics
				Parameters - none
				Return - Always returns zero

		init_types() - copy the init, event, and term entry points from
						the application's process table into the types table
				Parameters - none
				Return - Always returns zero

Implementation:

		set_max_acks(), set_objstksize(), set_nostdout(),
		set_nogvtout(), and enable_memory_stats() all do pretty much
		what they sound like.  Typically, they are only called at the
		begining of a run, to set a parameter for the entire run.

		init_types() copies information about the various object
		types into the type_table[] array.
*/


#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"
#include "tester.h"

#if TWUSRLIB
extern void* twulib_init_type();
#endif
extern int      hlog;
extern VTime    hlogVTime;
extern int max_acks;
extern int max_neg_acks;
extern int maxFreeMsgs;
extern int recv_q_limit;
extern int its_a_feature;
extern int no_gvtout;
extern int mem_stats_enabled;
extern int manual_init();
extern int manual_event();
#define manual_term 0
extern int null_init();
extern int null_event();
extern int initing_type;
extern int interval;

Int		allowNowFlag = FALSE;
int     cancellation_penalty = 0;
int     cancellation_reward = 0;
int     peek_limit = 2;
STime   time_window;

#ifdef MARK3
extern VTime stdout_sent_time;
#endif

FILE * cpulog;
FILE * HOST_fopen ();

set_cpulog ()
{
	cpulog = HOST_fopen ( "cpulog", "w" );

	if ( cpulog == 0 )
		printf ( "can't open cpulog\n" );
}

STime cutoff_time = POSINF;

set_cutoff_time ( time )

	STime * time;
{
	cutoff_time = *time;
}

#ifdef EVTLOG

set_evtlog ()
{
	evtlog = TRUE;
}

Byte * evtlog_area;

set_chklog ()
{
	int i;

	for ( i = 0; i < MAX_TW_FILES; i++ )
	{
		if ( strcmp ( tw_file[i].name, "evtlog" ) == 0 )
			break;
	}

	if ( i < MAX_TW_FILES )
	{
		chklog = TRUE;
		evtlog_area = tw_file[i].area;
	}
	else
		_pprintf ( "evtlog file not found\n" );
}

#endif EVTLOG

void hoglog(hog_log_time)
	STime       *hog_log_time;
{
	hlog = TRUE;
	hlogVTime = newVTime(*hog_log_time,0,0);

}  /* hoglog */

allowNow()
/* Allow messages to be sent with receive time equal to now */

{
	allowNowFlag = TRUE;
}	/* allowNow */

set_time_window ( window )

	STime * window;
{
	time_window = *window;
}

set_max_acks ( number )

	int * number;
{
#ifdef BBN
	extern int number_of_buffers;

	if ( *number > 0 && *number <= number_of_buffers/2 )
#else
	if ( *number > 0 && *number <= MAX_ACKS )
#endif
		max_acks = *number;
	else
		_pprintf ( "Invalid Max_Acks. Left at %d\n", max_acks );
}

#if !MARK3
#define MAX_RECV_Q_LIMIT 100

set_recv_q_limit ( number )

	int * number;
{
	if ( *number > 0 && *number <= MAX_RECV_Q_LIMIT )
		recv_q_limit = *number;
	else
		_pprintf ( "Invalid recv_q_limit. Left at %d\n", recv_q_limit );
}
#endif

set_obj_time_mode ( number )

	int * number;
{
#ifdef MICROTIME
	if ( *number >= NOOBJTIME && *number <= WALLOBJTIME )
		object_timing_mode = *number;
	else
		_pprintf ( "Invalid object_timing_mode. Left at %d\n",
			object_timing_mode );
#endif
}

set_its_a_feature ( number )

	int * number;
{

/*
	if ( *number > 0 && *number <= MAX_RECV_Q_LIMIT )
*/
		its_a_feature = *number;
/*
	else
*/
	if ( tw_node_num == 0 )
		_pprintf ( "its_a_feature. set to %d\n", its_a_feature );
}

#if !BBN
set_max_neg_acks ( number )

	int * number;
{
	if ( *number > 0 && *number <= MAX_ACKS )
		max_neg_acks = *number;
	else
		_pprintf ( "Invalid max_neg_acks. Left at %d\n", max_neg_acks );
}
#endif

setMaxFreeMsgs ( number )
	int		*number;
{
	maxFreeMsgs = *number;
}	/* setMaxFreeMsgs */

set_objstksize ( number )

	int * number;
{
	if ( *number < objstksize )
	{
		_pprintf ( "Can't decrease stack size from %d to %d\n",
			objstksize, *number );
		return;
	}

	objstksize = *number;
	objstksize = (objstksize + 7) & ~7; /* round to 8 byte boundary */
}

set_pktlen ( number )

	int * number;
{
	if ( *number < MAXPKTL )
	{
		pktlen = MAXPKTL;
		if ( tw_node_num == 0 )
			_pprintf( "pktlen %d too small - set to %d\n", *number, MAXPKTL );
	}
	else
	if ( *number > MAXPKTL )
	{
		pktlen = MAXPKTL;
		if ( tw_node_num == 0 )
		   _pprintf ( "pktlen %d too large - set to %d\n", *number, MAXPKTL );
	}
	else
		pktlen = *number;

	msgdefsize = sizeof(Msgh) + pktlen;
}

set_penalty ( number )

	int * number;
{
	cancellation_penalty = *number;
}

set_reward ( number )

	int * number;
{
	cancellation_reward = *number;
}

set_plimit ( plimit )

	int * plimit;
{
	peek_limit = *plimit;
}

disable_message_sendback ()
{
	no_message_sendback = TRUE;
}

set_nostdout ()
{
	no_stdout = TRUE;

#ifdef MARK3
	stdout_sent_time = posinf;
#endif
}

set_nogvtout ()
{
	no_gvtout = TRUE;
}

enable_mem_stats ()
{
	mem_stats_enabled = TRUE;
}

init_types ()
{
	extern ObjectType * objectTypeList[];

	ObjectType ** ob;
	Typtbl * tw;

	type_table[0].type = "manual";
	type_table[0].init  = manual_init;
	type_table[0].event = manual_event;
	type_table[0].term  = manual_term;
	type_table[0].statesize = 100;

	/* If the NULL object type is moved to any other place in the type
		table, be sure to change the definition of NULL_TYPE in twsys.h
		to point to the new location. */

	type_table[1].type = "NULL";
	type_table[1].init = null_init;
	type_table[1].event = null_event;
	type_table[1].statesize = 0;

	tw = &type_table[2];

	ob = objectTypeList;

	while ( *ob )
	{
		tw->type = (*ob)->type;

		if ( strlen ( tw->type ) >= TOBJLEN )
		{
			twerror ( "init_types: type name %s too long; only %d characters allowed\n", tw->type, TOBJLEN );
			tw_exit ();
		}

		tw->init  = (*ob)->init;
		tw->event = (*ob)->event;
		tw->term  = (*ob)->term;
		tw->displayMsg  = (*ob)->displayMsg;
		tw->displayState  = (*ob)->displayState;
		tw->statesize = (*ob)->statesize;
		tw->initType = (*ob)->initType;
		tw->typeArea = NULL;
#if TWUSRLIB
		tw->libTable = twulib_init_type(tw,*ob);        /* do library init */
#endif

		tw++;
		ob++;
	}
}  /* init_types */

void typeinit(type_name,msg)
	Type        type_name;
	Name		msg;
{
	Ocb         *o;
	State       *s;
	Typtbl      *tp;

	tp = find_object_type(type_name);   /* check type validity */
	if (tp == NULL)
		{    /* invalid type */
		twerror("Invalid type: %s",type_name);
		}
	else
		{    /* call the type initialization routine */
		if (tp->typeArea)
			twerror("Memory already allocated for type: %s",type_name);
		else
			{  /* do the type init */

			/* make the phony ocb to support i/o */ 
			o = mkocb();
			strcpy(o->name,"TYPEINIT");
			s = (State *) m_create(sizeof(State),neginf,CRITICAL);
			clear(s,sizeof(State));
			o->sb = s;          /* point to phony state */
			xqting_ocb = o;

			/* allow type malloc and do the call */
			initing_type = TRUE;
			tp->typeArea = (*tp->initType)(msg);	/* save allocated area */

			/* reset flag and dump ocb */
			initing_type = FALSE;
			nukocb(o);
			xqting_ocb = NULLOCB;
			}
		}
}  /* typeinit */

extern STime gvt_sync;

set_gvt_sync ( number )

	STime * number;
{
	gvt_sync = *number;
}

extern int tw_num_nodes;
extern int node_offset, node_limit;
int subcube_num, num_subcubes;

subcube ( node, number, config )

	int * node;
	int * number;
	char * config;
{
	char * argv[2];

	num_subcubes++;

	if ( node_offset != 0 )
		return;

	node_limit = tw_node_num;

	if ( tw_node_num < *node )
		return;

	subcube_num = num_subcubes;

	node_offset = *node;

	tw_num_nodes = *number;

	node_limit = node_offset + tw_num_nodes - 1;

	tw_node_num -= node_offset;

	miparm.me = tw_node_num;
	miparm.maxnprc = tw_num_nodes;

	if ( tw_node_num == 0 )
	{
#ifdef MARK3

	  /* I'm dubious about what's going on here.  At any rate, msg type
			  10 is an ISLOG_DATA message, so we probably should use
			  that symbol here. PLR */

	  send_message ( 0, 0, CP, ISLOG_DATA );
#endif
		argv[1] = config;
		init_command ( argv );
	}
}

#ifdef MARK3
extern int interrupt_disable;

disable_interrupts ()
{
	interrupt_disable = 1;
}

enable_interrupts ()
{
	interrupt_disable = 0;
}
#endif

set_prop_delay ( delay )

	double *delay;
{
	if (*delay > 1.0)
		{
		prop_delay = TRUE;

		delay_factor = *delay - 1.0;
		}
}

#if DLM
int dlm = TRUE;
#else
int dlm = FALSE;
#endif

int migrGraph = FALSE;

#ifdef DLM

int batchRun = FALSE;

setBatch()
{
	batchRun = TRUE;
}

set_dlm ()

{
	if ( dlm == FALSE)
	{
		_pprintf("cannot enable dynamic load management while calculating critical path\n");
	}
	else
		dlm = FALSE;
}

float minUtilDiff = .1;

set_threshold ( threshold )

int * threshold;
{
	if ( threshold <= 0 )
	{
		_pprintf ( "illegal value for dlm threshold %d; threshold not reset\n",
			threshold);
	}
	else
	{
		minUtilDiff = ((float)*threshold)/100.;
	}
}

extern float minUtilRatio ;

setMigrRatio ( ratio )

int * ratio;
{
    if ( *ratio < 0 )
	{
		_pprintf ( "illegal value for migration ratio %d; default not changed\n", 
			ratio );
	}
	else
	{
		minUtilRatio = (((float)*ratio)/100.) + 1;
		if ( tw_node_num == 0 )
			_pprintf ( "migration ratio set to %f\n", minUtilRatio ) ;
	}
}

int migrPerInt = 8;

setNumMigrs ( numMigr )

int * numMigr;
{
	/* If the user asked for more migrations than the number of overloaded and
		underload node pairs possible, cut it down to that. */

	if ( (  *numMigr ) * 2  > tw_num_nodes)
	{
		migrPerInt = tw_num_nodes / 2;

		if ( tw_node_num == 0 )

				_pprintf("Fewer migrations (%d) permitted than requested(%d)\n",
						 migrPerInt, *numMigr);
	}
	else
	{
		migrPerInt = *numMigr;
	}

	/* If the number of migrations requested won't fit into the message
		used to pass the info around, cut it down so it will. */

	if ( migrPerInt > MAX_MIGR )
	{
		migrPerInt = MAX_MIGR;
		if ( tw_node_num == 0 )
				_pprintf("Only %d migrations permitted, hard coded limit\n",
						migrPerInt);
	}

	if ( tw_node_num == 0 )
		_pprintf ("%d migrations per cycle permitted\n",migrPerInt);

}

extern char chooseStrat;

setChooseStrat ( chooseIn )

int * chooseIn;
{
	char chooseInChar;

	chooseInChar = ( char ) *chooseIn;

	if ( chooseInChar == BEST_FIT || chooseInChar == NEXT_BEST_FIT )
	{
		chooseStrat = chooseInChar;
	}
	else
	{
		if ( tw_node_num == 0 )
				_pprintf( "Illegal choose code %d, defaulting to BEST_FIT\n",
						*chooseIn);
		chooseStrat = BEST_FIT;
	}
}

extern char splitStrat;

setSplitStrat ( splitIn )

int * splitIn;
{
	char splitInChar;

	splitInChar = ( char ) *splitIn;

	if ( splitInChar == MINIMAL_SPLIT || splitInChar == NEAR_FUTURE ||
				splitInChar == NO_SPLIT || splitInChar == LIMIT_EMSGS )
	{
		splitStrat = splitInChar;
	}
	else
	{
		if ( tw_node_num == 0 )
				_pprintf( "Illegal split code %d, defaulting to NEAR_FUTURE\n",
						*splitIn);
		splitStrat = NEAR_FUTURE;
	}
}

extern int maxMigr;

setMaxMigr ( migrations )

int * migrations;
{
	if ( *migrations < 0 )
	{
		if ( tw_node_num == 0 )
				_pprintf ( "Illegal # of simulataneous migrations = %d; ignored\n",
						*migrations);
		return ;
	}
	else
	{
		_pprintf("setting maxMigr to %d\n",*migrations);
		maxMigr = *migrations;
	}

	if ( tw_node_num == 0 )
		_pprintf ( "Maximum simultaneous migrations per node = %d\n", 
						maxMigr );

}

/* Set the number of cycles at start of run during which no dlm is permitted. */

extern int idleDlmCycles;

setIdleDlmCycles ( cycles )

int * cycles;
{

	if ( *cycles < 0 )
	{
		if ( tw_node_num == 0 )
				_pprintf ( "Illegal # of cycles without dlm = %d; ignored\n",
						*cycles );
		return ;
	}

	idleDlmCycles = *cycles;
}

extern int dlmInterval;

setDlmInt ( interval )

int * interval;
{

	if ( *interval <= 0 )
	{
		if ( tw_node_num == 0 )
				_pprintf ( "Illegal dlm interval setting = %d; ignored\n",
						*interval );
		return ;
	}

	dlmInterval = *interval;
}

/*  Toggle whether migration graphics data will be stored in the migration
	  log or just normal information. */

setMigrGraph()
{
	if ( migrGraph )
	{  
	  migrGraph = FALSE;
	}  
	else
	{  
	  migrGraph = TRUE;
	}  
}
#endif DLM

