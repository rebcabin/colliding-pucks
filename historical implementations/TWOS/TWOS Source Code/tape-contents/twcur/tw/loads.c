/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	loads.c,v $
 * Revision 1.12  92/02/25  10:34:14  reiher
 * changed ifdef 0 to if 0
 * 
 * Revision 1.11  91/12/30  09:05:23  reiher
 * Changed migration policy so that no cost would be assigned to a migration,
 * when determining if it was worthwhile.  (Old method available by 
 * conditional compilation using MIGRATIONCOSTING define.)
 * 	
 * 
 * Revision 1.10  91/12/30  08:56:02  reiher
 * Removed debugging code and made small change for lower load management
 * overhead.
 * 
 * Revision 1.9  91/12/27  08:43:44  reiher
 * Added code to support ratio migration
 * 
 * Revision 1.8  91/11/01  09:39:25  reiher
 * Added code to make migration decisions on ratios of utilizations instead of
 * differences (PLR)
 * 
 * Revision 1.7  91/07/17  15:09:14  judy
 * New copyright notice.
 * 
 * Revision 1.6  91/07/09  13:59:29  steve
 * Added MicroTime support for Sun. Replaced miparm.me with tw_node_num
 * 
 * Revision 1.4  91/06/03  14:26:35  configtw
 * Change #ifdef 0 to #if 0
 * 
 * Revision 1.3  91/06/03  12:24:36  configtw
 * Tab conversion.
 * 
 * Revision 1.2  91/04/01  15:38:51  reiher
 * Several bug fixes to support dynamic load management.
 * 
 * Revision 1.1  90/08/07  15:39:57  configtw
 * Initial revision
 * 
*/

/*      Copyright (C) 1989, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */


#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"

/* Only temporary; remember to remove this include later. */
#include "tester.h"

/*  minUtilRatio is used to determine how much of a difference in utilization
	ratios will be permitted before trying to migrate.  The ratio is figured
	by dividing the higher by the lower, so it will always be at least one.*/


float minUtilRatio = 2.0;
#ifdef DLM


#define EMPTY_ENTRY -5000

#define PER_MIGRATION_COST 0

/* This variable specifies how many load intervals to wait before starting to
		migrate objects. */

int idleDlmCycles =  3;

extern int dlm;
extern float minUtilDiff;
extern int migrPerInt;
extern int undoneWork;
extern int blockObjects;
extern float undoneUtil;

int migrFlag = 0;
int loadCount = 0;

float nodeUtil = EMPTY_ENTRY;
int myLoadArray[MAX_MIGR];
int loadArray[MAX_MIGR];
int updateReceived;
int loadImbalance = 0;
int noPhaseChosen = 0;
int ratioMigrate = FALSE;
int migrateCandidate;

extern int myNum;
extern int numNodes;
extern int numFrom, numTo, from[2], to[2];
extern int Out0, Out1;

extern int migrGraph;

int numLoadArrive  = 0;

/* When the load management timer goes off (it is only set on node 0, so it
		only goes off there), start up a load management cycle. */

FUNCTION loadinterrupt ()
{
  Debug
	loadStart ( loadArray );
	dispatch ( );
}


/*  At system initialization time, have node 0 put unused flags in certain
		arrays, and set the load timer to go off. */

FUNCTION loadinit ()
{
	int i;

  Debug

	if ( miparm.me == 0 )
	{
		for ( i = 0; i < MAX_MIGR; i++ )
		{
			loadArray [i] = EMPTY_ENTRY;
			myLoadArray [i] = EMPTY_ENTRY;
		}

#ifdef MICROTIME
		schedule_next_dlm();
#else
		timlint ( loadinterrupt );
#endif

		if ( tw_num_nodes > MAX_MIGR )
		{
				_pprintf("more nodes (%d) than permitted migrations (%d)\n",
								tw_num_nodes, MAX_MIGR);
				tester();
		}
	}
}


/* Send a load message.  Get a system buffer, copy in relevant information,
		copy in the load array provided as a parameter, and call sysmsg ()
		to send it off. */

FUNCTION loadmessage ( type, time, dest, loadArray )

	Int         type;
	VTime       time;
	Int         dest;
	int         loadArray[];
{
	Msgh        *p;
	Loadmsg     *q;
	int i;

  Debug



	p = sysbuf (); /*SFB maybe sometimes output_buf()??? */
	q = (Loadmsg *) (p + 1);
	q->msgtype = type;
	q->sender = miparm.me;
	q->time = time;

	for (i = 0 ; i < MAX_MIGR; i++)
	{
		q->util[i] = loadArray[i];
	}


	sprintf ( p->snder, "LOAD%d", miparm.me );
	sprintf ( p->rcver, "LOAD%d", dest );
	p->sndtim = gvt;
	p->rcvtim = time;

	sysmsg ( LOADSYS, p, sizeof (Loadmsg), dest );
}

/* Handle an incoming load message.  If it's a LOADSTART message, call
		loadStart ().  If it's a LOADUPDATE message, call loadUpdate (). */

FUNCTION loadproc ( load_message )

	Loadmsg     *load_message;
{

  Debug


	switch ( load_message->msgtype )
	{
		case LOADSTART:         /* command to start a local load calculation */
			loadStart ( load_message->util );
			break;

		case LOADUPDATE:                /* announcement of new load values */
			loadUpdate ( load_message->util );
			break;

		default:
			twerror ( "Unknown load message type %d, msg ptr %x\n",
						load_message->msgtype, load_message);
			tester();
			break;
	}

}

/* The first phase of a load management cycle.  If we are node 0, and dlm
		isn't on at the moment, just set up another interrupt.  Otherwise,
		calculate the local effective utilization and put it in a local
		load array.  Depending on where we are in the graph describing how
		messages are passed in this protocol, either copy the incoming array 
		into a local array and wait for the second incoming message, or
		send the array out to the node(s) further along the graph.  Or, if
		this is the last stop in the first phase of the protocol, start the
		second phase, by calling loadUpdate(). */

FUNCTION loadStart ( loadArray )

	int loadArray[];
{
	int i;

  Debug

/* If the dlm flag isn't on, don't bother sending out a bunch of messages to
		gather data. */


	blockObjects = FALSE;

	if ( !dlm  || leVTime ( gvt, neginfPlus1 ) )
	{
		if ( miparm.me == 0 )
		{
#ifdef MICROTIME
			schedule_next_dlm();
#else
			timlint ( loadinterrupt );
#endif
		}

		return;
	}

	updateReceived = 0;

	if ( nodeUtil <= -4999 )
	{
		char buff[80];

#if 0
		/* Print out undoneWork to the migration log. */

		sprintf(buff, "		%d: undone work = %d\n", tw_node_num, undoneWork);

		send_to_IH ( buff, strlen ( buff ) + 1, MIGR_LOG );
#endif 0
		
		nodeUtil = calculateUtilization();

		/* undoneUtil is set as a side effect of calculateUtilization(). */

		if ( undoneUtil > nodeUtil )
		{
			blockObjects = TRUE;
		}	

		/* Zero out undoneWork for the next cycle */

		undoneWork = 0;


		if ( migrGraph )
		{
			sprintf ( buff, "Util %d %f\n", tw_node_num, nodeUtil );
		}
		else
		{
			sprintf ( buff, "   %d %d %f \n", tw_node_num,loadCount,nodeUtil );
		}

		send_to_IH ( buff, strlen ( buff ) + 1, MIGR_LOG );

		loadArray[miparm.me] = (int) (100 * nodeUtil);
	}


	if ( numFrom == 2 )
	{
		if ( numLoadArrive == 0 ) 
		{

		/*  Copy the incoming loadArray into the temporary local array.
				Then fit the local information into place. */

			for ( i = 0; i < tw_num_nodes ; i ++ )
			{
				myLoadArray[i] = loadArray[i];
			}

			myLoadArray[miparm.me] = (int) (100 * nodeUtil);

			numLoadArrive = 1;
			return;
		}
		else
		{
			/* Merge the information saved from the first incoming message
				with the new load data. */

			for ( i = 0; i < tw_num_nodes ; i ++ )
			{
				if ( loadArray[i] != EMPTY_ENTRY )
				{
					myLoadArray[i] = loadArray[i];
				}

			}

			numLoadArrive = 0;
		}
	}
	else
	{
		/*  Copy the incoming loadArray into the temporary local array.
				Then fit the local information into place. */

		 for ( i = 0; i < tw_num_nodes ; i ++ )
		 {
				myLoadArray[i] = loadArray[i];
		 }

		 myLoadArray[miparm.me] = (int) (100 * nodeUtil);
	}

	if ( numTo == 2 )
	{
		loadmessage ( LOADSTART, gvt, to[0], myLoadArray );
		loadmessage ( LOADSTART, gvt, to[1], myLoadArray );
	}
	else if ( numTo == 1 )
	{
		loadmessage ( LOADSTART, gvt, to[0], myLoadArray );
	}
	else /* we are the last one */
	{

/*
showLoadArray ( myLoadArray );
*/
		loadUpdate ( myLoadArray );
	}
}

/* The second phase of the load passing protocol.  At this point, the incoming
		load array contains a complete table of the effective utilizations
		of all nodes.  Pass the table on to anyone downstream in the protocol's
		tree, then deal with any local decisions needed.  Check to see if
		this node is one of the overloaded nodes that is supposed to perform
		a migration.  If so, choose something and send it to the underloaded
		partner.  */

FUNCTION loadUpdate ( loadArray )

	int  loadArray[];
{

#if 0
	/*  loadTempArray[] is a temporary debugging structure. */
	int loadTempArray[128];
#endif 0
	extern int qlog;
	int i, j;
	int order = 0;

  Debug

/* If we have received an update message already, ignore any others that come
		in.  It should be identical to the one we got, we already acted on
		that one, and we also sent it on to anyone who should get it. */

	if ( updateReceived )
	{
		return;
	}

	updateReceived = 1;

	if ( numFrom == 2 )
	{
		loadmessage ( LOADUPDATE, gvt, from[0], loadArray );
		loadmessage ( LOADUPDATE, gvt, from[1], loadArray );
	}
	else if ( numFrom == 1 )
	{
		loadmessage ( LOADUPDATE, gvt, from[0], loadArray );
	}

#if 0
/* instead of the B'Cast */

	if ( Out0 < numNodes )
	{
		loadmessage ( LOADUPDATE, gvt, Out0, loadArray );
	}

	if ( Out1 < numNodes )
	{
		loadmessage ( LOADUPDATE, gvt, Out1, loadArray );
	}
#endif 0


	if ( qlog )
		quelog_entry ( nodeUtil );
 

	nodeUtil = EMPTY_ENTRY;

	if ( miparm.me == 0 && gvt.simtime < POSINF )
	{
/*
		showLoadArray( loadArray );
		tester();
*/
	}

	if ( dlm  && loadCount > idleDlmCycles && gvt.simtime < POSINF )
	{
		VTime migrTime;
		int migrations,i;
 
		if ( migrPerInt * 2 > tw_num_nodes )
		{
				migrations = tw_num_nodes/2;
		}
		else
		{
				migrations = migrPerInt;
		}

		/* Look through the load array to find if you are one of the heavily
				loaded nodes that should be migrating something off.  Do this
				by first finding the highest/lowest pair, discard them, then
				the next pair, etc., until you've gone through all permitted
				pairs, or found a pair that you are a member of.  If the local
				node is the highly loaded member, look for something to 
				migrate. 
		*/

#if 0
/*  Temporary code for debugging */
for ( i = 0; i < tw_num_nodes; i++ )
{
	loadTempArray[i] = loadArray[i];
}
#endif 0

		for (i = 0; i < migrations ; i ++)
		{
			int max, min;
			int highNode, lowNode;

#define LOW_USED 4000
#define HIGH_USED -4000

			max = HIGH_USED + 1 ;
			min = LOW_USED - 1 ;

			for ( j = 0; j < tw_num_nodes ; j++ )
			{
				if ( loadArray[j] > max && loadArray[j] < LOW_USED )
				{
					max = loadArray[j];
					highNode = j;
				}
				
				if ( loadArray[j] < min && loadArray[j] > HIGH_USED )
				{
					min = loadArray[j];
					lowNode = j;
				}
			}

			if ( lowNode == highNode )
			{
				/*  All remaining nodes have the same utilization. */

				break;
			}
			else
			if ( max > HIGH_USED + 1 && min < LOW_USED - 1 )
			{

				loadArray[highNode] = HIGH_USED ;
				loadArray[lowNode] = LOW_USED ;
				order++;
			}
			else
			{
				/* No more pairs available. */

				break;
			}   

			if ( highNode == miparm.me )
			{
				int cost;

				/*  Each migration puts a strain on the system, so the
						pair of nodes with the greatest difference is more 
						likely to migrate a phase than the pair with the 
						twentieth greatest difference.  This is handled by
						adding a cost into each decision, based on the
						number of the paired nodes in the order of pairs.
				*/

#ifdef MIGRATIONCOSTING
				cost = order * PER_MIGRATION_COST;
#else
				cost = 0;
#endif MIGRATIONCOSTING

/*
_pprintf("I am a high node, paired with node %d\n",lowNode );
*/
				migrateCandidate = FALSE;

				/*  If the ratio of the maximum and minimum chosen utilizations
					is higher than some minimum value, this pair of nodes
					is a candidate for migration.  Try to find some phase
					to move.  If the minimum utilization is zero, or it is
					less than 1, don't calculate the ratio, just look for 
					something to migrate.   There may be weirdness if both
					the high and low nodes have negative utilizations, so
					don't even try, in that case.  */

				/*  We may also want to check to see if the max utilization
					is so low that any difference between min and max must
					be in the noise.  We don't check that, at the moment. */

				if ( ratioMigrate == TRUE )
				{
					if ( !( min <= 0 && max <= 0 ) && 
					  ( min == 0 ||
					  ( (float) max/min ) > ( (float) minUtilRatio )  ))
						migrateCandidate = TRUE;
				}
				else
				{
				/*  If the difference between the maximum and minimum chosen 
					utilizations is higher than some minimum value, this pair 
					of nodes is a candidate for migration.  Try to find some 
					phase to move. */

					if ( max - min > ( cost + (int ) ( 100 *minUtilDiff) ) )
						migrateCandidate = TRUE;
				}

				if ( migrateCandidate  = TRUE )
				{
					Ocb     *candidate;
					float   utilization;
						
					loadImbalance++;
					if ( highNode == lowNode )
					{
						twerror ( "loadUpdate: node %d matched with itself;  about to select object for migration\n",
									lowNode );
						_pprintf("	min = %d, max = %d\n",min,max);
#if 0
						showLoadArray( loadTempArray );
#endif 0
						tester();
					}
					utilization = (float) min/100. ;

					candidate = chooseObject ( utilization );
 
					/*  If chooseObject found something, move it. */

					if ( candidate != NULL )
					{
						migrTime = candidate->phase_begin;
/*
					_pprintf ( "migrating %s, VT %2f, util %2f to node %d\n",
								candidate->name,migrTime.simtime,
								candidate->stats.utilization,lowNode );
*/

						move_phase ( candidate, lowNode );
					}
					else
					{
						noPhaseChosen++;
/*
	_pprintf("no candidate  max %d min %d dif %f\n",
						   max,  min, ( 100 *minUtilDiff) );
*/

					}
				}
				else
				{
/*
_pprintf("utilization difference %f too small (max = %d, min = %d)\n", (float) (max-min), max, min );
*/
				}

				/* Only one migration per node, so if you've already gotten
						through this code, you don't need to examine the
						load array any further.
				*/

				break;
			}      
			else
			{
				if ( lowNode == tw_node_num )
				{
					/* If we're a low node, we'll not also be a high node,
						so break out of the loop. */

					break;
				}
			}
		}
	}
	else
	if ( miparm.me == 0 )
	{  
/*
		_pprintf("either gvt >= POSINF or loadCount too low to migrate\n");
*/
	}

	loadCount++;

	for (i = 0; i < tw_num_nodes; i++ )
	{

		loadArray[i] = EMPTY_ENTRY;
	}

	/* Start up a new cycle, if you are the load master. */

	if ( miparm.me == 0 )
	{
#ifdef MICROTIME
		schedule_next_dlm();
#else
		timlint ( loadinterrupt );
#endif
	}
}

FUNCTION turnRatioMigrationOn ( )
{
	if ( ratioMigrate != TRUE )
	{
		if ( tw_node_num == 0 )
			_pprintf("Dynamic load management working on utilization ratios\n");

		ratioMigrate = TRUE;
	}
}

#endif DLM

