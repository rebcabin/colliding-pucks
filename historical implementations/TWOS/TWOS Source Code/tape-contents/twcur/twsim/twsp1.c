/* "Copyright (C) 1989, California Institute of Technology. 
     U. S. Government Sponsorship under NASA Contract 
   NAS7-918 is acknowledged." */
 /***************************************************************
 *                     TIME WARP SIMULATOR			*
 *                         file TWSP1.C				*
 *								*
 * Modified:  JJW 10/3/86;					*
 * Fix display and init  JJW 10/22/86				*
 * add interrupt facility JJW 12/3/86				*
 * fix void declarations JJW 12/8/86				*
 * add internal IBMPC only timing function JJW 12/8/86		*
 * add numerical breakpoint JJW 1/9/87				*
 * fix exit from pause2 JJW 6/15/87				*
 * put in emq_endmsg_ptr JJW 6/18/87				*
 * put in trace file and getargs function JJW 6/20/87		*
 * merge with MARKIII under MKIII conditional 7/1/87		*
 * adapt for new config file parser JJW 7/8/87			*
 * Update for MarkIII / cubix implementation PJH 7/8/87		*
 * Added timing of objects on MarkIII	     PJH 7/12/87	*
 * fix bugs JJW 7/18/87						*
 * NOTE: field elapsed time in archive file is currently zero	*
 * added instrumentation fields to trace file JJW 7/28/87	*
 * add termination section stuff JJW 8/3/87			*
 * add things for critical path determination JJW 8/26/87	*
 * reestablish normal event timing under crit JJW 9/22/87	*
 * add integer timekeeper to total sim time print 10/15/87	*
 * make this tw107			10/18/87		*
 * add data file  11/22/87					*
 * now tw108; make main vars global  12/4/87			*
 * remove display functions and add them to twhelp 12/7/87	*
 * add proportional delay 12/7/87				*
 * add notime switch to turn off internal timing  1/22/88	*
 * add support for queueing module twqueue.c	2/23/88		*
 * add message file with incoming messages rather than evtmsg's *
 * put this under switch M   3/30/88				*
 * special version for flow analysis JJW 3/31/88		*
 * diff with twsp1.c in tw109 - same except for flow 4-6-88	*
 * update to twsim 110 4/25/88					*
 * THIS VERSION DOESNT USE SIGALRM,except on the sun		*
 * tw112 8/4/88     also include stackcheck under STKCHK	*
 * tw112 8/16/88    make above a runtime option always compiled	*
 *    and put some switches (alrm_sw, stkchk_sw) into twsd.h	*
 * tw112hs 8/19/88  high speed version, also remove CRIT	*
 * tw113 9/30/88 fix for new init and type table		*
 * tw114 11/14/88 fix for new interface and merge for machines	*
 * tw114 1/16/89 fix type problems in timing variables
 * tw2   tw2.1  	new interfaces				*
 * move pr_delete_messages to the queue module	5/16/89		*
 * add objstats	interval data to file 6/15/89			*
 * added flag arg to pr_setup_messages for speed in		*
 * newobj and delobj 7/7/89
 * changed objstats to use XL_STATS and be consistent with tw	*
 * add conditional MEMUS to get memory usage under sun          *
 * put in NOVMMAP and arbitrary time objstats
 ***************************************************************/

#define STKCHK 
/* above always true, now a runtime option under switch -y */

/* define HISP to get version which does no internal timing and
 * which produces no output files.  Ifdefined  HISP   *DO NOT* define
 * either STKCHK or FLOW because SOME of their ifdefs are not buried
 * inside HISP ifdefs   */
#ifdef HISP
#undef FLOW
#undef STKCHK
#endif

#include <stdio.h>
#include "twcommon.h"
#include "machdep.h"

/* define NOVMMAP and BF_MACH to avoid using the vm_map command and
   to use malloc instead.  This also affects simmem.c and faults.c by
   ifdef to get rid of all of faults and change simmem to define sim_malloc
   to be the same as the Sun. */

#ifdef NOVMMAP
#undef BF_MACH
#define MACH_NOVM
#endif

/* define SPEC_OBJSTATS to get arbitrary time objstats stuff. Also needs
   same def in newconf.cyy */

#ifdef SPEC_OBJSTATS
extern int init_obs;
extern double time_array[];
#endif

/* #define MEMUS
   if defined use sbrk to get before/after memory data for sun */


/***************************************************************
 *
 *    		  Comments on initialization
 *
 *  The first object is created by CONFIGSIM as the first
 *  object it finds in the configuration file. The creation
 *  time of this object is the start of virtual time.  The 
 *  first message is also created by CONFIGSIM as the first
 *  event message it finds in the configuration file.  This
 *  also sets up the initial queue pointers and it is essential
 *  that the config file contain an initial event message to 
 *  start the simulation. 11/88 Can send msgs from init section
 *  Q no longer set up by config file msg.
 *
 **************************************************************/

/* define PAD to fork pad if SUN is defined.  conditional with -p switch */


#ifdef SUN
typedef long	time_t; /* /usr/include/sys/types.h */
#ifdef CRAY     /*PJH stupid include convention!!!*/
#include "timeb.h"
#else
#include <sys/timeb.h>
#endif

#include <signal.h>
#include <errno.h>
/* #define PAD  */
#endif SUN

#ifdef BBN
#include <stdio.h>
#ifndef NOVMMAP
#ifndef BF_MACH
#include "timeb.h"
#endif
#endif
extern unsigned long clock();
#endif

#ifdef BF_MACH
#include <sys/cluster.h>
#include <sys/kern_return.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/timeb.h>
#endif
#ifdef MACH_NOVM
#include <signal.h>
#include <sys/types.h>
#include <sys/timeb.h>
extern unsigned long clock();
#endif



#ifdef MARK3
#include "stdio.h"	
#include <errno.h>
#include "timeb.h"
#endif MARK3

#define  DEFAULTARCFILE		"arc.jrn"
#define  DEFAULTTRACEFILE	"TRACE"
#define  DEFAULTMESSFIL		"MTRACE"
#define  DEFAULTDATAFIL		"SIMDATA"
#define  DEFAULTIMESSFIL	"IMTRACE"
#define  DEFAULTELOGFIL		"EVTLOG"
#ifdef FLOW
#define  DEFAULTFLOWFILE	"SIMFLOW"
#endif
#define  DEFAULTOBJSTFIL	"XL_STATS"

#define  MAX_SIGNED_LONG	2147483647


#include "twcommon.h"
#include "tws.h"
#include "twsd.h"
#include "twctype.h"

/************************************************************************
*
*	type table - entry points for the application's init, term,
*       displayMsg and displayState code sections (considered processes when
*	             executed) as well as the type name and statesize.
*	Loaded from data in the table.c module created by the application
*		programmer.  This table format is used both by the simulator
*		and by Tw.
*	The array (named process) is now allocated in twsd module
*
*************************************************************************/

		/* for the Flow  option*/ 
typedef struct
	{int start_time;
	 int end_time;
	 int objidx;
	 VTime svt;
	} ENTRY;

/*PJH_IS	*/

int     IS_delta;
unsigned int     IS_clock1, IS_clock2, IS_clock3;
int     ISLOG_enabled;

typedef struct
{
    int seqnum;
    double  cputime;
    VTime  minvt;
}
    IS_LOG_ENTRY;
 
IS_LOG_ENTRY * IS_log, *IS_logp, *IS_loge;
int num_IS_entries;
FILE	*IS_log_file;


/************************************************************************
*
*	 variables local to this file.  All main() vars are now global
*        except for status which is also local in other routines.
*
*************************************************************************/
#ifndef HISP
FILE           *arc_file;
FILE	       *t_file;
FILE	       *m_file;
FILE	       *im_file;
FILE	       *el_file;
FILE	       *pf_file;	/* for objstats */
#ifdef FLOW
FILE	       *flow_file;
#endif
int		imfile_sw;		/* file of incoming msgs switch */
int	ctr;				/* for imfile */
#endif
FILE           *cnfg_file;
FILE	       *data_file;
int	go_flag;
int	zflag;	/* stop printing of time values in trace file (t_file) */
#ifdef PAD
int pad_sw;
#endif
int		d_file_sw;		/* data file switch */

long    tot_evtcount;
long	start_time;

#ifdef MARK3
long	total2;	/*&TIME*/
#endif

#ifdef BBN
long	time1,time2,total2;
#endif

double  statint = 0.0;		/* reset by s option */
double  stattime = 0.0;
double  savelvt = 0.0;	     /* the current lvt or the very last one if sim over */
long	sec_time;	/*&TIME*/
double	total;
int     elap_time = 0;
double	tot_evt_time = 0.0;
/* evt_parttime and evt_realtime are defined (as double) in twsd.h */
char    cnfg_name[80];
struct timeb startit;
struct hog_st {
	   double	hog_time;
	   int hog_evt;
	   Name_object  hog_name;
	   } hog;
Name_object  what_obj;  /* needs to be an array for true CRIT determination */
int what_evt;
ENTRY	*Flog, *Flogp, *Floge;		/*Flow*/
int	cpu_time;
int	num_entries;
char    fdata[30];
#ifdef FLOW
int	flow_sw;
#endif
int	alrm_cnt = 0;
double  time_1;		/* intermediate real time for alarm */
double  time_2;		/* another one ^^^ (used only in main-proc) */
double  time_3;		/* total time so far */

#ifdef STKCHK
int     *stkdata;	/* ary of ints for data */
#endif

#ifdef SUN
#ifdef MEMUS
long startmem;
long endmem;
#endif
#endif

extern char versionno[];
extern char date[];
static char twsp1ida[] = "%W%\t%G%";

/* extern variable declarations */

extern int file_del_flag;	/* twsp3 */
/* Function declarations */

char *sim_malloc();
double atof();
int brkfct();		/* #br */
extern  double itimer();
extern FILE *HOST_fopen();

#ifdef MARK3
extern long clock();
extern long time();	/*&TIME*/
#endif

void ftime();
void main();
void getargs();
void open_file();
void main_process();
double pr_setup_pathtime();
void pr_archive_messages();
void pr_delete_messages();
void warning();
void sim_debug();
void error();
void gnxt_option();
void gnxt_reply();
void gnxt_token();
void perform_option();
void set_bp();
void dis_messages();
void dm_queue();
void dm_select();
void dm_one_message();
void dm_header();
void dis_objects();
void do_one_object();
void do_header();
void dis_stats();
void dis_times();
void record_obj_stats();
void find_hog();
void compute_time();
void term_process();
#ifdef STKCHK
int  *stackarray();
#endif
#ifdef SUN
#ifdef MEMUS
long memuse();
#endif
#endif


/************************************************************************
*************************************************************************
*
*               Time Warp Simulator - (Time Wrinkle?)
*               Perpetrator - S. Hughes
*
*************************************************************************
*
*	main - time warp simulator program entry point
*
*	called by - none
*
*	- initialize the simulator including opening output files 
*	- perform "main_process" as long as messages exist in the queue
*	- output statistics and clean up and exit
*
*************************************************************************/

void FUNCTION main(argc, argv)
int             argc;
char          **argv;
{
    int             status =  0;
    int  prog;
    int  fildes[2];
#ifdef BF_MACH
    kern_return_t rv;
    int nodes_requested = 1;
    cluster_type_t mach_type = 0;
    cluster_id_t  cluster_id;
    int nodes_allocated;
    int child_pid;
#endif

#ifdef SUN
#ifdef MEMUS
    startmem = memuse(0);
#endif
#endif SUN
    go_flag = 1;
    step_switch = 0;
    zflag = 0;
/* don't stop at beginning unless getargs finds the q option.
   step_switch =0, sf;  =1, st;  =2, tt;  */
    strcpy(obj_hdr[0].name, "NULL"); /* for the very first cr_create */
    status = global_init();
    getargs(argc,argv);

#ifdef BF_MACH
    if ( 0 != ( rv = cluster_create(nodes_requested, mach_type,
	&cluster_id, &nodes_allocated) ) )
    {
	fprintf (stderr,  "cluster_create returned %d\n", rv );
	exit ( 1 );
    }

    if ( 0 != ( rv = fork_and_bind( 0, cluster_id, &child_pid) ) )
    {
	fprintf (stderr, "fork_and_bind returned %d\n", rv );
	exit ( 1 );
    }

    if ( child_pid ) /* I am the parent */
    {
	signal(SIGINT,brkfct);  /* it gets the control-c too */

	wait ( (union wait *) 0 );
	fprintf (stderr,  "End Parent pid is %d\n", getpid() );
	exit ( 0 );
    }

    /* I am the child */
    init_sim_mem();
#endif

#ifdef PAD
if (! pad_sw)
    {
    prog = 1;
#endif
    fprintf
	(
	 stderr, "Timewarp Sequential Simulator Version %s  %s\n",versionno,date);

#ifdef HISP
    fprintf("(no internal timing nor output files)\n");
#endif
#ifdef PAD
     }
else
     {
	go_flag = 1;
	step_switch = 0;
	if (pipe(fildes)) perror(0);
	prog = fork();
     }

if (!prog)  /* child */
    {
    if (close(0)== -1) perror("close 0");
    if (dup(fildes[0])== -1) perror("dup 0");  /* reader */
    close(fildes[0]);
    close(fildes[1]);
    execl("/usr/local/bin/pad","pad", "-n","-W560",(char *) 0);
    fprintf(stderr, "exec failed for pad\n");
    exit(1);
    }

if (prog && pad_sw)
    {
    if (close(1)== -1) perror("close 1");
    if (dup(fildes[1])== -1) perror("dup 1");  /* writer */
    close(fildes[1]);
    close(fildes[0]);
    }
#endif PAD


#ifndef HISP
    open_file();
#endif
    if (!go_flag)
      sim_debug(" <<< simulation start >>> "); /*  munges evt_realtime */
    evt_realtime = 0.0;	/* reset for configuration created messages */
    if (cr_typetable() == FAILURE)
	exit(1);	/* this routine is in newconf */
    if (configure_simulation(cnfg_name) == FAILURE)  /* resets fmsg_flag */
	status = FAILURE;

#ifdef BF_MACH
    signal(SIGINT,brkfct);
#endif

#ifdef SUN
    signal(SIGINT,brkfct);
#endif
   
    if (status == SUCCESS)		/* configured successfully */
    {
#ifdef FLOW
    if (flow_sw == TRUE);		/*Flow*/
      {
      Flog = (ENTRY *) sim_malloc(num_entries * sizeof(ENTRY));
      if (Flog == NULL)
	{
	fprintf(stderr,"no memory for flow table\n");
	flow_sw = FALSE;
	}
      else
	{
	Flogp = Flog;
	Floge = Flog + num_entries;
	}
      }
#endif

#ifdef MARK3 
    sec_time = time();	/*&TIME*/
    time_1 = (double)sec_time;		/* initialize it  - seconds */
    time_3 = 0.0;		/* initialize it */
    start_time = clock(); 
#endif
#ifdef BBN
#ifdef BF_MACH
    set_faults();
#endif
    start_time = time1 = clock();
    time_3 = 0.0;		/* initialize it */
#endif

#ifdef SPEC_OBJSTATS
#define SPEC_OBJSTATS_SIZE 20
/* above definition also in newconf.cyy !!!! */
    if (init_obs == 0)
	{
	statint = -1;
	stattime = time_array[0];
	init_obs++;
	}
/* init_obs set to zero in newconf if we got a objstats in config file. It 
   will now keep track of where we are in the array */
#endif

#ifdef SUN	
#ifndef CRAY
    ftime(&startit);
    start_time = startit.time; 
    time_1 = (double)startit.time;	/* initialize it  - seconds*/
    time_3 = 0.0;		/* initialize it */
#else
    start_time = time();
    time_1 = (double)(start_time /1000000.0);
    time_3 = 0.0;
#endif

#endif
/* reset times because of changes by tells in config file */
    evt_realtime = 0.0;
    tot_evt_time = 0.0;  
 

/*    while (prog_status == WSCONTINUE)  do MAIN LOOP */
    main_process();

#ifdef MARK3
     total = ((double)time() - time_1);
     total2 = time() - sec_time;	/*&TIME*/
#endif

#ifdef BBN
     time2 = clock();
     total = ( (time2 - start_time) * 62.5 ) /1000000. ;
     fprintf ( stderr, "total = %f\n", total );
#endif

#ifdef SUN	
#ifndef CRAY
     ftime(&startit);
     total = (double)startit.time
	   - (double)start_time;
#else
     total = (double) (( time() - start_time)/1000000.0);
#endif

#endif 

#ifndef HISP
	find_hog( (cpath_sw == FALSE)?1:2);
#ifdef STKCHK
	if (stkchk_sw) stackarray(2);
#endif
	if ( (int)arc_file) HOST_fclose(arc_file);
	if (tfile_sw == TRUE) HOST_fclose(t_file);
	if (mfile_sw == TRUE) HOST_fclose(m_file);
	if (imfile_sw == TRUE) HOST_fclose(im_file);
	if (elog_sw == TRUE) HOST_fclose(el_file);
#endif
#ifdef PAD
	if (! pad_sw)
	{
#endif
	dis_times();
	if (!go_flag)  pause1();
	dis_stats();
#ifdef PAD
	}
        else
	   { 
	   sleep(4);
	   printf("quit\n");
	   }
#endif
#ifndef HISP
 	if (d_file_sw == TRUE) HOST_fclose(data_file);
        if (statfile_sw)  {
		  if (statint != 0.0)
	             HOST_fprintf(pf_file,"# = %lf\n",savelvt);
		  record_obj_stats(total,statint);
		  HOST_fclose(pf_file);
		  }
#endif

/*PJH  */
#if defined(BF_PLUS) || defined(BF_MACH)
        if ( ISLOG_enabled )
		IS_dumplog ();
#endif
    }
#ifdef SUN
#ifdef MEMUS
    endmem = memuse(0);
#ifdef PAD
    if (! pad_sw) printf ( "memory usage: %ld / %ld\n", startmem, endmem );
#else
    printf ( "memory usage: %ld / %ld\n", startmem, endmem );
#endif PAD
#endif
#endif


#ifdef FLOW
    if (flow_sw == TRUE) do_flow_file();
#endif
}   /* end of main() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*    getargs()
*        called only by main at beginning.
*        Command line argument collector. 
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void FUNCTION getargs(argc,argv)
int argc;
char **argv;
   
   {
   char *s;

   while (--argc > 0 && (*++argv)[0] == '-')
      for ( s = argv[0]+1; *s != '\0'; s++)
	  switch (*s) {
#ifndef HISP
	     case 't':
		tfile_sw = TRUE;   /* see also switch z */
		break;
	     case 'l':
		statfile_sw = TRUE;
		statint = atof(*++argv);
		--argc;
		printf("objstats interval: %lf\n",statint);
		break;
	     case 's':
		statfile_sw = FALSE;
		break;
	     case 'a':
		archive_switch = TRUE;
		break;
	     case 'm':
		mfile_sw = TRUE;
		break;
	     case 'M':
		imfile_sw = TRUE;
		break;
	     case 'c':
		cpath_sw = TRUE;
		break;
	     case 'e':
		elog_sw = TRUE;
		break;
	     case 'n':
		notime_sw = TRUE;
		break;
#ifdef PAD
	     case 'p':
		pad_sw = TRUE;
		break;
#endif
#ifdef STKCHK
	     case 'y':
		stkchk_sw = TRUE;
     		stackarray(1);    /*initialize the array */
		break;
#endif
	     case 'x':
		multiple = atof(*++argv);
		--argc;
		printf("time multiplier: %lf\n",multiple);
		break;
	     case 'b':
		strcpy(fdata,(*++argv));
		--argc;
		printf("default file suffix:  %s\n",fdata);
		break;
	     case 'd':
		d_file_sw = TRUE;
		break;
#endif
	     case 'k':
		alrm_cnt = atoi(*++argv);
		--argc;
		fprintf(stderr, "alarm interval: %d\n",alrm_cnt);
		break;
	     case 'g':
		go_flag = 1;
		step_switch = 0;
		break;
	     case 'q':
		go_flag = 0;
		step_switch = 1;
		break;
#ifdef FLOW
	     case 'f':
		num_entries = atoi(*++argv);	/* Flow */
		--argc;
		printf("flow table entries: %d\n",num_entries);
		flow_sw = TRUE;
		break;
#endif
	     case 'z':
		zflag = 1;
		tfile_sw = TRUE;
		break;
	     default:
		fprintf(stderr,"bad arg: -%c\n",*s);
		break;
	     }
   if (argc >= 1) strcpy(cnfg_name , *argv);
   if (argc > 1) fprintf(stderr,"too many args\n");
   }

/************************************************************************
*
*	global_init - global initialization routine
*
*	called by - main
*
*       - initialize things that need to be set before calling getargs
	- initialize switches and pointers
*	- perform routine to initialize tw module entry counts

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int
FUNCTION global_init()		/* all things done one time go here */
  {
  tfile_sw = FALSE;
  mfile_sw = FALSE;
  cpath_sw = FALSE;
  alrm_sw = FALSE;
  stkchk_sw = FALSE;
#ifdef PAD
  pad_sw = FALSE;
#endif
#ifdef FLOW
  flow_sw = FALSE;
#endif
  notime_sw = FALSE;
  d_file_sw = FALSE;
  test_var1 = 0;	/* 2 gen purpose vars defined in twsd and -- */
  test_var2 = 0;	/* printed out in statistics */
  archive_switch = FALSE;
#ifndef HISP
  arc_file = NULL;		/* switch indeterminate when closed */
  imfile_sw = FALSE;
#endif
  hog.hog_time = 0.0;
  statfile_sw = TRUE;
  strcpy(cnfg_name,"cfg");	/* reset by getargs() if we have a file */
  debug_switch = FALSE;
  elog_sw = FALSE;
  bp_switch = FALSE;
  multiple = 1.0;		/* may be reset by getargs */
  prog_status = WSCONTINUE;
  fmsg_flag = TRUE;
  num_objects = 0;
  emq_first_ptr = NULL;
  emq_endmsg_ptr = NULL;
  emq_current_ptr = NULL;
  su_counts();
  inode = 0;
  cutoff_time = POSINF;
  return (SUCCESS);
  }


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   open_file.
 *
 *    open the archive file.
 *    open the trace file if tracing enabled.
 *    open the data file if the d_file_sw is true.
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef HISP
void FUNCTION open_file()
    {
    char  namef[40];
   
    if (archive_switch == TRUE )
	{
          arc_file = HOST_fopen( DEFAULTARCFILE, "w"); 
	  fprintf(arc_file, "version: %s\n",versionno);
        }
    if (tfile_sw == TRUE) 
	{ 
	   t_file = HOST_fopen(DEFAULTTRACEFILE, "w"); 
	   fprintf(t_file,"Version: %s\n",versionno);
	   if(!zflag)
	   fprintf(t_file,"Object\t LVT\t evt-time\t tot-time\t [what]\t nimsgs\t nomsgs\t create_count\n\n");
	   else
	   fprintf(t_file,"Object\t LVT\t [what]\t nimsgs\t nomsgs\t create_count\n\n");

	}
    if (mfile_sw == TRUE) 
	{ 
	   m_file = HOST_fopen(DEFAULTMESSFIL, "w"); 
	   fprintf(m_file,"version: %s\n",versionno);
	   fprintf(m_file,"Sender\t Send-time\t Rcvr\t Rcv-time\t selector\n");
	}
    if (imfile_sw == TRUE) 
	{ 
	   im_file = HOST_fopen(DEFAULTIMESSFIL, "w"); 
	   fprintf(im_file,"version: %s\n",versionno);
	   fprintf(im_file,"Sender\t Send-time\t Rcvr\t Rcv-time\t selector\n");
	}
    if (d_file_sw == TRUE) 
	{ 
	   strcpy(namef,DEFAULTDATAFIL);
	   if (fdata[0] != 0) strcat(namef,fdata);
	   data_file =  HOST_fopen(namef, "w"); 
	   fprintf(data_file,"version: %s\n",versionno);
	}

    if (elog_sw == TRUE) 
	{ 
	   el_file = HOST_fopen(DEFAULTELOGFIL, "w"); 
/*	   fprintf(el_file,"version: %s\n",versionno);
	   fprintf(el_file,"object  Vtime  out-msgs\n\n");  */
	}
    if (statfile_sw == TRUE)
	{
	   strcpy(namef,DEFAULTOBJSTFIL);
	   if (fdata[0] != 0) strcat(namef,fdata);
	   pf_file = HOST_fopen(namef, "w");

	}
    }
#endif


/************************************************************************
*
*	su_counts - initialize the tw module entry counts and statistics
*		    gathering variables
*
*	called by - global_init
*
*************************************************************************/

FUNCTION su_counts()
{
    simtm_cnt = 0;
    me_cnt = 0;
    evtmsg_cnt = 0;
    event_cnt = 0;
    stdout_cnt = 0;
    time_cnt = 0;
    obcre_cnt = 0;
    obdes_cnt = 0;
    mcount_cnt = 0;
    getmsg_cnt = 0;
    getsel_cnt = 0;
    sender_cnt = 0;
    sndtm_cnt = 0;
    req_blk_cnt = 0;
    all_blk_cnt = 0;
    num_mesg_bytes = 0;
    num_state_bytes = 0;
    tot_evtcount = 0L;  /* declared in this file */
    dequeue_time = 0.0;
    queue_time = 0.0;
    return (SUCCESS);
}



/************************************************************************
*
*	main_process - main driver routine
*
*	called by - main
*
*	- loop
*	- if there is another message
*	-    if trace switch is on, enter data in file
*	-    if the step switch is 1, stop and allow debug
*	-    if the step switch is 2, print next object but don't stop
*	-    if the debug switch is on, check receive object  JJW
*	-    if the next event message is an object create message
*	-       perform routine to handle the creation of objects
*	-    else if the next event message is an object destroy message
*	-       perform routine to handle the destruction of objects
*	-    else (we have your typical run-of-the-mill event message)
*	-       perform routine to find the receiving object in object table
*	-       perform routine to find all event messages to be 
*	-          received by the object at the current virtual time
*	-	perform routine to save the objects prior state
*	-	perform routine to find max realtime in object saved time
*	-	   or an incoming event message
*	-       <perform the objects event code>
*	-	note the time and compute elapsed time
*	-	set the objects current virtual time
*	-       if the archive switch is set, archive message
*	-       perform routine to mark all processed messages as deleted
*	- else
*	-   return to main
*
*	note: time spent in sim_debug routine is not counted in elapsed
*		time data kept if critical path is turned on
*************************************************************************/

void FUNCTION main_process()
{
    struct timeb    before,
                    after ;
/* the two items in struct of interest: uns short millitm time_t=long time */

    long   evt_time;
#ifdef BBN
    char  line[30];
#endif
#ifdef STKCHK
    int  stcheck;
#endif

/* get next event in the queue  - loop  */
    while (find_next_event() == SUCCESS && prog_status == WSCONTINUE )
	{
	savelvt = emq_current_ptr->rlvt.simtime; /* current or the very last one */
/* delete any files whose time has come */
	if (file_del_flag != 0)
	   io_rel_file(savelvt);
#ifndef HISP
	tot_evtcount++;  /* if we have create/destroy msgs will != event_cnt */

	if (tfile_sw == TRUE)
	  {
#ifdef MARK3
	  evt_time = clock() - start_time;
#endif

#ifdef BBN
	  evt_time = clock();
          evt_time = ( evt_time - start_time );
#endif

#ifdef SUN
#ifndef CRAY
	  ftime(&before);
	  evt_time = before.time - start_time;
#else
          evt_time = time() - start_time;
#endif
#endif

	  HOST_fprintf(t_file, "%s\t%.4f [%d,%d]\t",
	      emq_current_ptr->rname,emq_current_ptr->rlvt.simtime,
	      emq_current_ptr->rlvt.sequence1,emq_current_ptr->rlvt.sequence2);
	  
	  }
#endif

	if (alrm_cnt) {
#ifdef SUN
	   if (alrm_sw++ >= alrm_cnt) {
#ifndef CRAY
		     ftime(&startit);
		     time_3 = startit.time - time_1;
#else
                     time_3 = (double)((time() - time_1) /1000000.0);
#endif

		     fprintf(stderr, "time\t%.2lf\trlvt\t%.4lf\n",
			time_3, emq_current_ptr->rlvt.simtime);
		     alrm_sw = 0;
		     }
#endif
#ifdef MARK3
	   if (alrm_sw++ >= alrm_cnt) {
		     alrm_sw = 0;
		     time_3 = (time() - time_1);
		     fprintf(stderr, "time\t%.2lf\trlvt\t%.4lf\n",
			time_3, emq_current_ptr->rlvt.simtime);
		     }
#endif
#ifdef BBN
	   if (alrm_sw++ >= alrm_cnt) {
		     alrm_sw = 0;
		     time_3 = ( (clock() - time1)* 62.5) / 1000000.;
		     fprintf(stderr, "time\t%.2lf\trlvt\t%.4lf\n",
			time_3, emq_current_ptr->rlvt.simtime);
		     }
#endif

	  }
#ifdef BF_PLUS
	    if ( ChannelHasInput ( stdin ) ){
		     gets( line);  
		     step_switch = 1;
		     }
#endif
/* possibly stop and/or print next event name and time */
/* ind's are set up already for this event */
	gl_type_ind = obj_bod[gl_bod_ind].proc_ptr;
	type_ind = gl_type_ind;    /* JJWXXX */
	if (step_switch >= 1)
	  {
/*	    sprintf(reason, "< %s > @%.4f ",
		    emq_current_ptr->rname,emq_current_ptr->rlvt.simtime); */
	    sprintf(reason, "< %s > @%.4f, %d, %d ",
		    emq_current_ptr->rname,emq_current_ptr->rlvt.simtime,
		    emq_current_ptr->rlvt.sequence1,
		    emq_current_ptr->rlvt.sequence2);

	    if (step_switch == 2)
		 printf("\ntwsim> .....%s.....\n", reason);
	    else
                 sim_debug(reason);     /* debug does printf */
	  }
#ifndef HISP
	if (statfile_sw && statint != 0.0 &&
		emq_current_ptr->rlvt.simtime >= stattime)
	  {
/* note we only compare simtime !! */
	  HOST_fprintf(pf_file,"# = %lf\n",emq_current_ptr->rlvt.simtime);
	  record_obj_stats(total,statint);
#ifdef SPEC_OBJSTATS
	  if (init_obs < SPEC_OBJSTATS_SIZE)
	    {
	    stattime = time_array[init_obs];
	    init_obs++;
	    }
	  else
	    stattime = POSINF;
#else	  
	  stattime += statint;
#endif
	  }  
#endif
	if (bp_switch)
	  {
	  sprintf(reason, "< %s > @%.4f, %d, %d ",
		    emq_current_ptr->rname,emq_current_ptr->rlvt.simtime,
		    emq_current_ptr->rlvt.sequence1,
		    emq_current_ptr->rlvt.sequence2);
	  if ((bp_switch == 1 || bp_switch == 3) &&
		(GetSimTime(emq_current_ptr->rlvt) >= atof(bp_timetoken)))
	    {
		bp_switch--;
		step_switch = 1;       /* stop from now on */
		sim_debug(reason);
	    }
	  if ((bp_switch == 2 || bp_switch == 3) &&
	        match(bp_token, emq_current_ptr->rname))
	    {
		step_switch = 1;       /* stop from now on */
		sim_debug(reason);
	    }
	  }
/** if create/destroy **/
	if (emq_current_ptr->rname[0] == '+' ) {   /* +obcreate */
		pr_create_object();
#ifndef HISP
		if (tfile_sw == TRUE) HOST_fprintf(t_file,"\n");
#endif
		} 
	else if (emq_current_ptr->rname[0] == '}') {  /* }obdestroy */
		pr_destroy_object();
#ifndef HISP
		if (tfile_sw == TRUE) HOST_fprintf(t_file,"\n");
#endif
		}
	else {
	gl_type_ind = obj_bod[gl_bod_ind].proc_ptr;
	obj_bod[gl_bod_ind].obj_msgsent = 0;	/* zero schedule (per evt) ctr */ 
/* set up the new event */
	  if (pr_setup_messages(0) == SUCCESS)	    /* also record msglimit*/ 
	    {
#ifndef HISP
	    if (imfile_sw == TRUE)
		   for (ctr = 0; ctr < mesg_lmt; ctr++)
	HOST_fprintf(im_file,"%s\t%.4f [%d,%d]\t%s\t%.4f [%d,%d]\t%ld\n",
        mesg_ptr[ctr]->sname,
	mesg_ptr[ctr]->slvt.simtime, mesg_ptr[ctr]->slvt.sequence1, 
	mesg_ptr[ctr]->slvt.sequence2, mesg_ptr[ctr]->rname,
	mesg_ptr[ctr]->rlvt.simtime, mesg_ptr[ctr]->rlvt.sequence1,
        mesg_ptr[ctr]->rlvt.sequence2, mesg_ptr[ctr]->select);
	    if (notime_sw == FALSE)
		 {
		 if (cpath_sw == TRUE)
		   evt_realtime =  pr_setup_pathtime();		
		 else
		   evt_realtime = 0.0;

		 itimer();		/* start timer, set initial time */
		 }
#endif

#ifdef FLOW
#ifdef  MARK3
		cpu_time += mark3time();
#else
		cpu_time = tot_evt_time;
#endif

		if (flow_sw == TRUE && Flogp < Floge)
		    Flogp->start_time = cpu_time;
#endif

/*PJH_IS	*/
#ifdef	BF_PLUS
		if ( ISLOG_enabled )
		{
		   check_for_events ();
		}
#endif

#ifdef BF_MACH
		if ( ISLOG_enabled )
		{
		   I_speedup();
		}
#endif
/* run the event section */
#ifdef STKCHK
		if (stkchk_sw) mark();
                (*process[gl_type_ind].event)(obj_bod[gl_bod_ind].current);
		if (stkchk_sw)  { 
		  stcheck =look();
		  if (stcheck) {
		     if (stkdata[gl_type_ind] > stcheck)
		        stkdata[gl_type_ind] = stcheck;
		  }
		}
#else
                (*process[gl_type_ind].event)(obj_bod[gl_bod_ind].current);
#endif
#ifdef FLOW

		if (flow_sw == TRUE && Flogp < Floge)
		   {
#ifdef  MARK3
		   cpu_time += mark3time();
#else
		   cpu_time = tot_evt_time;   /* not yet updated */
#endif
		   Flogp->svt = emq_current_ptr->rlvt;
		   Flogp->objidx = gl_hdr_ind;
		   (Flogp++)->end_time = cpu_time;
		   }
#endif

#ifndef HISP
		if (notime_sw == FALSE)
		{


		evt_parttime = itimer() * multiple;
		evt_realtime += evt_parttime;
		tot_evt_time += evt_parttime;  

	/* JJW????  sim_debug will now screw up the timing */
		obj_bod[gl_bod_ind].obj_time  = evt_realtime;
		obj_bod[gl_bod_ind].cum_obj_time  += evt_realtime;
		if (tfile_sw == TRUE)
		  {
		  if (cpath_sw == TRUE)
	            HOST_fprintf(t_file, " %d\t %d \n", tot_evtcount, what_evt );
		  else
		    if (!zflag) 
		      HOST_fprintf(t_file, "%.1f\t %ld \t",evt_realtime, evt_time );
		    else
		      HOST_fprintf(t_file,"\t");
		  }
		}  /* end notime is false */ 

		if ((tfile_sw == TRUE) && ! cpath_sw)
			HOST_fprintf(t_file, "%d\t%d\t%d\n",
			 obj_bod[gl_bod_ind].obj_msglimit,
			   obj_bod[gl_bod_ind].obj_msgsent,
			     obj_bod[gl_bod_ind].cr_count); 
                if (elog_sw)
		  if (obj_bod[gl_bod_ind].obj_msgsent >0)
                HOST_fprintf(el_file, "%s\t%.4f\t%d\t%d\t%d\n",
		  emq_current_ptr->rname,emq_current_ptr->rlvt.simtime,
		  emq_current_ptr->rlvt.sequence1,emq_current_ptr->rlvt.sequence2,
		  obj_bod[gl_bod_ind].obj_msgsent);
		    

/* statistics code */
		if (eqVTime(obj_bod[gl_bod_ind].lastime,emq_current_ptr->rlvt))
		             time_cnt++;
	 	else if (strncmp(emq_current_ptr->rname,
			    "stdout", 6) != 0)
		       {
		        event_cnt++;
/*sfb*
#ifdef BF_MACH
if ( 0 == (event_cnt % 1000) )
    print_faults();
#endif
*sfb*/
		        obj_bod[gl_bod_ind].num_events++;
		       }
		     else 
		       {
		        stdout_cnt++;
 		        obj_bod[gl_bod_ind].num_events++;
		       }
                     
                obj_bod[gl_bod_ind].lastime = emq_current_ptr->rlvt;
		obj_bod[gl_bod_ind].what_evt = tot_evtcount;
                    
/* end statistics code */

		obj_bod[gl_bod_ind].current_lvt = emq_current_ptr->rlvt;
		if (archive_switch)
			pr_archive_messages();
/* restart timer for measuring dequeueing time
   statistics and file writing time not recorded anywhere */
		itimer();
#endif

		pr_delete_messages();
#ifndef HISP
		dequeue_time += itimer();
#endif

	      }		/* end of setup message was ok */
	     }		/* end of if cr/dest else part */
	   }		/* end of found next process 'while' loop */
 /* queue is empty  or error*/
	if (prog_status != WSERROR) {
	  prog_status = WSTERM;
	  if (step_switch >= 1) {
	     sprintf(reason, "< queue is empty, terminating >");
	     if (step_switch == 2)
		 printf("\ntwsim> .....%s.....\n", reason);
	     else
                 sim_debug(reason);     /* debug does printf */
	     }
	  term_process();
	}
	else
	fprintf(stderr, "twsim>  error termination\n");
	fprintf(stderr, "twsim>  ***simulation end ***\n");
}

/************************************************************************
*
*	term_process - run the termination section of all objects that
*	  have not been destroyed
*	  called at the end of main_process
*
*	(we go through the obj_hdr table because there is no back pointer
*	  from the ob_bod table and we need the gl_hdr_ind so that the
*	  me() timewarp entry call will work in the term section)
*
*************************************************************************/
void FUNCTION term_process()

   {
	for (hdr_ind =0; hdr_ind <num_objects; hdr_ind++)
	   {
	    bod_ind = obj_hdr[hdr_ind].obj_bod_ptr;
	    type_ind = obj_bod[bod_ind].proc_ptr;
	    gl_hdr_ind = hdr_ind;
	    gl_bod_ind = bod_ind;
	    gl_type_ind = type_ind;
	    if (process[type_ind].term != NULL  &&
		 obj_bod[bod_ind].status != TW_DELETED) 
                   (*process[type_ind].term)(obj_bod[bod_ind].current);
	   }
   }

/************************************************************************
*
*	pr_create_object - create an object 
*
*	called by - main_process when an obcreate mesg is received
*
*	- process event message and create the object
*	- delete the message 
*	
*
*************************************************************************/
FUNCTION pr_create_object()
{
	create_mesg   *txt;

	   txt = (create_mesg *) emq_current_ptr->text;
	   if (cr_create_object(txt->rcvr,
			     txt->rcvtim,
			     txt->type)
	       == FAILURE)
	     {
	     error("object create message failed");
             return (FAILURE);
	     }
	   else {
		mesg_lmt = 1;
		mesg_ptr[0] = emq_current_ptr;
		pr_setup_messages(1);
		pr_delete_messages();
	   	}
	   return(SUCCESS);
}

/************************************************************************
*
*	pr_destroy_object - destroy an object 
*
*	called by - main_process when an obdestroy mesg is received
*
*	- process event message and destroy the object
*	- delete the message 
*	
*
*************************************************************************/
FUNCTION pr_destroy_object()
{
	create_mesg   *txt;
	   txt = (create_mesg *) emq_current_ptr->text;
	   if (ds_destroy_object (txt->rcvr, txt->rcvtim) == FAILURE)
		{
		error("object destroy message failed");
		return(FAILURE);
		}
	   else {
		mesg_lmt = 1;
		mesg_ptr[0] = emq_current_ptr;
		pr_setup_messages(1);
		pr_delete_messages();
	   	}
	   return(SUCCESS);

}

/************************************************************************
*
*	pr_find_next_process - validate receiving object
*
*	called by - main_process
*
* --> SETS UP gl_hdr_ind and gl_bod_ind <--
*	- perform routine to search for object name in object table
*	- save object header index (object names are store alphabetically
*	-    for binary search
*	- get the object body index
*	- confirm that the object is active
*
*************************************************************************/

FUNCTION pr_find_next_process(oname)
Name_object     oname;
{
    if (cr_sch_obj_table(oname) == SUCCESS)
    {
	gl_hdr_ind = hdr_ind;
	gl_bod_ind = obj_hdr[hdr_ind].obj_bod_ptr;
	if (obj_bod[gl_bod_ind].status == TW_ACTIVE)
	    return (SUCCESS);
	else
	    error("object destroyed");
    }
    else
    {
	fprintf(stderr, "%s does not exist - event\n", oname);
    }
    return (FAILURE);
}


/************************************************************************
*
*	pr_setup_pathtime
*
*	called by - main_process if critical path switch is on.
*
*	determine max of saved time in event messages and saved time 
*	for object and return value. global
*	indexes should point to object about to be processed.
*	If the same time occurs more than once, only one is used.
*
*************************************************************************/
#ifndef HISP
double  FUNCTION pr_setup_pathtime()
{
    int count, recordit;
    double this_time;
    double final_time;
    this_time = 0.0;
    for(count = 0; count < mesg_lmt; count++)
	if (mesg_ptr[count]->msg_time > this_time) {
	    this_time = mesg_ptr[count]->msg_time;
	    recordit = count;
	    }
    if (this_time > obj_bod[gl_bod_ind].obj_time) {
	final_time = this_time;
	what_evt = mesg_ptr[recordit]->what_evt;
	}
    else {
	  final_time = obj_bod[gl_bod_ind].obj_time;
	  what_evt = obj_bod[gl_bod_ind].what_evt;
	  }
    return(final_time);
}
#endif


/************************************************************************
*
*	pr_archive_messages - write deleted messages to archive file
*
*	called by - main_process
*
*	- format and then write the message
*
*	NOTE: schedule, ask, and reply (in TWSP2) are also archived
*
*************************************************************************/
#ifndef HISP
void FUNCTION pr_archive_messages()
{
    int             i;
    mesg_block     *mptr;

    for (i = 0; i < mesg_lmt; i++)
    {
	mptr = mesg_ptr[i];
	HOST_fprintf
	    (
	     arc_file,
	     "twsim ( event %s %s %.4f %.4f %d %d);\n",
	     mptr->sname,
	     mptr->rname,
	     mptr->slvt.simtime,
	     mptr->rlvt.simtime,
	     obj_bod[gl_bod_ind].pnode,
	     elap_time
	    );
    }
}
#endif

/************************************************************************
*
*	warning - error routine that allow continued processing
*
*	called by - you name it
*
*************************************************************************/

void FUNCTION warning(strng)
char           *strng;
{
    fprintf(stderr,"twsim> warning - %s\n", strng);
}

/************************************************************************
*
*	error - error routine that causes system shutdown
*
*	called by - many places
*
*************************************************************************/

void FUNCTION error(strng)
char           *strng;
{
    prog_status = WSERROR;
    fprintf(stderr,"twsim> error - %s\n", strng);
    dm_one_message(emq_current_ptr, 0);
    do_one_object(gl_hdr_ind);
    sim_debug("Try DO or DM");
}

/************************************************************************
*
*	pause1 - general purpose pause routine - no option
*
*	called by - global_term
*
*************************************************************************/

FUNCTION pause1()
{
    int             c;

    fprintf(stderr,"tws> <CR> to Continue\n");
    c = getchar();
    return (SUCCESS);
}

/************************************************************************
*
*	pause2 - general purpose pause routine - optional quit
*
*	called by - dis_objects, dm_queue,
*		    dm_select
*
*************************************************************************/

FUNCTION pause2()
{
    int             c;

    fprintf(stderr,"twsim> <CR> to Continue or 'Q' to Quit\n");
    c = getchar();
    if (c == 'Q' || c == 'q')
	return (SUCCESS);
    else
	return (FAILURE);
}

/************************************************************************
*
*	debug - general purpose routine to allow user debug options
*
*	called by - various and sundry places
*
*	- set continue flag
*	- print the debug message
*	- while continue flag is true
*	-   get user option
*	- NULL reply resets continue flag
*
*************************************************************************/

void FUNCTION sim_debug(strng)
char           *strng;
{
    int             cflag;

    cflag = TRUE;
    fprintf(stderr,"\ntwsim> .....%s.....\n", strng);
#ifndef HISP
    if (notime_sw == FALSE) {
		evt_parttime = itimer();
		evt_realtime += evt_parttime;
		tot_evt_time += evt_parttime;
		}
#endif
    while (cflag)
    {
	gnxt_option();
	if (match(mreply, ""))
	    cflag = FALSE;
	else
	    perform_option();
    }
#ifndef HISP
    if (notime_sw == FALSE) itimer();
	/* restart timer but don't count time spent in this routine */
#endif
}
/************************************************************************
*
*       set_bp  - set breakpoint          JJW
*
*	called by - perform_option
*
*       - set breakpoint switch true
*       - get breakpoint token
*	- ix = 0, time breakpoint; ix=1, object breakpoint
*
*************************************************************************/

void FUNCTION set_bp(ix)
int ix;
{
    gnxt_token(1);
    if (strlen(token) == 0)
	fprintf(stderr, "no receive object specified\n");
    else
    {
	if (ix == 0)
	{
	strcpy(bp_timetoken, token);
	printf ("time breakpoint: %s\n", bp_timetoken);
	if (!bp_switch) bp_switch = 1;
	if (bp_switch == 2) bp_switch = 3;
	}
	else
	{
	strcpy(bp_token, token);
	printf ("object breakpoint: %s\n", bp_token);
	if (!bp_switch) bp_switch = 2;
	if (bp_switch == 1) bp_switch = 3;
	}
	
    }
}
/************************************************************************
*
*	dis_times - display global timing statistics
*
*	called by - main,  also perform_option ????JJW
*
*************************************************************************/
void FUNCTION dis_times()

{
   double avg_evt_time;
   double float_time;
   double float_queue;
   double float_dequeue;
   /* also uses many global vars */

   printf("total clock time = %.2lf seconds\n",total);
#ifndef HISP
   if (d_file_sw == TRUE)
     HOST_fprintf(data_file,"total_clock_time:\t%.2lf seconds\n",total);
#endif
#ifdef MARK3
   printf("total integer clock time = %ld\n",total2);  /* &TIME */
#ifndef HISP
   if (d_file_sw == TRUE)
     HOST_fprintf(data_file,"total_integer_clock_time:\t%ld\n",total2);
#endif
#endif
#ifndef HISP
  if (notime_sw == FALSE) {
   float_queue = ((double)queue_time) / 1000000.0;
   float_dequeue = ((double)dequeue_time) / 1000000.0;
   float_time = tot_evt_time / 1000000.0;
     avg_evt_time = float_time / (double)(event_cnt + stdout_cnt);
	/* no time_cnt bacuse equal time are only 1 event in TW itself */
     printf("sum of event times = %.2lf seconds\n",float_time);
     printf("avg time per event = %.3lf seconds\n",avg_evt_time);
   if (cpath_sw == FALSE){
     printf("hog time = %.3f  microseconds by %s\n",hog.hog_time,hog.hog_name);
     }
   else printf("crit path time = %.3f  microseconds by %s (%d)\n",hog.hog_time,hog.hog_name,hog.hog_evt);
   if (d_file_sw == TRUE) { 
     HOST_fprintf(data_file,"sum_of_event_times:\t%.2lf seconds\n",float_time);
     HOST_fprintf(data_file,"avg_time_per_event:\t%.3lf seconds\n",avg_evt_time);
     if ( cpath_sw == FALSE) {
       HOST_fprintf(data_file,"hog_time_microseconds:\t%.3f\n",hog.hog_time);
       HOST_fprintf(data_file,"hog_name:\t%s\n",hog.hog_name);
       }
     else {
       HOST_fprintf(data_file,"crit_path_time_microseconds:\t%f\n",hog.hog_time);
       HOST_fprintf(data_file,"crit_path_name:\t%s\n",hog.hog_name);
       HOST_fprintf(data_file,"latest_event:\t%d\n",hog.hog_evt);
       }
   }
   printf("queue time = %.2lf seconds\n",(double)float_queue);
   printf("dequeue time = %.2lf seconds\n",(double)float_dequeue);
#ifdef BF_MACH
    printf("page faults:%d\n", num_faults());
#endif
   if (d_file_sw == TRUE) {
    HOST_fprintf(data_file,"queue_time:\t%.2lf seconds\n",(double)float_queue);
    HOST_fprintf(data_file,"dequeue_time:\t%.2lf seconds\n",(double)float_dequeue);
#ifdef BF_MACH
     HOST_fprintf(data_file,"page_faults:\t%d\n", num_faults());
#endif
   }

  }    /* end of notime is false */
#endif
}


/************************************************************************
*
*	dis_stats - display time warp module entry counts and
*		        various other run statistics
*
*	called by - global_term, perform_option
*
*************************************************************************/

void FUNCTION dis_stats()
{
#ifdef fuji
   char* qustatd;
   qustatd = (char *) qustats();
#endif
   if (!go_flag)
    {

    printf("\n ....... Run Statistics .......\n\n");
    printf("   event messages:%lu\n", evtmsg_cnt);
    printf("events sans stdout:%lu\n", event_cnt);
    printf("    stdout events:%lu\n", stdout_cnt);
    printf("events,equal time:%lu\n", time_cnt);
    printf("    final simtime:%.4f\n", savelvt);
    printf("       multiplier:%lf\n\n", multiple);
    printf("... Module entry counts ...\n");
    printf("  get current lvt:%lu\n", simtm_cnt);
    printf("     get own name:%lu\n", me_cnt);
    printf("    create object:%lu\n", obcre_cnt);
    printf("   destroy object:%lu\n", obdes_cnt);
    printf("    message count:%lu\n", mcount_cnt);
    printf("      get message:%lu\n", getmsg_cnt);
    printf(" get msg selector:%lu\n", getsel_cnt);
    printf("       get sender:%lu\n", sender_cnt);
    printf("    get send time:%lu\n", sndtm_cnt);
    printf("\n ....... Space Statistics .......\n");
    printf(" requested message blocks:%lu\n", req_blk_cnt);
    printf(" allocated message blocks:%lu\n", all_blk_cnt);
    printf("     number message bytes:%lu\n", num_mesg_bytes);
    printf("       number state bytes:%lu\n", num_state_bytes);
    if (test_var1 !=0)
       printf("                   test 1:%d\n", test_var1);
    if (test_var2 !=0)
       printf("               queue-size:%d\n", test_var2);
    printf("\n");
#ifdef fuji
    if (qustatd)  printf("%s\n",qustatd);
#endif
    }
#ifndef HISP
  if (d_file_sw == TRUE)
    {
    HOST_fprintf(data_file,"event_messages:\t%lu\n", evtmsg_cnt);
    HOST_fprintf(data_file,"events_sans_stdout:\t%lu\n", event_cnt);
    HOST_fprintf(data_file,"stdout_events:\t%lu\n", stdout_cnt);
    HOST_fprintf(data_file,"events_equal_time:\t%lu\n", time_cnt);
    HOST_fprintf(data_file,"final_simtime:\t%.4lf\n", savelvt);
    HOST_fprintf(data_file,"multiplier:\t%lf\n", multiple);
    HOST_fprintf(data_file,"get_current_lvt:\t%lu\n", simtm_cnt);
    HOST_fprintf(data_file,"get_own_name:\t%lu\n", me_cnt);
    HOST_fprintf(data_file,"create_object:\t%lu\n", obcre_cnt);
    HOST_fprintf(data_file,"destroy_object:\t%lu\n", obdes_cnt);
    HOST_fprintf(data_file,"message_count:\t%lu\n", mcount_cnt);
    HOST_fprintf(data_file,"get_message:\t%lu\n", getmsg_cnt);
    HOST_fprintf(data_file,"get_msg_selector:\t%lu\n", getsel_cnt);
    HOST_fprintf(data_file,"get_sender:\t%lu\n", sender_cnt);
    HOST_fprintf(data_file,"get_send_time:\t%lu\n", sndtm_cnt);
    HOST_fprintf(data_file,"requested_message_blocks:\t%lu\n", req_blk_cnt);
    HOST_fprintf(data_file,"allocated_message_blocks:\t%lu\n", all_blk_cnt);
    HOST_fprintf(data_file,"number_message_bytes:\t%lu\n", num_mesg_bytes);
    HOST_fprintf(data_file,"number_state_bytes:\t%lu\n", num_state_bytes);
  if (test_var1 !=0)
    HOST_fprintf(data_file,"test_1:\t%d\n", test_var1);
  if (test_var2 !=0)
    HOST_fprintf(data_file,"queue-size:\t%d\n", test_var2);
    HOST_fprintf(data_file,"\n");
#ifdef fuji
    if (qustatd) HOST_fprintf(data_file,"%s\n",qustatd);
#endif
    }
#endif
}


/**********************************************************************
*
*	find hog object  - object with greatest cumulated object time
*
*	-- called by main
*
*	output on global struct hog
*
*	-- arg= 1 for hog time, arg =2 for critical path time
*
*********************************************************************/
#ifndef HISP
void FUNCTION find_hog(swtch)
   int swtch;

   {
   double time_fld;
   int hdr_indx, bdy_indx;

    for (hdr_indx = 0; hdr_indx < num_objects; hdr_indx++)  
	{
	bdy_indx = obj_hdr[hdr_indx].obj_bod_ptr;
	if (swtch == 1)
		time_fld = obj_bod[bdy_indx].cum_obj_time;
	else  time_fld = obj_bod[bdy_indx].obj_time;

	if (time_fld >= hog.hog_time)
		{ hog.hog_time = time_fld;
		  strncpy(hog.hog_name,obj_hdr[hdr_indx].name,NOBJLEN);
		  hog.hog_evt = obj_bod[bdy_indx].what_evt;
		}
	}
   }

#endif  /* ifndef HISP */
/*********************************************************************
make the flow file 	#####  Flow     ####
*
* (puts in first field as a fixed 0, this is the node in timewarp)
*
*************************************************************************/
#ifdef FLOW
FUNCTION do_flow_file()

{
   ENTRY *ptr;

  printf("writing flow file\n"); 
  ptr = Flog;
   flow_file = HOST_fopen(DEFAULTFLOWFILE, "w");

   while (ptr <= Flogp)
      {
       HOST_fprintf(flow_file,"0\t%d\t%d\t%s\t%ld\n",
		ptr->start_time,ptr->end_time,
		obj_hdr[ptr->objidx].name,ptr->svt);
       ptr++;
      }

   HOST_fclose(flow_file);
}
#endif

/***********************************************************************
* SIGINT handler
************************************************************************/
FUNCTION brkfct()
{
step_switch = 1;
return(0);
}	/* #br */


/***********************************************************************
* Memory usage 
************************************************************************/
#ifdef SUN
FUNCTION long  memuse(howmuch)
int howmuch;
{
   return ( (long)sbrk(howmuch));
/* sbrk really returns a caddr_t which is a char pointer - see sys/types  */
}
#endif



/*************************************************************************
* functions to check for stack overflow. 
* see event run code in main_process
* S_ARSZ is size of checking array,  S_CHKINT is space interval of check 
* Note the array is an array of chars 
*************************************************************************/

#ifdef STKCHK
#define S_ARSZ 10001
#define S_CHKINT 250
/* sum of above */
#define S_C2 10251


mark()  /* Load the checking array with a value */
{
 register int idx,mrk;
 char  ary[S_ARSZ];
 for (idx = 0; idx <S_ARSZ; idx += S_CHKINT)
 ary[idx] = (char) 55;
 return(0);
}

look() /* Check every S_CHKINT to see if the value is still there */
{
 register int idx,mrk;
 char ary[S_ARSZ];
 mrk = 0;
 for (idx = S_ARSZ-1; idx >=0; idx -= S_CHKINT) {
  if (ary[idx] != (char) 55) mrk = idx;
  }
 return ( mrk);
}

int* stackarray(sw) /* initialiaze and print the data array */
int sw;
{
    int idx;
    switch (sw)  {
      case 1:  /* initialize */
	stkdata = (int * ) sim_malloc ((long)(sizeof(int) * MAX_NUM_TYPES));
	for (idx = 0; idx <MAX_NUM_TYPES; idx++)
	    stkdata[idx] = S_ARSZ-1;
	return(stkdata);
    case 2:   /* print contents */
	printf(" usage     by type\n");
	for (idx = 0; idx<num_types; idx++) 
	   printf("%d     %s\n",(S_C2 -stkdata[idx]),process[idx].type);
	return(0);
    }

}

#endif


/*PJH_IS */
#ifdef BF_PLUS
FUNCTION I_speedup ()

{
    num_IS_entries++;
    if ( IS_logp == NULL )
    {
       printf ( "no IS log space\n" );
       return;
    }
    if ( IS_logp >= IS_loge )
    {
      printf ( "IS log full\n" );
      return;
    }

    IS_clock2 = clock();
    IS_logp->seqnum = num_IS_entries;
    IS_logp->cputime = ( ( (IS_clock2 - IS_clock1 ) *62.5 ) /1000000.);
    IS_logp->minvt = emq_current_ptr->rlvt;
    IS_logp++;
    malarm ( IS_delta );
}
#endif

/* JJW IS from PJH_IS */
#ifdef BF_MACH

FUNCTION I_speedup ()
{
    unsigned int time_delta;
    if (IS_clock3 == 0) IS_clock3 = IS_clock1;
    IS_clock2 = getrtc();
    time_delta = ( (IS_clock2 - IS_clock3 ) *62.5 ) /1000.0; /* milliseconds */
/* for debug 
    printf("islog: delta=%u    now=%u\n", time_delta, IS_clock2); */

    if (time_delta  <= IS_delta )return;

   /* else */
    IS_clock3 = IS_clock2;
    num_IS_entries++;
    if ( IS_logp == NULL )
    {
       printf ( "no IS log space\n" );
       return;
    }
    if ( IS_logp >= IS_loge )
    {
      printf ( "IS log full\n" );
      return;
    }
/* for debug 
    printf("islog: rlvt= %lf time= %u\n",emq_current_ptr->rlvt.simtime,IS_clock2);
*/

    IS_logp->seqnum = num_IS_entries;
    IS_logp->cputime = ((IS_clock2  - IS_clock1) *62.5)/ 1000000. ;   /* seconds */
    IS_logp->minvt = emq_current_ptr->rlvt;
    IS_logp++;
}
#endif

#if defined(BF_MACH) || defined(BF_PLUS)
FUNCTION IS_dumplog ()
{
   printf ("IS_dumplog %d entries\n", num_IS_entries );

   IS_log_file = HOST_fopen ("ISLOG","w");

   while ( IS_log < IS_logp )
   {
      
     HOST_fprintf ( IS_log_file, "0 %u %f %f\n",
			IS_log->seqnum,
			IS_log->cputime,
			IS_log->minvt.simtime
		  );
     IS_log++;
   }
   HOST_fclose ( IS_log_file );

 }
#endif
