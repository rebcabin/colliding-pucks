/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	cubeio.c,v $
 * Revision 1.18  91/11/08  14:07:32  configtw
 * Let's not divide cputime twice by 1,000,000.
 * 
 * Revision 1.17  91/11/06  11:10:24  configtw
 * Fix DLM errors on Suns.
 * 
 * Revision 1.16  91/11/04  10:14:44  pls
 * Add changes for TWSIM version of cubeio.c.
 * 
 * Revision 1.15  91/11/01  09:33:23  reiher
 *  Added printouts to XL_STATS for critical path stuff, object location stats,
 * special timing printouts, and free msg pool printouts (PLR)
 * 
 * Revision 1.14  91/07/17  15:07:54  judy
 * New copyright notice.
 * 
 * Revision 1.13  91/07/09  13:32:18  steve
 * MicroTime support for Sun. XL_STATS now includes names of which Sun
 * machines were which node. Version updated to 2.5.1
 * 
 * Revision 1.12  91/06/03  14:27:53  configtw
 * Fix code to work with Sun.
 *
 * Revision 1.11  91/06/03  12:23:58  configtw
 * Tab conversion.
 *
 * Revision 1.10  91/04/01  15:35:51  reiher
 * Added some dynamic load management statistics to the XL_STATS file.  Also
 * added stuff to show whether Butterfly nodes went to sleep during the run.
 * 
 * Revision 1.9  91/03/28  16:15:13  configtw
 * Put hoglog output in XL_STATS.
 * 
 * Revision 1.8  91/03/25  16:07:02  csupport
 * Got rid of unused sequal field
 * 
 * Revision 1.7  90/12/12  10:05:42  configtw
 * change version number
 * 
 * Revision 1.6  90/09/18  13:29:00  configtw
 * divide comtime as appropriate for each port
 * 
 * Revision 1.5  90/08/27  10:23:13  configtw
 * Changed cycle time to committed time
 * 
 * Revision 1.4  90/08/16  11:01:25  steve
 * also in revision 1.2: the XL_STATS file now gets a line with
 * tw node #, cluster node #, mach node #, physical node #
 * from everynode in the BF_MACH version
 * 
 * Revision 1.3  90/08/16  10:29:53  steve
 * updated version number to 2.4.1
 * 
 * Revision 1.2  90/08/09  16:37:43  steve
 * Now stforw (states forwarded) field gets dumped to xlstats file
 * 
 * Revision 1.1  90/08/07  15:38:10  configtw
 * Initial revision
 * 
*/
char cubeio_id [] = "@(#)cubeio.c       1.47\t10/2/89\t14:15:56\tTIMEWARP";


/*

Purpose:

		cubeio.c contains routines that format statistics produced
		by a Time Warp run, and dumps them.  The statistics are
		currently produced in a format that is usable by EXCEL,
		a Macintosh spreadsheet program.  The first three routines
		in this module create a header, a body line, and a trailing
		line for the statistics.  These statistics are then sent to
		the IH, which is node 128, in order to be output.  These
		routines are run on all nodes in the system.  They require some
		special actions by the 0th node.

		The routines output statistics in Excel format--with tabs
		between each field. Tab stops are assumed to be set every 9 
		spaces, with the fields being 6 spaces long (so there are three 
		blank spaces between the end of one field and the beginning of 
		another). Four files are used; two contain the object-by-object 
		statistics, and the other two contain the processor-by-processor 
		statistics.  The large number of files is required because the 
		statistics for a given entity (object or processor) must fit on 
		one line. There is one set of four files per node; so if you have 
		5 nodes, there will be 5 * 4 = 20 files. 


Functions:

		Excel_body1(o) - format and send a line of statistics
				Parameters - Ocb *o
				Returns - Always returns 0

		Excel_head(elapsed_time) - format and send statistics header lines
				Parameters - float elapsed_time
				Returns - Always returns 0

		Excel_tail() - format and send trailing statistics lines
				Parameters - none
				Returns - Always returns 0

		dump_stats(elapsed_time) - prepare and send statistics for output
				Parameters - float elapsed_time
				Returns - Always returns 0
****** how does it return zero when there is no return statement ?? ****

		void record_obj_stats(tottime,interv) - dump_stats equivalent for
				the sequential simulator.
				Parameters - double elapsed time
					   - double recording intervsl
				Returns - none, void function


Implementation:

		Excel_body() is called once for each object on a node.  If the
		object is the "stdout" object, then its committed event bundle
		field is set to 0.  Next, some statistics keeping track of
		some counts for the entire node are updated to include the
		partial counts from this object.  (Overall events, event
		messages, and rollbacks are counted this way.)  A string is
		set up to contain all of the object's statistics, and then
		send_to_IH() is called to ship it off.

		Excel_head() is called for all nodes during the dump_stats()
		routine, but only does real work when executed on node 0.
		On that node, it creates two header lines, and sends them to 
		IH.  Three calls to send_to_IH() are necessary, presumably
		because of message lengths.  

		Excel_tail() is executed for all nodes.  It is run after
		Excel_body has been run on all objects on a node.  Excel_tail()
		takes the accumulated per node counts, and some other node-
		related statistics, bundles them into two messages, and sends
		the messages to the IH.

		dump_stats() is the engine driving the various Excel functions.
		It calls Excel_head(), runs through each object in the local
		object list calling Excel_body1(), and then calls Excel_tail().
		It also does two pieces of unused busywork.  It changes gvt
		into a more pleasing visual format, and it keeps count of
		the number of lines that it has printed.  (But it doesn't
		do the latter correctly.)

		record_obj_stats() is called from the timewarp simulator after
		the run is finished. it calls Excel_head to record the header and
		then calls Excel_body for each object. Because the simulator has
		no Ocb and stores per-object data in a structure called obj_bod,
		it copies the data into the single Ocb in this file before calling
		Excel_body.  Thus Excel_head and Excel_body are identical for TW
		and the sequential simulator.  To add new stats: if they exist in
		the simulator they should be copied into the Ocb by this function.
		Currently the simulator does not execute Excel_tail because all
		of its data is zero.

		The parameter <interval> is required by the special object statistics
		recording that can be done by the simulator (see SPEC_OBJSTATS) This
		variable is normally 0.0 and everything is only done once at the end
		as stated above.
*/

#include <stdio.h>
#include "twcommon.h"
#if SIMULATOR
#include "tws.h"
#include "twsd.h"
extern char *strcpy();
#if BF_MACH
#define BBN
#endif
#endif  /* SIMULATOR */

#include "twsys.h"
#include "machdep.h"
#ifndef SIMULATOR
#include "tester.h"
#endif

extern char * config_file;
extern int config_eposfs;
extern Ulong chit,cmiss;
extern int      hlog;
extern int      maxSlice;
extern Ocb*     maxSliceObj;
extern VTime    maxSliceTime;
extern int phaseNaksSent ;
extern int phaseNaksRecv ;
extern int stateNaksSent ;
extern int stateNaksRecv ;
#ifdef BF_MACH
extern int weLooped;
#endif
extern int max_msg_buffers_alloc ;
extern int num_msg_buffs_released_from_pool;

#if DLM
extern int loadImbalance;
extern int noPhaseChosen;
extern int loadCount;
extern int objectBlocked;
#endif

#if TIMING
extern int find_count;
#endif

#ifdef SIMULATOR
/* The following variables are in this file but are not used by the sequential
simulator. defined here to make the loader happy */
Ulong        chit;
Ulong        cmiss;
int          config_eposfs;
int          didNotSendState;
int          dupStateSend;
int          fstatein;
int          fstateout;
int          gvtcount;
int          hlog;
int          maxSlice;
Ocb          *maxSliceObj;
VTime        maxSliceTime;
int          migrin;
int          migrout;
int          num_objects;
int          phaseNaksRecv;
int          phaseNaksSent;
int          rfaults;
int          stateNaksRecv;
int          stateNaksSent;
/* accessed when using simulator */
int          tw_node_num;
char	     sunname[20];
char	    *config_file;
/* defined in sequential simulator */
extern char cnfg_name[];


/* replaces timewarp function when using sequential simulator */

send_to_IH(hdr_ptr,size,stream_no)
Byte *hdr_ptr;
int size;
int stream_no;
{
hdr_ptr[size]=0;
HOST_fputs(hdr_ptr,XL_STATS);
HOST_fputc('\n',pf_file);
return(0);
}
#endif /* SIMULATOR */



Excel_body1 (o)
	Ocb            *o;
{
	Byte            excelA[MINPKTL];

	/*
	 * stdout really has no committed events, even though stats.c counts them
	 * as such. 
	 */
	if ( o->runstat == ITS_STDOUT )
	{
		o->stats.cebdls = 0;
	}

	if ( gvtcount > 0 )
	{
		o->stats.sqlen /= gvtcount;
		o->stats.iqlen /= gvtcount;
		o->stats.oqlen /= gvtcount;
	}

	sprintf (excelA,
	  "\n%d\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%f\t%f\t%d\n",
		tw_node_num, o->name, o->stats.eposfs,
		o->stats.enegfs, o->stats.eposfr, o->stats.enegfr, o->stats.cemsgs,
		o->stats.numestart, o->stats.numecomp, o->stats.cebdls, o->stats.ezaps,
		o->stats.nssave, o->stats.nscom, o->stats.evtmsg,
		o->stats.eposrs, o->stats.enegrs, o->stats.eposrr, o->stats.enegrr,
		o->stats.numcreate, o->stats.numdestroy,
		o->stats.ccrmsgs, o->stats.cdsmsgs,
		o->stats.nummigr,o->stats.numstmigr,o->stats.numimmigr,
		o->stats.numommigr,o->stats.sqlen,o->stats.iqlen,o->stats.oqlen,
		o->stats.sqmax,o->stats.iqmax,o->stats.oqmax,
		o->stats.stforw,

#ifdef MICROTIME
		((float) o->stats.comtime ) / ((float) TICKS_PER_SECOND ),
		((float) o->stats.cputime ) / ((float) TICKS_PER_SECOND ),
#else
#ifdef MARK3
		(float) o->stats.comtime / 500000.,
		(float) o->stats.cputime / 500000.,
#endif
#ifdef BBN
		((float) o->stats.comtime * 62.5) / 1000000.,
		((float) o->stats.cputime * 62.5) / 1000000.,
#endif
#ifndef MARK3
#ifndef BBN
		(float) o->stats.comtime ,
		(float) o->stats.cputime ,
#endif
#endif
#endif
		o->stats.segMax);
	send_to_IH (excelA, strlen (excelA) + 1, XL_STATS);
}


Excel_head (elapsed_time)
	double            elapsed_time;
{
	Byte            header[MINPKTL];

	if (tw_node_num == 0)
	{                           /* send output to IH */
		strcpy ( header, "Version 2.7\n\n" );

		send_to_IH ( header, strlen ( header ) + 1, XL_STATS );

		sprintf ( header, "Config File %s\n\n", config_file );

		send_to_IH(header, strlen(header)+1, XL_STATS);

		sprintf (header, "Elapsed time for this run was %.2f seconds\n\n",
				 elapsed_time);

		send_to_IH(header, strlen(header)+1, XL_STATS);

		strcpy (header, "\nNode\tObj\tE+FT\tE-FT\tE+FR\tE-FR\tE+FC");

		strcat (header, "\te starts\te cmps\te com\te zap");
		strcat (header, "\t# ssave\t# scom\tE Sent");

		send_to_IH(header, strlen(header)+1, XL_STATS); 

		strcpy (header, "\tE+RT\tE-RT\tE+RR\tE-RR");
		strcat (header, "\tcr strts\tds strts\tcm cr\tcm ds");
		strcat (header, "\tmigr\tstmigr\timigr\tomigr");
		strcat (header, "\tsqlen\tiqlen\toqlen");
		strcat (header, "\tsqmax,\tiqmax,\toqmax\tstforw\tctime\tcpu");
		strcat (header, "\tsegmax\n");

		send_to_IH(header, strlen(header)+1, XL_STATS); 
	}
}

#ifndef SIMULATOR

#ifdef STATETIME
extern long statetime ;
extern long statecount;
#endif STATETIME

#ifdef MSGTIMER
extern long msgstart, msgend;
extern long msgtime ;
extern long msgcount ;
extern int mlog;
#endif MSGTIMER

extern long statesPruned;
extern long msgsPruned;
extern long statesTruncated;
extern long msgsTruncated;
extern long critEnabled;
extern int msgsOnCritPath ;
extern long maxLocalEpt ;
extern Ocb * maxEptObj ;
extern int noHomeAskNeeded;
extern int totHomeAsks ;
extern int totHomeAsksRecv ;
extern int totHomeAns ;
extern int totHomeAnsRecv ;

Excel_tail ()
{
	Byte            message[MINPKTL];
	double hit_ratio = 0;

#ifdef MARK3
	extern int ioint_cnt, ioint_spurious, ioint_merc_cnt, ioint_cp_cnt,
				ioint_ack_cnt, rcvack_cnt,
				ioint_twsys_cnt, ioint_msg_cnt, ioint_rcvtim_cnt,
				ioint_preempt_cnt, ioint_frame_cnt;

	sprintf ( message,
"\n%2d ioint %d spur %d merc %d cp %d ack %d rcv %d sys %d msg %d tim %d pre %d frm %d\n",
		tw_node_num, ioint_cnt, ioint_spurious, ioint_merc_cnt, ioint_cp_cnt,
		ioint_ack_cnt, rcvack_cnt,
		ioint_twsys_cnt, ioint_msg_cnt, ioint_rcvtim_cnt,
		ioint_preempt_cnt, ioint_frame_cnt );

	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
#endif

#ifdef GETRUSAGE
	extern int total_faults;
#endif
#ifdef SUN
	extern char sunname[];
#endif

	if ( tw_node_num == 0 )
	{
		sprintf ( message, "\nConfig eposfs %d\n", config_eposfs );

		send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
	}

	if ( chit + cmiss > 0 )
		hit_ratio = ((float)chit/((float)chit+(float)cmiss))*100.;
	sprintf (message, "Node %d: Cache Probes %ld, Cache Hits %ld, Cache Misses %ld Hit Ratio %.2f percent\n", 
		tw_node_num, chit+cmiss,chit,cmiss,hit_ratio);

	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );

	 sprintf(message,"Migration Count Node %d: Migrations In %d, Migrations Out %d, Routing Faults %d, States Out %d, States In %d, States Not Sent %d\n",
	  tw_node_num,migrin,migrout,rfaults,fstateout,fstatein,didNotSendState);

	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );

#if DLM		/* needs to be here for Suns */
	sprintf ( message, "Load Stats: Cycles %d, Imbalances %d, Phase Not Chosen %d\n",
					loadCount, loadImbalance, noPhaseChosen );

	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
#endif

#ifdef GETRUSAGE
	sprintf ( message, "Page Fault Node\t%d:\t%d\n", tw_node_num, total_faults );
	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
#endif
#ifdef SUN
	sprintf ( message, "TW node\t%d\tSun Named\t%s\n", tw_node_num, sunname );
	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
#endif 
#ifdef BF_MACH
	{
		int clus, mach, phys;
		which_nodes ( &clus, &mach, &phys );

		sprintf ( message,
		  "TW node\t%d\tCluster node\t%d\tMach node\t%d\tPhysical node\t0x%x\n",
			tw_node_num, clus, mach, phys );
		send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
	}
#endif

#ifdef BF_MACH
/* If weLooped is non-zero, the low-level message buffering facility got
	  swamped one or more times because some foreign node was unresponsive.
	  Timings for such runs are known to be inferior, so this condition
	  will be flagged in the XL_STATS file with the following message. */

	if ( weLooped )
	{  
	  sprintf ( message, "Foreign node went to sleep %d times\n",
			  weLooped );
	  send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
	}  
#endif

/*  Check to see if this node sent or received any migration-related
	  naks.  If so, print a message about them. */

	if ( phaseNaksSent + phaseNaksRecv + stateNaksSent + stateNaksRecv )
	{  
	  sprintf ( message, "Node %d: Migration Naks: %d phase sent, %d phase recv, %d state sent, %d state recv\n", 
		tw_node_num, phaseNaksSent, phaseNaksRecv, stateNaksSent, 
		stateNaksRecv );

	  send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
	}  

/* if dupStateSend is non-zero, then the limited jump forward optimization
	  paid off at least once.  Record this fact in the XL_STATS file. */

	if ( dupStateSend != 0 )
	{  
	  sprintf ( message, "Node %d: Limited jump forward saved %d state transmissions\n",
			  tw_node_num, dupStateSend );

	  send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
	}  

	if (hlog)
			{
			sprintf(message,"Max slice for object %s node %d:\t%.0f ms @ %f %lu\n",
				maxSliceObj->name, tw_node_num,
#ifdef BBN
				((float) maxSlice * 62.5) / 1000.,
#endif
#ifdef MARK3
				((float) maxSlice) / 500.,
#endif
#ifndef MARK3
#ifndef BBN
				((float) maxSlice * 1000.),
#endif
#endif
				maxSliceTime.simtime, maxSliceTime.sequence1);
			send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
			}

#if TIMING
	sprintf(message,"find_count for Node %d: %d\n",tw_node_num,find_count);
	send_to_IH(message,strlen(message)+1,XL_STATS);

    sprintf ( message, "Node %d Tester %3.2f Timewarp %3.2f Objects %3.2f System %3.2f Idle %3.2f Rollback %3.2f IQ %3.2f Sched %3.2f Dlvr %3.2f Serve %3.2f Objend %3.2f Gvt %3.2f OQ %3.2f IQFind %3.2f IQAccept %3.2f IQCmp %3.2f\n",
        tw_node_num,
        ((float) timing[0]) / 1000000.0,
        ((float) timing[1]) / 1000000.0,
        ((float) timing[2]) / 1000000.0,
        ((float) timing[3]) / 1000000.0,
        ((float) timing[4]) / 1000000.0,
        ((float) timing[5]) / 1000000.0,
        ((float) timing[6]) / 1000000.0,
        ((float) timing[7]) / 1000000.0,
        ((float) timing[8]) / 1000000.0,
        ((float) timing[9]) / 1000000.0,
        ((float) timing[10]) / 1000000.0,
        ((float) timing[11]) / 1000000.0,
        ((float) timing[12]) / 1000000.0,
        ((float) timing[13]) / 1000000.0,
        ((float) timing[14]) / 1000000.0,
        ((float) timing[15]) / 1000000.0);
	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
#endif	

	/*  Now dump per-node statistics concerning the size and activity of
		the free message buffer pool. */

	sprintf ( message, "Node %d: max msg buffs allocated to free pool = %d\n",
				tw_node_num, max_msg_buffers_alloc ) ;
	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
	sprintf ( message, "Node %d: num msg buffs released from free pool = %d\n",
				tw_node_num, num_msg_buffs_released_from_pool) ;
	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
#ifdef STATETIME
	sprintf( message, "Node %d: state save time = %f, %d states saved, %f msec/save\n",
				tw_node_num,   (statetime * 62.5)/1000., statecount,
				 ((statetime * 62.5)/1000.)/statecount );
	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );

#endif STATETIME
#ifdef MSGTIMER
	/*  This code uses a field used by mlog.  If mlog is in use, then the
		message timing code will not run. */

	if ( !mlog )
	{
		sprintf( message, "Node %d: msg send time = %f, %d msgs sent, %f msec/msg\n",
				tw_node_num, (msgtime * 62.5)/1000., msgcount,
				((msgtime * 62.5 )/1000.)/msgcount );
		send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
	}
#endif MSGTIMER

	if ( critEnabled )
	{
		sprintf( message, "Node %d: %d msgs pruned by CP, %d states pruned\n",
			tw_node_num, msgsPruned, statesPruned );

		send_to_IH ( message, strlen ( message ) + 1, XL_STATS );

		sprintf( message, "Node %d: %d msgs truncated by CP, %d states truncated\n",
			tw_node_num, msgsTruncated, statesTruncated );

		send_to_IH ( message, strlen ( message ) + 1, XL_STATS );

	}

	if ( msgsOnCritPath != 0 )
	{
		sprintf( message, "Node %d: %d msgs on critical path\n",
			tw_node_num, msgsOnCritPath);

		send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
	}

	if ( maxEptObj != NULL )
	{
		int maxLocMsec = maxLocalEpt * 62.5;

		sprintf ( message, "Node %d: Max Local Time %d for Object %s\n", tw_node_num,
					maxLocMsec , maxEptObj->name ); 

		send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
	}


	sprintf ( message, "Node %d: No home ask needed for pending entry %d times\n",
				tw_node_num, noHomeAskNeeded);
	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );

#if DLM		/* needs to be here for Suns */
	sprintf ( message, "Node %d: Objects blocked %d times\n",
				tw_node_num, objectBlocked);

	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
#endif

	sprintf ( message, "Node %d: Homeasks sent %d Homeasks received %d\n",
				tw_node_num, totHomeAsks, totHomeAsksRecv );

	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );

	sprintf ( message, "Node %d: Homeans sent %d Homeans received %d\n",
				tw_node_num, totHomeAns, totHomeAnsRecv );

	send_to_IH ( message, strlen ( message ) + 1, XL_STATS );
}
#endif /* ifndef simulator */

#ifndef SIMULATOR
/* if using TW and not the TW simulator */

/* The following routine is the entry point into the statistics module.
   It can be called from anywhere within the Executive.  This particular
   version differs from the one in the module CUBEIO.C in that this one
   calls the VAX IO functions which write to a file; such functions will
   not work on the Hypercube.  The two (VAX and Hypercube) were split up
   so that code complexity may be minimized. */

int not_dumping_stats = 1;

dump_stats (elapsed_time)
	double            elapsed_time;     /* in seconds */
{
	Ocb            *o;

	not_dumping_stats = 0;

	Excel_head (elapsed_time);

	for (o = fstocb_macro; o; o = nxtocb_macro (o))
	{
		Excel_body1 (o);
	}

	Excel_tail ();
}

#endif /* SIMULATOR */
#ifdef SIMULATOR
/* if using TW simulator and not TW */

/**********************************************************************
*
*	record object statistics
*	-- if XL_STATS file to be created
*	called at end of run if interval was 0.0 (statint)
*	called at end of every interval otherwise.
*
*********************************************************************/
#ifndef HISP
void FUNCTION record_obj_stats(tottime,interv)
    double tottime;
    double interv;

    {
    int iindex, oindex;
    Ocb  oo;
    Ocb  *o;
    int  xxx;

    o = &oo;

    clear(&oo,sizeof(Ocb));
    strcpy(sunname, "SIMULATOR\n\0");
    config_file = cnfg_name;
    tw_node_num = 0;
    if (interv == 0.0)  Excel_head(tottime);

    for (iindex = 0; iindex < num_objects; iindex++)  
	{
	oindex = obj_hdr[iindex].obj_bod_ptr;
	
	strcpy(o->name, obj_hdr[iindex].name);
	o->stats.numestart = obj_bod[oindex].num_events;
	o->stats.numecomp = o->stats.numestart;
	o->stats.evtmsg = obj_bod[oindex].num_evtmsgs;  /* tells */
	o->stats.numcreate =  obj_bod[oindex].cr_count;
	o->stats.numdestroy = obj_bod[oindex].dst_count;
	o->stats.ccrmsgs = obj_bod[oindex].obj_new;
	o->stats.cdsmsgs = obj_bod[oindex].obj_del;
	/* in the simulator no cr or de messages are used if the obj is
	cr or destroyed at time now. May cause stats to disagree with the
	timewarp values */
	xxx = obj_bod[oindex].obj_cemsgs;
	o->stats.cemsgs = xxx;
	o->stats.eposfr = xxx;
        o->stats.eposfs = xxx;
	o->stats.comtime = 0;
	o->stats.segMax = obj_bod[oindex].segMax;
#ifdef BF_MACH
	o->stats.cputime =  (long)( obj_bod[oindex].cum_obj_time /62.5);
#endif
#ifdef SUN
	o->stats.cputime =  (long)( obj_bod[oindex].cum_obj_time );
#endif
	Excel_body1(o);
/* if special objstats recording is done at intervals (see statint and -l switch) the
following is used to reset the data. If the data is only taken once at the
end of the run the following is unecessary. */
	obj_bod[oindex].num_events = 0;
	obj_bod[oindex].cum_obj_time = 0.0;
	obj_bod[oindex].num_evtmsgs = 0;
	obj_bod[oindex].cr_count = 0;
	obj_bod[oindex].dst_count = 0;
	obj_bod[oindex].obj_new = 0;
	obj_bod[oindex].obj_del = 0;
	obj_bod[oindex].obj_cemsgs = 0;
	}
/*   Excel_tail();    not currently compiled */
    }
#endif /* HISP */
#endif /* SIMULATOR */
